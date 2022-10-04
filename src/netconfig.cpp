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
    bus.call(Dbus::networkService, Dbus::objectRoot, Dbus::resetInterface,
             Dbus::resetMethod);
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
    bus.set(Dbus::networkService, object.c_str(), Dbus::macInterface,
            Dbus::macSet, mac);
    puts(completeMessage);
}

/** @brief Set BMC host name: `hostname NAME` */
static void cmdHostname(Dbus& bus, Arguments& args)
{
    std::string name = args.asIpOrFQDN();
    args.expectEnd();

    printf("Set new host name %s...\n", name.c_str());
    bus.set(Dbus::networkService, Dbus::objectConfig, Dbus::syscfgInterface,
            Dbus::syscfgHostname, name);
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

    bus.set(Dbus::networkService, Dbus::objectConfig, Dbus::syscfgInterface,
            property, ip);

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

        bus.call(Dbus::networkService, object.c_str(), Dbus::ipCreateInterface,
                 Dbus::ipCreateMethod, ipInterface, ip, mask, "");

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
                bus.call(Dbus::networkService, it.object.c_str(),
                         Dbus::deleteInterface, Dbus::deleteMethod);
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

    bus.set(Dbus::networkService, object.c_str(), Dbus::ethInterface,
            Dbus::ethDhcpEnabled, enable);

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
        bus.set(Dbus::networkService, Dbus::objectDhcp, Dbus::dhcpInterface,
                Dbus::dhcpDnsEnabled, enable);
    }
    else
    {
        printf("%s NTP over DHCP...\n", enable ? "Enable" : "Disable");
        bus.set(Dbus::networkService, Dbus::objectDhcp, Dbus::dhcpInterface,
                Dbus::dhcpNtpEnabled, enable);
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
        bus.append(Dbus::networkService, object.c_str(), Dbus::ethInterface,
                   Dbus::ethStNameServers, servers);
    }
    else
    {
        bus.remove(Dbus::networkService, object.c_str(), Dbus::ethInterface,
                   Dbus::ethStNameServers, servers);
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
        bus.append(Dbus::networkService, object.c_str(), Dbus::ethInterface,
                   Dbus::ethNtpServers, servers);
    }
    else
    {
        bus.remove(Dbus::networkService, object.c_str(), Dbus::ethInterface,
                   Dbus::ethNtpServers, servers);
    }

    puts(completeMessage);
}

/** @brief Check VLAN ID for IEEE 802.1Q conformance */
static void checkVlanId(const uint32_t id)
{
    if ((id < minVlanId) || (id > maxVlanId))
    {
        std::string err = "Invalid VLAN ID. Must be [2 - 4094], see IEEE 802.1Q.";
        throw std::invalid_argument(err);
    }
}

/** @brief Add/remove VLAN: `vlan {add|del} {INTERFACE} ID` */
static void cmdVlan(Dbus& bus, Arguments& args)
{
    const Action action = args.asAction();
    const char* iface = args.asNetInterface();
    const uint32_t id = static_cast<uint32_t>(args.asNumber());
    args.expectEnd();

    checkVlanId(id);

    printf("%s VLAN with ID %u...\n",
           action == Action::add ? "Adding" : "Removing", id);

    try
    {
        if (action == Action::add)
        {
            bus.call(Dbus::networkService, Dbus::objectRoot,
                     Dbus::vlanCreateInterface, Dbus::vlanCreateMethod, iface,
                     id);
        }
        else
        {
            const std::string object =
                Dbus::ethToPath(iface) + '_' + std::to_string(id);
            bus.call(Dbus::networkService, object.c_str(),
                     Dbus::deleteInterface, Dbus::deleteMethod);
        }
    }
    catch(const std::exception& e)
    {
        if ((action == Action::del) &&
            (strstr(e.what(), "org.freedesktop.DBus.Error.UnknownObject") != NULL))
        {
            puts("Can't delete a nonexistent interface.");
        }
        throw;
    }

    puts(completeMessage);
}

static void cmdSyslogSet(Dbus& bus, Arguments& args)
{
    const auto [addr, port] = args.parseAddrAndPort();
    if (args.peek() != nullptr)
    {
        args.asText();
        args.expectEnd();
    }

    printf("Set remote syslog server %s:%u...\n", addr.c_str(), port);
    bus.set(Dbus::syslogService, Dbus::objectSyslog, Dbus::syslogInterface,
            Dbus::syslogAddr, addr);
    bus.set(Dbus::syslogService, Dbus::objectSyslog, Dbus::syslogInterface,
            Dbus::syslogPort, port);
    puts(completeMessage);
}

static void cmdSyslogReset(Dbus& bus, Arguments& args)
{
    std::string addr{""};
    unsigned short port{0};
    args.expectEnd();
    bus.set(Dbus::syslogService, Dbus::objectSyslog, Dbus::syslogInterface,
            Dbus::syslogAddr, addr);
    bus.set(Dbus::syslogService, Dbus::objectSyslog, Dbus::syslogInterface,
            Dbus::syslogPort, port);
    puts(completeMessage);
}

static void cmdSyslogShow(Dbus& bus, Arguments& args)
{
    args.expectEnd();
    std::string addr =
        bus.get<std::string>(Dbus::syslogService, Dbus::objectSyslog,
                             Dbus::syslogInterface, Dbus::syslogAddr);
    unsigned short port =
        bus.get<unsigned short>(Dbus::syslogService, Dbus::objectSyslog,
                                Dbus::syslogInterface, Dbus::syslogPort);

    printf("Remote syslog server: ");
    if (addr == "" || port == 0)
    {
        printf("(none)\n");
    }
    else if (addr != "")
    {
        printf("%s:%u (tcp)\n", addr.c_str(), port);
    }
}

// clang-format off
/** @brief List of command descriptions. */
static const Command ifconfigCommands[] = {
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

static const Command syslogCommands[] = {
    {"set", "ADDR[:PORT]", "Configure remote syslog server (Address and an optional TCP port (default is 514))", cmdSyslogSet},
    {"reset", nullptr, "Reset syslog settings. Alias for the syslog set command without arguments.", cmdSyslogReset},
    {"show", nullptr, "Show the configured remote syslog server", cmdSyslogShow},
};
// clang-format on

static std::tuple<const Command*, unsigned int>
    getCommandsArray(const char* app)
{
    const Command* cmdArr;
    unsigned int arrSize;

    if (!strcmp(app, cliIfconfig) || !strcmp(app, cliDatetime) ||
        !strcmp(app, rootIfconfig))
    {
        cmdArr = ifconfigCommands;
        arrSize = sizeof(ifconfigCommands) / sizeof(Command);
    }
    else if (!strcmp(app, cliSyslog) || !strcmp(app, rootSyslog))
    {
        cmdArr = syslogCommands;
        arrSize = sizeof(syslogCommands) / sizeof(Command);
    }
    else
    {
        std::string err = "Invalid argument: ";
        err += app;
        throw std::invalid_argument(err);
    }

    return std::make_tuple(cmdArr, arrSize);
}

void execute(const char* app, Arguments& args)
{
    const char* cmdName = args.asText();
    std::tuple<const Command*, unsigned short> cmds = getCommandsArray(app);

    for (const auto* it = std::get<0>(cmds);
         it != std::get<0>(cmds) + std::get<1>(cmds); ++it)
    {
        if (strcmp(cmdName, it->name) == 0)
        {
            Dbus bus;
            it->fn(bus, args);
            return;
        }
    }

    std::string err = "Invalid command: ";
    err += cmdName;
    throw std::invalid_argument(err);
}

void help(CLIMode mode, const char* app, Arguments& args)
{
    const char* helpForCmd = args.peek();
    std::tuple<const Command*, unsigned short> cmds = getCommandsArray(app);
    if (helpForCmd)
    {
        const Command* cmdEntry = nullptr;
        for (const auto* it = std::get<0>(cmds);
             it != std::get<0>(cmds) + std::get<1>(cmds); ++it)
        {
            if (strcmp(helpForCmd, it->name) == 0)
            {
                cmdEntry = it;
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
        if (mode != CLIMode::cliModeNoCommand)
        {
            printf("%s ", cmdEntry->name);
        }
        printf("%s\n", cmdEntry->fmt ? cmdEntry->fmt : "");
    }
    else
    {
        for (const auto* it = std::get<0>(cmds);
             it != std::get<0>(cmds) + std::get<1>(cmds); ++it)
        {
            printf("  %-10s %s\n", it->name, it->help);
            if (it->fmt)
            {
                printf("  %-10s Command format: %s %s\n", "", it->name,
                       it->fmt);
            }
            printf("\n");
        }
    }
}
