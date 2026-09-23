#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <phosg/Filesystem.hh>

#include "PortPrefs.hpp"

int main() {
  auto path = std::filesystem::temp_directory_path() /
      ("realmz-port-prefs-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
  auto filename = path.string();
  try {
    if (load_port_prefs(filename).ui_layout != UiLayout::Expanded) {
      throw std::runtime_error("Missing preference did not select expanded mode");
    }
    phosg::save_file(filename, R"({"ui_layout":"unrecognized","window_w":100,"window_h":100})");
    auto invalid = load_port_prefs(filename);
    if (invalid.ui_layout != UiLayout::Expanded || invalid.window_w != 800 || invalid.window_h != 600) {
      throw std::runtime_error("Invalid layout did not use the expanded default");
    }
    PortPrefs classic;
    classic.ui_layout = UiLayout::Classic;
    classic.window_w = 1280;
    classic.window_h = 960;
    save_port_prefs(classic, filename);
    auto loaded = load_port_prefs(filename);
    if (loaded.ui_layout != UiLayout::Classic || loaded.window_w != 1280 || loaded.window_h != 960) {
      throw std::runtime_error("Classic layout preference was not saved");
    }
    loaded.ui_layout = UiLayout::Expanded;
    save_port_prefs(loaded, filename);
    if (load_port_prefs(filename).ui_layout != UiLayout::Expanded) {
      throw std::runtime_error("Expanded layout preference was not saved");
    }
    std::filesystem::remove(path);
  } catch (...) {
    std::filesystem::remove(path);
    throw;
  }
  return 0;
}
