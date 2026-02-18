# HyprFX

GPU-accelerated window open/close effects for [Hyprland](https://hyprland.org).

## Effects

| Effect | Preview |
|--------|---------|
| **Fire** — Windows burn away with a procedural flame simulation | ![Fire effect](assets/fire.gif) |
| **TV** — CRT television power-off effect with horizontal/vertical collapse | ![TV effect](assets/tv.gif) |
| **Pixelate** — Windows dissolve into increasingly large pixels | ![Pixelate effect](assets/pixelate.gif) |

## Installation

### hyprpm

```sh
hyprpm add https://github.com/sandwich/hyprfx
hyprpm enable hyprfx
```

### Manual

```sh
cmake -B build
cmake --build build
```

Then add to your Hyprland config:

```ini
plugin = /path/to/libhyprfx.so
```

## Configuration

```ini
plugin:hfx:close_effect = fire
plugin:hfx:open_effect = fire
plugin:hfx:duration = 1.0
```

### Fire

```ini
plugin:hfx:fire_color_1 = rgba(c8200000)
plugin:hfx:fire_color_2 = rgba(f04000a0)
plugin:hfx:fire_color_3 = rgba(ff8800dd)
plugin:hfx:fire_color_4 = rgba(ffcc00f0)
plugin:hfx:fire_color_5 = rgba(ffff66fe)
plugin:hfx:fire_scale = 1.0
plugin:hfx:fire_3d_noise = 1
plugin:hfx:fire_movement_speed = 1.0
```

### TV

```ini
plugin:hfx:tv_color = rgba(c8c8c8ff)
```

### Pixelate

```ini
plugin:hfx:pixelate_pixel_size = 40.0
plugin:hfx:pixelate_noise = 0.5
```

## Adding a New Effect

Create a single file in `src/effects/` — no changes to any other file required:

```cpp
#include "EffectRegistry.hpp"

static const std::string MY_FRAG = R"#(
// your GLSL fragment shader here
void main() {
    // ...
    setOutputColor(oColor);
}
)#";

class MyEffect : public IEffect {
  public:
    std::string name() const override { return "myeffect"; }
    std::string fragmentSource() const override { return MY_FRAG; }
    void registerConfig(HANDLE handle) const override { /* ... */ }
    std::vector<std::string> uniformNames() const override { return {}; }
    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override { /* ... */ }
};

static bool _ = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<MyEffect>());
    return true;
}();
```

Then rebuild. The effect is available as `plugin:hfx:open_effect = myeffect`.

## Credits

Shader effects ported from [Burn My Windows](https://github.com/Schneegans/Burn-My-Windows) by Simon Schneegans.

## License

GPL-3.0
