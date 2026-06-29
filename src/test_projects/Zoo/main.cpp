#include <Animal.h>
#include <pugg/Kernel.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main()
{
  std::cout << "Zoo example\n";
  std::cout << "Loading plugins...\n";

  pugg::Kernel kernel;
  kernel.add_server<Animal>();

#ifdef _WIN32
  constexpr auto DLL_PANTHALASSA = "PanthalassaAnimals.dll";
  constexpr auto DLL_PANGEA      = "PangeaAnimals.dll";
#else
  constexpr auto DLL_PANTHALASSA = "libPanthalassaAnimals.so";
  constexpr auto DLL_PANGEA      = "libPangeaAnimals.so";
#endif

  if (!kernel.load_plugin(DLL_PANTHALASSA))
    std::cerr << "Warning: failed to load " << DLL_PANTHALASSA << "\n";
  if (!kernel.load_plugin(DLL_PANGEA))
    std::cerr << "Warning: failed to load " << DLL_PANGEA << "\n";

  if (auto *driver = kernel.get_driver<AnimalDriver>(Animal::server_name(), "DogDriver"))
    std::cout << "Found driver: " << driver->name() << "\n";

  auto drivers = kernel.get_all_drivers<AnimalDriver>(Animal::server_name());
  std::vector<std::unique_ptr<Animal>> animals;
  for (auto *driver : drivers)
    animals.push_back(std::unique_ptr<Animal>(driver->create()));

  for (auto &animal : animals)
    std::cout << "  " << animal->kind() << " (can swim: " << std::boolalpha << animal->can_swim() << ")\n";

  std::cout << "Press ENTER to exit...\n";
  std::cin.get();
}
