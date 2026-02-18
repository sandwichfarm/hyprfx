#define WLR_USE_UNSTABLE

#include <unistd.h>
#include <any>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/managers/HookSystemManager.hpp>

#include "globals.hpp"
#include "decoration.hpp"
#include "shaderManager.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

static SP<HOOK_CALLBACK_FN> g_pOpenWindowHook;
static SP<HOOK_CALLBACK_FN> g_pCloseWindowHook;

static eBMWEffect effectFromString(const std::string& str) {
    if (str == "fire") return BMW_EFFECT_FIRE;
    if (str == "tv") return BMW_EFFECT_TV;
    if (str == "pixelate") return BMW_EFFECT_PIXELATE;
    return BMW_EFFECT_NONE;
}

int onTick(void* data) {
    EMIT_HOOK_EVENT("bmwTick", nullptr);

    const int TIMEOUT = g_pHyprRenderer->m_mostHzMonitor ? 1000.0 / g_pHyprRenderer->m_mostHzMonitor->m_refreshRate : 16;
    wl_event_source_timer_update(g_pGlobalState->tick, TIMEOUT);

    return 0;
}

void onOpenWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:open_effect")->getDataStaticPtr();
    if (effectFromString(*PEFFECT) == BMW_EFFECT_NONE)
        return;

    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, makeUnique<CBMWDecoration>(PWINDOW, false));
}

void onCloseWindow(void* self, std::any data) {
    const auto PWINDOW = std::any_cast<PHLWINDOW>(data);

    static auto* const PEFFECT = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:close_effect")->getDataStaticPtr();
    if (effectFromString(*PEFFECT) == BMW_EFFECT_NONE)
        return;

    HyprlandAPI::addWindowDecoration(PHANDLE, PWINDOW, makeUnique<CBMWDecoration>(PWINDOW, true));
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    const std::string HASH = __hyprland_api_get_hash();

    if (HASH != GIT_COMMIT_HASH) {
        HyprlandAPI::addNotification(PHANDLE,
            "[BMW] Failure in initialization: Version mismatch (headers ver is not equal to running hyprland ver)",
            CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error("[BMW] Version mismatch");
    }

    // Register config values
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:close_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:open_effect", Hyprlang::STRING{"fire"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:duration", Hyprlang::FLOAT{1.0});

    // Fire gradient colors (RGBA hex)
    // Default: classic orange-yellow fire
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_1", Hyprlang::INT{*configStringToInt("rgba(f8b80000)")});  // transparent orange
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_2", Hyprlang::INT{*configStringToInt("rgba(f8d87880)")});  // orange, 50% alpha
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_3", Hyprlang::INT{*configStringToInt("rgba(f8f878cc)")});  // yellow-orange, 80%
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_4", Hyprlang::INT{*configStringToInt("rgba(f8f8b8e6)")});  // pale yellow, 90%
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:bmw:fire_color_5", Hyprlang::INT{*configStringToInt("rgba(fffffffe)")});  // white, full alpha

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

    // Initialize global state and shaders
    g_pGlobalState = makeUnique<SGlobalState>();

    g_pHyprRenderer->makeEGLCurrent();
    ShaderManager::compileAllShaders();

    // Set up tick timer for continuous animation redraws
    g_pGlobalState->tick = wl_event_loop_add_timer(g_pCompositor->m_wlEventLoop, &onTick, nullptr);
    wl_event_source_timer_update(g_pGlobalState->tick, 1);

    HyprlandAPI::reloadConfig();

    HyprlandAPI::addNotification(PHANDLE, "[BMW] Burn My Windows initialized successfully!", CHyprColor{0.2, 1.0, 0.2, 1.0}, 5000);

    return {"burn-my-windows", "GPU-accelerated window open/close effects (fire, TV, pixelate)", "sandwich", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    if (g_pGlobalState) {
        if (g_pGlobalState->tick)
            wl_event_source_remove(g_pGlobalState->tick);

        g_pHyprRenderer->makeEGLCurrent();
        ShaderManager::destroyAllShaders();
    }

    g_pHyprRenderer->m_renderPass.removeAllOfType("CBMWPassElement");
}
