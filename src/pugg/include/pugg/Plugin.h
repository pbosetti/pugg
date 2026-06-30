#pragma once

#include <string>

/// @file Plugin.h
/// @brief Cross-platform dynamic library loading utilities used to load plugins.

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

/// @brief Implementation details not part of pugg's public API.
namespace detail
{

#ifdef _WIN32
/// @brief Native handle type for a loaded dynamic library.
using HandleType = HMODULE;

/// @brief Unload a dynamic library previously loaded with loadDll().
inline void freeDll(HandleType handle) { FreeLibrary(handle); }

/// @brief Look up a symbol in a loaded dynamic library and cast it to a function pointer.
/// @tparam FuncType Function signature to cast the resolved symbol to.
/// @param handle Library handle returned by loadDll().
/// @param name Symbol name to resolve.
/// @return Pointer to the resolved function, or nullptr if not found.
template <typename FuncType>
[[nodiscard]] auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(GetProcAddress(handle, name.c_str()));
}

/// @brief Load a dynamic library from disk.
/// @param filename Path to the library file (.dll).
/// @return Handle to the loaded library, or nullptr on failure.
[[nodiscard]] inline auto loadDll(const std::string &filename) -> HandleType
{
  return LoadLibraryA(filename.c_str());
}

#else
/// @brief Native handle type for a loaded dynamic library.
using HandleType = void *;

/// @brief Unload a dynamic library previously loaded with loadDll().
inline void freeDll(HandleType handle) { dlclose(handle); }

/// @brief Look up a symbol in a loaded dynamic library and cast it to a function pointer.
/// @tparam FuncType Function signature to cast the resolved symbol to.
/// @param handle Library handle returned by loadDll().
/// @param name Symbol name to resolve.
/// @return Pointer to the resolved function, or nullptr if not found.
template <typename FuncType>
[[nodiscard]] auto getFunction(HandleType handle, const std::string &name) -> FuncType *
{
  return reinterpret_cast<FuncType *>(dlsym(handle, name.c_str()));
}

/// @brief Load a dynamic library from disk.
/// @param filename Path to the library file (.so/.dylib).
/// @return Handle to the loaded library, or nullptr on failure.
[[nodiscard]] inline auto loadDll(const std::string &filename) -> HandleType
{
  return dlopen(filename.c_str(), RTLD_NOW);
}

#endif

/// @brief RAII wrapper around a dynamically loaded library handle.
///
/// Owns a HandleType and unloads it on destruction or move-assignment. Move-only
/// to guarantee a single owner per handle.
class DllHandle
{
public:
  /// @brief Take ownership of an already-loaded library handle.
  /// @param handle Handle returned by loadDll(), may be nullptr (invalid handle).
  explicit DllHandle(HandleType handle) : _handle(handle) {}

  DllHandle(const DllHandle &) = delete;
  DllHandle &operator=(const DllHandle &) = delete;

  /// @brief Transfer ownership of the wrapped handle from another instance.
  DllHandle(DllHandle &&other) noexcept : _handle(other._handle) { other._handle = nullptr; }

  /// @brief Unload the currently held handle (if any) and take ownership of another's.
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

  /// @brief Whether this instance holds a successfully loaded library handle.
  [[nodiscard]] bool isValid() const { return _handle != nullptr; }

  /// @brief Resolve a symbol from the wrapped library.
  /// @tparam FuncType Function signature to cast the resolved symbol to.
  /// @param name Symbol name to resolve.
  /// @return Pointer to the resolved function, or nullptr if not found.
  template <typename FuncType>
  [[nodiscard]] auto getFunction(const std::string &name) const -> FuncType *
  {
    return ::pugg::detail::getFunction<FuncType>(_handle, name);
  }

  /// @brief Native handle wrapped by this instance.
  [[nodiscard]] auto handle() const -> HandleType { return _handle; }

private:
  HandleType _handle;
};

} // namespace detail
} // namespace pugg
