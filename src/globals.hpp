#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>

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

struct SGlobalState {
    SBMWShader      fireShader;
    SBMWShader      tvShader;
    SBMWShader      pixelateShader;
    wl_event_source* tick = nullptr;
};

inline UP<SGlobalState> g_pGlobalState;
