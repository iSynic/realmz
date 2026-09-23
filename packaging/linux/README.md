# Linux build

Extract the archive, then run `bin/Realmz` from any working directory. Game
resources and bundled libraries must stay beside the executable in `bin`.

To add a desktop launcher, install the build with
`cmake --install build-linux --prefix "$HOME/.local"` and make sure
`$HOME/.local/bin` is on your PATH. The installed desktop entry and icon are
in `$HOME/.local/share`.

Realmz stores preferences, saves, and user-added scenarios under the SDL
preference path for `Fantasoft/Realmz` (normally
`$HOME/.local/share/Fantasoft/Realmz/`). This keeps the installation read-only.

The portable build targets Ubuntu 22.04 or newer on x86-64 with an X11 or
Wayland desktop. SDL uses the system's desktop and audio services.

See `PARITY-CHECKLIST.md` in the source tree for current verification status.
