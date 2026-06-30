#pragma once
#include <string>

/// @file Driver.h
/// @brief Base class for plugin drivers registered with a pugg::Kernel.

namespace pugg
{

/// @brief Base class for a plugin-provided implementation of a server interface.
///
/// A Driver is the concrete object a plugin registers with a Kernel. Each driver
/// declares which server it implements (server_name()), its own identifying
/// name() within that server, and a version() used for compatibility checks
/// against the server's minimum required driver version.
class Driver
{
public:
  /// @brief Construct a driver.
  /// @param server_name Name of the server interface this driver implements.
  /// @param name Unique name identifying this driver within its server.
  /// @param version Driver version, checked against the server's minimum required version.
  Driver(std::string server_name, std::string name, int version)
      : _name(std::move(name)), _server_name(std::move(server_name)), _version(version)
  {
  }
  virtual ~Driver() = default;

  /// @brief Name of the server interface this driver implements.
  [[nodiscard]] const std::string &server_name() const { return _server_name; }
  /// @brief Unique name identifying this driver within its server.
  [[nodiscard]] const std::string &name() const { return _name; }
  /// @brief Driver version, checked against the server's minimum required version.
  [[nodiscard]] int version() const { return _version; }

private:
  std::string _name;
  std::string _server_name;
  int _version;
};

} // namespace pugg
