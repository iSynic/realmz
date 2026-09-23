#include "PortMenu.hpp"
#include "PortPrefs.hpp"
#include "WindowManager.hpp"

void PortMenu_Apply(int id) {
  WindowManager& wm = WindowManager::instance();
  if (id < kPortScaleId) {
    wm.set_scale_mode(kPortFilters[id - kPortFilterId].mode);
  } else if (id < kPortAspectLockId) {
    const auto& scale = kPortScales[id - kPortScaleId];
    wm.set_window_size(ui_layout_width() * scale.half_steps / 2, ui_layout_height() * scale.half_steps / 2);
  } else if (id < kPortGammaId) {
    wm.set_aspect_locked(!wm.get_aspect_locked());
  } else if (id < kPortInterfaceId) {
    wm.set_gamma_idx(id - kPortGammaId);
  } else {
    wm.set_pending_ui_layout(id == kPortInterfaceId ? UiLayout::Expanded : UiLayout::Classic);
  }
}

void PortMenu_ItemState(int id, int* checked, int* enabled) {
  WindowManager& wm = WindowManager::instance();
  int is_checked = 0;
  int is_enabled = 1;
  bool fullscreen = wm.is_fullscreen();
  if (id < kPortScaleId) {
    is_checked = kPortFilters[id - kPortFilterId].mode == wm.get_scale_mode();
  } else if (id < kPortAspectLockId) {
    const auto& scale = kPortScales[id - kPortScaleId];
    int scale_w = ui_layout_width() * scale.half_steps / 2;
    int scale_h = ui_layout_height() * scale.half_steps / 2;
    is_enabled = !fullscreen && wm.size_fits(scale_w, scale_h);
    int cur_w = 0, cur_h = 0;
    wm.get_window_size(&cur_w, &cur_h);
    is_checked = !fullscreen && (cur_w == scale_w) && (cur_h == scale_h);
  } else if (id < kPortGammaId) {
    is_enabled = !fullscreen;
    is_checked = wm.get_aspect_locked();
  } else if (id < kPortInterfaceId) {
    is_checked = (id - kPortGammaId) == wm.get_gamma_idx();
  } else {
    is_checked = wm.get_pending_ui_layout() == (id == kPortInterfaceId ? UiLayout::Expanded : UiLayout::Classic);
  }
  if (checked) {
    *checked = is_checked;
  }
  if (enabled) {
    *enabled = is_enabled;
  }
}
