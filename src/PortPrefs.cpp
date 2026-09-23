#include "PortPrefs.hpp"

#include <SDL3/SDL_filesystem.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

#include "PortMenu.hpp"

#include <phosg/Filesystem.hh>
#include <phosg/JSON.hh>
#include <phosg/Strings.hh>

#include "Types.hpp"

static phosg::PrefixedLogger prefs_log("[PortPrefs] ", DEFAULT_LOG_LEVEL);
static UiLayout current_layout = UiLayout::Expanded;

UiLayout active_ui_layout() { return current_layout; }
void set_active_ui_layout(UiLayout layout) { current_layout = layout; }
int ui_layout_width() { return current_layout == UiLayout::Classic ? 640 : 800; }
int ui_layout_height() { return current_layout == UiLayout::Classic ? 480 : 600; }

static std::string prefs_path() {
  char* base = SDL_GetPrefPath("Fantasoft", "Realmz");
  if (!base) {
    return std::string();
  }
  std::string path = std::string(base) + "port_settings.json";
  SDL_free(base);
  return path;
}

static const char* name_for_scale_mode(SDL_ScaleMode mode) {
  switch (mode) {
    case SDL_SCALEMODE_LINEAR:
      return "linear";
    case SDL_SCALEMODE_NEAREST:
      return "nearest";
    default:
      return "pixelart";
  }
}

static SDL_ScaleMode scale_mode_for_name(const std::string& name) {
  if (name == "linear") {
    return SDL_SCALEMODE_LINEAR;
  } else if (name == "nearest") {
    return SDL_SCALEMODE_NEAREST;
  }
  return SDL_SCALEMODE_PIXELART;
}

static double gamma_value_for_idx(int idx) {
  assert(idx >= 0 && idx < kPortGammaCount);
  return kPortGammaOptions[idx].display_gamma;
}

static int gamma_idx_for_value(double value) {
  int best = 0;
  for (int i = 1; i < kPortGammaCount; ++i) {
    if (std::abs(kPortGammaOptions[i].display_gamma - value) < std::abs(kPortGammaOptions[best].display_gamma - value)) {
      best = i;
    }
  }
  if (kPortGammaOptions[best].display_gamma != value) {
    prefs_log.warning_f("Unknown gamma value {}; using nearest ({})", value, kPortGammaOptions[best].display_gamma);
  }
  return best;
}

PortPrefs load_port_prefs() {
  return load_port_prefs(prefs_path());
}

PortPrefs load_port_prefs(const std::string& path) {
  PortPrefs prefs;
  if (path.empty()) {
    prefs_log.warning_f("Could not get pref path: {}; using defaults", SDL_GetError());
    return prefs;
  }

  std::string data;
  try {
    data = phosg::load_file(path);
  } catch (const std::exception& e) {
    prefs_log.info_f("Could not read {} ({}); using defaults", path, e.what());
    return prefs;
  }

  try {
    auto root = phosg::JSON::parse(data);
    prefs.ui_layout = root.get_string("ui_layout", "expanded_800x600") == "classic_640x480"
        ? UiLayout::Classic : UiLayout::Expanded;
    int base_w = prefs.ui_layout == UiLayout::Classic ? 640 : 800;
    int base_h = prefs.ui_layout == UiLayout::Classic ? 480 : 600;
    prefs.window_w = std::clamp(static_cast<int>(root.get_int("window_w", base_w)), base_w, base_w * 4);
    prefs.window_h = std::clamp(static_cast<int>(root.get_int("window_h", base_h)), base_h, base_h * 4);
    prefs.window_x = static_cast<int>(root.get_int("window_x", prefs.window_x));
    prefs.window_y = static_cast<int>(root.get_int("window_y", prefs.window_y));
    prefs.scale_mode = scale_mode_for_name(root.get_string("filter", name_for_scale_mode(prefs.scale_mode)));
    prefs.aspect_locked = root.get_bool("aspect_locked", prefs.aspect_locked);
    prefs.gamma_idx = gamma_idx_for_value(root.get_float("gamma", gamma_value_for_idx(prefs.gamma_idx)));
  } catch (const std::exception& e) {
    prefs_log.warning_f("Could not parse {} ({}); using defaults", path, e.what());
    return PortPrefs{};
  }

  prefs_log.info_f("Loaded prefs: window {}x{}, filter {}", prefs.window_w, prefs.window_h, name_for_scale_mode(prefs.scale_mode));
  return prefs;
}

void save_port_prefs(const PortPrefs& prefs) {
  save_port_prefs(prefs, prefs_path());
}

void save_port_prefs(const PortPrefs& prefs, const std::string& path) {
  if (path.empty()) {
    prefs_log.warning_f("Could not get pref path: {}; not saving prefs", SDL_GetError());
    return;
  }

  phosg::JSON root = phosg::JSON::dict();
  root.emplace("window_w", static_cast<int64_t>(prefs.window_w));
  root.emplace("window_h", static_cast<int64_t>(prefs.window_h));
  if (!SDL_WINDOWPOS_ISCENTERED(prefs.window_x) && !SDL_WINDOWPOS_ISUNDEFINED(prefs.window_x) &&
      !SDL_WINDOWPOS_ISCENTERED(prefs.window_y) && !SDL_WINDOWPOS_ISUNDEFINED(prefs.window_y)) {
    root.emplace("window_x", static_cast<int64_t>(prefs.window_x));
    root.emplace("window_y", static_cast<int64_t>(prefs.window_y));
  }
  root.emplace("filter", name_for_scale_mode(prefs.scale_mode));
  root.emplace("aspect_locked", prefs.aspect_locked);
  root.emplace("gamma", gamma_value_for_idx(prefs.gamma_idx));
  root.emplace("ui_layout", prefs.ui_layout == UiLayout::Classic ? "classic_640x480" : "expanded_800x600");

  std::string tmp_path = path + ".tmp";
  try {
    phosg::save_file(tmp_path, root.serialize());
    if (!SDL_RenamePath(tmp_path.c_str(), path.c_str())) {
      std::remove(tmp_path.c_str());
      prefs_log.warning_f("Could not rename {} to {}: {}", tmp_path, path, SDL_GetError());
    }
  } catch (const std::exception& e) {
    std::remove(tmp_path.c_str());
    prefs_log.warning_f("Could not write {} ({})", tmp_path, e.what());
  }
}
