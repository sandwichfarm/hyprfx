#define WLR_USE_UNSTABLE

#include <unistd.h>
#include <any>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>

#include "globals.hpp"
#include "decoration.hpp"
#include "passElement.hpp"
#include "effects/EffectRegistry.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SP<HOOK_CALLBACK_FN> g_pOpenWindowHook;
static SP<HOOK_CALLBACK_FN> g_pCloseWindowHook;
static SP<HOOK_CALLBACK_FN> g_pRenderHook;

void onOpenWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);
    Log::logger->log(Log::DEBUG, "[HFX] onOpenWindow fired for window {:x}", (uintptr_t)PWINDOW.get());

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:open_effect")->getDataStaticPtr();
    std::string effectName = *PEFFECT;
    if (effectName.empty() || effectName == "none" || !EffectRegistry::instance().hasEffect(effectName))
        return;

    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, makeUnique<CHFXDecoration>(PWINDOW, false));
    Log::logger->log(Log::DEBUG, "[HFX] open decoration attached");
}

void onCloseWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);
    Log::logger->log(Log::DEBUG, "[HFX] onCloseWindow fired for window {:x}", (uintptr_t)PWINDOW.get());

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:close_effect")->getDataStaticPtr();
    std::string effectName = *PEFFECT;
    if (effectName.empty() || effectName == "none" || !EffectRegistry::instance().hasEffect(effectName))
        return;

    static auto* const PDURATION = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:hfx:duration")->getDataStaticPtr();
    float duration = static_cast<float>(**PDURATION);
    if (duration <= 0.0f)
        duration = 1.0f;

    SClosingAnimation anim;
    anim.window = PWINDOW;
    anim.effectName = effectName;
    anim.duration = duration;
    anim.seed = static_cast<float>(PWINDOW->getPID());
    anim.startTime = std::chrono::steady_clock::now();

    anim.windowPos = PWINDOW->m_realPosition->value();
    anim.windowSize = PWINDOW->m_realSize->value();

    PHLWINDOWREF ref{PWINDOW};
    if (g_pHyprOpenGL->m_windowFramebuffers.contains(ref)) {
        anim.snapshotTex = g_pHyprOpenGL->m_windowFramebuffers.at(ref).getTexture();
        Log::logger->log(Log::DEBUG, "[HFX] close animation: captured snapshot tex={}", anim.snapshotTex ? anim.snapshotTex->m_texID : 0);
    } else {
        auto wlSurf = PWINDOW->wlSurface();
        auto surface = wlSurf ? wlSurf->resource() : nullptr;
        if (surface && surface->m_current.texture) {
            anim.snapshotTex = surface->m_current.texture;
            Log::logger->log(Log::DEBUG, "[HFX] close animation: using surface tex={}", anim.snapshotTex->m_texID);
        }
    }

    CBox dm = {anim.windowPos.x, anim.windowPos.y,
               anim.windowSize.x, anim.windowSize.y};

    bool hasTex = anim.snapshotTex != nullptr;
    g_pGlobalState->closingAnimations.push_back(std::move(anim));

    g_pHyprRenderer->damageBox(dm);

    Log::logger->log(Log::DEBUG, "[HFX] close animation tracked (duration={}, hasTex={})", duration, hasTex);
}

void onRender(void* self, std::any data) {
    const auto renderStage = std::any_cast<eRenderStage>(data);
    if (renderStage != RENDER_POST_WINDOWS)
        return;

    if (!g_pGlobalState || g_pGlobalState->closingAnimations.empty())
        return;

    auto pMonitor = g_pHyprOpenGL->m_renderData.pMonitor.lock();
    if (!pMonitor)
        return;

    for (auto it = g_pGlobalState->closingAnimations.begin(); it != g_pGlobalState->closingAnimations.end();) {
        float progress = it->getProgress();
        if (progress >= 1.0f) {
            it = g_pGlobalState->closingAnimations.erase(it);
            continue;
        }

        if (!it->snapshotTex || it->snapshotTex->m_texID == 0) {
            auto PWINDOW = it->window.lock();
            if (PWINDOW) {
                PHLWINDOWREF ref{PWINDOW};
                if (g_pHyprOpenGL->m_windowFramebuffers.contains(ref)) {
                    it->snapshotTex = g_pHyprOpenGL->m_windowFramebuffers.at(ref).getTexture();
                }
            }
            if (!it->snapshotTex || it->snapshotTex->m_texID == 0) {
                ++it;
                continue;
            }
        }

        auto passData = CHFXPassElement::SHFXData{};
        passData.alpha = 1.0f;
        passData.pMonitor = pMonitor;
        passData.closingAnim = &(*it);
        g_pHyprRenderer->m_renderPass.add(makeUnique<CHFXPassElement>(passData));

        CBox dm = {it->windowPos.x, it->windowPos.y,
                   it->windowSize.x, it->windowSize.y};
        g_pHyprRenderer->damageBox(dm);

        ++it;
    }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();
    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE,
            std::format("[HFX] Warning: hash mismatch (server={}, client={})", HASH, GIT_COMMIT_HASH),
            CHyprColor{1.0, 1.0, 0.2, 1.0}, 8000);
    }

    // Core config values
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hfx:close_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hfx:open_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:hfx:duration", Hyprlang::FLOAT{1.0});

    // Register all effect-specific config values
    EffectRegistry::instance().registerAllConfigs(PHANDLE);

    // Register event hooks
    g_pOpenWindowHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "openWindow",
        [&](void* self, SCallbackInfo& info, std::any data) { onOpenWindow(self, data); });

    g_pCloseWindowHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "closeWindow",
        [&](void* self, SCallbackInfo& info, std::any data) { onCloseWindow(self, data); });

    g_pRenderHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "render",
        [&](void* self, SCallbackInfo& info, std::any data) { onRender(self, data); });

    // Initialize global state and shaders
    g_pGlobalState = makeUnique<SGlobalState>();

    g_pHyprRenderer->makeEGLCurrent();
    EffectRegistry::instance().compileAllShaders();

    HyprlandAPI::reloadConfig();

    HyprlandAPI::addNotification(PHANDLE, "[HFX] HyprFX initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    return {"hyprfx", "GPU-accelerated window open/close effects (fire, TV, pixelate)", "sandwich", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    g_pHyprRenderer->m_renderPass.removeAllOfType("CHFXPassElement");

    if (g_pGlobalState) {
        g_pGlobalState->closingAnimations.clear();
        g_pHyprRenderer->makeEGLCurrent();
        EffectRegistry::instance().destroyAllShaders();
    }
}
