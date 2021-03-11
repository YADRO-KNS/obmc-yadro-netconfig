// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <vector>

/**
 * @class Action
 * @brief Action type
 */
enum class Action
{
    add,
    del
};

/**
 * @class Toggle
 * @brief Toggle state
 */
enum class Toggle
{
    enable,
    disable
};

/**
 * @class IpVer
 * @brief IP version.
 */
enum class IpVer
{
    v4 = 4,
    v6 = 6
};

/**
 * @class Arguments
 * @brief Arguments parser.
 */
class Arguments
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] argc number of command line arguments
     * @param[in] argv array of command line arguments
     */
    Arguments(int argc, char* argv[]);

    /**
     * @brief Move argument pointer to the next entry.
     *
     * @throw std::invalid_argument if no more arguments are available
     */
    Arguments& operator++();

    /**
     * @brief Check for last argument.
     *
     * @throw std::invalid_argument if current argument is not handled yet
     */
    void expectEnd() const;

    /**
     * @brief Peek current argument.
     *
     * @return argument text or nullptr if no more arguments are available
     */
    const char* peek() const;

    /**
     * @brief Peek next argument.
     *
     * @return argument text or nullptr if no argument is available
     */
    const char* peekNext() const;

    /**
     * @brief Get current argument as pointer to text data.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *
     * @return argument value as text
     */
    const char* asText();

    /**
     * @brief Get current argument as one of expected value.
     *        Argument pointer will be moved to the next entry.
     *
     * @param[in] expected list of expected parameters
     *
     * @throw std::invalid_argument if there are no more arguments to handle or
     *                              argument has unexpected value
     *
     * @return argument value as text
     */
    const char* asOneOf(const std::initializer_list<const char*>& expected);

    /**
     * @brief Get current argument as unsigned number.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not a numeric
     *
     * @return argument value as unsigned number
     */
    size_t asNumber();

    /**
     * @brief Get current argument as action.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not an action
     *
     * @return argument value as action type
     */
    Action asAction();

    /**
     * @brief Get current argument as toggle.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not an toggle
     *
     * @return argument value as toggle state
     */
    Toggle asToggle();

    /**
     * @brief Get current argument as network interface name.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not a valid interface name
     *
     * @return argument value: network interface name
     */
    const char* asNetInterface();

    /**
     * @brief Get current argument as MAC address.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not a valid MAC address
     *
     * @return argument value: MAC address
     */
    const char* asMacAddress();

    /**
     * @brief Get current argument as IP address.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument is not a valid IP address
     *
     * @return argument value: IP version and address
     */
    std::tuple<IpVer, std::string> asIpAddress();

    /**
     * @brief Get current argument as IP address with prefix length.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument has invalid format
     *
     * @return argument value: IP version, address and prefix len as bits count
     */
    std::tuple<IpVer, std::string, uint8_t> asIpAddrMask();

    /**
     * @brief Get current argument as IP address or hostname.
     *        Argument pointer will be moved to the next entry.
     *
     * @throw std::invalid_argument if there are no more arguments to handle
     *                              or argument has invalid format
     *
     * @return IP address in unified format or hostname
     */
    std::string asIpOrFQDN();

    /**
     * @brief Check for unsigned numeric format.
     *
     * @param[in] arg value to check
     *
     * @return false if checked value is not a number
     */
    static bool isNumber(const char* arg);

    /**
     * @brief Trying to parse string as an IP address.
     *
     * @param[in] arg value to parse
     *
     * @throw std::invalid_argument if parsing is failed
     *
     * @return IP version and string with the IP address in unified format.
     */
    static std::tuple<IpVer, std::string> parseIpAddress(const char* arg);

  private:
    using Args = std::vector<char*>;

    /** @brief Array of arguments. */
    Args args;
    /** @brief Current argument. */
    Args::const_iterator current;
};
