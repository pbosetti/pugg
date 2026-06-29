#pragma once
#include <string>

namespace pugg
{

class Driver
{
public:
  Driver(std::string server_name, std::string name, int version)
      : _name(std::move(name)), _server_name(std::move(server_name)), _version(version)
  {
  }
  virtual ~Driver() = default;

  [[nodiscard]] const std::string &server_name() const { return _server_name; }
  [[nodiscard]] const std::string &name() const { return _name; }
  [[nodiscard]] int version() const { return _version; }

private:
  std::string _name;
  std::string _server_name;
  int _version;
};

} // namespace pugg
