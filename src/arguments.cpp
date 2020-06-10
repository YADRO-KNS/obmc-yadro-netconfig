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
#include <stdexcept>

/** @brief Max length of a numeric value. */
static constexpr size_t maxNumericLen = 10;

Arguments::Arguments(int argc, char* argv[]) : index(0), args(argv, argv + argc)
{}

Arguments& Arguments::operator++()
{
    if (index == args.size())
    {
        throw std::invalid_argument("Not enough arguments");
    }
    ++index;
    return *this;
}

void Arguments::expectEnd() const
{
    if (peek())
    {
        std::string err = "Unexpected arguments: ";
        err += args.at(index);
        throw std::invalid_argument(err);
    }
}

const char* Arguments::peek() const
{
    return index < args.size() ? args.at(index) : nullptr;
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

std::tuple<IpVer, const char*> Arguments::asIpAddress()
{
    const char* arg = asText();
    const std::optional<IpVer> ver = isIpAddress(arg);
    if (!ver.has_value())
    {
        std::string err = "Invalid IP address: ";
        err += arg;
        err += ", expected IPv4 or IPv6 address";
        throw std::invalid_argument(err);
    }
    return std::make_tuple(ver.value(), arg);
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
            const std::optional<IpVer> ver = isIpAddress(addr.c_str());
            if (ver.has_value())
            {
                constexpr size_t ip4MaxMask = 32;
                constexpr size_t ip6MaxMask = 64;
                const size_t mask = strtoul(maskText, nullptr, 0);
                if (mask && ((ver == IpVer::v4 && mask <= ip4MaxMask) ||
                             (ver == IpVer::v6 && mask <= ip6MaxMask)))
                {
                    return std::make_tuple(ver.value(), addr, mask);
                }
            }
        }
    }
    std::string err = "Invalid argument: ";
    err += arg;
    err += ", expected IP/MASK (e.g. 10.0.0.1/8)";
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

std::optional<IpVer> Arguments::isIpAddress(const char* arg)
{
    if (arg)
    {
        in_addr dummy4;
        if (inet_pton(AF_INET, arg, &dummy4) == 1)
        {
            return IpVer::v4;
        }

        in6_addr dummy6;
        if (inet_pton(AF_INET6, arg, &dummy6) == 1)
        {
            return IpVer::v6;
        }
    }

    return std::nullopt;
}
