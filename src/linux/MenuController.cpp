#include "MenuController.hpp"

#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cctype>
#include <iconv.h>
#include <memory>
#include <phosg/Strings.hh>
#include <string>
#include <unordered_set>
#include <vector>

#include "../MenuController.h"
#include "../MenuManager-C-Interface.h"
#include "../PortMenu.hpp"
#include "../QuickDraw.hpp"
#include "../WindowManager.hpp"
#include "../EventManager.h"
#include "../PortPrefs.hpp"

namespace {
constexpr int kRowHeight = kLinuxMenuRowHeight;
constexpr int kSidePadding = 14;
constexpr int kMaxPopupWidth = 380;
static phosg::PrefixedLogger log("[LinuxMenu] ");

struct Popup {
  std::shared_ptr<Menu> menu;
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  int selected = -1;
  int first_row = 0;
  int visible_rows = 0;
  int width = 0;
  bool port = false;
  bool port_root = false;
};

class Controller {
public:
  ~Controller() {
    while (!popups.empty()) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); }
    if (font) TTF_CloseFont(font);
  }

  void sync(std::shared_ptr<MenuList> new_list, void (*new_callback)(int16_t, int16_t)) {
    close(true);
    list = std::move(new_list);
    callback = new_callback;
    WindowManager::instance().present_screen();
  }

  bool has_bar() const { return list != nullptr; }
  bool tracking() const { return !popups.empty(); }

  void draw_bar(SDL_Renderer* renderer = nullptr) {
    if (!list) return;
    auto* parent = WindowManager::instance().get_sdl_window().get();
    if (!parent) return;
    if (!renderer) renderer = SDL_GetRenderer(parent);
    if (!renderer) return;
    SDL_FRect bar{0, 0, static_cast<float>(ui_layout_width()), kLinuxMenuHeight};
    SDL_SetRenderDrawColor(renderer, 221, 221, 221, 255);
    SDL_RenderFillRect(renderer, &bar);
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderLine(renderer, 0, kLinuxMenuHeight - 1, ui_layout_width(), kLinuxMenuHeight - 1);
    int x = 8;
    int index = 0;
    for (const auto& menu : list->menus) {
      int width = text_width(menu->title) + 16;
      if (index == active_bar) {
        SDL_FRect active{static_cast<float>(x), 2, static_cast<float>(width), kLinuxMenuHeight - 4};
        SDL_SetRenderDrawColor(renderer, 62, 91, 146, 255);
        SDL_RenderFillRect(renderer, &active);
      }
      render_text(renderer, menu->title, x + 8, 3, index == active_bar, !menu->enabled);
      x += width;
      ++index;
    }
    int width = text_width("Port") + 16;
    if (active_bar == index) {
      SDL_FRect active{static_cast<float>(x), 2, static_cast<float>(width), kLinuxMenuHeight - 4};
      SDL_SetRenderDrawColor(renderer, 62, 91, 146, 255);
      SDL_RenderFillRect(renderer, &active);
    }
    render_text(renderer, "Port", x + 8, 3, active_bar == index, false);
  }

  void cancel() { close(true); }

  void show_context(std::shared_ptr<Menu> menu, int x, int y, void (*done)(int16_t, int16_t)) {
    close(true);
    contextual = true;
    context_callback = done;
    if (!open_popup(menu, WindowManager::instance().get_sdl_window().get(), x, y, false)) finish(0, 0);
  }

  bool handle(const SDL_Event& e) {
    if (!list && !tracking()) return false;
    SDL_WindowID id = event_window_id(e);
    auto* parent = WindowManager::instance().get_sdl_window().get();
    SDL_WindowID main_id = parent ? SDL_GetWindowID(parent) : 0;
    if (e.type == SDL_EVENT_KEY_UP && consumed_keys.erase(e.key.key)) return true;
    if (suppress_release && e.type == SDL_EVENT_MOUSE_BUTTON_UP && id == main_id) {
      suppress_release = false;
      return true;
    }
    if (e.type == SDL_EVENT_QUIT) { close(true); return false; }
    if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && id == main_id) { close(true); return false; }
    if (e.type == SDL_EVENT_WINDOW_RESIZED || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || e.type == SDL_EVENT_WINDOW_EXPOSED) {
      for (auto& p : popups) if (id == SDL_GetWindowID(p.window)) {
        int width = 0, height = 0;
        SDL_GetWindowSize(p.window, &width, &height);
        p.width = width;
        p.visible_rows = std::max(1, height / kRowHeight);
        draw_popup(p);
      }
      return tracking() && id != main_id;
    }
    if ((e.type == SDL_EVENT_WINDOW_HIDDEN || e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) && tracking()) {
      for (auto& p : popups) if (id == SDL_GetWindowID(p.window)) {
        // SDL_CreateRenderer may briefly reconfigure a just-created popup. Its
        // queued HIDDEN event is stale if the popup has already been shown again.
        if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED || (SDL_GetWindowFlags(p.window) & SDL_WINDOW_HIDDEN)) {
          finish(0, 0);
        }
        return true;
      }
    }
    if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && tracking()) {
      SDL_Window* focus = SDL_GetKeyboardFocus();
      bool ours = focus == parent;
      for (const auto& p : popups) ours |= focus == p.window;
      if (!ours) { finish(0, 0); return true; }
    }
    int popup_idx = -1;
    for (int i = 0; i < static_cast<int>(popups.size()); ++i) {
      if (id == SDL_GetWindowID(popups[i].window)) { popup_idx = i; break; }
    }
    if (popup_idx >= 0) {
      if (e.type == SDL_EVENT_MOUSE_WHEEL) {
        while (static_cast<int>(popups.size()) > popup_idx + 1) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); }
        auto& p = popups[popup_idx];
        p.first_row = std::clamp(p.first_row - static_cast<int>(e.wheel.y) * 3, 0,
            std::max(0, static_cast<int>(p.menu->items.size()) - p.visible_rows));
        draw_popup(p);
      } else if (e.type == SDL_EVENT_MOUSE_MOTION) {
        auto& p = popups[popup_idx];
        int row = p.first_row + static_cast<int>(e.motion.y) / kRowHeight;
        select_row(popup_idx, row);
      } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        auto& p = popups[popup_idx];
        int row = p.first_row + static_cast<int>(e.button.y) / kRowHeight;
        activate(popup_idx, row);
      } else if (e.type == SDL_EVENT_KEY_DOWN) {
        key(e.key.key);
      }
      return true;
    }
    if (id == main_id) {
      if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || e.type == SDL_EVENT_MOUSE_BUTTON_UP || e.type == SDL_EVENT_MOUSE_MOTION) {
        if (contextual) {
          if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { suppress_release = true; finish(0, 0); }
          return true;
        }
        float x = 0, y = 0;
        auto* renderer = SDL_GetRenderer(parent);
        if (e.type == SDL_EVENT_MOUSE_MOTION) SDL_RenderCoordinatesFromWindow(renderer, e.motion.x, e.motion.y, &x, &y);
        else SDL_RenderCoordinatesFromWindow(renderer, e.button.x, e.button.y, &x, &y);
        if (y >= 0 && y < kLinuxMenuHeight) {
          if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN || (tracking() && e.type == SDL_EVENT_MOUSE_MOTION)) open_bar_at(x);
          return true;
        }
        if (tracking()) {
          if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) { suppress_release = true; finish(0, 0); }
          return true;
        }
      }
      if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.key == SDLK_F10 && !(e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT))) {
          if (tracking()) finish(0, 0); else open_bar(0);
          return true;
        }
        if (tracking()) { key(e.key.key); return true; }
        if ((e.key.mod & SDL_KMOD_LCTRL) && !(e.key.mod & SDL_KMOD_ALT) &&
            !e.key.repeat && e.key.key > 0 && e.key.key < 128) {
          char ch = static_cast<char>(std::toupper(static_cast<unsigned char>(e.key.key)));
          int32_t choice = MenuKey(ch);
          if (choice) {
            consumed_keys.insert(e.key.key);
            if (callback) callback(static_cast<int16_t>(choice >> 16), static_cast<int16_t>(choice));
            return true;
          }
        }
      }
      if (tracking() && (e.type == SDL_EVENT_KEY_UP || e.type == SDL_EVENT_TEXT_INPUT || e.type == SDL_EVENT_TEXT_EDITING)) return true;
      if (e.type == SDL_EVENT_WINDOW_FOCUS_LOST && !tracking()) return false;
    }
    return false;
  }

private:
  std::shared_ptr<MenuList> list;
  std::vector<Popup> popups;
  TTF_Font* font = nullptr;
  void (*callback)(int16_t, int16_t) = nullptr;
  void (*context_callback)(int16_t, int16_t) = nullptr;
  bool contextual = false;
  bool suppress_release = false;
  std::unordered_set<SDL_Keycode> consumed_keys;
  int active_bar = -1;

  static SDL_WindowID event_window_id(const SDL_Event& e) {
    switch (e.type) {
      case SDL_EVENT_MOUSE_MOTION: return e.motion.windowID;
      case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: return e.button.windowID;
      case SDL_EVENT_MOUSE_WHEEL: return e.wheel.windowID;
      case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: return e.key.windowID;
      case SDL_EVENT_TEXT_INPUT: case SDL_EVENT_TEXT_EDITING: return e.text.windowID;
      default: return e.window.windowID;
    }
  }

  TTF_Font* get_font() {
    if (!font) {
      auto path = std::string(SDL_GetBasePath()) + "InterVariable.ttf";
      font = TTF_OpenFont(path.c_str(), 12);
      if (!font) log.error_f("Menu font: {}", SDL_GetError());
    }
    return font;
  }

  static std::string utf8(const std::string& mac) {
    iconv_t converter = iconv_open("UTF-8", "MACINTOSH");
    if (converter == reinterpret_cast<iconv_t>(-1)) return mac;
    std::string result(mac.size() * 4 + 1, 0);
    char* input = const_cast<char*>(mac.data());
    size_t remaining = mac.size();
    char* output = result.data();
    size_t capacity = result.size();
    if (iconv(converter, &input, &remaining, &output, &capacity) == static_cast<size_t>(-1)) result = mac;
    else result.resize(output - result.data());
    iconv_close(converter);
    return result;
  }

  int text_width(const std::string& mac) {
    if (!get_font()) return static_cast<int>(mac.size()) * 9;
    auto value = utf8(mac);
    int width = 0, height = 0;
    TTF_GetStringSize(font, value.data(), value.size(), &width, &height);
    return width;
  }

  void render_text(SDL_Renderer* r, const std::string& mac, int x, int y, bool selected, bool disabled, bool already_utf8 = false, uint8_t style_flags = 0) {
    if (!get_font()) return;
    auto value = already_utf8 ? mac : utf8(mac);
    SDL_Color color = disabled ? SDL_Color{125, 125, 125, 255} : selected ? SDL_Color{255, 255, 255, 255} : SDL_Color{20, 20, 20, 255};
    int old_style = TTF_GetFontStyle(font);
    int style = (style_flags & 1 ? TTF_STYLE_BOLD : 0) |
        (style_flags & 2 ? TTF_STYLE_ITALIC : 0) |
        (style_flags & 4 ? TTF_STYLE_UNDERLINE : 0);
    TTF_SetFontStyle(font, style);
    SDL_Surface* surface = TTF_RenderText_Blended(font, value.data(), value.size(), color);
    TTF_SetFontStyle(font, old_style);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(r, surface);
    if (texture) {
      SDL_FRect target{static_cast<float>(x), static_cast<float>(y), static_cast<float>(surface->w), static_cast<float>(surface->h)};
      SDL_RenderTexture(r, texture, nullptr, &target);
      SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
  }

  std::shared_ptr<Menu> port_menu() {
    auto menu = std::make_shared<Menu>();
    menu->menu_id = -1;
    menu->title = "Port";
    menu->enabled = true;
    auto add = [&](const char* title, int id) {
      auto& item = menu->items.emplace_back();
      item.name = title;
      if (id >= 0) {
        int checked = 0, enabled = 1;
        PortMenu_ItemState(id, &checked, &enabled);
        item.checked = checked;
        item.enabled = enabled;
      }
    };
    add("Filter", -1);
    add("Scale", -1);
    add("Lock Aspect Ratio", kPortAspectLockId);
    add("-", -1);
    add("Color Correction", -1);
    add("Interface (restart required)", -1);
    return menu;
  }

  std::shared_ptr<Menu> port_submenu(int row) {
    auto menu = std::make_shared<Menu>();
    menu->menu_id = -1;
    menu->enabled = true;
    int start = row == 0 ? kPortFilterId : row == 1 ? kPortScaleId : row == 4 ? kPortGammaId : kPortInterfaceId;
    int count = row == 0 ? kPortFilterCount : row == 1 ? kPortScaleCount : row == 4 ? kPortGammaCount : 2;
    for (int i = 0; i < count; ++i) {
      auto& item = menu->items.emplace_back();
      item.name = row == 0 ? kPortFilters[i].title : row == 1 ? kPortScales[i].title :
          row == 4 ? kPortGammaOptions[i].title : i == 0 ? "Expanded (800 x 600)" : "Classic (640 x 480)";
      int checked = 0, enabled = 1;
      PortMenu_ItemState(start + i, &checked, &enabled);
      item.checked = checked;
      item.enabled = enabled;
    }
    return menu;
  }

  int bar_count() const { return list ? static_cast<int>(list->menus.size()) + 1 : 0; }

  void open_bar_at(float x) {
    if (!list) return;
    int left = 8, index = 0;
    for (const auto& menu : list->menus) {
      int width = text_width(menu->title) + 16;
      if (x >= left && x < left + width) { open_bar(index); return; }
      left += width; ++index;
    }
    if (x >= left && x < left + text_width("Port") + 16) open_bar(index);
  }

  void open_bar(int index) {
    if (!list || index < 0 || index >= bar_count()) return;
    if (active_bar == index && tracking()) return;
    close(false);
    active_bar = index;
    int x = 8, i = 0;
    std::shared_ptr<Menu> target;
    for (const auto& menu : list->menus) {
      if (i++ == index) { target = menu; break; }
      x += text_width(menu->title) + 16;
    }
    bool is_port = index == bar_count() - 1;
    if (is_port) target = port_menu();
    if (target && target->enabled) {
      auto* parent = WindowManager::instance().get_sdl_window().get();
      float px = x, py = kLinuxMenuHeight;
      SDL_RenderCoordinatesToWindow(SDL_GetRenderer(parent), x, kLinuxMenuHeight, &px, &py);
      if (!open_popup(target, parent, static_cast<int>(px), static_cast<int>(py), is_port)) active_bar = -1;
    } else active_bar = -1;
    WindowManager::instance().present_screen();
  }

  bool open_popup(std::shared_ptr<Menu> menu, SDL_Window* parent, int x, int y, bool port) {
    if (!menu || !parent) return false;
    int width = 130;
    for (auto& item : menu->items) width = std::max(width, text_width(item.name) + 82);
    width = std::min(width, kMaxPopupWidth);
    SDL_Rect display{};
    int max_height = 600;
    if (SDL_GetDisplayUsableBounds(SDL_GetDisplayForWindow(parent), &display)) max_height = std::max(kRowHeight, display.h - 24);
    int rows = std::max(1, std::min(static_cast<int>(menu->items.size()), max_height / kRowHeight));
    auto* window = SDL_CreatePopupWindow(parent, x, y, width, rows * kRowHeight, SDL_WINDOW_POPUP_MENU);
    if (!window) { log.error_f("Cannot create popup: {}", SDL_GetError()); return false; }
    auto* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
      log.error_f("Cannot create popup renderer: {}", SDL_GetError());
      SDL_DestroyWindow(window);
      return false;
    }
    popups.push_back(Popup{menu, window, renderer, -1, 0, rows, width, port, port && popups.empty()});
    draw_popup(popups.back());
    return true;
  }

  void draw_popup(Popup& popup) {
    auto* r = popup.renderer;
    SDL_SetRenderDrawColor(r, 245, 245, 245, 255);
    SDL_RenderClear(r);
    for (int visible = 0; visible < popup.visible_rows; ++visible) {
      int i = popup.first_row + visible;
      if (i >= static_cast<int>(popup.menu->items.size())) break;
      const auto& item = popup.menu->items[i];
      int y = visible * kRowHeight;
      if (item.name == "-") {
        SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
        SDL_RenderLine(r, 9, y + 12, popup.width - 9, y + 12);
        continue;
      }
      bool selected = popup.selected == i;
      if (selected) {
        SDL_FRect rect{2, static_cast<float>(y + 1), static_cast<float>(popup.width - 4), kRowHeight - 2};
        SDL_SetRenderDrawColor(r, 62, 91, 146, 255);
        SDL_RenderFillRect(r, &rect);
      }
      if (item.key_equivalent != 0x1B && (item.checked || item.mark_character)) {
        const char* mark = item.mark_character == 19 ? "◆" : item.checked ? "✓" : "−";
        render_text(r, mark, 5, y + 3, selected, !item.enabled, true);
      }
      if (item.icon_id) {
        try {
          auto icon = DecodeCIconImage(item.icon_id);
          auto* surface = SDL_CreateSurfaceFrom(icon.get_width(), icon.get_height(), SDL_PIXELFORMAT_RGBA8888,
              const_cast<uint32_t*>(icon.get_data()), icon.get_width() * 4);
          if (surface) {
            auto* texture = SDL_CreateTextureFromSurface(r, surface);
            if (texture) {
              SDL_FRect dest{24, static_cast<float>(y + 2), 20, 20};
              SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
              SDL_RenderTexture(r, texture, nullptr, &dest);
              SDL_DestroyTexture(texture);
            }
            SDL_DestroySurface(surface);
          }
        } catch (const std::exception&) { }
      }
      render_text(r, item.name, 49, y + 3, selected, !item.enabled, false, item.style_flags);
      bool child = popup.port ? (popup.port_root && (i == 0 || i == 1 || i == 4 || i == 5)) :
          item.key_equivalent == 0x1B && item.mark_character;
      if (child) render_text(r, ">", popup.width - 20, y + 3, selected, !item.enabled);
      else if (item.key_equivalent && item.key_equivalent != 0x1B) {
        std::string key = "Ctrl+";
        key += static_cast<char>(std::toupper(static_cast<unsigned char>(item.key_equivalent)));
        render_text(r, key, popup.width - text_width(key) - 9, y + 3, selected, !item.enabled);
      }
    }
    SDL_RenderPresent(r);
  }

  std::shared_ptr<Menu> submenu_for(const Popup& popup, int row) {
    if (row < 0 || row >= static_cast<int>(popup.menu->items.size())) return nullptr;
    const auto& item = popup.menu->items[row];
    if (popup.port) return (popup.port_root && (row == 0 || row == 1 || row == 4 || row == 5)) ? port_submenu(row) : nullptr;
    if (item.key_equivalent != 0x1B || !item.mark_character || !list) return nullptr;
    for (const auto& menu : list->submenus)
      if (menu->menu_id == static_cast<unsigned char>(item.mark_character)) return menu;
    log.warning_f("Missing hierarchical menu {}", static_cast<int>(static_cast<unsigned char>(item.mark_character)));
    return nullptr;
  }

  void select_row(int depth, int row) {
    if (depth < 0 || depth >= static_cast<int>(popups.size())) return;
    auto& popup = popups[depth];
    if (row < 0 || row >= static_cast<int>(popup.menu->items.size()) || !popup.menu->items[row].enabled || popup.menu->items[row].name == "-") row = -1;
    if (popup.selected == row) return;
    while (static_cast<int>(popups.size()) > depth + 1) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); }
    popup.selected = row;
    draw_popup(popup);
    if (row < 0) return;
    auto child = submenu_for(popup, row);
    if (!child) return;
    int x = popup.width - 5;
    int y = (row - popup.first_row) * kRowHeight;
    open_popup(child, popup.window, x, y, popup.port);
  }

  void activate(int depth, int row) {
    if (depth < 0 || depth >= static_cast<int>(popups.size())) return;
    auto& popup = popups[depth];
    if (row < 0 || row >= static_cast<int>(popup.menu->items.size())) return;
    const auto& item = popup.menu->items[row];
    if (!item.enabled || item.name == "-") return;
    if (submenu_for(popup, row)) { select_row(depth, row); return; }
    if (item.key_equivalent == 0x1B && item.mark_character) return;
    if (popup.port) {
      int id = depth == 0 ? kPortAspectLockId :
          (popups[0].selected == 0 ? kPortFilterId : popups[0].selected == 1 ? kPortScaleId :
           popups[0].selected == 4 ? kPortGammaId : kPortInterfaceId) + row;
      close(false);
      PortMenu_Apply(id);
      return;
    }
    finish(popup.menu->menu_id, row + 1);
  }

  void key(SDL_Keycode keycode) {
    if (!tracking()) return;
    int depth = static_cast<int>(popups.size()) - 1;
    if (keycode == SDLK_F10) {
      finish(0, 0);
    } else if (keycode == SDLK_ESCAPE) {
      if (depth > 0) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); draw_popup(popups.back()); }
      else finish(0, 0);
    } else if (keycode == SDLK_LEFT) {
      if (depth > 0) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); }
      else if (!contextual) open_bar((active_bar + bar_count() - 1) % bar_count());
    } else if (keycode == SDLK_RIGHT) {
      auto& p = popups.back();
      if (p.selected >= 0 && submenu_for(p, p.selected)) select_row(depth, p.selected);
      else if (depth == 0 && !contextual) open_bar((active_bar + 1) % bar_count());
    } else if (keycode == SDLK_UP || keycode == SDLK_DOWN) {
      auto& p = popups.back();
      int count = p.menu->items.size();
      if (count == 0) return;
      for (int n = 0; n < count; ++n) {
        int next = (p.selected + (keycode == SDLK_DOWN ? 1 : count - 1) + count) % count;
        if (p.selected < 0) next = keycode == SDLK_DOWN ? 0 : count - 1;
        if (p.menu->items[next].enabled && p.menu->items[next].name != "-") {
          if (next < p.first_row) p.first_row = next;
          else if (next >= p.first_row + p.visible_rows) p.first_row = next - p.visible_rows + 1;
          select_row(depth, next);
          break;
        }
        p.selected = next;
      }
    } else if (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER) activate(depth, popups.back().selected);
  }

  void close(bool notify) {
    bool was_contextual = contextual;
    auto done = context_callback;
    context_callback = nullptr;
    contextual = false;
    while (!popups.empty()) { SDL_DestroyWindow(popups.back().window); popups.pop_back(); }
    active_bar = -1;
    if (notify && was_contextual && done) done(0, 0);
    if (list) WindowManager::instance().present_screen();
  }

  void finish(int16_t menu_id, int16_t item_id) {
    auto done = contextual ? context_callback : callback;
    close(false);
    if (done) done(menu_id, item_id);
  }
};

Controller controller;
} // namespace

void MCSync(std::shared_ptr<MenuList> list, void (*callback)(int16_t, int16_t)) { controller.sync(std::move(list), callback); }
void MCCreatePopupMenu(void*, std::shared_ptr<Menu> menu, std::pair<int16_t, int16_t> loc,
    void (*callback)(int16_t, int16_t)) {
  controller.show_context(std::move(menu), loc.second, loc.first, callback);
}
void LinuxMenuDrawBar(SDL_Renderer* renderer) { controller.draw_bar(renderer); }
bool LinuxMenuHandleEvent(const SDL_Event& event) { return controller.handle(event); }
bool LinuxMenuIsTracking() { return controller.tracking(); }
bool LinuxMenuHasBar() { return controller.has_bar(); }
int LinuxMenuBarHeight() { return kLinuxMenuHeight; }
void LinuxMenuCancel() { controller.cancel(); }
