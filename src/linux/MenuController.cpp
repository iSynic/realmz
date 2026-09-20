#include "../MenuController.h"

// Linux does not currently have a native menu backend. Keep menu calls
// non-blocking so the rest of the SDL port can build and run while preserving
// the platform-specific menu implementations on macOS and Windows.
void MCSync(std::shared_ptr<MenuList>, void (*)(int16_t, int16_t)) {}

void MCCreatePopupMenu(
    void*,
    std::shared_ptr<Menu>,
    std::pair<int16_t, int16_t>,
    void (*callback)(int16_t, int16_t)) {
  if (callback != nullptr) {
    callback(0, 0);
  }
}
