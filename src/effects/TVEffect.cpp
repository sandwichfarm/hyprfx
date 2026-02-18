#include "EffectRegistry.hpp"

static const std::string TV_FRAG = R"#(
uniform vec4 uColor;

const float BLUR_WIDTH = 0.01;
const float TB_TIME    = 0.7;
const float LR_TIME    = 0.4;
const float LR_DELAY   = 0.6;
const float FF_TIME    = 0.1;
const float SCALING    = 0.5;

void main() {
    float prog = uForOpening ? 1.0 - easeOutQuad(uProgress) : easeOutQuad(uProgress);

    float scale = 1.0 / mix(1.0, SCALING, prog) - 1.0;
    vec2 coords = iTexCoord.st;
    coords.y = coords.y * (scale + 1.0) - scale * 0.5;

    float tbProg = smoothstep(0.0, 1.0, clamp(prog / TB_TIME, 0.0, 1.0));
    float lrProg = smoothstep(0.0, 1.0, clamp((prog - LR_DELAY) / LR_TIME, 0.0, 1.0));
    float ffProg = smoothstep(0.0, 1.0, clamp((prog - 1.0 + FF_TIME) / FF_TIME, 0.0, 1.0));

    float tb = coords.y * 2.0;
    tb = tb < 1.0 ? tb : 2.0 - tb;

    float lr = coords.x * 2.0;
    lr = lr < 1.0 ? lr : 2.0 - lr;

    float tbMask = 1.0 - smoothstep(0.0, 1.0, clamp((tbProg - tb) / BLUR_WIDTH, 0.0, 1.0));
    float lrMask = 1.0 - smoothstep(0.0, 1.0, clamp((lrProg - lr) / BLUR_WIDTH, 0.0, 1.0));
    float ffMask = 1.0 - smoothstep(0.0, 1.0, ffProg);

    float mask = tbMask * lrMask * ffMask;

    vec4 oColor = getInputColor(coords);
    oColor.rgb = mix(oColor.rgb, uColor.rgb * oColor.a, uColor.a * smoothstep(0.0, 1.0, prog));
    oColor.a *= mask;

    setOutputColor(oColor);
}
)#";

// Hyprland stores colors as AARRGGBB (see Color.cpp)
static void colorToVec4(Hyprlang::INT val, float out[4]) {
    uint32_t c = static_cast<uint32_t>(val);
    out[0] = ((c >> 16) & 0xFF) / 255.0f; // R
    out[1] = ((c >> 8) & 0xFF) / 255.0f;  // G
    out[2] = (c & 0xFF) / 255.0f;         // B
    out[3] = ((c >> 24) & 0xFF) / 255.0f; // A
}

class TVEffect : public IEffect {
  public:
    std::string name() const override { return "tv"; }
    std::string fragmentSource() const override { return TV_FRAG; }

    void registerConfig(HANDLE handle) const override {
        HyprlandAPI::addConfigValue(handle, "plugin:bmw:tv_color", Hyprlang::INT{*configStringToInt("rgba(c8c8c8ff)")});
    }

    std::vector<std::string> uniformNames() const override {
        return {"uColor"};
    }

    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
        static auto* const PTVCOLOR = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:bmw:tv_color")->getDataStaticPtr();
        float tc[4];
        colorToVec4(**PTVCOLOR, tc);
        glUniform4f(shader.uniforms.at("uColor"), tc[0], tc[1], tc[2], tc[3]);
    }
};

static bool _tvReg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<TVEffect>());
    return true;
}();
