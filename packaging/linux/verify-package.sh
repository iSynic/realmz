#!/usr/bin/env bash
set -euo pipefail

archive=${1:?Pass the Linux package tarball}
audit_dir=$(mktemp -d)
trap 'rm -rf -- "$audit_dir"' EXIT
tar -xzf "$archive" -C "$audit_dir"
package_root=$(find "$audit_dir" -mindepth 1 -maxdepth 1 -type d -print -quit)
test -n "$package_root"
game_dir="$package_root/bin"
export XDG_DATA_HOME="$audit_dir/user-data"
test -x "$game_dir/Realmz"
test -f "$game_dir/realmz.rsrc"
test -f "$game_dir/realmz.png"
test -f "$game_dir/ChicagoFLF.ttf"
test -f "$game_dir/InterVariable.ttf"
test -d "$game_dir/Scenarios/Prelude to Pestilence"
test -d "$game_dir/Character Files"
test -d "$game_dir/Data Files"
desktop-file-validate "$package_root/share/applications/org.fantasoft.realmz.desktop"
test -f "$package_root/share/icons/hicolor/256x256/apps/realmz.png"

if [ -d base/Realmz ]; then
  for directory in 'Data Files' Scenarios 'Character Files'; do
    diff -u \
      <(cd "base/Realmz/$directory" && find . -type f -print | LC_ALL=C sort) \
      <(cd "$game_dir/$directory" && find . -type f -print | LC_ALL=C sort)
    while IFS= read -r -d '' source; do
      relative=${source#base/Realmz/}
      cmp -s "$source" "$game_dir/$relative" || {
        echo "Game asset differs: $relative" >&2
        exit 1
      }
    done < <(find "base/Realmz/$directory" -type f -print0)
  done
fi

for binary in "$game_dir/Realmz" "$game_dir"/*.so*; do
  test -f "$binary" || continue
  if ! file -b "$binary" | grep -q ELF; then continue; fi
  if ! readelf -d "$binary" | grep -Fq '$ORIGIN'; then
    echo "$binary has no relative runtime search path" >&2
    exit 1
  fi
  required=$(readelf --version-info "$binary" | grep -oE 'GLIBC_[0-9]+\.[0-9]+' | sed 's/GLIBC_//' | sort -Vu | tail -n 1 || true)
  if [ -n "$required" ] && ! dpkg --compare-versions "$required" le 2.35; then
    echo "$binary requires glibc $required (limit 2.35)" >&2
    exit 1
  fi
  if ldd "$binary" | grep -q 'not found'; then
    echo "$binary has unresolved dependencies" >&2
    ldd "$binary" >&2
    exit 1
  fi
done

chmod -R a-w "$game_dir"
trap 'chmod -R u+w "$game_dir"; rm -rf -- "$audit_dir"' EXIT

# A real display server is required here; SDL's dummy video driver does not
# exercise window creation, popup support or font rendering.
run_smoke() {
  local driver=$1
  local output=$2
  local status=0
  (cd / && SDL_VIDEODRIVER="$driver" timeout 15s "$game_dir/Realmz") >"$output" 2>&1 || status=$?
  if [ "$status" -ne 124 ]; then
    echo "$driver startup exited unexpectedly ($status)" >&2
    tail -n 80 "$output" >&2
    return 1
  fi
  grep -q 'Loading MBAR:' "$output"
}

xvfb-run -a bash -c '
  set -e
  game_dir=$1
  output=$2
  cd /
  status=0
  SDL_VIDEODRIVER=x11 timeout 15s "$game_dir/Realmz" >"$output" 2>&1 || status=$?
  test "$status" -eq 124
  grep -q "Loading MBAR:" "$output"
' _ "$game_dir" "$audit_dir/x11.log"

mkdir "$audit_dir/runtime"
chmod 700 "$audit_dir/runtime"
XDG_RUNTIME_DIR="$audit_dir/runtime" weston --backend=headless-backend.so --socket=realmz-test \
  --idle-time=0 >"$audit_dir/weston.log" 2>&1 &
weston_pid=$!
trap 'kill "$weston_pid" 2>/dev/null || true; chmod -R u+w "$game_dir"; rm -rf -- "$audit_dir"' EXIT
for i in $(seq 1 50); do
  if [ -S "$audit_dir/runtime/realmz-test" ]; then break; fi
  sleep 0.1
done
test -S "$audit_dir/runtime/realmz-test"
export XDG_RUNTIME_DIR="$audit_dir/runtime"
export WAYLAND_DISPLAY=realmz-test
run_smoke wayland "$audit_dir/wayland.log"
