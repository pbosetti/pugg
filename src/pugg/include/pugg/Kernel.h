#pragma once
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Driver.h"
#include "Plugin.h"

/// @file Kernel.h
/// @brief The pugg::Kernel class, the central registry that loads plugins and owns drivers.

namespace pugg
{
namespace detail
{

/// @brief A registered server interface and the drivers implementing it.
struct Server
{
  std::string name;                                   ///< Server interface name.
  int min_driver_version{0};                           ///< Minimum driver version accepted by this server.
  std::map<std::string, std::unique_ptr<Driver>> drivers; ///< Registered drivers, keyed by driver name.
};

} // namespace detail

/// @brief Central registry for server interfaces, plugins, and the drivers they provide.
///
/// A Kernel declares which server interfaces it accepts (add_server()), loads plugin
/// shared libraries (load_plugin()) which in turn register Driver instances
/// (add_driver()), and exposes typed lookup of those drivers (get_driver(),
/// get_all_drivers()).
///
/// @note Declaration order of the member variables matters: loaded plugin libraries
/// (_plugins) must outlive the registered servers/drivers (_servers), since a Driver's
/// vtable lives inside its owning DLL. clear() and the destructor both rely on this
/// order to tear things down safely.
class Kernel
{
public:
  /// @brief Register a server interface that plugins can provide drivers for.
  /// @param name Name of the server interface.
  /// @param min_driver_version Minimum driver version accepted for this server;
  /// drivers below this version are rejected by add_driver().
  void add_server(std::string name, int min_driver_version)
  {
    detail::Server server;
    server.name = name;
    server.min_driver_version = min_driver_version;
    _servers[std::move(name)] = std::move(server);
  }

  /// @brief Register a server interface, taking its name and version from the type.
  /// @tparam T Server descriptor type providing a static `server_name()` and a static
  /// `version` member.
  template <class T>
  void add_server()
  {
    add_server(T::server_name(), T::version);
  }

  /// @brief Register a driver with its server.
  ///
  /// Takes ownership of @p driver. Registration fails (and @p driver is destroyed) if
  /// no matching server is registered, or if the driver's version is below the
  /// server's minimum required version.
  /// @param driver Driver instance to register; must not be null.
  /// @return true if the driver was registered, false otherwise.
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

  /// @brief Raw-pointer overload of add_driver(), kept for backwards compatibility.
  ///
  /// Takes ownership of @p driver, deleting it if registration fails.
  /// @param driver Driver instance to register; must not be null.
  /// @return true if the driver was registered, false otherwise.
  [[nodiscard]] bool add_driver(Driver *driver)
  {
    return add_driver(std::unique_ptr<Driver>(driver));
  }

  /// @brief Look up a registered driver by server and driver name, cast to a concrete type.
  /// @tparam DriverType Concrete Driver subclass to cast the result to.
  /// @param server_name Name of the server interface the driver implements.
  /// @param name Name of the driver to look up.
  /// @return Pointer to the driver if found and the dynamic_cast succeeds, nullptr otherwise.
  /// The returned pointer is owned by the Kernel and remains valid until clear(),
  /// clear_drivers(), or the Kernel's destruction.
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

  /// @brief Look up all registered drivers for a server that match a concrete type.
  /// @tparam DriverType Concrete Driver subclass to filter and cast results to.
  /// @param server_name Name of the server interface to look up drivers for.
  /// @return Drivers registered for @p server_name whose dynamic_cast to DriverType
  /// succeeds; empty if the server is unknown or has no matching drivers. Returned
  /// pointers are owned by the Kernel and remain valid until clear(), clear_drivers(),
  /// or the Kernel's destruction.
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

  /// @brief Load a plugin shared library and let it register its drivers with this Kernel.
  ///
  /// Loads @p filename and calls its exported `register_pugg_plugin` function, passing
  /// this Kernel so the plugin can call add_driver(). The loaded library is kept open
  /// for the lifetime of this Kernel (or until clear()).
  /// @param filename Path to the plugin shared library (.so/.dylib/.dll).
  /// @return true if the library was loaded and exposed a valid registration function,
  /// false otherwise.
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

  /// @brief Destroy all registered drivers, keeping registered servers and loaded plugins.
  void clear_drivers()
  {
    for (auto &[name, server] : _servers)
      server.drivers.clear();
  }

  /// @brief Destroy all registered servers/drivers and unload all loaded plugins.
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
