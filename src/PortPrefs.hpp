#pragma once

#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <string>

#include "PortMenu.hpp"

enum class UiLayout { Expanded, Classic };

UiLayout active_ui_layout();
void set_active_ui_layout(UiLayout layout);
int ui_layout_width();
int ui_layout_height();

struct PortPrefs {
  int window_w = kLogicalWindowWidth;
  int window_h = kLogicalWindowHeight;
  int window_x = SDL_WINDOWPOS_CENTERED;
  int window_y = SDL_WINDOWPOS_CENTERED;
  SDL_ScaleMode scale_mode = SDL_SCALEMODE_PIXELART;
  bool aspect_locked = true;
  int gamma_idx = 0; // index into kPortGammaOptions
  UiLayout ui_layout = UiLayout::Expanded;
};

PortPrefs load_port_prefs();
PortPrefs load_port_prefs(const std::string& path);
void save_port_prefs(const PortPrefs& prefs);
void save_port_prefs(const PortPrefs& prefs, const std::string& path);
