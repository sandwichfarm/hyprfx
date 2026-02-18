# HyprFX — Agent Guide

Instructions for AI agents working on, building, installing, or using this project.

## Project Overview

HyprFX is a Hyprland plugin that provides GPU-accelerated window open/close effects (fire, TV shutdown, pixelate, etc.). It uses a modular effect registry — adding a new effect means creating one file in `src/effects/` with zero changes to core code.

## Project Layout

```
src/
  main.cpp              — Plugin entry point (PLUGIN_INIT/PLUGIN_EXIT, event hooks)
  globals.hpp           — Global state, PHANDLE, SClosingAnimation
  shaders.hpp           — Shared GLSL: HFX_VERT (vertex shader) + HFX_COMMON (fragment utilities)
  decoration.hpp/cpp    — CHFXDecoration: window decoration for open animations
  passElement.hpp/cpp   — CHFXPassElement: render pass that dispatches to effects
  effects/
    IEffect.hpp         — IEffect interface + SHFXShader struct
    EffectRegistry.hpp  — Singleton registry (compile, lookup, config registration)
    EffectRegistry.cpp  — Registry implementation + shader compilation helpers
    FireEffect.cpp      — Fire effect (self-registering)
    TVEffect.cpp        — TV shutdown effect (self-registering)
    PixelateEffect.cpp  — Pixelate effect (self-registering)
CMakeLists.txt          — Build config (uses GLOB_RECURSE, new files auto-included)
hyprpm.toml             — hyprpm package manifest
test.conf               — Minimal Hyprland config for testing in nested instance
demo.sh                 — Demo script that cycles through effects
EFFECTS.md              — Guide for writing new effects
```

## Naming Conventions

- Plugin name: **hyprfx**
- Config prefix: `plugin:hfx:`
- C++ class/struct prefix: `HFX` (e.g., `CHFXDecoration`, `SHFXShader`, `SHFXData`)
- GLSL constant prefix: `HFX_` (e.g., `HFX_VERT`, `HFX_COMMON`)
- Log prefix: `[HFX]`
- Output binary: `libhyprfx.so`

## Building

### Prerequisites

- Hyprland development headers (matching the installed Hyprland version)
- CMake 3.27+
- C++23 compiler
- pkg-config dependencies: hyprland, libdrm, libinput, libudev, pangocairo, pixman-1, wayland-server, xkbcommon

### Build Commands

```sh
cmake -B build
cmake --build build
```

Output: `build/libhyprfx.so`

If a clean rebuild is needed:

```sh
rm -rf build && cmake -B build && cmake --build build
```

### Build Troubleshooting

- **Header mismatch errors**: The Hyprland headers must match the running Hyprland version exactly. If using hyprpm, it handles this automatically.
- **Missing `fullVerts`**: This comes from `hyprland/src/render/OpenGL.hpp` — make sure hyprland dev headers are installed.

## Installation

### Via hyprpm (recommended)

```sh
hyprpm add .        # from the repo root
hyprpm enable hyprfx
```

### Manual

Add to `hyprland.conf`:

```ini
plugin = /absolute/path/to/build/libhyprfx.so
```

## Configuration

All config keys use the `plugin:hfx:` prefix.

### Core Settings

```ini
plugin:hfx:close_effect = fire       # Effect for closing windows (fire|tv|pixelate|none)
plugin:hfx:open_effect = fire        # Effect for opening windows
plugin:hfx:duration = 1.0            # Animation duration in seconds
```

### Per-Effect Settings

See `README.md` for the full config reference per effect.

### Runtime Changes

Effects can be switched at runtime without reloading:

```sh
hyprctl keyword plugin:hfx:close_effect tv
hyprctl keyword plugin:hfx:duration 0.5
```

## Testing

Launch a nested Hyprland instance with the test config:

```sh
Hyprland -c test.conf
```

This loads the plugin and runs `demo.sh` which cycles through all effects. Use `ALT+RETURN` to open kitty, `ALT+SHIFT+X` to close a window, `ALT+SHIFT+M` to exit the nested session.

## Architecture Notes

### How Effects Work

1. **Open animations**: `onOpenWindow` attaches a `CHFXDecoration` to the window. The decoration's `draw()` method creates a `CHFXPassElement` which renders the effect over the live window texture.

2. **Close animations**: `onCloseWindow` captures a snapshot of the window texture and stores a `SClosingAnimation` in global state. The `onRender` hook (at `RENDER_POST_WINDOWS` stage) creates `CHFXPassElement` instances for active closing animations, rendering the effect over the snapshot.

3. **Effect dispatch**: `CHFXPassElement::renderEffect()` looks up the effect and shader by name from `EffectRegistry`, sets common uniforms, calls `effect->setUniforms()` for effect-specific uniforms, then draws a fullscreen quad through the shader.

### Adding a New Effect

Create a single `.cpp` file in `src/effects/`. See `EFFECTS.md` for the complete guide. The minimal structure:

```cpp
#include "EffectRegistry.hpp"

static const std::string MY_FRAG = R"#(
void main() {
    float progress = uForOpening ? 1.0 - uProgress : uProgress;
    vec4 oColor = getInputColor(iTexCoord.st);
    oColor.a *= 1.0 - progress;
    setOutputColor(oColor);
}
)#";

class MyEffect : public IEffect {
  public:
    std::string name() const override { return "myeffect"; }
    std::string fragmentSource() const override { return MY_FRAG; }
    void registerConfig(HANDLE handle) const override {}
    std::vector<std::string> uniformNames() const override { return {}; }
    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {}
};

static bool _ = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<MyEffect>());
    return true;
}();
```

No CMake changes needed — `file(GLOB_RECURSE)` picks it up automatically.

### Modifying Core Code

If you need to change core rendering behavior (not just add an effect), the relevant files are:

| What | Where |
|------|-------|
| Plugin lifecycle, event hooks | `src/main.cpp` |
| Open animation decoration | `src/decoration.hpp/cpp` |
| Render pass (uniform dispatch, GL calls) | `src/passElement.hpp/cpp` |
| Global state, closing animation struct | `src/globals.hpp` |
| Shared GLSL (vertex shader, common fragment utilities) | `src/shaders.hpp` |
| Effect interface | `src/effects/IEffect.hpp` |
| Shader compilation, registry | `src/effects/EffectRegistry.hpp/cpp` |

## Common Pitfalls

- **Color byte order**: Hyprland stores colors as **AARRGGBB** (not RRGGBBAA). When decoding an `Hyprlang::INT` color: `A = c >> 24`, `R = (c >> 16) & 0xFF`, `G = (c >> 8) & 0xFF`, `B = c & 0xFF`. See `Hyprland/src/helpers/Color.cpp` for reference.
- **GLSL version/precision**: Do not declare `#version` or `precision` in effect fragment shaders — `HFX_COMMON` already declares them.
- **Common uniforms**: Do not redeclare `uProgress`, `uSize`, `tex`, etc. in your fragment shader — they exist in `HFX_COMMON`.
- **Static config pointers**: Use `static auto* const` for config value pointers in `setUniforms()` — they are looked up once and cached.
- **sudo**: Do not attempt sudo commands. If elevated privileges are needed, provide the user with copy-paste commands to run themselves.
