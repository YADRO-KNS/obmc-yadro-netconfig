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
     * @brief Print property value.
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

  private:
    /** @brief D-Bus connection. */
    Dbus& bus;
    /** @brief Array of D-Bus network configuration objects. */
    Dbus::ManagedObject netObjects;
};
