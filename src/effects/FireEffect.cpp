#include "EffectRegistry.hpp"

static const std::string FIRE_FRAG = R"#(
uniform bool u3DNoise;
uniform float uScale;
uniform float uMovementSpeed;
uniform vec4 uGradient1;
uniform vec4 uGradient2;
uniform vec4 uGradient3;
uniform vec4 uGradient4;
uniform vec4 uGradient5;
uniform float uSeed;

const float EDGE_FADE  = 70.0;
const float FADE_WIDTH = 0.1;
const float HIDE_TIME  = 0.4;

vec4 getFireColor(float v, vec4 c0, vec4 c1, vec4 c2, vec4 c3, vec4 c4) {
    float steps[5];
    steps[0] = 0.0; steps[1] = 0.2; steps[2] = 0.35; steps[3] = 0.5; steps[4] = 0.8;
    vec4 colors[5];
    colors[0] = c0; colors[1] = c1; colors[2] = c2; colors[3] = c3; colors[4] = c4;
    if (v < steps[0]) return colors[0];
    for (int i = 0; i < 4; ++i) {
        if (v <= steps[i + 1]) {
            return mix(colors[i], colors[i + 1], vec4(v - steps[i]) / (steps[i + 1] - steps[i]));
        }
    }
    return colors[4];
}

vec2 effectMask(float hideTime, float fadeWidth, float edgeFadeWidth) {
    float progress = easeOutQuad(uProgress);
    float burnProgress = clamp(progress / hideTime, 0.0, 1.0);
    float afterBurnProgress = clamp((progress - hideTime) / (1.0 - hideTime), 0.0, 1.0);
    float t = iTexCoord.t * (1.0 - fadeWidth);
    float windowMask = 1.0 - clamp((burnProgress - t) / fadeWidth, 0.0, 1.0);
    float effectMask = clamp(t * (1.0 - windowMask) / burnProgress, 0.0, 1.0);
    if (progress > hideTime) {
        float fade = sqrt(1.0 - afterBurnProgress * afterBurnProgress);
        effectMask *= mix(1.0, 1.0 - t, afterBurnProgress) * fade;
    }
    effectMask *= getAbsoluteEdgeMask(edgeFadeWidth, 0.5);
    if (uForOpening) {
        windowMask = 1.0 - windowMask;
    }
    return vec2(windowMask, effectMask);
}

void main() {
    vec2 uv = iTexCoord.st * uSize / vec2(400.0, 600.0) / uScale;
    uv.y += uProgress * uDuration * uMovementSpeed;

    float noise = u3DNoise
        ? simplex3DFractal(vec3(uv * 4.0, uProgress * uDuration * uMovementSpeed * 1.5))
        : simplex2DFractal(uv * 4.0);

    vec2 mask = effectMask(HIDE_TIME, FADE_WIDTH, EDGE_FADE);
    noise *= mask.y;

    vec4 fire = getFireColor(noise, uGradient1, uGradient2, uGradient3, uGradient4, uGradient5);

    vec4 oColor = getInputColor(iTexCoord.st);
    oColor.a *= mask.x;
    oColor = alphaOver(oColor, fire);

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

class FireEffect : public IEffect {
  public:
    std::string name() const override { return "fire"; }
    std::string fragmentSource() const override { return FIRE_FRAG; }

    void registerConfig(HANDLE handle) const override {
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_color_1", Hyprlang::INT{*configStringToInt("rgba(c8200000)")});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_color_2", Hyprlang::INT{*configStringToInt("rgba(f04000a0)")});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_color_3", Hyprlang::INT{*configStringToInt("rgba(ff8800dd)")});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_color_4", Hyprlang::INT{*configStringToInt("rgba(ffcc00f0)")});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_color_5", Hyprlang::INT{*configStringToInt("rgba(ffff66fe)")});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_scale", Hyprlang::FLOAT{1.0});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_3d_noise", Hyprlang::INT{1});
        HyprlandAPI::addConfigValue(handle, "plugin:hfx:fire_movement_speed", Hyprlang::FLOAT{1.0});
    }

    std::vector<std::string> uniformNames() const override {
        return {"uGradient1", "uGradient2", "uGradient3", "uGradient4", "uGradient5",
                "u3DNoise", "uScale", "uMovementSpeed", "uSeed"};
    }

    void setUniforms(const SHFXShader& shader, HANDLE handle, float seed) const override {
        static auto* const PCOLOR1 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_color_1")->getDataStaticPtr();
        static auto* const PCOLOR2 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_color_2")->getDataStaticPtr();
        static auto* const PCOLOR3 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_color_3")->getDataStaticPtr();
        static auto* const PCOLOR4 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_color_4")->getDataStaticPtr();
        static auto* const PCOLOR5 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_color_5")->getDataStaticPtr();
        static auto* const PSCALE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_scale")->getDataStaticPtr();
        static auto* const P3DNOISE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_3d_noise")->getDataStaticPtr();
        static auto* const PSPEED = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(handle, "plugin:hfx:fire_movement_speed")->getDataStaticPtr();

        float c1[4], c2[4], c3[4], c4[4], c5[4];
        colorToVec4(**PCOLOR1, c1);
        colorToVec4(**PCOLOR2, c2);
        colorToVec4(**PCOLOR3, c3);
        colorToVec4(**PCOLOR4, c4);
        colorToVec4(**PCOLOR5, c5);

        glUniform4f(shader.uniforms.at("uGradient1"), c1[0], c1[1], c1[2], c1[3]);
        glUniform4f(shader.uniforms.at("uGradient2"), c2[0], c2[1], c2[2], c2[3]);
        glUniform4f(shader.uniforms.at("uGradient3"), c3[0], c3[1], c3[2], c3[3]);
        glUniform4f(shader.uniforms.at("uGradient4"), c4[0], c4[1], c4[2], c4[3]);
        glUniform4f(shader.uniforms.at("uGradient5"), c5[0], c5[1], c5[2], c5[3]);
        glUniform1f(shader.uniforms.at("uScale"), static_cast<float>(**PSCALE));
        glUniform1i(shader.uniforms.at("u3DNoise"), **P3DNOISE ? 1 : 0);
        glUniform1f(shader.uniforms.at("uMovementSpeed"), static_cast<float>(**PSPEED));
        glUniform1f(shader.uniforms.at("uSeed"), seed);
    }
};

static bool _fireReg = [] {
    EffectRegistry::instance().registerEffect(std::make_unique<FireEffect>());
    return true;
}();
