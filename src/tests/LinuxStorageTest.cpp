#include <SDL3/SDL.h>
#undef NDEBUG
#include <cassert>
#include <cstring>
#include <filesystem>
#include <string>
#include <unistd.h>

#include "FileManager.h"
#include "FileManager.hpp"
#include "ResourceManager.h"

int main() {
  const char* mac_name = ":Data Files:Data ID";
  auto bundled = host_filename_for_mac_filename(mac_name, false);
  assert(std::filesystem::is_regular_file(bundled));
  char* pref_path = SDL_GetPrefPath("Fantasoft", "Realmz");
  assert(pref_path);
  std::filesystem::path pref_root(pref_path);
  std::filesystem::path copied = pref_root / "Data Files" / "Data ID";
  SDL_free(pref_path);

  FILE* file = mac_fopen(mac_name, "r+b");
  assert(file);
  assert(std::filesystem::is_regular_file(copied));
  assert(std::filesystem::file_size(copied) == std::filesystem::file_size(bundled));
  int first = fgetc(file);
  assert(first != EOF);
  assert(fseek(file, 0, SEEK_SET) == 0);
  assert(fputc(first, file) == first);
  assert(fclose(file) == 0);
  assert(std::filesystem::file_size(copied) == std::filesystem::file_size(bundled));

  FSSpec resource_spec{};
  const char* resource_name = ":Data Files:Data Caste";
  resource_spec.name[0] = std::strlen(resource_name);
  std::memcpy(resource_spec.name + 1, resource_name, resource_spec.name[0]);
  auto bundled_resource = host_filename_for_mac_filename(resource_name, false) + ".rsrc";
  assert(std::filesystem::is_regular_file(bundled_resource));
  auto user_resource = copied.parent_path() / "Data Caste.rsrc";
  int16_t ref = FSpOpenResFile(&resource_spec, fsRdWrPerm);
  assert(ref > 0);
  assert(std::filesystem::is_regular_file(user_resource));
  assert(std::filesystem::file_size(user_resource) == std::filesystem::file_size(bundled_resource));
  CloseResFile(ref);

  std::string empty_name = "Realmz Empty Resource Test " + std::to_string(getpid());
  FSSpec empty_spec{};
  empty_spec.parID = 1;
  empty_spec.name[0] = empty_name.size();
  std::memcpy(empty_spec.name + 1, empty_name.data(), empty_name.size());
  auto empty_resource = pref_root / (empty_name + ".rsrc");
  assert(!std::filesystem::exists(empty_resource));
  FSpCreateResFile(&empty_spec, 0, 0, 0);
  assert(std::filesystem::is_regular_file(empty_resource));
  ref = FSpOpenResFile(&empty_spec, fsCurPerm);
  assert(ref > 0);
  CloseResFile(ref);
  assert(std::filesystem::remove(empty_resource));
  return 0;
}
