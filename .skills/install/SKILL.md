---
name: install
description: Install, update, or uninstall the HyprFX plugin for Hyprland. Use when the user wants to install hyprfx, set up the plugin, load it into Hyprland, update to a new version, or remove it.
compatibility: Requires Hyprland compositor with matching development headers, cmake 3.27+, and a C++23 compiler. Linux only.
---

# HyprFX Installation

## Determine Installation Method

Ask the user which method they prefer if not obvious from context:

1. **hyprpm** (recommended) — Hyprland's built-in plugin manager. Handles header matching, building, and loading automatically.
2. **Manual** — Build from source and load the `.so` directly. More control, useful for development.

---

## Method 1: Install via hyprpm

### From a Git Remote

```sh
hyprpm add https://github.com/sandwich/hyprfx
hyprpm enable hyprfx
```

### From a Local Clone

```sh
hyprpm add /path/to/hyprfx
hyprpm enable hyprfx
```

### Verify

```sh
hyprpm list
```

The output should show `hyprfx` as enabled.

### Update

```sh
hyprpm update
```

### Uninstall

```sh
hyprpm disable hyprfx
hyprpm remove hyprfx
```

---

## Method 2: Manual Installation

### Prerequisites

The following system packages are required. Provide the user with the appropriate install command for their distro — do NOT run sudo yourself.

**Arch Linux:**
```sh
sudo pacman -S cmake hyprland libdrm libinput systemd pango pixman wayland libxkbcommon
```

**Fedora:**
```sh
sudo dnf install cmake hyprland-devel libdrm-devel libinput-devel systemd-devel pango-devel pixman-devel wayland-devel libxkbcommon-devel
```

**NixOS:** Use the Hyprland flake's development shell or ensure `hyprland.dev` is available.

### Build

```sh
cmake -B build
cmake --build build
```

The output binary is `build/libhyprfx.so`.

### Load into Hyprland

Add to `~/.config/hypr/hyprland.conf`:

```ini
plugin = /absolute/path/to/hyprfx/build/libhyprfx.so
```

Then reload Hyprland:

```sh
hyprctl reload
```

### Verify

A green notification saying "HyprFX initialized successfully!" should appear. You can also check:

```sh
hyprctl plugin list
```

### Configure

Add effect settings to `hyprland.conf`:

```ini
plugin:hfx:close_effect = fire
plugin:hfx:open_effect = fire
plugin:hfx:duration = 1.0
```

Available effects: `fire`, `tv`, `pixelate`, `none`.

Effects can be changed at runtime without reloading:

```sh
hyprctl keyword plugin:hfx:close_effect tv
```

### Update (Manual)

Pull latest changes and rebuild:

```sh
git pull
cmake -B build && cmake --build build
hyprctl reload
```

### Uninstall (Manual)

1. Remove the `plugin = ...` line from `hyprland.conf`
2. Reload: `hyprctl reload`
3. Optionally delete the build directory: `rm -rf build/`

---

## Troubleshooting

### "Hash mismatch" warning on load

The plugin was built against different Hyprland headers than the running Hyprland version. Rebuild against the correct headers:

```sh
rm -rf build && cmake -B build && cmake --build build
```

If using hyprpm, run `hyprpm update` which handles header matching automatically.

### Plugin fails to load silently

Check the Hyprland log:

```sh
cat /tmp/hypr/$HYPRLAND_INSTANCE_SIGNATURE/hyprland.log | grep -i "hfx\|plugin"
```

Common causes:
- Wrong absolute path in `plugin = ...`
- Binary built for a different Hyprland version
- Missing shared library dependencies (`ldd build/libhyprfx.so` to check)

### Build fails with missing headers

Ensure hyprland development headers are installed and match the running version. On Arch: `pacman -S hyprland`. For git builds of Hyprland, the headers must come from the same commit.
