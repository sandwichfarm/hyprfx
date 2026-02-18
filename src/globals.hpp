#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>

inline HANDLE PHANDLE = nullptr;

struct SClosingAnimation {
    PHLWINDOWREF    window;
    std::string     effectName;
    float           duration;
    float           seed;
    std::chrono::steady_clock::time_point startTime;

    Vector2D        windowPos;
    Vector2D        windowSize;
    SP<CTexture>    snapshotTex;

    float getProgress() const {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - startTime).count();
        return std::clamp(elapsed / duration, 0.0f, 1.0f);
    }
};

struct SGlobalState {
    std::vector<SClosingAnimation> closingAnimations;
};

inline UP<SGlobalState> g_pGlobalState;
