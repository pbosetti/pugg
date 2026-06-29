#pragma once

#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace pugg
{

namespace detail
{

#ifdef _WIN32
using HandleType = HMODULE;

inline void freeDll(HandleType handle) { FreeLibrary(handle); }

template <typename FuncType>
[[nodiscard]] auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(GetProcAddress(handle, name.c_str()));
}

[[nodiscard]] inline auto loadDll(const std::string &filename) -> HandleType
{
  return LoadLibraryA(filename.c_str());
}

#else
using HandleType = void *;

inline void freeDll(HandleType handle) { dlclose(handle); }

template <typename FuncType>
[[nodiscard]] auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(dlsym(handle, name.c_str()));
}

[[nodiscard]] inline auto loadDll(const std::string &filename) -> HandleType
{
  return dlopen(filename.c_str(), RTLD_NOW);
}

#endif

class DllHandle
{
public:
  explicit DllHandle(HandleType handle) : _handle(handle) {}

  DllHandle(const DllHandle &) = delete;
  DllHandle &operator=(const DllHandle &) = delete;

  DllHandle(DllHandle &&other) noexcept : _handle(other._handle) { other._handle = nullptr; }

  DllHandle &operator=(DllHandle &&other) noexcept
  {
    if (this != &other)
    {
      if (_handle)
        freeDll(_handle);
      _handle = other._handle;
      other._handle = nullptr;
    }
    return *this;
  }

  ~DllHandle()
  {
    if (_handle)
      freeDll(_handle);
  }

  [[nodiscard]] bool isValid() const { return _handle != nullptr; }

  template <typename FuncType>
  [[nodiscard]] auto getFunction(const std::string &name) const -> FuncType *
  {
    return ::pugg::detail::getFunction<FuncType>(_handle, name);
  }

  [[nodiscard]] auto handle() const -> HandleType { return _handle; }

private:
  HandleType _handle;
};

} // namespace detail
} // namespace pugg
