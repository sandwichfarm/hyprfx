#pragma once

#include <chrono>
#include <vector>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>

inline HANDLE PHANDLE = nullptr;

enum eBMWEffect : uint8_t {
    BMW_EFFECT_NONE = 0,
    BMW_EFFECT_FIRE,
    BMW_EFFECT_TV,
    BMW_EFFECT_PIXELATE,
};

struct SBMWShader {
    GLuint program = 0;

    // Common uniforms
    GLint proj       = -1;
    GLint tex        = -1;
    GLint uProgress  = -1;
    GLint uDuration  = -1;
    GLint uForOpening = -1;
    GLint uSize      = -1;
    GLint uPadding   = -1;

    // Fire-specific
    GLint uGradient1      = -1;
    GLint uGradient2      = -1;
    GLint uGradient3      = -1;
    GLint uGradient4      = -1;
    GLint uGradient5      = -1;
    GLint u3DNoise         = -1;
    GLint uScale           = -1;
    GLint uMovementSpeed   = -1;
    GLint uSeed            = -1;

    // TV-specific
    GLint uColor = -1;

    // Pixelate-specific
    GLint uPixelSize = -1;
    GLint uNoise     = -1;

    // Vertex attribs / VAO
    GLuint vao = 0;
    GLuint vboPos = 0;
    GLuint vboUV = 0;
    GLint  posAttrib = -1;
    GLint  texAttrib = -1;
};

struct SClosingAnimation {
    PHLWINDOWREF    window;      // keep ref to prevent premature cleanup
    eBMWEffect      effect;
    float           duration;
    float           seed;
    std::chrono::steady_clock::time_point startTime;

    // Captured at close time (window may be destroyed before we render)
    Vector2D        windowPos;
    Vector2D        windowSize;
    SP<CTexture>    snapshotTex;  // held reference to snapshot texture

    float getProgress() const {
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - startTime).count();
        return std::clamp(elapsed / duration, 0.0f, 1.0f);
    }
};

struct SGlobalState {
    SBMWShader      fireShader;
    SBMWShader      tvShader;
    SBMWShader      pixelateShader;

    std::vector<SClosingAnimation> closingAnimations;
};

inline UP<SGlobalState> g_pGlobalState;
