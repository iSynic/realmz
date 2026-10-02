#include "../MenuController.h"

// Linux has no native menu backend yet.
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
