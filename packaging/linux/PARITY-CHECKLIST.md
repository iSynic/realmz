# Linux parity verification

This checklist tracks evidence separately from implementation. A passing build or
headless smoke test is not a substitute for gameplay parity.

| Area | Evidence required | Status |
| --- | --- | --- |
| Resource-backed menus | Open real MBAR/MENU resources and exercise popup fixture selection/cancellation/submenus | Local X11 and Wayland CTest pass on Ubuntu 24.04 WSLg |
| Keyboard and input | Shortcuts, no duplicate dispatch or click-through, focus/quit cancellation, long-menu scroll | Partly covered by synthetic menu test; interactive checks pending |
| Geometry | 800x600 content and 26 logical-pixel menu strip, round trips, 100/150/200% scaling, aspect lock | Default geometry covered by CTest; display-scaling checks pending |
| Rendering | Current-main dialog frames and icons unchanged after menu redraw | Source change kept in Linux-only rendering path; screenshot comparison pending |
| Storage | Writable copy of bundled data from a read-only install | LinuxStorageTest passes locally; gameplay save/reload pending |
| Package | Complete assets, relative ELF paths, glibc 2.35 baseline, desktop entry | CPack tarball generated locally; clean Ubuntu 22.04/24.04 artifact audit pending |
| X11/Wayland | Real SDL windows and menu popups | WSLg tests pass; Xvfb/Weston clean-runner smoke pending |
| Audio | Sound effects and volume controls | WSLg launch detects RDP Sink; interactive audio check pending |
| Game parity | Identical scenario/save action sequence versus Windows at same revision | Pending |
| Desktop environments | GNOME/Wayland, KDE/Wayland, X11, WSLg | WSLg exercised; other desktops pending |
| Other ports | Rebuild and test Windows/macOS native menus | CI build pending; interactive native-menu check pending |

Workflow-dispatch runs upload test artifacts without publishing a GitHub release.
The `release` job is guarded to run only for pushed `v*` tags.
