#include "decoration.hpp"
#include "passElement.hpp"
#include "shaderManager.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/Renderer.hpp>

static eBMWEffect effectFromString(const std::string& str) {
    if (str == "fire") return BMW_EFFECT_FIRE;
    if (str == "tv") return BMW_EFFECT_TV;
    if (str == "pixelate") return BMW_EFFECT_PIXELATE;
    return BMW_EFFECT_NONE;
}

CBMWDecoration::CBMWDecoration(PHLWINDOW pWindow, bool isClosing)
    : IHyprWindowDecoration(pWindow), m_pWindow(pWindow), m_bIsClosing(isClosing) {

    m_startTime = std::chrono::steady_clock::now();

    // Read config
    const std::string configKey = isClosing ? "plugin:bmw:close_effect" : "plugin:bmw:open_effect";
    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, configKey)->getDataStaticPtr();
    m_effect = effectFromString(*PEFFECT);

    static auto* const PDURATION = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:duration")->getDataStaticPtr();
    m_fDuration = static_cast<float>(**PDURATION);

    if (m_fDuration <= 0.0f)
        m_fDuration = 1.0f;
}

CBMWDecoration::~CBMWDecoration() {
    damageEntire();
}

float CBMWDecoration::getProgress() const {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();
    return std::clamp(elapsed / m_fDuration, 0.0f, 1.0f);
}

SDecorationPositioningInfo CBMWDecoration::getPositioningInfo() {
    return {DECORATION_POSITION_ABSOLUTE};
}

void CBMWDecoration::onPositioningReply(const SDecorationPositioningReply& reply) {
    // ignored
}

void CBMWDecoration::draw(PHLMONITOR pMonitor, const float& a) {
    const auto PWINDOW = m_pWindow.lock();
    if (!PWINDOW || m_effect == BMW_EFFECT_NONE)
        return;

    float progress = getProgress();

    if (progress >= 1.0f && !m_bDone) {
        m_bDone = true;
        // Animation complete - remove ourselves
        // For closing windows, the window will be destroyed by Hyprland
        return;
    }

    if (m_bDone)
        return;

    auto data = CBMWPassElement::SBMWData{this, a, pMonitor};
    g_pHyprRenderer->m_renderPass.add(makeUnique<CBMWPassElement>(data));

    // Request continuous redraws during animation
    damageEntire();
}

eDecorationType CBMWDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CBMWDecoration::updateWindow(PHLWINDOW pWindow) {
    damageEntire();
}

void CBMWDecoration::damageEntire() {
    const auto PWINDOW = m_pWindow.lock();
    if (!PWINDOW)
        return;

    CBox dm = {PWINDOW->m_realPosition->value().x, PWINDOW->m_realPosition->value().y,
               PWINDOW->m_realSize->value().x, PWINDOW->m_realSize->value().y};
    g_pHyprRenderer->damageBox(dm);
}

eDecorationLayer CBMWDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVERLAY;
}

std::string CBMWDecoration::getDisplayName() {
    return "BMWEffect";
}
