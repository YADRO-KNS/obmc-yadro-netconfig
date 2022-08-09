// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "arguments.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/ether.h>
#include <sys/types.h>

#include <algorithm>
#include <cstring>
#include <numeric>
#include <regex>
#include <stdexcept>

/** @brief Max length of a numeric value. */
static constexpr size_t maxNumericLen = 10;

Arguments::Arguments(int argc, char* argv[]) :
    args(argv, argv + argc), current(args.begin())
{}

Arguments& Arguments::operator++()
{
    if (current == args.end())
    {
        throw std::invalid_argument("Not enough arguments");
    }
    ++current;
    return *this;
}

void Arguments::expectEnd() const
{
    if (current != args.end())
    {
        std::string err = "Unexpected arguments: ";
        err += *current;
        throw std::invalid_argument(err);
    }
}

const char* Arguments::peek() const
{
    return current == args.end() ? nullptr : *current;
}

const char* Arguments::peekNext() const
{
    if (peek() && (current + 1) != args.end()) {
        return *(current + 1);
    }

    return nullptr;
}

const char* Arguments::asText()
{
    const char* arg = peek();
    ++(*this);
    return arg;
}

const char*
    Arguments::asOneOf(const std::initializer_list<const char*>& expected)
{
    const char* arg = asText();
    if (std::find_if(expected.begin(), expected.end(), [arg](const char* a) {
            return strcmp(a, arg) == 0;
        }) == expected.end())
    {
        std::string plainList;
        for (const auto& val : expected)
        {
            if (!plainList.empty())
            {
                plainList += ", ";
            }
            plainList += val;
        }
        std::string err = "Invalid action: ";
        err += arg;
        err += ", expected one of [";
        err += plainList;
        err += ']';
        throw std::invalid_argument(err);
    }
    return arg;
}

size_t Arguments::asNumber()
{
    const char* arg = asText();
    if (!isNumber(arg))
    {
        std::string err = "Invalid numeric argument: ";
        err += arg;
        throw std::invalid_argument(err);
    }
    return strtoul(arg, nullptr, 0);
}

Action Arguments::asAction()
{
    const char* add = "add";
    const char* del = "del";
    const char* arg = asOneOf({add, del});
    return strcmp(arg, add) == 0 ? Action::add : Action::del;
}

Toggle Arguments::asToggle()
{
    const char* enable = "enable";
    const char* disable = "disable";
    const char* arg = asOneOf({enable, disable});
    return strcmp(arg, enable) == 0 ? Toggle::enable : Toggle::disable;
}

const char* Arguments::asNetInterface()
{
    const char* arg = asText();

    bool found = false;
    ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == 0)
    {
        for (ifaddrs* ifa = ifaddr; ifa && !found; ifa = ifa->ifa_next)
        {
            found = strcmp(arg, ifa->ifa_name) == 0;
        }
        freeifaddrs(ifaddr);
    }
    if (!found)
    {
        std::string err = "Invalid network interface name: ";
        err += arg;
        throw std::invalid_argument(err);
    }

    return arg;
}

const char* Arguments::asMacAddress()
{
    const char* arg = asText();
    if (!ether_aton(arg))
    {
        std::string err = "Invalid MAC address: ";
        err += arg;
        err += ", expected hex-digits-and-colons notation";
        throw std::invalid_argument(err);
    }
    return arg;
}

std::tuple<IpVer, std::string> Arguments::asIpAddress()
{
    const char* arg = asText();

    try
    {
        return parseIpAddress(arg);
    }
    catch (const std::invalid_argument&)
    {
        std::string err = "Invalid IP address: ";
        err += arg;
        err += ", expected IPv4 or IPv6 address";
        throw std::invalid_argument(err);
    }
}

std::tuple<IpVer, std::string, uint8_t> Arguments::asIpAddrMask()
{
    const char* arg = asText();
    const char* delim = strrchr(arg, '/');
    if (delim)
    {
        const char* maskText = delim + 1;
        if (isNumber(maskText))
        {
            const std::string addr(arg, delim);
            try
            {
                const auto [ver, ip] = parseIpAddress(addr.c_str());

                constexpr size_t ip4MaxPrefix = 32;
                constexpr size_t ip6MaxPrefix = 64;
                const size_t prefix = strtoul(maskText, nullptr, 0);
                if (prefix && ((ver == IpVer::v4 && prefix <= ip4MaxPrefix) ||
                               (ver == IpVer::v6 && prefix <= ip6MaxPrefix)))
                {
                    return std::make_tuple(ver, ip, prefix);
                }
            }
            catch (const std::invalid_argument&)
            {
                // pass
            }
        }
    }
    else
    {
        try
        {
            const auto [ver, ip] = parseIpAddress(arg);
            constexpr uint8_t ip4Prefix = 24;
            constexpr uint8_t ip6Prefix = 64;

            return std::make_tuple(ver, ip,
                                   ver == IpVer::v4 ? ip4Prefix : ip6Prefix);
        }
        catch (const std::invalid_argument&)
        {
            // pass
        }
    }
    std::string err = "Invalid argument: ";
    err += arg;
    err += ", expected IP[/PREFIX] (e.g. 10.0.0.1/8 or 192.168.1.1)";
    throw std::invalid_argument(err);
}

std::string Arguments::asIpOrFQDN(const std::optional<std::string>& param)
{
    const char* arg;
    if (!param)
    {
        arg = asText();
    }
    else
    {
        arg = param.value().c_str();
    }

    try
    {
        auto [_, addr] = parseIpAddress(arg);
        return addr;
    }
    catch (const std::invalid_argument&)
    {
        // pass
    }

    static const std::regex r(
        // According to RFC2181:
        // A full domain name is limited to 255 octets
        // (including separators),
        "(?=^.{1,255}$)"
        "(^"
        // - The length of any single label is limited to 63 octets.
        // - labels must not start or end with hyphens.
        // According to RFC1123 (section 2.1):
        // "...a segment of a host domain name is now allowed
        // to begin with a digit and could legally be entirely numeric".
        // - Total number of labels is limited to 127
        "((?!-)[a-z0-9-]{0,62}[a-z0-9]\\.){0,126}"
        // Trailing dot is optional.
        // According to RFC1738 (section 3.1):
        // "The rightmost domain label will never start with a digit".
        "((?![0-9-])[a-z0-9-]{0,62}[a-z0-9]\\.?)"
        "$)",
        std::regex_constants::icase);

    if (std::regex_match(arg, r))
    {
        return arg;
    }

    std::string err = "Invalid argument: ";
    err += arg;
    err += ", expected IP address or FQDN. ";
    err += "Please, enter IPv4-addresses in dotted-decimal format.";
    throw std::invalid_argument(err);
}

bool Arguments::isNumber(const char* arg)
{
    const size_t len = arg ? strlen(arg) : 0;
    if (!len || len > maxNumericLen)
    {
        return false;
    }
    return std::all_of(arg, arg + len, isdigit);
}

std::tuple<IpVer, std::string> Arguments::parseIpAddress(const char* arg)
{
    if (arg)
    {
        constexpr auto bufferMaxSize = sizeof(struct in6_addr);
        char dummy[bufferMaxSize];
        int family = 0;

        if (inet_pton(AF_INET, arg, dummy) == 1)
        {
            family = AF_INET;
        }
        else if (inet_pton(AF_INET6, arg, dummy) == 1)
        {
            family = AF_INET6;
        }

        if (family != 0)
        {
            std::string ip(INET6_ADDRSTRLEN, '\0');
            if (inet_ntop(family, dummy, ip.data(), ip.size()) != nullptr)
            {
                ip.resize(strlen(ip.c_str()));
                return std::make_tuple(
                    (family == AF_INET ? IpVer::v4 : IpVer::v6), ip);
            }
        }
    }

    std::string err = "Invalid IP address: ";
    err += arg;
    throw std::invalid_argument(err);
}

std::tuple<std::string, unsigned short> Arguments::parseAddrAndPort()
{
    std::tuple<std::string, unsigned short> remoteSrv("", 0);
    const char* arg = peek();
    if (arg != nullptr)
    {
        std::string srv{arg};
        std::string::size_type delim;
        size_t colonCnt = std::count(srv.begin(), srv.end(), ':');
        switch (colonCnt)
        {
            case 0: // IPv4 or FQDN without port
                remoteSrv = setDefaultPort(srv);
                break;
            case 1: // IPv4 or FQDN with port (ADDR:PORT)
                delim = srv.find(":");
                std::get<0>(remoteSrv) = asIpOrFQDN(srv.substr(0, delim));
                std::get<1>(remoteSrv) =
                    parsePortFromString(srv.substr(delim + 1));
                break;
            default: // an IPv6 address contains at least two colons.
                // If the argument is given as IPv6 address with a port,
                // the address should be taken in '[' and ']', i.e.
                // [IPv6-ADDR]:PORT
                if (srv.front() == '[')
                {
                    delim = srv.find("]");
                    std::get<0>(remoteSrv) =
                        asIpOrFQDN(srv.substr(1, delim - 1));
                    std::get<1>(remoteSrv) =
                        parsePortFromString(srv.substr(delim + 2));
                }
                else
                {
                    remoteSrv = setDefaultPort(srv);
                }
                break;
        }
    }
    return remoteSrv;
}

std::tuple<std::string, unsigned short>
    Arguments::setDefaultPort(const std::string& str)
{
    return std::tuple<std::string, unsigned short>(asIpOrFQDN(str),
                                                   syslogDefPort);
}

unsigned short Arguments::parsePortFromString(const std::string& str)
{
    try
    {
        int prt = stoi(str);
        if (prt > 0 && prt <= UINT16_MAX)
        {
            return prt;
        }
    }
    catch (const std::invalid_argument&)
    {
        // pass
    }

    std::string err = "Invalid port number: ";
    err += str;
    err += ", expected an integer in the range 1 - 65535";
    throw std::invalid_argument(err);
}
