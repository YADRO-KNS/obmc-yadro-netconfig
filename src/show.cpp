// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "show.hpp"

Show::Show(Dbus& bus) : bus(bus)
{
    bus.call(Dbus::objectRoot, Dbus::objmgrInterface, Dbus::objmgrGet)
        .read(netObjects);
}

void Show::print()
{
    // Global config
    const auto globalCfg =
        netObjects.find(sdbusplus::message::object_path(Dbus::objectConfig));
    if (globalCfg != netObjects.end())
    {
        const auto cfg = globalCfg->second.find(Dbus::syscfgInterface);
        if (cfg != globalCfg->second.end())
        {
            const Dbus::Properties& properties = cfg->second;
            puts("Global network configuration:");
            printProperty("Host name", Dbus::syscfgHostname, properties);
            printProperty("Default IPv4 gateway", Dbus::syscfgDefGw4,
                          properties);
            printProperty("Default IPv6 gateway", Dbus::syscfgDefGw6,
                          properties);
        }
    }

    // DHCP config
    const auto dhcpCfg =
        netObjects.find(sdbusplus::message::object_path(Dbus::objectDhcp));
    if (dhcpCfg != netObjects.end())
    {
        const auto cfg = dhcpCfg->second.find(Dbus::dhcpInterface);
        if (cfg != dhcpCfg->second.end())
        {
            const Dbus::Properties& properties = cfg->second;
            puts("Global DHCP configuration:");
            printProperty("DNS over DHCP", Dbus::dhcpDnsEnabled, properties);
            printProperty("NTP over DHCP", Dbus::dhcpNtpEnabled, properties);
        }
    }

    // eth0 config
    puts("General Ethernet interface configuration:");
    printInterface(Dbus::objectEth0);

    // VLAN config
    for (const auto& it : netObjects)
    {
        const auto vlan = it.second.find(Dbus::vlanInterface);
        if (vlan != it.second.end())
        {
            const Dbus::Properties& properties = vlan->second;
            const uint32_t id =
                std::get<uint32_t>(properties.find(Dbus::vlanId)->second);
            printf("VLAN %u configuration:\n", id);
            printInterface(static_cast<std::string>(it.first).c_str());
        }
    }
}

void Show::printInterface(const char* obj)
{
    const auto genCfg = netObjects.find(sdbusplus::message::object_path(obj));
    if (genCfg != netObjects.end())
    {
        // IP addresses
        for (const auto& it : bus.getAddresses(obj))
        {
            std::string val = it.address;
            val += '/';
            val += std::to_string(it.mask);
            if (!it.gateway.empty())
            {
                val += ", gateway ";
                val += it.gateway;
            }
            printProperty("IP address", val.c_str());
        }

        // MAC address
        const auto cfgMac = genCfg->second.find(Dbus::macInterface);
        if (cfgMac != genCfg->second.end())
        {
            printProperty("MAC address", Dbus::macSet, cfgMac->second);
        }

        // Common interface configuration
        const auto cfgEth = genCfg->second.find(Dbus::ethInterface);
        if (cfgEth != genCfg->second.end())
        {
            const Dbus::Properties& properties = cfgEth->second;
            printProperty("DHCP", Dbus::ethDhcpEnabled, properties);
            printProperty("Domains", Dbus::ethDomainName, properties);
            printProperty("DNS servers", Dbus::ethNameServers, properties);
            printProperty("Static DNS servers", Dbus::ethStNameServers,
                          properties);
            printProperty("NTP servers", Dbus::ethNtpServers, properties);
        }
    }
}

void Show::printProperty(const char* title, const char* name,
                         const Dbus::Properties& properties) const
{
    const auto it = properties.find(name);
    if (it == properties.end())
    {
        printProperty(title, nullptr);
    }
    else
    {
        std::string val;
        std::visit(
            [&val](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, bool>)
                {
                    val = arg ? "Enabled" : "Disabled";
                }
                else if constexpr (std::is_arithmetic<T>::value)
                {
                    val = std::to_string(arg);
                }
                else if constexpr (std::is_same_v<T, std::string>)
                {
                    val = arg;
                }
                else if constexpr (std::is_same_v<T, std::vector<std::string>>)
                {
                    for (const auto& a : arg)
                    {
                        if (!val.empty())
                        {
                            val += ", ";
                        }
                        val += a;
                    }
                }
                else
                {
                    static_assert(T::value, "Unhandled value type");
                }
            },
            it->second);
        printProperty(title, val.c_str());
    }
}

void Show::printProperty(const char* name, const char* value) const
{
    if (!value)
    {
        value = "N/A";
    }
    else if (!*value)
    {
        value = "-";
    }

    // Size of the column with property title (formatting output)
    static const int nameWidth = 20;

    const int nameLen = static_cast<int>(strlen(name));
    printf("  %s: %*s%s\n", name, nameLen < nameWidth ? nameWidth - nameLen : 0,
           "", value);
}
