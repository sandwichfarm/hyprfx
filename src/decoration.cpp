#include "decoration.hpp"
#include "passElement.hpp"
#include "effects/EffectRegistry.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/Renderer.hpp>

CHFXDecoration::CHFXDecoration(PHLWINDOW pWindow, bool isClosing)
    : IHyprWindowDecoration(pWindow), m_pWindow(pWindow), m_bIsClosing(isClosing) {

    m_startTime = std::chrono::steady_clock::now();

    if (isClosing) {
        static auto* const PCLOSE = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:close_effect")->getDataStaticPtr();
        m_effectName = *PCLOSE;
    } else {
        static auto* const POPEN = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:open_effect")->getDataStaticPtr();
        m_effectName = *POPEN;
    }

    static auto* const PDURATION = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:duration")->getDataStaticPtr();
    m_fDuration = static_cast<float>(**PDURATION);

    if (m_fDuration <= 0.0f)
        m_fDuration = 1.0f;
}

CHFXDecoration::~CHFXDecoration() {
    damageEntire();
}

float CHFXDecoration::getProgress() const {
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();
    return std::clamp(elapsed / m_fDuration, 0.0f, 1.0f);
}

SDecorationPositioningInfo CHFXDecoration::getPositioningInfo() {
    return {DECORATION_POSITION_ABSOLUTE};
}

void CHFXDecoration::onPositioningReply(const SDecorationPositioningReply& reply) {
    // ignored
}

void CHFXDecoration::draw(PHLMONITOR pMonitor, const float& a) {
    const auto PWINDOW = m_pWindow.lock();
    if (!PWINDOW || m_effectName.empty() || !EffectRegistry::instance().hasEffect(m_effectName))
        return;

    float progress = getProgress();

    if (progress >= 1.0f) {
        m_bDone = true;
        return;
    }

    auto data = CHFXPassElement::SHFXData{};
    data.deco = this;
    data.alpha = a;
    data.pMonitor = pMonitor;
    g_pHyprRenderer->m_renderPass.add(makeUnique<CHFXPassElement>(data));

    damageEntire();
}

eDecorationType CHFXDecoration::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHFXDecoration::updateWindow(PHLWINDOW pWindow) {
    if (!m_bDone)
        damageEntire();
}

void CHFXDecoration::damageEntire() {
    const auto PWINDOW = m_pWindow.lock();
    if (!PWINDOW)
        return;

    CBox dm = {PWINDOW->m_realPosition->value().x, PWINDOW->m_realPosition->value().y,
               PWINDOW->m_realSize->value().x, PWINDOW->m_realSize->value().y};
    g_pHyprRenderer->damageBox(dm);
}

eDecorationLayer CHFXDecoration::getDecorationLayer() {
    return DECORATION_LAYER_OVERLAY;
}

std::string CHFXDecoration::getDisplayName() {
    return "HFXEffect";
}
