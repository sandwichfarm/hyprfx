#pragma once

#include <string>

// Vertex shader - passthrough with texture coordinates
inline const std::string HFX_VERT = R"#(
#version 300 es
precision mediump float;

uniform mat3 proj;

in vec2 pos;
in vec2 texcoord;

out vec2 iTexCoord;

void main() {
    gl_Position = vec4(proj * vec3(pos, 1.0), 1.0);
    iTexCoord = texcoord;
}
)#";

// Common GLSL utilities (ported from common.glsl (originally from Burn My Windows))
// Stripped of GNOME/KWin platform abstractions, adapted for direct texture access.
inline const std::string HFX_COMMON = R"#(
#version 300 es
precision mediump float;

in vec2 iTexCoord;
layout(location = 0) out vec4 fragColor;

uniform sampler2D tex;
uniform bool uForOpening;
uniform float uProgress;
uniform float uDuration;
uniform vec2 uSize;
uniform float uPadding;

// --- Texture access (replaces getInputColor / setOutputColor) ---
vec4 getInputColor(vec2 coords) {
    vec4 color = texture(tex, coords);
    // Window textures in Hyprland are premultiplied alpha, convert to straight
    if (color.a > 0.0) {
        color.rgb /= color.a;
    }
    return color;
}

void setOutputColor(vec4 outColor) {
    // Convert back to premultiplied alpha for compositing
    fragColor = vec4(outColor.rgb * outColor.a, outColor.a);
}

// --- Compositing ---
vec4 alphaOver(vec4 under, vec4 over) {
    if (under.a == 0.0 && over.a == 0.0) {
        return vec4(0.0);
    }
    float alpha = mix(under.a, 1.0, over.a);
    return vec4(mix(under.rgb * under.a, over.rgb, over.a) / alpha, alpha);
}

// --- Color helpers ---
vec3 tritone(float val, vec3 shadows, vec3 midtones, vec3 highlights) {
    if (val < 0.5) {
        return mix(shadows, midtones, smoothstep(0.0, 1.0, val * 2.0));
    }
    return mix(midtones, highlights, smoothstep(0.0, 1.0, val * 2.0 - 1.0));
}

vec3 darken(vec3 color, float fac) { return color * (1.0 - fac); }
vec3 lighten(vec3 color, float fac) { return color + (vec3(1.0) - color) * fac; }

vec3 offsetHue(vec3 color, float hueOffset) {
    float maxC  = max(max(color.r, color.g), color.b);
    float minC  = min(min(color.r, color.g), color.b);
    float delta = maxC - minC;
    float hue = 0.0;
    if (delta > 0.0) {
        if (maxC == color.r) hue = mod((color.g - color.b) / delta, 6.0);
        else if (maxC == color.g) hue = (color.b - color.r) / delta + 2.0;
        else hue = (color.r - color.g) / delta + 4.0;
    }
    hue /= 6.0;
    float saturation = (maxC > 0.0) ? (delta / maxC) : 0.0;
    float value = maxC;
    hue = mod(hue + hueOffset, 1.0);
    float c = value * saturation;
    float x = c * (1.0 - abs(mod(hue * 6.0, 2.0) - 1.0));
    float m = value - c;
    vec3 rgb;
    if (hue < 1.0 / 6.0) rgb = vec3(c, x, 0.0);
    else if (hue < 2.0 / 6.0) rgb = vec3(x, c, 0.0);
    else if (hue < 3.0 / 6.0) rgb = vec3(0.0, c, x);
    else if (hue < 4.0 / 6.0) rgb = vec3(0.0, x, c);
    else if (hue < 5.0 / 6.0) rgb = vec3(x, 0.0, c);
    else rgb = vec3(c, 0.0, x);
    return rgb + m;
}

// --- Easing functions ---
float easeOutQuad(float x) { return -1.0 * x * (x - 2.0); }
float easeInQuad(float x) { return x * x; }

// --- Hash functions (MIT License, shadertoy.com/view/4djSRW) ---
float hash11(float p) {
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx + p3.yz) * p3.zy);
}

vec3 hash33(vec3 p3) {
    p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

// --- 2D Simplex Noise (MIT License, shadertoy.com/view/Msf3WH) ---
float simplex2D(vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;
    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2 o = vec2(m, 1.0 - m);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3 n = h * h * h * h *
        vec3(dot(a, -1.0 + 2.0 * hash22(i + 0.0)),
             dot(b, -1.0 + 2.0 * hash22(i + o)),
             dot(c, -1.0 + 2.0 * hash22(i + 1.0)));
    return 0.5 + 0.5 * dot(n, vec3(70.0));
}

float simplex2DFractal(vec2 p) {
    mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
    float f = 0.5000 * simplex2D(p); p = m * p;
    f += 0.2500 * simplex2D(p); p = m * p;
    f += 0.1250 * simplex2D(p); p = m * p;
    f += 0.0625 * simplex2D(p);
    return f;
}

// --- 3D Simplex Noise (MIT License, shadertoy.com/view/XsX3zB) ---
float simplex3D(vec3 p) {
    const float F3 = 0.3333333;
    const float G3 = 0.1666667;
    vec3 s = floor(p + dot(p, vec3(F3)));
    vec3 x = p - s + dot(s, vec3(G3));
    vec3 e = step(vec3(0.0), x - x.yzx);
    vec3 i1 = e * (1.0 - e.zxy);
    vec3 i2 = 1.0 - e.zxy * (1.0 - e);
    vec3 x1 = x - i1 + G3;
    vec3 x2 = x - i2 + 2.0 * G3;
    vec3 x3 = x - 1.0 + 3.0 * G3;
    vec4 w, d;
    w.x = dot(x, x); w.y = dot(x1, x1); w.z = dot(x2, x2); w.w = dot(x3, x3);
    w = max(0.6 - w, 0.0);
    d.x = dot(-0.5 + hash33(s), x);
    d.y = dot(-0.5 + hash33(s + i1), x1);
    d.z = dot(-0.5 + hash33(s + i2), x2);
    d.w = dot(-0.5 + hash33(s + 1.0), x3);
    w *= w; w *= w; d *= w;
    return dot(d, vec4(52.0)) * 0.5 + 0.5;
}

float simplex3DFractal(vec3 m) {
    const mat3 rot1 = mat3(-0.37, 0.36, 0.85, -0.14, -0.93, 0.34, 0.92, 0.01, 0.4);
    const mat3 rot2 = mat3(-0.55, -0.39, 0.74, 0.33, -0.91, -0.24, 0.77, 0.12, 0.63);
    const mat3 rot3 = mat3(-0.71, 0.52, -0.47, -0.08, -0.72, -0.68, -0.7, -0.45, 0.56);
    return 0.5333333 * simplex3D(m * rot1) + 0.2666667 * simplex3D(2.0 * m * rot2) +
           0.1333333 * simplex3D(4.0 * m * rot3) + 0.0666667 * simplex3D(8.0 * m);
}

// --- Edge mask helpers ---
// uIsFullscreen is not used in Hyprland port; always apply edge mask
float getEdgeMask(vec2 uv, vec2 maxUV, float fadeWidth) {
    float mask = 1.0;
    mask *= smoothstep(0.0, 1.0, clamp(uv.x / fadeWidth, 0.0, 1.0));
    mask *= smoothstep(0.0, 1.0, clamp(uv.y / fadeWidth, 0.0, 1.0));
    mask *= smoothstep(0.0, 1.0, clamp((maxUV.x - uv.x) / fadeWidth, 0.0, 1.0));
    mask *= smoothstep(0.0, 1.0, clamp((maxUV.y - uv.y) / fadeWidth, 0.0, 1.0));
    return mask;
}

float getAbsoluteEdgeMask(float fadePixels, float offset) {
    float padding = max(0.0, uPadding - fadePixels * offset);
    vec2 uv = iTexCoord.st * uSize - padding;
    return getEdgeMask(uv, uSize - 2.0 * padding, fadePixels);
}
)#";

