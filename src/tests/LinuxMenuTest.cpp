#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#undef NDEBUG
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "EventManager.h"
#include "FileManager.h"
#include "MenuController.h"
#include "MenuManager-C-Interface.h"
#include "WindowManager.hpp"
#include "ResourceManager.h"
#include "linux/MenuController.hpp"

extern "C" void InitRealmzCocoa();

static int calls = 0;
static int selected_menu = 0;
static int selected_item = 0;

static void selected(int16_t menu, int16_t item) {
  ++calls;
  selected_menu = menu;
  selected_item = item;
}

static SDL_Window* popup_window(SDL_Window* main) {
  int count = 0;
  SDL_Window** windows = SDL_GetWindows(&count);
  SDL_Window* popup = nullptr;
  for (int i = 0; i < count; ++i) if (windows[i] != main) popup = windows[i];
  SDL_free(windows);
  return popup;
}

static SDL_Window* child_popup(SDL_Window* parent) {
  int count = 0;
  SDL_Window** windows = SDL_GetWindows(&count);
  SDL_Window* child = nullptr;
  for (int i = 0; i < count; ++i) if (SDL_GetWindowParent(windows[i]) == parent) child = windows[i];
  SDL_free(windows);
  return child;
}

int main() {
  assert(SDL_Init(SDL_INIT_VIDEO));
  assert(TTF_Init());
  auto& wm = WindowManager::instance();
  wm.create_sdl_window();
  auto* main = wm.get_sdl_window().get();
  assert(main);
  int content_w = 0, content_h = 0;
  wm.get_window_size(&content_w, &content_h);
  assert(content_w == 800 && content_h == 600);
  float window_x = 0, window_y = 0, logical_x = 0, logical_y = 0;
  auto* renderer = SDL_GetRenderer(main);
  assert(SDL_RenderCoordinatesToWindow(renderer, 400, kLinuxMenuHeight + 300, &window_x, &window_y));
  assert(SDL_RenderCoordinatesFromWindow(renderer, window_x, window_y, &logical_x, &logical_y));
  assert(std::abs(logical_x - 400) < 0.5f && std::abs(logical_y - kLinuxMenuHeight - 300) < 0.5f);
  InitRealmzCocoa();
  FSSpec jewels{};
  const char* jewels_name = ":Data Files:The Family Jewels";
  jewels.name[0] = std::strlen(jewels_name);
  std::memcpy(jewels.name + 1, jewels_name, jewels.name[0]);
  assert(FSpOpenResFile(&jewels, fsRdPerm) > 0);
  auto resource_bar = GetNewMBar(128);
  assert(resource_bar);
  SetMenuBar(resource_bar);
  assert(GetMenuHandle(129));
  assert(CountMItems(GetMenuHandle(129)) > 0);
  assert(MenuKey(0x1B) == 0);
  DrawMenuBar();
  assert(LinuxMenuHasBar());
  auto list = std::make_shared<MenuList>();
  auto menu = std::make_shared<Menu>();
  menu->menu_id = 131;
  menu->title = "Game";
  menu->enabled = true;
  Menu::Item first;
  first.name = "Choose";
  first.key_equivalent = 'C';
  menu->items.emplace_back(first);
  Menu::Item disabled;
  disabled.name = "Disabled";
  disabled.enabled = false;
  menu->items.emplace_back(disabled);
  Menu::Item separator;
  separator.name = "-";
  menu->items.emplace_back(separator);
  Menu::Item parent_item;
  parent_item.name = "Volume";
  parent_item.key_equivalent = 0x1B;
  parent_item.mark_character = static_cast<char>(132);
  menu->items.emplace_back(parent_item);
  Menu::Item missing_item;
  missing_item.name = "Unavailable submenu";
  missing_item.key_equivalent = 0x1B;
  missing_item.mark_character = static_cast<char>(133);
  menu->items.emplace_back(missing_item);
  list->menus.emplace_back(menu);
  auto child = std::make_shared<Menu>();
  child->menu_id = 132;
  child->title = "Volume";
  child->enabled = true;
  Menu::Item child_item;
  child_item.name = "Fast";
  child->items.emplace_back(child_item);
  list->submenus.emplace_back(child);
  MCSync(list, selected);
  assert(LinuxMenuHasBar());

  MCCreatePopupMenu(nullptr, menu, {50, 50}, selected);
  assert(LinuxMenuIsTracking());
  auto* popup = popup_window(main);
  assert(popup);
  SDL_Event initial_event{};
  while (SDL_PollEvent(&initial_event)) {
    LinuxMenuHandleEvent(initial_event);
  }
  // Renderer initialization can queue a transient HIDDEN/SHOWN pair for the
  // popup. Processing that real SDL queue must not dismiss a visible menu.
  assert(LinuxMenuIsTracking());
  if (const char* capture = std::getenv("REALMZ_MENU_CAPTURE")) {
    SDL_Surface* shot = SDL_RenderReadPixels(SDL_GetRenderer(popup), nullptr);
    assert(shot);
    assert(SDL_SaveBMP(shot, capture));
    SDL_DestroySurface(shot);
  }
  SDL_Event click{};
  click.type = SDL_EVENT_MOUSE_BUTTON_UP;
  click.button.windowID = SDL_GetWindowID(popup);
  click.button.button = SDL_BUTTON_LEFT;
  click.button.y = kLinuxMenuRowHeight + 5;
  assert(LinuxMenuHandleEvent(click));
  assert(calls == 0); // disabled item
  click.button.y = 5;
  assert(LinuxMenuHandleEvent(click));
  assert(calls == 1 && selected_menu == 131 && selected_item == 1);
  assert(!LinuxMenuIsTracking());

  MCCreatePopupMenu(nullptr, menu, {50, 50}, selected);
  popup = popup_window(main);
  assert(popup);
  SDL_Event escape{};
  escape.type = SDL_EVENT_KEY_DOWN;
  escape.key.windowID = SDL_GetWindowID(popup);
  escape.key.key = SDLK_ESCAPE;
  assert(LinuxMenuHandleEvent(escape));
  assert(calls == 2 && selected_menu == 0 && selected_item == 0);
  assert(!LinuxMenuIsTracking());

  MCCreatePopupMenu(nullptr, menu, {50, 50}, selected);
  popup = popup_window(main);
  assert(popup);
  click.button.windowID = SDL_GetWindowID(popup);
  click.button.y = 4 * kLinuxMenuRowHeight + 5;
  assert(LinuxMenuHandleEvent(click));
  assert(calls == 2 && LinuxMenuIsTracking()); // missing submenu cannot select
  MCSync(list, selected);
  assert(calls == 3 && selected_menu == 0 && selected_item == 0);
  assert(!LinuxMenuIsTracking());
  LinuxMenuCancel();
  assert(calls == 3); // replacement cancels exactly once

  SDL_Event bar_click{};
  bar_click.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
  bar_click.button.windowID = SDL_GetWindowID(main);
  bar_click.button.button = SDL_BUTTON_LEFT;
  bar_click.button.x = 25;
  bar_click.button.y = 10;
  assert(LinuxMenuHandleEvent(bar_click));
  assert(LinuxMenuIsTracking());
  while (SDL_PollEvent(&initial_event)) LinuxMenuHandleEvent(initial_event);
  assert(LinuxMenuIsTracking());
  SDL_Event release = bar_click;
  release.type = SDL_EVENT_MOUSE_BUTTON_UP;
  assert(LinuxMenuHandleEvent(release));
  assert(LinuxMenuIsTracking()); // click and click-hold both keep the dropdown open
  popup = popup_window(main);
  assert(popup);
  SDL_Event hover{};
  hover.type = SDL_EVENT_MOUSE_MOTION;
  hover.motion.windowID = SDL_GetWindowID(popup);
  hover.motion.x = 70;
  hover.motion.y = 3 * kLinuxMenuRowHeight + 4;
  assert(LinuxMenuHandleEvent(hover));
  auto* submenu_window = child_popup(popup);
  assert(submenu_window && submenu_window != popup);
  SDL_WindowID submenu_id = SDL_GetWindowID(submenu_window);
  assert(LinuxMenuHandleEvent(hover));
  assert(child_popup(popup) && SDL_GetWindowID(child_popup(popup)) == submenu_id);
  while (SDL_PollEvent(&initial_event)) {
    LinuxMenuHandleEvent(initial_event);
  }
  assert(LinuxMenuIsTracking());
  assert(SDL_GetWindowFromID(submenu_id));
  click.button.windowID = SDL_GetWindowID(submenu_window);
  click.button.y = 5;
  assert(LinuxMenuHandleEvent(click));
  assert(calls == 4 && selected_menu == 132 && selected_item == 1);
  assert(!LinuxMenuIsTracking());

  SDL_Event key{};
  key.type = SDL_EVENT_KEY_DOWN;
  key.key.windowID = SDL_GetWindowID(main);
  key.key.key = SDLK_F10;
  assert(LinuxMenuHandleEvent(key));
  assert(LinuxMenuIsTracking());
  popup = popup_window(main);
  assert(popup);
  key.key.windowID = SDL_GetWindowID(popup);
  key.key.key = SDLK_RIGHT;
  assert(LinuxMenuHandleEvent(key)); // move from Game to Port
  popup = popup_window(main);
  assert(popup);
  hover.motion.windowID = SDL_GetWindowID(popup);
  hover.motion.y = kLinuxMenuRowHeight + 4; // Scale
  assert(LinuxMenuHandleEvent(hover));
  submenu_window = child_popup(popup);
  assert(submenu_window);
  hover.motion.windowID = SDL_GetWindowID(submenu_window);
  hover.motion.y = 4; // 1x (Default) is a leaf, not another Port submenu
  assert(LinuxMenuHandleEvent(hover));
  assert(child_popup(submenu_window) == nullptr);
  assert(LinuxMenuIsTracking());
  click.button.windowID = SDL_GetWindowID(submenu_window);
  click.button.y = 4;
  assert(LinuxMenuHandleEvent(click));
  assert(!LinuxMenuIsTracking());
  return 0;
}
