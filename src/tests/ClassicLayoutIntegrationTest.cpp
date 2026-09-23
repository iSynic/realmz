#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#include <SDL3/SDL_filesystem.h>
#include <phosg/Filesystem.hh>
#include <resource_file/IndexFormats/Formats.hh>
#include <resource_file/ResourceFile.hh>

#include "PortPrefs.hpp"
#include "ResourceManager.h"

static int16_t be16(const uint8_t* data, size_t offset) {
  return (data[offset] << 8) | data[offset + 1];
}

static void check_family_window(UiLayout layout, int expected_width, int expected_bottom) {
  set_active_ui_layout(layout);
  FSSpec spec{};
  const std::string name = ":Data Files:The Family Jewels";
  spec.name[0] = static_cast<uint8_t>(name.size());
  std::memcpy(spec.name + 1, name.data(), name.size());
  int16_t refnum = FSpOpenResFile(&spec, fsRdPerm);
  if (refnum < 0) {
    throw std::runtime_error("Could not open Family Jewels resource fork");
  }
  auto window = GetResource(0x57494E44, 128);
  auto bytes = window ? reinterpret_cast<const uint8_t*>(*window) : nullptr;
  if (!bytes || be16(bytes, 6) != expected_width || be16(bytes, 4) != expected_bottom) {
    throw std::runtime_error("The loaded window does not match the selected layout");
  }
  CloseResFile(refnum);
}

int main() {
  check_family_window(UiLayout::Expanded, 800, 575);
  check_family_window(UiLayout::Classic, 640, 480);

  // A scenario opened after Family Jewels still takes precedence for overlapping IDs.
  auto scenario_path = std::string(SDL_GetBasePath()) + "ClassicLayoutScenario.rsrc";
  ResourceDASM::ResourceFile scenario;
  std::string scenario_window(8, '\0');
  scenario_window[4] = 1;
  scenario_window[5] = 65; // bottom = 321
  scenario_window[6] = 2;
  scenario_window[7] = 22; // right = 534
  scenario.add(std::make_shared<ResourceDASM::ResourceFile::Resource>(
      0x57494E44, 128, 0, "scenario window", scenario_window));
  phosg::save_file(scenario_path, ResourceDASM::serialize_resource_fork(scenario));
  try {
    FSSpec family_spec{};
    const std::string family_name = ":Data Files:The Family Jewels";
    family_spec.name[0] = static_cast<uint8_t>(family_name.size());
    std::memcpy(family_spec.name + 1, family_name.data(), family_name.size());
    int16_t family_refnum = FSpOpenResFile(&family_spec, fsRdPerm);
    FSSpec scenario_spec{};
    const std::string scenario_name = ":ClassicLayoutScenario";
    scenario_spec.name[0] = static_cast<uint8_t>(scenario_name.size());
    std::memcpy(scenario_spec.name + 1, scenario_name.data(), scenario_name.size());
    int16_t scenario_refnum = FSpOpenResFile(&scenario_spec, fsRdPerm);
    auto window = GetResource(0x57494E44, 128);
    auto bytes = window ? reinterpret_cast<const uint8_t*>(*window) : nullptr;
    if (!bytes || be16(bytes, 6) != 534 || be16(bytes, 4) != 321) {
      throw std::runtime_error("Scenario resource did not retain precedence");
    }
    CloseResFile(scenario_refnum);
    CloseResFile(family_refnum);
  } catch (...) {
    std::filesystem::remove(scenario_path);
    throw;
  }
  std::filesystem::remove(scenario_path);

  // A write to a non-UI resource must never persist the in-memory classic UI override.
  auto family_path = std::string(SDL_GetBasePath()) + "Data Files/The Family Jewels.rsrc";
  auto original_bytes = phosg::load_file(family_path);
  try {
    set_active_ui_layout(UiLayout::Classic);
    FSSpec spec{};
    const std::string name = ":Data Files:The Family Jewels";
    spec.name[0] = static_cast<uint8_t>(name.size());
    std::memcpy(spec.name + 1, name.data(), name.size());
    int16_t refnum = FSpOpenResFile(&spec, fsRdWrPerm);
    if (refnum < 0) {
      throw std::runtime_error("Could not open writable Family Jewels fixture");
    }
    auto strings = GetResource(0x53545223, 6002); // STR#
    if (!strings) {
      throw std::runtime_error("Expected name strings are missing");
    }
    ChangedResource(strings);
    CloseResFile(refnum);
    auto persisted = ResourceDASM::parse_resource_fork(phosg::load_file(family_path));
    if (be16(reinterpret_cast<const uint8_t*>(persisted.get_resource(0x57494E44, 128)->data.data()), 6) != 800) {
      throw std::runtime_error("Classic UI window was written into Family Jewels");
    }
  } catch (...) {
    phosg::save_file(family_path, original_bytes);
    throw;
  }
  phosg::save_file(family_path, original_bytes);

  auto overlay_path = std::filesystem::path(SDL_GetBasePath()) / "classic-ui.rsrc";
  auto hidden_path = std::filesystem::path(SDL_GetBasePath()) / "classic-ui.rsrc.test-hidden";
  std::filesystem::rename(overlay_path, hidden_path);
  try {
    FSSpec spec{};
    const std::string name = ":Data Files:The Family Jewels";
    spec.name[0] = static_cast<uint8_t>(name.size());
    std::memcpy(spec.name + 1, name.data(), name.size());
    bool failed_clearly = false;
    try {
      FSpOpenResFile(&spec, fsRdPerm);
    } catch (const std::runtime_error& e) {
      failed_clearly = std::string(e.what()).find("Classic UI resource setup failed") != std::string::npos;
    }
    if (!failed_clearly) {
      throw std::runtime_error("Missing classic UI resources did not fail clearly");
    }
  } catch (...) {
    std::filesystem::rename(hidden_path, overlay_path);
    throw;
  }
  std::filesystem::rename(hidden_path, overlay_path);

  auto overlay_bytes = phosg::load_file(overlay_path.string());
  try {
    auto incomplete = ResourceDASM::parse_resource_fork(overlay_bytes);
    incomplete.remove(0x57494E44, 128);
    phosg::save_file(overlay_path.string(), ResourceDASM::serialize_resource_fork(incomplete));
    FSSpec spec{};
    const std::string name = ":Data Files:The Family Jewels";
    spec.name[0] = static_cast<uint8_t>(name.size());
    std::memcpy(spec.name + 1, name.data(), name.size());
    bool failed_clearly = false;
    try {
      FSpOpenResFile(&spec, fsRdPerm);
    } catch (const std::runtime_error& e) {
      failed_clearly = std::string(e.what()).find("required resource") != std::string::npos;
    }
    if (!failed_clearly) {
      throw std::runtime_error("Incomplete classic UI resources did not fail clearly");
    }
  } catch (...) {
    phosg::save_file(overlay_path.string(), overlay_bytes);
    throw;
  }
  phosg::save_file(overlay_path.string(), overlay_bytes);
  return 0;
}
