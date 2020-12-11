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
        getProperties(Dbus::objectConfig, Dbus::syscfgInterface);
    puts("Global network configuration:");
    printProperty("Host name", Dbus::syscfgHostname, globalCfg);
    printProperty("Default IPv4 gateway", Dbus::syscfgDefGw4, globalCfg);
    printProperty("Default IPv6 gateway", Dbus::syscfgDefGw6, globalCfg);

    // DHCP config
    const auto dhcpCfg = getProperties(Dbus::objectDhcp, Dbus::dhcpInterface);
    puts("Global DHCP configuration:");
    printProperty("DNS over DHCP", Dbus::dhcpDnsEnabled, dhcpCfg);
    printProperty("NTP over DHCP", Dbus::dhcpNtpEnabled, dhcpCfg);

    // Network interfaces
    for (const auto& it : netObjects)
    {
        if (it.second.find(Dbus::ethInterface) != it.second.end())
        {
            printInterface(static_cast<std::string>(it.first).c_str());
        }
    }
}

void Show::printInterface(const char* obj)
{
    const auto cfgEth = getProperties(obj, Dbus::ethInterface);
    const auto cfgVlan = getProperties(obj, Dbus::vlanInterface);
    const auto cfgMac = getProperties(obj, Dbus::macInterface);

    const Dbus::PropertyValue& nameProp = cfgEth.find(Dbus::ethName)->second;
    printf("Ethernet interface %s:\n", std::get<std::string>(nameProp).c_str());

    if (!cfgVlan.empty())
    {
        printProperty("VLAN Id", Dbus::vlanId, cfgVlan);
    }
    printProperty("MAC address", Dbus::macSet, cfgMac);

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

    printProperty("DHCP", Dbus::ethDhcpEnabled, cfgEth);

    const auto cfgDnsNtp = getProperties(obj, Dbus::ethInterface);
    printProperty("DNS servers", Dbus::ethNameServers, cfgDnsNtp);
    printProperty("Static DNS servers", Dbus::ethStNameServers, cfgDnsNtp);
    printProperty("NTP servers", Dbus::ethNtpServers, cfgDnsNtp);
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

const Dbus::Properties& Show::getProperties(const char* obj,
                                            const char* iface) const
{
    const auto objPath = sdbusplus::message::object_path(obj);
    const auto dbusObject = netObjects.find(objPath);
    if (dbusObject != netObjects.end())
    {
        const auto props = dbusObject->second.find(iface);
        if (props != dbusObject->second.end())
        {
            return props->second;
        }
    }
    static Dbus::Properties empty;
    return empty;
}
