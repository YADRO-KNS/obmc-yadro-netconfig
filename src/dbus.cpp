// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "dbus.hpp"

#include <stdexcept>

Dbus::Dbus() : bus(sdbusplus::bus::new_default())
{}

void Dbus::append(const char* object, const char* interface, const char* name,
                  const char* value)
{
    auto array = get<std::vector<std::string>>(object, interface, name);
    const auto it = std::find(array.begin(), array.end(), value);
    if (it != array.end())
    {
        std::string err = "Value ";
        err += value;
        err += " already exists";
        throw std::invalid_argument(err);
    }
    array.push_back(value);
    set(object, interface, name, array);
}

void Dbus::remove(const char* object, const char* interface, const char* name,
                  const char* value)
{
    auto array = get<std::vector<std::string>>(object, interface, name);
    const auto it = std::find(array.begin(), array.end(), value);
    if (it == array.end())
    {
        std::string err = "Value ";
        err += value;
        err += " not found";
        throw std::invalid_argument(err);
    }
    array.erase(it);
    set(object, interface, name, array);
}

std::vector<Dbus::IpAddress> Dbus::getAddresses(const char* ethObject)
{
    std::vector<IpAddress> addresses;

    std::string pathPrefix = ethObject;
    pathPrefix += "/ip";

    Dbus::ManagedObject objects;
    call(objectRoot, objmgrInterface, objmgrGet).read(objects);

    for (const auto& it : objects)
    {
        const std::string& path = it.first;
        if (path.compare(0, pathPrefix.length(), pathPrefix) == 0)
        {
            const auto ip = it.second.find(Dbus::ipInterface);
            if (ip != it.second.end())
            {
                const auto& ipProperties = ip->second;
                IpAddress addr = {
                    path,
                    std::get<std::string>(ipProperties.find(ipAddress)->second),
                    std::get<uint8_t>(ipProperties.find(ipPrefix)->second),
                    std::get<std::string>(ipProperties.find(ipGateway)->second),
                };
                addresses.emplace_back(addr);
            }
        }
    }

    return addresses;
}

std::string Dbus::ethToPath(const char* name)
{
    std::string dbusName = name;
    std::replace(dbusName.begin(), dbusName.end(), '.', '_');
    return std::string(objectRoot) + '/' + dbusName;
}
