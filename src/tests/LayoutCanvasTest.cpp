#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>

#include "PortPrefs.hpp"
#include "WindowManager.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }
  SDL_setenv_unsafe("SDL_VIDEODRIVER", "dummy", 1);
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error(SDL_GetError());
  }
  PortPrefs prefs;
  prefs.ui_layout = std::string(argv[1]) == "classic" ? UiLayout::Classic : UiLayout::Expanded;
  int scale = std::stoi(argv[2]);
  prefs.window_w = (prefs.ui_layout == UiLayout::Classic ? 640 : 800) * scale;
  prefs.window_h = (prefs.ui_layout == UiLayout::Classic ? 480 : 600) * scale;
  auto& wm = WindowManager::instance();
  wm.create_sdl_window(prefs);
  int logical_w = prefs.ui_layout == UiLayout::Classic ? 640 : 800;
  int logical_h = prefs.ui_layout == UiLayout::Classic ? 480 : 600;
  if (ui_layout_width() != logical_w || ui_layout_height() != logical_h ||
      wm.screen_port.data.get_width() != logical_w || wm.screen_port.data.get_height() != logical_h) {
    throw std::runtime_error("The renderer has the wrong logical dimensions");
  }
  float x = 0, y = 0;
  auto renderer = SDL_GetRenderer(wm.get_sdl_window().get());
  if (!SDL_RenderCoordinatesFromWindow(renderer, prefs.window_w / 2, prefs.window_h / 2, &x, &y) ||
      std::abs(x - logical_w / 2) > 1 || std::abs(y - logical_h / 2) > 1) {
    throw std::runtime_error("Mouse coordinates do not map to the logical canvas");
  }
  SDL_Quit();
  return 0;
}
