#include <SDL3/SDL.h>
#undef NDEBUG
#include <cassert>
#include <filesystem>
#include <string>

#include "FileManager.h"
#include "FileManager.hpp"

int main() {
  const char* mac_name = ":Data Files:Data ID";
  auto bundled = host_filename_for_mac_filename(mac_name, false);
  assert(std::filesystem::is_regular_file(bundled));
  char* pref_path = SDL_GetPrefPath("Fantasoft", "Realmz");
  assert(pref_path);
  std::filesystem::path copied = std::filesystem::path(pref_path) / "Data Files" / "Data ID";
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
  return 0;
}
