// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <sdbusplus/bus.hpp>

/**
 * @class Dbus
 * @brief D-Bus warpper to work with Network configuration interfaces.
 */
class Dbus
{
  public:
    // Default ethernet interface used to manipulate wth DNS/NTP servers
    static constexpr const char* defaultEth = "eth0";

    // Network service name
    static constexpr const char* networkService = "xyz.openbmc_project.Network";

    // Objects (paths to them)
    static constexpr const char* objectRoot = "/xyz/openbmc_project/network";
    static constexpr const char* objectConfig =
        "/xyz/openbmc_project/network/config";
    static constexpr const char* objectDhcp =
        "/xyz/openbmc_project/network/config/dhcp";

    // System Configuration interface, its methods and properties
    static constexpr const char* syscfgInterface =
        "xyz.openbmc_project.Network.SystemConfiguration";
    static constexpr const char* syscfgHostname = "HostName";
    static constexpr const char* syscfgDefGw4 = "DefaultGateway";
    static constexpr const char* syscfgDefGw6 = "DefaultGateway6";

    // DHCP interface, its methods and properties
    static constexpr const char* dhcpInterface =
        "xyz.openbmc_project.Network.DHCPConfiguration";
    static constexpr const char* dhcpDnsEnabled = "DNSEnabled";
    static constexpr const char* dhcpNtpEnabled = "NTPEnabled";

    // MAC interface, its methods and properties
    static constexpr const char* macInterface =
        "xyz.openbmc_project.Network.MACAddress";
    static constexpr const char* macSet = "MACAddress";

    // Ethernet interface, its methods and properties
    static constexpr const char* ethInterface =
        "xyz.openbmc_project.Network.EthernetInterface";
    static constexpr const char* ethName = "InterfaceName";
    static constexpr const char* ethDhcpEnabled = "DHCPEnabled";
    static constexpr const char* ethNtpServers = "NTPServers";
    static constexpr const char* ethNameServers = "Nameservers";
    static constexpr const char* ethStNameServers = "StaticNameServers";

    // VLAN interface, its methods and properties
    static constexpr const char* vlanInterface =
        "xyz.openbmc_project.Network.VLAN";
    static constexpr const char* vlanId = "Id";
    static constexpr const char* vlanCreateInterface =
        "xyz.openbmc_project.Network.VLAN.Create";
    static constexpr const char* vlanCreateMethod = "VLAN";

    // IP interface, its methods and properties
    static constexpr const char* ipCreateInterface =
        "xyz.openbmc_project.Network.IP.Create";
    static constexpr const char* ipCreateMethod = "IP";
    static constexpr const char* ipInterface = "xyz.openbmc_project.Network.IP";
    static constexpr const char* ipAddress = "Address";
    static constexpr const char* ipGateway = "Gateway";
    static constexpr const char* ipPrefix = "PrefixLength";

    // IP version interfaces
    static constexpr const char* ip4Interface =
        "xyz.openbmc_project.Network.IP.Protocol.IPv4";
    static constexpr const char* ip6Interface =
        "xyz.openbmc_project.Network.IP.Protocol.IPv6";

    // "Delete" interface, its methods and properties
    static constexpr const char* deleteInterface =
        "xyz.openbmc_project.Object.Delete";
    static constexpr const char* deleteMethod = "Delete";

    // "Reset" interface, its methods and properties
    static constexpr const char* resetInterface =
        "xyz.openbmc_project.Common.FactoryReset";
    static constexpr const char* resetMethod = "Reset";

    // Properties interface, its methods and properties
    static constexpr const char* propertiesInterface =
        "org.freedesktop.DBus.Properties";
    static constexpr const char* propertiesGet = "Get";
    static constexpr const char* propertiesSet = "Set";

    // Object manager interface, its methods and typedefs
    static constexpr const char* objmgrInterface =
        "org.freedesktop.DBus.ObjectManager";
    static constexpr const char* objmgrGet = "GetManagedObjects";
    using PropertyValue = std::variant<uint8_t, uint32_t, bool, std::string,
                                       std::vector<std::string>>;
    using Properties = std::map<std::string, PropertyValue>;
    using ManagedObject = std::map<sdbusplus::message::object_path,
                                   std::map<std::string, Properties>>;

    /** @brief Constructor. */
    Dbus();

    /**
     * @brief Call network manager's method via D-Bus.
     *
     * @param[in] object D-Bus object path
     * @param[in] interface interface name
     * @param[in] name method name
     * @param[in] ... method parameters
     *
     * @throw std::exception in case of errors
     *
     * @return response message
     */
    template <typename... T>
    auto call(const char* object, const char* interface, const char* name,
              T&&... args)
    {
        auto mcall =
            bus.new_method_call(Dbus::networkService, object, interface, name);
        mcall.append(std::forward<T>(args)...);
        return bus.call(mcall);
    }

    /**
     * @brief Get property value.
     *
     * @param[in] object path to D-Bus object
     * @param[in] interface property's interface name
     * @param[in] name property's name
     *
     * @throw std::exception in case of errors
     *
     * @return property's value
     */
    template <typename T>
    T get(const char* object, const char* interface, const char* name)
    {
        std::variant<T> value;
        call(object, propertiesInterface, propertiesGet, interface, name)
            .read(value);
        return std::get<T>(value);
    }

    /**
     * @brief Set property value.
     *
     * @param[in] object path to D-Bus object
     * @param[in] interface property's interface name
     * @param[in] name property's name
     * @param[in] value value to set
     *
     * @throw std::exception in case of errors
     */
    template <typename T>
    void set(const char* object, const char* interface, const char* name,
             const T& value)
    {
        call(object, propertiesInterface, propertiesSet, interface, name,
             std::variant<T>(value));
    }

    /**
     * @brief Append string value to property array.
     *
     * @param[in] object path to D-Bus object
     * @param[in] interface property's interface name
     * @param[in] name property's name
     * @param[in] value value to add
     *
     * @throw std::invalid_arguments if value already exists
     * @throw std::exception in case of errors
     */
    void append(const char* object, const char* interface, const char* name,
                const char* value);

    /**
     * @brief Remove string value from property array.
     *
     * @param[in] object path to D-Bus object
     * @param[in] interface property's interface name
     * @param[in] name property's name
     * @param[in] value value to remove
     *
     * @throw std::invalid_arguments if value is not exist
     * @throw std::exception in case of errors
     */
    void remove(const char* object, const char* interface, const char* name,
                const char* value);

    /**
     * @struct IpAddress
     * @brief Description of IP address.
     */
    struct IpAddress
    {
        /** @brief D-Bus path to the IP object. */
        std::string object;
        /** @brief IP address. */
        std::string address;
        /** @brief Mask bits. */
        uint8_t mask;
        /** @brief Gateway IP. */
        std::string gateway;
    };

    /**
     * @brief Get list of IP addresses for specified Ethernet object.
     *
     * @param[in] ethObject path to Ethernet object (eth0 or VLAN)
     *
     * @return array with IP addresses description
     */
    std::vector<IpAddress> getAddresses(const char* ethObject);

    /**
     * @brief Convert network interface name to its D-Bus object path.
     *
     * @param[in] name network interface name
     *
     * @return path to D-Bus object
     */
    static std::string ethToPath(const char* name);

  private:
    /** @brief D-Bus connection. */
    sdbusplus::bus::bus bus;
};
