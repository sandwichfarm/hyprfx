# Writing HyprFX Effects

This guide walks through creating a new window effect for HyprFX. Each effect is a single self-contained `.cpp` file in `src/effects/` — no changes to any other file required.

## Quick Start

Create `src/effects/FadeEffect.cpp`:

```cpp
#include "EffectRegistry.hpp"

static const std::string FADE_FRAG = R"#(
void main() {
    float progress = uForOpening ? 1.0 - uProgress : uProgress;
    vec4 oColor = getInputColor(iTexCoord.st);
    oColor.a *= 1.0 - progress;
    setOutputColor(oColor);
}
)#";

class FadeEffect : public IEffect {
  public:
    std::string name() const override { return "fade"; }
    std::string fragmentSource() const override { return FADE_FRAG; }
    void registerConfig(HANDLE handle) const override {}
    std::vector<std::string> uniformNames() const override { return {}; }
    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {}
};

static bool _reg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<FadeEffect>());
    return true;
}();
```

Rebuild (`cmake --build build`) and use it:

```ini
plugin:hfx:close_effect = fade
```

That's it. The rest of this guide covers each piece in detail.

## File Structure

Every effect file follows the same pattern:

```
1. #include "EffectRegistry.hpp"
2. Fragment shader as a static string
3. Class implementing IEffect
4. Static self-registration
```

Only `EffectRegistry.hpp` needs to be included — it pulls in everything else.

## The IEffect Interface

```cpp
class IEffect {
    virtual std::string name() const = 0;
    virtual std::string fragmentSource() const = 0;
    virtual void registerConfig(HANDLE handle) const = 0;
    virtual std::vector<std::string> uniformNames() const = 0;
    virtual void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const = 0;
};
```

| Method | Purpose |
|--------|---------|
| `name()` | Unique string identifier. This is what users type in config (`plugin:hfx:close_effect = yourname`). |
| `fragmentSource()` | GLSL ES 3.0 fragment shader source. Gets prepended with common utilities automatically. |
| `registerConfig()` | Register config values with Hyprland during plugin init. |
| `uniformNames()` | List of effect-specific uniform names to look up after shader compilation. |
| `setUniforms()` | Called every frame. Read config values and set your uniforms via `glUniform*`. |

## Writing the Fragment Shader

Your fragment shader is prepended with `HFX_COMMON`, which provides:

### Available Uniforms (set automatically)

| Uniform | Type | Description |
|---------|------|-------------|
| `uProgress` | `float` | Animation progress from `0.0` (start) to `1.0` (end) |
| `uDuration` | `float` | Total animation duration in seconds |
| `uForOpening` | `bool` | `true` for window open, `false` for close |
| `uSize` | `vec2` | Window size in scaled pixels |
| `uPadding` | `float` | Window padding (usually `0.0`) |
| `tex` | `sampler2D` | The window texture |

### Available Varyings

| Varying | Type | Description |
|---------|------|-------------|
| `iTexCoord` | `vec2` | Texture coordinates (`0.0` to `1.0`) |

### Helper Functions

**Texture access:**
- `vec4 getInputColor(vec2 coords)` — Sample the window texture (handles premultiplied alpha conversion)
- `void setOutputColor(vec4 color)` — Write the output color (converts back to premultiplied alpha)

**Compositing:**
- `vec4 alphaOver(vec4 under, vec4 over)` — Alpha-composite two colors

**Color manipulation:**
- `vec3 tritone(float val, vec3 shadows, vec3 midtones, vec3 highlights)`
- `vec3 darken(vec3 color, float fac)`
- `vec3 lighten(vec3 color, float fac)`
- `vec3 offsetHue(vec3 color, float hueOffset)`

**Easing:**
- `float easeOutQuad(float x)`
- `float easeInQuad(float x)`

**Noise:**
- `float hash11(float)`, `float hash12(vec2)`, `vec2 hash22(vec2)`, `vec3 hash33(vec3)`
- `float simplex2D(vec2)`, `float simplex2DFractal(vec2)`
- `float simplex3D(vec3)`, `float simplex3DFractal(vec3)`

**Edge masks:**
- `float getEdgeMask(vec2 uv, vec2 maxUV, float fadeWidth)`
- `float getAbsoluteEdgeMask(float fadePixels, float offset)`

### Important: Handle Open vs Close

Effects play for both opening and closing windows. The convention is to reverse progress for opening animations so the visual effect plays in reverse:

```glsl
void main() {
    float progress = uForOpening ? 1.0 - uProgress : uProgress;
    // ... rest of effect
}
```

### Shader Rules

- Use GLSL ES 3.0 syntax (`#version 300 es` is already declared in the common header — do **not** redeclare it)
- Do **not** declare `precision` — already set in the common header
- Do **not** redeclare common uniforms (`uProgress`, `tex`, etc.) — they exist in the common header
- **Do** declare your own effect-specific uniforms at the top of your fragment source
- Always call `setOutputColor()` rather than writing to `fragColor` directly

## Config Values

Register config values in `registerConfig()` using `HyprlandAPI::addConfigValue`. All config keys must start with `plugin:hfx:`.

Supported types:

```cpp
// Float
HyprlandAPI::addConfigValue(handle, "plugin:hfx:myeffect_speed", Hyprlang::FLOAT{1.0});

// Integer (also used for booleans: 0/1)
HyprlandAPI::addConfigValue(handle, "plugin:hfx:myeffect_enabled", Hyprlang::INT{1});

// Color (RGBA hex string, stored internally as AARRGGBB)
HyprlandAPI::addConfigValue(handle, "plugin:hfx:myeffect_color", Hyprlang::INT{*configStringToInt("rgba(ff8800ff)")});
```

## Setting Uniforms

In `setUniforms()`, read config values and pass them to your shader. Access your uniform locations through the `shader.uniforms` map using the names from `uniformNames()`.

### Reading Config Values

```cpp
void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
    // Float config
    static auto* const PSPEED = (Hyprlang::FLOAT* const*)
        HyprlandAPI::getConfigValue(handle, "plugin:hfx:myeffect_speed")->getDataStaticPtr();
    glUniform1f(shader.uniforms.at("uSpeed"), static_cast<float>(**PSPEED));

    // Integer/bool config
    static auto* const PENABLED = (Hyprlang::INT* const*)
        HyprlandAPI::getConfigValue(handle, "plugin:hfx:myeffect_enabled")->getDataStaticPtr();
    glUniform1i(shader.uniforms.at("uEnabled"), **PENABLED ? 1 : 0);
}
```

### Reading Color Config Values

Hyprland stores colors as **AARRGGBB** internally. Use this helper to extract RGBA floats:

```cpp
static void colorToVec4(Hyprlang::INT val, float out[4]) {
    uint32_t c = static_cast<uint32_t>(val);
    out[0] = ((c >> 16) & 0xFF) / 255.0f; // R
    out[1] = ((c >> 8) & 0xFF) / 255.0f;  // G
    out[2] = (c & 0xFF) / 255.0f;         // B
    out[3] = ((c >> 24) & 0xFF) / 255.0f; // A
}

// In setUniforms():
static auto* const PCOLOR = (Hyprlang::INT* const*)
    HyprlandAPI::getConfigValue(handle, "plugin:hfx:myeffect_color")->getDataStaticPtr();
float c[4];
colorToVec4(**PCOLOR, c);
glUniform4f(shader.uniforms.at("uColor"), c[0], c[1], c[2], c[3]);
```

### The `seed` Parameter

The `seed` parameter is derived from the window's PID and is unique per window. Use it to make effects vary between windows (e.g., different random patterns):

```glsl
uniform float uSeed;
// Use in noise: simplex2D(uv + uSeed)
```

```cpp
// In uniformNames():
return {"uSeed"};

// In setUniforms():
glUniform1f(shader.uniforms.at("uSeed"), seed);
```

## Self-Registration

The static initializer at the bottom of your file registers the effect before `main()` runs:

```cpp
static bool _reg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<MyEffect>());
    return true;
}();
```

This is picked up automatically — CMake uses `file(GLOB_RECURSE)` so new files in `src/effects/` are included in the build with no CMake changes.

## Complete Example: Dissolve Effect

```cpp
#include "EffectRegistry.hpp"

static const std::string DISSOLVE_FRAG = R"#(
uniform float uGrainSize;
uniform float uSeed;

void main() {
    float progress = uForOpening ? 1.0 - uProgress : uProgress;

    vec2 grain = floor(iTexCoord.st * uSize / uGrainSize) * uGrainSize;
    float noise = hash12(grain + uSeed);

    vec4 oColor = getInputColor(iTexCoord.st);

    float threshold = progress * 1.2;
    if (noise < threshold) {
        float edge = smoothstep(threshold - 0.1, threshold, noise);
        oColor.a *= edge;
    }

    setOutputColor(oColor);
}
)#";

class DissolveEffect : public IEffect {
  public:
    std::string name() const override { return "dissolve"; }
    std::string fragmentSource() const override { return DISSOLVE_FRAG; }

    void registerConfig(HANDLE handle) const override {
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:dissolve_grain_size", Hyprlang::FLOAT{4.0});
    }

    std::vector<std::string> uniformNames() const override {
        return {"uGrainSize", "uSeed"};
    }

    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
        static auto* const PGRAIN = (Hyprlang::FLOAT* const*)
            HyprlandAPI::getConfigValue(handle, "plugin:hfx:dissolve_grain_size")->getDataStaticPtr();
        glUniform1f(shader.uniforms.at("uGrainSize"), static_cast<float>(**PGRAIN));
        glUniform1f(shader.uniforms.at("uSeed"), seed);
    }
};

static bool _reg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<DissolveEffect>());
    return true;
}();
```

## Testing

1. Build: `cmake -B build && cmake --build build`
2. Launch a nested Hyprland: `Hyprland -c test.conf`
3. Switch effects at runtime: `hyprctl keyword plugin:hfx:close_effect dissolve`
4. Open/close windows to see your effect in action
