#include <catch2/catch_test_macros.hpp>

#include <Animal.h>
#include <pugg/Kernel.h>

#include <memory>
#include <string>

// Full paths to plugin libraries, injected by CMake via target_compile_definitions.
#ifndef PANGEA_PLUGIN_PATH
#define PANGEA_PLUGIN_PATH "libPangeaAnimals.so"
#endif
#ifndef PANTHALASSA_PLUGIN_PATH
#define PANTHALASSA_PLUGIN_PATH "libPanthalassaAnimals.so"
#endif

static constexpr auto PANGEA_PLUGIN      = PANGEA_PLUGIN_PATH;
static constexpr auto PANTHALASSA_PLUGIN = PANTHALASSA_PLUGIN_PATH;

// ── Plugin loading ───────────────────────────────────────────────────────────

TEST_CASE("load_plugin returns true for a valid plugin", "[integration][load]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  CHECK(kernel.load_plugin(PANGEA_PLUGIN));
}

TEST_CASE("load_plugin returns false for a missing file", "[integration][load]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  CHECK_FALSE(kernel.load_plugin("no_such_plugin.so"));
}

TEST_CASE("load_plugin registers drivers from plugin", "[integration][load]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));

  auto drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  CHECK(drivers.size() == 2); // Cat + Dog
}

// ── Multi-plugin loading ─────────────────────────────────────────────────────

TEST_CASE("multiple plugins register into the same server", "[integration][multi]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));
  REQUIRE(kernel.load_plugin(PANTHALASSA_PLUGIN));

  auto drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  CHECK(drivers.size() == 4); // Cat, Dog, Fish, Whale
}

// ── Driver retrieval from plugin ─────────────────────────────────────────────

TEST_CASE("get_driver returns correct driver by name", "[integration][get]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));

  auto *driver = kernel.get_driver<AnimalDriver>(Animal::server_name(), "DogDriver");
  REQUIRE(driver != nullptr);
  CHECK(driver->name() == "DogDriver");
}

// ── Object creation via driver ────────────────────────────────────────────────

TEST_CASE("drivers can create animal instances", "[integration][create]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));
  REQUIRE(kernel.load_plugin(PANTHALASSA_PLUGIN));

  auto drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  REQUIRE_FALSE(drivers.empty());

  int swimmers = 0;
  for (auto *drv : drivers)
  {
    std::unique_ptr<Animal> animal{drv->create()};
    REQUIRE(animal != nullptr);
    CHECK_FALSE(animal->kind().empty());
    if (animal->can_swim())
      ++swimmers;
  }
  // Dog, Fish, Whale can swim; Cat cannot
  CHECK(swimmers == 3);
}

// ── Kernel cleanup with loaded plugins ───────────────────────────────────────

TEST_CASE("Kernel destructs cleanly after plugin loading", "[integration][lifetime]")
{
  // Verifies no crash on destruction: Driver vtables must remain valid until
  // _servers is destroyed, before _plugins unloads the DLLs.
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));
  REQUIRE(kernel.load_plugin(PANTHALASSA_PLUGIN));
  // kernel destructs here — if member order is wrong this crashes
}

TEST_CASE("clear() then reload works correctly", "[integration][clear]")
{
  pugg::Kernel kernel;
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));

  kernel.clear();

  // Re-register and reload
  kernel.add_server<Animal>();
  REQUIRE(kernel.load_plugin(PANGEA_PLUGIN));
  auto drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  CHECK(drivers.size() == 2);
}
