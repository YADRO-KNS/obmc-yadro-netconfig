// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include "dbus.hpp"

/**
 * @class Show
 * @brief Prints Network configuration.
 */
class Show
{
  public:
    /**
     * @brief Constructor.
     *
     * @param[in] bus D-Bus instance
     */
    Show(Dbus& bus);

    /**
     * @brief Print current network configuration.
     */
    void print();

  private:
    /**
     * @brief Print network interface properties.
     *
     * @param[in] obj path to D-Bus network object
     */
    void printInterface(const char* obj);

    /**
     * @brief Print property value (fully customized)
     *
     * @param[in] title property title
     * @param[in] name property name
     * @param[in] properties array of properties
     * @param[in] boolVals a pair of string representations for false/true
     * @param[in] strMap a mapping for string-typed properties
     */
    void
        printProperty(const char* title, const char* name,
                      const Dbus::Properties& properties,
                      const std::pair<const char*, const char*>& boolVals,
                      const std::map<std::string, std::string>& strMap) const;
    /**
     * @brief Print property value (customized bools)
     *
     * @param[in] title property title
     * @param[in] name property name
     * @param[in] properties array of properties
     * @param[in] boolVals a pair of string representations for false/true
     */
    void
        printProperty(const char* title, const char* name,
                      const Dbus::Properties& properties,
                      const std::pair<const char*, const char*>& boolVals) const;

    /**
     * @brief Print property value (customized strings)
     *
     * @param[in] title property title
     * @param[in] name property name
     * @param[in] properties array of properties
     * @param[in] strMap a mapping for string-typed properties
     */
    void
        printProperty(const char* title, const char* name,
                      const Dbus::Properties& properties,
                      const std::map<std::string, std::string>& strMap) const;

    /**
     * @brief Print property value (bools and strings as is)
     *
     * @param[in] title property title
     * @param[in] name property name
     * @param[in] properties array of properties
     */
    void printProperty(const char* title, const char* name,
                       const Dbus::Properties& properties) const;

    /**
     * @brief Format and print property.
     *
     * @param[in] name param name
     * @param[in] value param value
     */
    void printProperty(const char* name, const char* value) const;

    /**
     * @brief Get properties defined by interface for specified object.
     *        If object or interface were not found - returns empty set,
     *        this case will handled by printPropert() as 'N/A' values.
     *
     * @param[in] obj path to D-Bus network object
     * @param[in] iface interface name
     *
     * @return properties
     */
    const Dbus::Properties& getProperties(const char* obj,
                                          const char* iface) const;

  private:
    /** @brief D-Bus connection. */
    Dbus& bus;
    /** @brief Array of D-Bus network configuration objects. */
    Dbus::ManagedObject netObjects;
};
