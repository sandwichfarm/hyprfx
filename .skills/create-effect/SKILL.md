---
name: create-effect
description: Create a new window effect for the HyprFX Hyprland plugin. Use when the user wants to add a new visual effect for window open/close animations, port a shader effect, or implement a custom GLSL effect.
compatibility: Requires cmake and hyprland development headers to build and test.
repository: https://github.com/sandwichfarm/hyprfx
---

# Creating a New HyprFX Effect

## Overview

Each effect is a single self-contained `.cpp` file in `src/effects/`. No other files need to be modified — the build system and effect registry handle everything automatically.

## Step-by-Step Process

### 1. Understand the Request

Clarify with the user:
- What the effect should look like (reference videos, descriptions, or existing shader code)
- Whether it needs user-configurable parameters
- Whether it should behave differently for open vs close animations

### 2. Read the Effect Guide

Read [EFFECTS.md](../../EFFECTS.md) for the complete reference on:
- The `IEffect` interface
- Available GLSL uniforms and helper functions
- Config value registration
- Color byte order (AARRGGBB)
- The `seed` parameter

### 3. Study an Existing Effect

Read one of the existing effects as a pattern to follow:
- `src/effects/PixelateEffect.cpp` — simplest, good starting point
- `src/effects/TVEffect.cpp` — uses color config
- `src/effects/FireEffect.cpp` — most complex, multiple uniforms and color gradient

### 4. Create the Effect File

Create `src/effects/YourEffect.cpp` with this structure:

```cpp
#include "EffectRegistry.hpp"

// 1. Fragment shader (GLSL ES 3.0, no #version or precision — provided by HFX_COMMON)
static const std::string EFFECT_FRAG = R"#(
// Declare effect-specific uniforms here
// uniform float uMyParam;

void main() {
    // Reverse progress for open animations
    float progress = uForOpening ? 1.0 - uProgress : uProgress;

    // Sample the window texture
    vec4 oColor = getInputColor(iTexCoord.st);

    // Apply your effect
    // ...

    // Write output (handles premultiplied alpha conversion)
    setOutputColor(oColor);
}
)#";

// 2. For color configs, include this helper
// static void colorToVec4(Hyprlang::INT val, float out[4]) {
//     uint32_t c = static_cast<uint32_t>(val);
//     out[0] = ((c >> 16) & 0xFF) / 255.0f; // R
//     out[1] = ((c >> 8) & 0xFF) / 255.0f;  // G
//     out[2] = (c & 0xFF) / 255.0f;         // B
//     out[3] = ((c >> 24) & 0xFF) / 255.0f; // A
// }

// 3. Effect class
class YourEffect : public IEffect {
  public:
    std::string name() const override { return "youreffect"; }
    std::string fragmentSource() const override { return EFFECT_FRAG; }

    void registerConfig(HANDLE handle) const override {
        // Register config values (prefix: plugin:hfx:youreffect_)
        // HyprlandAPI::addConfigValue(handle, "plugin:hfx:youreffect_param", Hyprlang::FLOAT{1.0});
    }

    std::vector<std::string> uniformNames() const override {
        // Return names of effect-specific uniforms to look up after compilation
        return {};
    }

    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
        // Read config and set uniforms each frame
        // static auto* const PPARAM = (Hyprlang::FLOAT* const*)
        //     HyprlandAPI::getConfigValue(handle, "plugin:hfx:youreffect_param")->getDataStaticPtr();
        // glUniform1f(shader.uniforms.at("uMyParam"), static_cast<float>(**PPARAM));
    }
};

// 4. Self-registration (runs at library load time)
static bool _reg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<YourEffect>());
    return true;
}();
```

### 5. Write the GLSL Shader

Key rules:
- Do NOT declare `#version 300 es` or `precision mediump float` — already in `HFX_COMMON`
- Do NOT redeclare common uniforms (`uProgress`, `uSize`, `tex`, `uForOpening`, `uDuration`, `uPadding`)
- DO declare your own effect-specific uniforms at the top
- Always use `getInputColor()` to sample and `setOutputColor()` to write
- Handle open vs close: `float progress = uForOpening ? 1.0 - uProgress : uProgress;`

Available helpers from `HFX_COMMON` (read `src/shaders.hpp` for full source):
- Noise: `simplex2D()`, `simplex2DFractal()`, `simplex3D()`, `simplex3DFractal()`, `hash11()`, `hash12()`, `hash22()`, `hash33()`
- Easing: `easeOutQuad()`, `easeInQuad()`
- Color: `alphaOver()`, `tritone()`, `darken()`, `lighten()`, `offsetHue()`
- Edge: `getEdgeMask()`, `getAbsoluteEdgeMask()`

### 6. Build and Test

```sh
cmake --build build
```

No cmake reconfigure needed — `file(GLOB_RECURSE)` picks up new files in `src/effects/`.

If the file is brand new and wasn't present during the last `cmake -B build`, run:

```sh
cmake -B build && cmake --build build
```

Test in a nested Hyprland:

```sh
Hyprland -c test.conf
```

Switch to the new effect at runtime:

```sh
hyprctl keyword plugin:hfx:close_effect youreffect
hyprctl keyword plugin:hfx:open_effect youreffect
```

### 7. Update Documentation

Add the new effect to:
- `README.md` — effects table with preview placeholder
- `README.md` — configuration section with the effect's config keys

## Common Mistakes

- **Redeclaring `#version` or common uniforms** — causes shader compilation failure
- **Wrong color byte order** — Hyprland uses AARRGGBB, not RRGGBBAA
- **Forgetting to list uniforms in `uniformNames()`** — uniform location won't be looked up, `shader.uniforms.at()` will throw
- **Not handling open animations** — always check `uForOpening` and reverse progress accordingly
- **Writing to `fragColor` directly** — use `setOutputColor()` which handles premultiplied alpha
