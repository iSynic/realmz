#ifndef UI_VIEWPORT_H
#define UI_VIEWPORT_H

/* The 90 by 90 field is shared by exploration and combat. */
typedef struct UiViewportGeometry {
  int columns;
  int rows;
  int exploration_center_x;
  int exploration_center_y;
  int combat_center_x;
  int combat_center_y;
} UiViewportGeometry;

static inline UiViewportGeometry ui_viewport_geometry(short large_screen) {
  if (large_screen) {
    UiViewportGeometry geometry = {15, 13, 8, 6, 7, 6};
    return geometry;
  }
  UiViewportGeometry geometry = {10, 10, 5, 5, 5, 5};
  return geometry;
}

static inline void ui_viewport_recenter(int world, int center, int visible, int* origin, int* local) {
  int next_origin = world - center;
  int max_origin = 90 - visible;
  if (next_origin < 0) {
    next_origin = 0;
  } else if (next_origin > max_origin) {
    next_origin = max_origin;
  }
  int next_local = world - next_origin;
  if (next_local < 0) {
    next_local = 0;
  } else if (next_local >= visible) {
    next_local = visible - 1;
  }
  *origin = next_origin;
  *local = next_local;
}

static inline int ui_viewport_map_origin(int viewport_pixels) {
  return (viewport_pixels - 320) / 2;
}

#endif
