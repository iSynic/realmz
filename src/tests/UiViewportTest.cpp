#include <stdexcept>

#include "UiViewport.h"

static void check(bool condition) {
  if (!condition) {
    throw std::runtime_error("Viewport geometry regression");
  }
}

static void check_world(int world, int center, int visible, int expected_origin = -1, int expected_local = -1) {
  int origin = -1, local = -1;
  ui_viewport_recenter(world, center, visible, &origin, &local);
  check(origin >= 0 && origin <= 90 - visible);
  check(local >= 0 && local < visible);
  check(origin + local == world);
  if (expected_origin >= 0) {
    check(origin == expected_origin && local == expected_local);
  }
  int second_origin = -1, second_local = -1;
  ui_viewport_recenter(origin + local, center, visible, &second_origin, &second_local);
  check(second_origin == origin && second_local == local);
}

int main() {
  auto classic = ui_viewport_geometry(0);
  auto expanded = ui_viewport_geometry(1);
  check(classic.columns == 10 && classic.rows == 10);
  check(classic.exploration_center_x == 5 && classic.exploration_center_y == 5);
  check(classic.combat_center_x == 5 && classic.combat_center_y == 5);
  check(expanded.columns == 15 && expanded.rows == 13);
  check(expanded.exploration_center_x == 8 && expanded.exploration_center_y == 6);
  check(expanded.combat_center_x == 7 && expanded.combat_center_y == 6);

  check_world(10, classic.exploration_center_x, classic.columns, 5, 5);
  check_world(8, classic.exploration_center_y, classic.rows, 3, 5);
  check_world(10, expanded.exploration_center_x, expanded.columns, 2, 8);
  check_world(8, expanded.exploration_center_y, expanded.rows, 2, 6);
  check_world(0, 5, 10, 0, 0);
  check_world(89, 5, 10, 80, 9);

  for (int large_screen = 0; large_screen < 2; large_screen++) {
    auto viewport = ui_viewport_geometry(large_screen);
    for (int world = 0; world < 90; world++) {
      check_world(world, viewport.exploration_center_x, viewport.columns);
      check_world(world, viewport.exploration_center_y, viewport.rows);
      check_world(world, viewport.combat_center_x, viewport.columns);
      check_world(world, viewport.combat_center_y, viewport.rows);
    }
    // Camera movement changes local positions equally for PCs and monsters.
    int old_origin = 45, new_origin = -1, local = -1;
    int character_local = 5, monster_local = 7;
    ui_viewport_recenter(old_origin + 4, viewport.combat_center_x, viewport.columns, &new_origin, &local);
    int delta = new_origin - old_origin;
    check(new_origin + (character_local - delta) == old_origin + character_local);
    check(new_origin + (monster_local - delta) == old_origin + monster_local);
  }

  check(ui_viewport_map_origin(320) == 0);
  check(ui_viewport_map_origin(480) == 80);
  check(ui_viewport_map_origin(416) == 48);
  return 0;
}
