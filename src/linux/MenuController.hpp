#pragma once

#include <SDL3/SDL.h>

// The game keeps an 800x600 canvas. The Linux menu strip occupies additional
// logical window space above it; these helpers keep Toolbox coordinates local
// to the game canvas.
inline constexpr int kLinuxMenuHeight = 20;
inline constexpr int kLinuxMenuRowHeight = 22;

void LinuxMenuDrawBar(SDL_Renderer* renderer);
bool LinuxMenuHandleEvent(const SDL_Event& event);
bool LinuxMenuIsTracking();
bool LinuxMenuHasBar();
int LinuxMenuBarHeight();
void LinuxMenuCancel();
