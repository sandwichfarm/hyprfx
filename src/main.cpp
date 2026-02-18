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
#include "shaderManager.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SP<HOOK_CALLBACK_FN> g_pOpenWindowHook;
static SP<HOOK_CALLBACK_FN> g_pCloseWindowHook;
static SP<HOOK_CALLBACK_FN> g_pRenderHook;

static eBMWEffect effectFromString(const std::string& str) {
    if (str == "fire") return BMW_EFFECT_FIRE;
    if (str == "tv") return BMW_EFFECT_TV;
    if (str == "pixelate") return BMW_EFFECT_PIXELATE;
    return BMW_EFFECT_NONE;
}

void onOpenWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);
    Log::logger->log(Log::DEBUG, "[BMW] onOpenWindow fired for window {:x}", (uintptr_t)PWINDOW.get());

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:open_effect")->getDataStaticPtr();
    if (effectFromString(*PEFFECT) == BMW_EFFECT_NONE)
        return;

    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, makeUnique<CBMWDecoration>(PWINDOW, false));
    Log::logger->log(Log::DEBUG, "[BMW] open decoration attached");
}

void onCloseWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);
    Log::logger->log(Log::DEBUG, "[BMW] onCloseWindow fired for window {:x}", (uintptr_t)PWINDOW.get());

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:close_effect")->getDataStaticPtr();
    eBMWEffect effect = effectFromString(*PEFFECT);
    if (effect == BMW_EFFECT_NONE)
        return;

    static auto* const PDURATION = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:duration")->getDataStaticPtr();
    float duration = static_cast<float>(**PDURATION);
    if (duration <= 0.0f)
        duration = 1.0f;

    // Track closing animation globally — decoration approach doesn't work for close
    // because renderWindow() returns early for m_fadingOut windows (skips decorations)
    SClosingAnimation anim;
    anim.window = PWINDOW;
    anim.effect = effect;
    anim.duration = duration;
    anim.seed = static_cast<float>(PWINDOW->getPID());
    anim.startTime = std::chrono::steady_clock::now();

    // Capture window geometry now (window may be destroyed before render)
    anim.windowPos = PWINDOW->m_realPosition->value();
    anim.windowSize = PWINDOW->m_realSize->value();

    // Try to grab the snapshot texture (Hyprland calls makeSnapshot before closeWindow event)
    PHLWINDOWREF ref{PWINDOW};
    if (g_pHyprOpenGL->m_windowFramebuffers.contains(ref)) {
        anim.snapshotTex = g_pHyprOpenGL->m_windowFramebuffers.at(ref).getTexture();
        Log::logger->log(Log::DEBUG, "[BMW] close animation: captured snapshot tex={}", anim.snapshotTex ? anim.snapshotTex->m_texID : 0);
    } else {
        // Snapshot not available yet — try surface texture as fallback
        auto wlSurf = PWINDOW->wlSurface();
        auto surface = wlSurf ? wlSurf->resource() : nullptr;
        if (surface && surface->m_current.texture) {
            anim.snapshotTex = surface->m_current.texture;
            Log::logger->log(Log::DEBUG, "[BMW] close animation: using surface tex={}", anim.snapshotTex->m_texID);
        }
    }

    // Damage the window area to kick off rendering
    CBox dm = {anim.windowPos.x, anim.windowPos.y,
               anim.windowSize.x, anim.windowSize.y};

    bool hasTex = anim.snapshotTex != nullptr;
    g_pGlobalState->closingAnimations.push_back(std::move(anim));

    g_pHyprRenderer->damageBox(dm);

    Log::logger->log(Log::DEBUG, "[BMW] close animation tracked (duration={}, hasTex={})", duration, hasTex);
}

void onRender(void* self, std::any data) {
    // Only act at RENDER_POST_WINDOWS stage
    const auto renderStage = std::any_cast<eRenderStage>(data);
    if (renderStage != RENDER_POST_WINDOWS)
        return;

    if (!g_pGlobalState || g_pGlobalState->closingAnimations.empty())
        return;



    auto pMonitor = g_pHyprOpenGL->m_renderData.pMonitor.lock();
    if (!pMonitor)
        return;

    // Process closing animations
    for (auto it = g_pGlobalState->closingAnimations.begin(); it != g_pGlobalState->closingAnimations.end();) {
        float progress = it->getProgress();
        if (progress >= 1.0f) {
            it = g_pGlobalState->closingAnimations.erase(it);
            continue;
        }

        if (!it->snapshotTex || it->snapshotTex->m_texID == 0) {
            // No texture available — try to grab snapshot now
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

        // Add our close effect pass element
        auto passData = CBMWPassElement::SBMWData{};
        passData.alpha = 1.0f;
        passData.pMonitor = pMonitor;
        passData.closingAnim = &(*it);
        g_pHyprRenderer->m_renderPass.add(makeUnique<CBMWPassElement>(passData));

        // Keep damaging to ensure continuous redraws
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
        // Log mismatch but don't throw — hyprpm headers may differ from installed binary
        HyprlandAPI::addNotification(PHANDLE,
            std::format("[BMW] Warning: hash mismatch (server={}, client={})", HASH, GIT_COMMIT_HASH),
            CHyprColor{1.0, 1.0, 0.2, 1.0}, 8000);
    }

    // Register config values
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:close_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:open_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:duration", Hyprlang::FLOAT{1.0});

    // Fire gradient colors (RGBA hex)
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_1", Hyprlang::INT{*configStringToInt("rgba(f8b80000)")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_2", Hyprlang::INT{*configStringToInt("rgba(f8d87880)")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_3", Hyprlang::INT{*configStringToInt("rgba(f8f878cc)")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_4", Hyprlang::INT{*configStringToInt("rgba(f8f8b8e6)")});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_5", Hyprlang::INT{*configStringToInt("rgba(fffffffe)")});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_scale", Hyprlang::FLOAT{1.0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_3d_noise", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_movement_speed", Hyprlang::FLOAT{1.0});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:tv_color", Hyprlang::INT{*configStringToInt("rgba(c8c8c8ff)")});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:pixelate_pixel_size", Hyprlang::FLOAT{40.0});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:pixelate_noise", Hyprlang::FLOAT{0.5});

    // Register event hooks
    g_pOpenWindowHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "openWindow",
        [&](void* self, SCallbackInfo& info, std::any data) { onOpenWindow(self, data); });

    g_pCloseWindowHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "closeWindow",
        [&](void* self, SCallbackInfo& info, std::any data) { onCloseWindow(self, data); });

    // Hook into render pipeline for close effects
    g_pRenderHook = HyprlandAPI::registerCallbackDynamic(PHANDLE, "render",
        [&](void* self, SCallbackInfo& info, std::any data) { onRender(self, data); });

    // Initialize global state and shaders
    g_pGlobalState = makeUnique<SGlobalState>();

    g_pHyprRenderer->makeEGLCurrent();
    ShaderManager::compileAllShaders();

    HyprlandAPI::reloadConfig();

    HyprlandAPI::addNotification(PHANDLE, "[BMW] Burn My Windows initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    return {"burn-my-windows", "GPU-accelerated window open/close effects (fire, TV, pixelate)", "sandwich", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    g_pHyprRenderer->m_renderPass.removeAllOfType("CBMWPassElement");

    if (g_pGlobalState) {
        g_pGlobalState->closingAnimations.clear();
        g_pHyprRenderer->makeEGLCurrent();
        ShaderManager::destroyAllShaders();
    }
}
