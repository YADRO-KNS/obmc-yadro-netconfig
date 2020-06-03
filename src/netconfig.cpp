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

/**
 * @brief Construct object path for network interface from optional argument.
 *
 * @param[in] args command arguments
 *
 * @return path to D-Bus object: eth0 or VLAN's
 */
static std::string pathFromOptional(Arguments& args)
{
    std::string objectPath;

    const char* nextArg = args.peek();
    if (nextArg && Arguments::isDigit(nextArg))
    {
        const size_t vlanId = args.asDigit();
        objectPath = Dbus::vlanObject(static_cast<uint32_t>(vlanId));
    }
    else
    {
        objectPath = Dbus::objectEth0;
    }

    return objectPath;
}

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

/** @brief Set MAC address for main network interface (eth0): `mac MAC` */
static void cmdMac(Dbus& bus, Arguments& args)
{
    const char* mac = args.asMacAddress();
    args.expectEnd();

    printf("Set new MAC address %s...\n", mac);
    bus.set(Dbus::objectEth0, Dbus::macInterface, Dbus::macSet, mac);
    puts(completeMessage);
}

/** @brief Set BMC host name: `hostname NAME` */
static void cmdHostname(Dbus& bus, Arguments& args)
{
    const char* name = args.asText();
    args.expectEnd();

    printf("Set new host name %s...\n", name);
    bus.set(Dbus::objectConfig, Dbus::syscfgInterface, Dbus::syscfgHostname,
            name);
    puts(completeMessage);
}

/** @brief Add/remove BMC dimain: `domain [VLANID] {add|del} NAME` */
static void cmdDomain(Dbus& bus, Arguments& args)
{
    const std::string object = pathFromOptional(args);
    const Action action = args.asAction();
    const char* domain = args.asText();
    args.expectEnd();

    printf("%s domain name %s...\n",
           action == Action::add ? "Adding" : "Removing", domain);

    if (action == Action::add)
    {
        bus.append(object.c_str(), Dbus::ethInterface, Dbus::ethDomainName,
                   domain);
    }
    else
    {
        bus.remove(object.c_str(), Dbus::ethInterface, Dbus::ethDomainName,
                   domain);
    }

    puts(completeMessage);
}

/** @brief Set default gateway: `getway IP` */
static void cmdGateway(Dbus& bus, Arguments& args)
{
    const auto [ver, ip] = args.asIpAddress();
    args.expectEnd();

    printf("Setting default gateway for IPv%i to %s...\n",
           static_cast<int>(ver), ip);

    const char* property =
        ver == IpVer::v4 ? Dbus::syscfgDefGw4 : Dbus::syscfgDefGw6;

    bus.set(Dbus::objectConfig, Dbus::syscfgInterface, property, ip);

    puts(completeMessage);
}

/** @brief Add/remove IP: `ip [VLANID] {add|del} IP[/MASK GATEWAY]` */
static void cmdIp(Dbus& bus, Arguments& args)
{
    const std::string object = pathFromOptional(args);
    const Action action = args.asAction();

    if (action == Action::add)
    {
        const auto [ipVer, ip, mask] = args.asIpAddrMask();
        const auto [gwVer, gwIp] = args.asIpAddress();
        args.expectEnd();
        if (ipVer != gwVer)
        {
            throw std::invalid_argument("IP version mismatch");
        }

        const char* ipInterface =
            ipVer == IpVer::v4 ? Dbus::ip4Interface : Dbus::ip6Interface;

        bus.call(object.c_str(), Dbus::ipCreateInterface, Dbus::ipCreateMethod,
                 ipInterface, ip, mask, gwIp);
    }
    else
    {
        const auto [_, ip] = args.asIpAddress();
        args.expectEnd();

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
    }

    puts(completeMessage);
}

/** @brief Enable/disable DHCP client: 'dhcp [VLANID] {enable|disable}` */
static void cmdDhcp(Dbus& bus, Arguments& args)
{
    const std::string object = pathFromOptional(args);
    const Toggle toggle = args.asToggle();
    args.expectEnd();

    const bool enable = toggle == Toggle::enable;
    printf("%s DHCP client...\n", enable ? "Enable" : "Disable");

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

/** @brief Add/remove DNS server: `dns [VLANID] {add|del} [static] IP` */
static void cmdDns(Dbus& bus, Arguments& args)
{
    const std::string object = pathFromOptional(args);
    const Action action = args.asAction();

    const char* nextArg = args.peek();
    const bool isStatic = nextArg && strcmp(nextArg, "static") == 0;
    const char* property;
    if (isStatic)
    {
        ++args;
        property = Dbus::ethStNameServers;
    }
    else
    {
        property = Dbus::ethNameServers;
    }

    const auto [_, srv] = args.asIpAddress();
    args.expectEnd();

    printf("%s DNS server %s...\n",
           action == Action::add ? "Adding" : "Removing", srv);

    if (action == Action::add)
    {
        bus.append(object.c_str(), Dbus::ethInterface, property, srv);
    }
    else
    {
        bus.remove(object.c_str(), Dbus::ethInterface, property, srv);
    }

    puts(completeMessage);
}

/** @brief Add/remove NTP server: `ntp [VLANID] {add|del} IP` */
static void cmdNtp(Dbus& bus, Arguments& args)
{
    const std::string object = pathFromOptional(args);
    const Action action = args.asAction();
    const char* srv = args.asText();
    args.expectEnd();

    printf("%s NTP server %s...\n",
           action == Action::add ? "Adding" : "Removing", srv);

    if (action == Action::add)
    {
        bus.append(object.c_str(), Dbus::ethInterface, Dbus::ethNtpServers,
                   srv);
    }
    else
    {
        bus.remove(object.c_str(), Dbus::ethInterface, Dbus::ethNtpServers,
                   srv);
    }

    puts(completeMessage);
}

/** @brief Add/remove VLAN: `vlan {add|del} ID` */
static void cmdVlan(Dbus& bus, Arguments& args)
{
    const Action action = args.asAction();
    const uint32_t id = static_cast<uint32_t>(args.asDigit());
    args.expectEnd();

    printf("%s VLAN with ID %u...\n",
           action == Action::add ? "Adding" : "Removing", id);

    if (action == Action::add)
    {
        bus.call(Dbus::objectRoot, Dbus::vlanCreateInterface,
                 Dbus::vlanCreateMethod, Dbus::eth0, id);
    }
    else
    {
        const std::string object = bus.vlanObject(id);
        bus.call(object.c_str(), Dbus::deleteInterface, Dbus::deleteMethod);
    }

    puts(completeMessage);
}

// clang-format off
/** @brief List of command descriptions. */
static const Command commands[] = {
    {"show", nullptr, "Show current configuration", cmdShow},
    {"reset", nullptr, "Reset configuration to factory defaults", cmdReset},
    {"mac", "MAC", "Set MAC address", cmdMac},
    {"hostname", "NAME", "Set host name", cmdHostname},
    {"domain", "[VLANID] {add|del} NAME", "Add or remove domain name", cmdDomain},
    {"gateway", "IP", "Set default gateway", cmdGateway},
    {"ip", "[VLANID] {add|del} IP[/MASK GATEWAY]", "Add or remove static IP address", cmdIp},
    {"dhcp", "[VLANID] {enable|disable}", "Enable or disable DHCP client", cmdDhcp},
    {"dhcpcfg", "{enable|disable} {dns|ntp}", "Enable or disable DHCP features", cmdDhcpcfg},
    {"dns", "[VLANID] {add|del} [static] IP", "Add or remove DNS server", cmdDns},
    {"ntp", "[VLANID] {add|del} IP", "Add or remove NTP server", cmdNtp},
    {"vlan", "{add|del} ID", "Add or remove VLAN", cmdVlan},
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

void help(Arguments& args)
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
        printf("%s %s\n", cmdEntry->name, cmdEntry->fmt ? cmdEntry->fmt : "");
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
