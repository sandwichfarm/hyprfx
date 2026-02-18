#include "EffectRegistry.hpp"

static const std::string PIXELATE_FRAG = R"#(
uniform float uPixelSize;
uniform float uNoise;

void main() {
    float progress = uForOpening ? 1.0 - uProgress : uProgress;

    float pixelSize = ceil(uPixelSize * progress + 1.0);
    vec2 pixelGrid = vec2(pixelSize) / uSize;
    vec2 texcoord = iTexCoord.st - mod(iTexCoord.st, pixelGrid) + pixelGrid * 0.5;
    vec4 oColor = getInputColor(texcoord);

    float random = simplex2DFractal(texcoord * uNoise * uSize / 1000.0) * 1.5 - 0.25;
    if (progress > random) {
        oColor.a *= max(0.0, 1.0 - (progress - random) * 20.0);
    }

    setOutputColor(oColor);
}
)#";

class PixelateEffect : public IEffect {
  public:
    std::string name() const override { return "pixelate"; }
    std::string fragmentSource() const override { return PIXELATE_FRAG; }

    void registerConfig(HANDLE handle) const override {
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:pixelate_pixel_size", Hyprlang::FLOAT{40.0});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:pixelate_noise", Hyprlang::FLOAT{0.5});
    }

    std::vector<std::string> uniformNames() const override {
        return {"uPixelSize", "uNoise"};
    }

    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
        static auto* const PPIXSIZE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:pixelate_pixel_size")->getDataStaticPtr();
        static auto* const PPIXNOISE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:pixelate_noise")->getDataStaticPtr();
        glUniform1f(shader.uniforms.at("uPixelSize"), static_cast<float>(**PPIXSIZE));
        glUniform1f(shader.uniforms.at("uNoise"), static_cast<float>(**PPIXNOISE));
    }
};

static bool _pixelateReg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<PixelateEffect>());
    return true;
}();
