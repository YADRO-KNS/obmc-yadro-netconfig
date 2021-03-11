// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "netconfig.hpp"

#include "dbus.hpp"
#include "show.hpp"

#include <stdexcept>

/**
 * @brief Command handler function.
 *
 * @param[in] bus D-Bus instance
 * @param[in] args command arguments without command name itself
 *
 * @throw std::exception in case of errors
 */
using Handler = void (*)(Dbus& bus, Arguments& args);

/**
 * @struct Command
 * @brief Command description.
 */
struct Command
{
    /** @brief Command name. */
    const char* name;
    /** @brief Command arguments format. */
    const char* fmt;
    /** @brief Help text. */
    const char* help;
    /** @brief Command handler. */
    Handler fn;
};

/** @brief Standard message to print after sending request. */
static const char* completeMessage = "Request has been sent";

/** @brief Show network configuration: `show` */
static void cmdShow(Dbus& bus, Arguments& args)
{
    args.expectEnd();
    Show(bus).print();
}

/** @brief Reset network configuration: `reset` */
static void cmdReset(Dbus& bus, Arguments& args)
{
    args.expectEnd();

    puts("Reset network configuration...");
    bus.call(Dbus::objectRoot, Dbus::resetInterface, Dbus::resetMethod);
    puts(completeMessage);
}

/** @brief Set MAC address: `mac {INTERFACE} MAC` */
static void cmdMac(Dbus& bus, Arguments& args)
{
    const char* iface = args.asNetInterface();
    const char* mac = args.asMacAddress();
    args.expectEnd();

    const std::string object = Dbus::ethToPath(iface);

    printf("Set new MAC address %s...\n", mac);
    bus.set(object.c_str(), Dbus::macInterface, Dbus::macSet, mac);
    puts(completeMessage);
}

/** @brief Set BMC host name: `hostname NAME` */
static void cmdHostname(Dbus& bus, Arguments& args)
{
    std::string name = args.asIpOrFQDN();
    args.expectEnd();

    printf("Set new host name %s...\n", name.c_str());
    bus.set(Dbus::objectConfig, Dbus::syscfgInterface, Dbus::syscfgHostname,
            name);
    puts(completeMessage);
}

/** @brief Set default gateway: `getway IP` */
static void cmdGateway(Dbus& bus, Arguments& args)
{
    const auto [ver, ip] = args.asIpAddress();
    args.expectEnd();

    printf("Setting default gateway for IPv%i to %s...\n",
           static_cast<int>(ver), ip.c_str());

    const char* property =
        ver == IpVer::v4 ? Dbus::syscfgDefGw4 : Dbus::syscfgDefGw6;

    bus.set(Dbus::objectConfig, Dbus::syscfgInterface, property, ip);

    puts(completeMessage);
}

/** @brief Add/remove IP: `ip {INTERFACE} {add|del} IP[/MASK]` */
static void cmdIp(Dbus& bus, Arguments& args)
{
    const char* iface = args.asNetInterface();
    const Action action = args.asAction();
    const auto [ipVer, ip, mask] = args.asIpAddrMask();
    args.expectEnd();

    const std::string object = Dbus::ethToPath(iface);

    if (action == Action::add)
    {
        const char* ipInterface =
            ipVer == IpVer::v4 ? Dbus::ip4Interface : Dbus::ip6Interface;

        bus.call(object.c_str(), Dbus::ipCreateInterface, Dbus::ipCreateMethod,
                 ipInterface, ip, mask, "");

        printf("Request for setting %s/%d on %s has been sent\n", ip.c_str(),
               mask, iface);
    }
    else
    {
        // Search for IP address' object
        bool handled = false;
        for (const auto& it : bus.getAddresses(object.c_str()))
        {
            if (it.address == ip)
            {
                bus.call(it.object.c_str(), Dbus::deleteInterface,
                         Dbus::deleteMethod);
                handled = true;
                break;
            }
        }
        if (!handled)
        {
            std::string err = "IP address ";
            err += ip;
            err += " not found";
            throw std::invalid_argument(err);
        }

        puts(completeMessage);
    }
}

/** @brief Enable/disable DHCP client: 'dhcp {INTERFACE} {enable|disable}` */
static void cmdDhcp(Dbus& bus, Arguments& args)
{
    const char* iface = args.asNetInterface();
    const Toggle toggle = args.asToggle();
    args.expectEnd();

    const std::string object = Dbus::ethToPath(iface);
    std::string enable;

    switch (toggle)
    {
        case Toggle::enable:
            enable =
                "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.both";
            break;

        case Toggle::disable:
            enable =
                "xyz.openbmc_project.Network.EthernetInterface.DHCPConf.none";
            break;
    }

    printf("%s DHCP client...\n",
           toggle == Toggle::enable ? "Enable" : "Disable");

    bus.set(object.c_str(), Dbus::ethInterface, Dbus::ethDhcpEnabled, enable);

    puts(completeMessage);
}

/** @brief Enable/disable DHCP features: 'dhcpcfg {enable|disable} {dns|ntp}` */
static void cmdDhcpcfg(Dbus& bus, Arguments& args)
{
    const char* featureDns = "dns";
    const char* featureNtp = "ntp";

    const Toggle toggle = args.asToggle();
    const char* feature = args.asOneOf({featureDns, featureNtp});
    args.expectEnd();

    const bool enable = toggle == Toggle::enable;

    if (strcmp(feature, featureDns) == 0)
    {
        printf("%s DNS over DHCP...\n", enable ? "Enable" : "Disable");
        bus.set(Dbus::objectDhcp, Dbus::dhcpInterface, Dbus::dhcpDnsEnabled,
                enable);
    }
    else
    {
        printf("%s NTP over DHCP...\n", enable ? "Enable" : "Disable");
        bus.set(Dbus::objectDhcp, Dbus::dhcpInterface, Dbus::dhcpNtpEnabled,
                enable);
    }

    puts(completeMessage);
}

/** @brief Add/remove DNS server: `dns {INTERFACE} {add|del} IP [IP..]` */
static void cmdDns(Dbus& bus, Arguments& args)
{
    const char* iface = args.asNetInterface();
    const Action action = args.asAction();

    std::vector<std::string> servers;
    while (args.peek() != nullptr)
    {
        const auto [_, srv] = args.asIpAddress();
        servers.emplace_back(srv);
        printf("%s DNS server %s...\n",
               action == Action::add ? "Adding" : "Removing", srv.c_str());
    }
    args.expectEnd();

    const std::string object = Dbus::ethToPath(iface);
    if (action == Action::add)
    {
        bus.append(object.c_str(), Dbus::ethInterface, Dbus::ethStNameServers,
                   servers);
    }
    else
    {
        bus.remove(object.c_str(), Dbus::ethInterface, Dbus::ethStNameServers,
                   servers);
    }

    puts(completeMessage);
}

/** @brief Add/remove NTP server: `ntp {INTERFACE} {add|del} ADDR [ADDR..]` */
static void cmdNtp(Dbus& bus, Arguments& args)
{
    const char* iface = args.asNetInterface();
    const Action action = args.asAction();

    std::vector<std::string> servers;
    while (args.peek() != nullptr)
    {
        std::string srv = args.asIpOrFQDN();
        printf("%s NTP server %s...\n",
               action == Action::add ? "Adding" : "Removing", srv.c_str());
        servers.emplace_back(srv);
    }
    args.expectEnd();

    const std::string object = Dbus::ethToPath(iface);

    if (action == Action::add)
    {
        bus.append(object.c_str(), Dbus::ethInterface, Dbus::ethNtpServers,
                   servers);
    }
    else
    {
        bus.remove(object.c_str(), Dbus::ethInterface, Dbus::ethNtpServers,
                   servers);
    }

    puts(completeMessage);
}

/** @brief Add/remove VLAN: `vlan {add|del} {INTERFACE} ID` */
static void cmdVlan(Dbus& bus, Arguments& args)
{
    const Action action = args.asAction();
    const char* iface = args.asNetInterface();
    const uint32_t id = static_cast<uint32_t>(args.asNumber());
    args.expectEnd();

    printf("%s VLAN with ID %u...\n",
           action == Action::add ? "Adding" : "Removing", id);

    try
    {
        if (action == Action::add)
        {
            bus.call(Dbus::objectRoot, Dbus::vlanCreateInterface,
                    Dbus::vlanCreateMethod, iface, id);
        }
        else
        {
            const std::string object =
                Dbus::ethToPath(iface) + '_' + std::to_string(id);
            bus.call(object.c_str(), Dbus::deleteInterface, Dbus::deleteMethod);
        }
    }
    catch(const std::exception& e)
    {
        if ((action == Action::del) &&
            (strstr(e.what(), "org.freedesktop.DBus.Error.UnknownObject") != NULL))
        {
            puts("Can't delete a nonexistent interface.");
        }
        else
        {
            puts(e.what());
        }
    }

    puts(completeMessage);
}

// clang-format off
/** @brief List of command descriptions. */
static const Command commands[] = {
    {"show", nullptr, "Show current configuration", cmdShow},
    {"reset", nullptr, "Reset configuration to factory defaults", cmdReset},
    {"mac", "{INTERFACE} MAC", "Set MAC address", cmdMac},
    {"hostname", "NAME", "Set host name", cmdHostname},
    {"gateway", "IP", "Set default gateway", cmdGateway},
    {"ip", "{INTERFACE} {add|del} IP[/MASK]", "Add or remove static IP address (default mask: IPv4/24, IPv6/64)", cmdIp},
    {"dhcp", "{INTERFACE} {enable|disable}", "Enable or disable DHCP client", cmdDhcp},
    {"dhcpcfg", "{enable|disable} {dns|ntp}", "Enable or disable DHCP features", cmdDhcpcfg},
    {"dns", "{INTERFACE} {add|del} IP [IP..]", "Add or remove DNS server", cmdDns},
    {"ntp", "{INTERFACE} {add|del} ADDR [ADDR..]", "Add or remove NTP server", cmdNtp},
    {"vlan", "{add|del} {INTERFACE} ID", "Add or remove VLAN", cmdVlan},
};
// clang-format on

void execute(Arguments& args)
{
    const char* cmdName = args.asText();

    for (const auto& cmd : commands)
    {
        if (strcmp(cmdName, cmd.name) == 0)
        {
            Dbus bus;
            cmd.fn(bus, args);
            return;
        }
    }

    std::string err = "Invalid command: ";
    err += cmdName;
    throw std::invalid_argument(err);
}

void help(bool cli_mode, const char* app, Arguments& args)
{
    const char* helpForCmd = args.peek();
    if (helpForCmd)
    {
        const Command* cmdEntry = nullptr;
        for (const auto& cmd : commands)
        {
            if (strcmp(helpForCmd, cmd.name) == 0)
            {
                cmdEntry = &cmd;
                break;
            }
        }
        if (!cmdEntry)
        {
            std::string err = helpForCmd;
            err += " is not a valid command, try --help option";
            throw std::invalid_argument(err);
        }
        puts(cmdEntry->help);

        // Command-specific help should not include the name of the command
        // in CLI mode as in that mode it will be passed in as part of `app`
        // and may differ from the actual command being processed, e.g.
        // CLI command `bmc datetime ntpconfig` equals `netconfig ntp`,
        // and the help must pretend it's the CLI command.
        printf("%s ", app);
        if (!cli_mode)
        {
            printf("%s ", cmdEntry->name);
        }
        printf("%s\n", cmdEntry->fmt ? cmdEntry->fmt : "");
    }
    else
    {
        for (const auto& cmd : commands)
        {
            printf("  %-10s %s\n", cmd.name, cmd.help);
            if (cmd.fmt)
            {
                printf("  %-10s Command format: %s %s\n", "", cmd.name,
                       cmd.fmt);
            }
            printf("\n");
        }
    }
}
