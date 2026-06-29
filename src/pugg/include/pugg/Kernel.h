#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Driver.h"
#include "Plugin.h"

namespace pugg
{
namespace detail
{

struct Server
{
  std::string name;
  int min_driver_version{0};
  std::map<std::string, std::unique_ptr<Driver>> drivers;
};

} // namespace detail

class Kernel
{
public:
  void add_server(std::string name, int min_driver_version)
  {
    detail::Server server;
    server.name = name;
    server.min_driver_version = min_driver_version;
    _servers[std::move(name)] = std::move(server);
  }

  template <class T>
  void add_server()
  {
    add_server(T::server_name(), T::version);
  }

  // Takes ownership of driver; returns false (and deletes driver) if it cannot be registered.
  [[nodiscard]] bool add_driver(std::unique_ptr<Driver> driver)
  {
    if (!driver)
      return false;

    auto server_iter = _servers.find(driver->server_name());
    if (server_iter == _servers.end())
      return false;

    if (server_iter->second.min_driver_version > driver->version())
      return false;

    server_iter->second.drivers[driver->name()] = std::move(driver);
    return true;
  }

  template <class DriverType>
  [[nodiscard]] DriverType *get_driver(const std::string &server_name, const std::string &name)
  {
    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return nullptr;

    auto driver_iter = server_iter->second.drivers.find(name);
    if (driver_iter == server_iter->second.drivers.end())
      return nullptr;

    return dynamic_cast<DriverType *>(driver_iter->second.get());
  }

  template <class DriverType>
  [[nodiscard]] std::vector<DriverType *> get_all_drivers(const std::string &server_name)
  {
    std::vector<DriverType *> drivers;

    auto server_iter = _servers.find(server_name);
    if (server_iter == _servers.end())
      return drivers;

    for (auto &[key, driver_ptr] : server_iter->second.drivers)
    {
      if (auto *typed = dynamic_cast<DriverType *>(driver_ptr.get()))
        drivers.push_back(typed);
    }
    return drivers;
  }

  [[nodiscard]] bool load_plugin(const std::string &filename)
  {
    using fnRegisterPlugin = void(pugg::Kernel *);

    auto dllHandle = detail::DllHandle{detail::loadDll(filename)};
    if (!dllHandle.isValid())
      return false;

    auto *registerFunc = dllHandle.getFunction<fnRegisterPlugin>("register_pugg_plugin");
    if (registerFunc)
    {
      registerFunc(this);
      _plugins.push_back(std::move(dllHandle));
      return true;
    }

    return false;
  }

  void clear_drivers()
  {
    for (auto &[name, server] : _servers)
      server.drivers.clear();
  }

  void clear()
  {
    // Destroy Driver objects before unloading DLLs — member destruction
    // order (reverse of declaration) ensures _servers goes before _plugins,
    // but clear() must replicate that same sequence explicitly.
    _servers.clear();
    _plugins.clear();
  }

protected:
  // Declaration order matters: _plugins must outlive _servers so that Driver
  // vtables (which live in the DLLs) remain valid during Server/Driver destruction.
  std::vector<detail::DllHandle> _plugins;
  std::map<std::string, detail::Server> _servers;
};

} // namespace pugg
