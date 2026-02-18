#include "passElement.hpp"
#include "decoration.hpp"
#include "shaderManager.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

CBMWPassElement::CBMWPassElement(const CBMWPassElement::SBMWData& data) : m_data(data) {}

void CBMWPassElement::draw(const CRegion& damage) {
    auto pMonitor = g_pHyprOpenGL->m_renderData.pMonitor.lock();
    if (!pMonitor)
        return;

    renderEffect(pMonitor, m_data.alpha);
}

bool CBMWPassElement::needsLiveBlur() {
    return false;
}

bool CBMWPassElement::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> CBMWPassElement::boundingBox() {
    Vector2D pos, size;

    if (m_data.closingAnim) {
        pos = m_data.closingAnim->windowPos;
        size = m_data.closingAnim->windowSize;
    } else if (m_data.deco) {
        auto w = m_data.deco->m_pWindow.lock();
        if (!w)
            return std::nullopt;
        pos = w->m_realPosition->value();
        size = w->m_realSize->value();
    } else {
        return std::nullopt;
    }

    auto pMonitor = m_data.pMonitor.lock();
    if (!pMonitor)
        return std::nullopt;

    return CBox{pos.x - pMonitor->m_position.x,
                pos.y - pMonitor->m_position.y,
                size.x, size.y};
}

static uint32_t colorConfigToRGBA(Hyprlang::INT val) {
    return static_cast<uint32_t>(val);
}

static void vec4FromRGBA(uint32_t rgba, float out[4]) {
    out[0] = ((rgba >> 24) & 0xFF) / 255.0f; // R
    out[1] = ((rgba >> 16) & 0xFF) / 255.0f; // G
    out[2] = ((rgba >> 8) & 0xFF) / 255.0f;  // B
    out[3] = (rgba & 0xFF) / 255.0f;         // A
}

void CBMWPassElement::renderEffect(PHLMONITOR pMonitor, float alpha) {
    // Determine window, effect, progress, duration, and whether this is an open or close animation
    eBMWEffect effect;
    float progress;
    float duration;
    bool isOpening;

    if (m_data.closingAnim) {
        // Close animation via render hook — window may already be destroyed
        effect = m_data.closingAnim->effect;
        progress = m_data.closingAnim->getProgress();
        duration = m_data.closingAnim->duration;
        isOpening = false;
    } else if (m_data.deco) {
        // Open animation via decoration
        auto PWINDOW = m_data.deco->m_pWindow.lock();
        if (!PWINDOW)
            return;
        effect = m_data.deco->m_effect;
        progress = m_data.deco->getProgress();
        duration = m_data.deco->m_fDuration;
        isOpening = !m_data.deco->m_bIsClosing;
    } else {
        return;
    }

    SBMWShader* shader = ShaderManager::getShader(effect);
    if (!shader || !shader->program) {
        Log::logger->log(Log::DEBUG, "[BMW] renderEffect: no shader");
        return;
    }

    // Get window texture and geometry
    SP<CTexture> texPtr;
    Vector2D winPos, winSize;

    if (m_data.closingAnim) {
        // Close: use pre-captured snapshot texture and geometry
        texPtr = m_data.closingAnim->snapshotTex;
        winPos = m_data.closingAnim->windowPos;
        winSize = m_data.closingAnim->windowSize;
    } else if (m_data.deco) {
        // Open: use live surface texture
        auto w = m_data.deco->m_pWindow.lock();
        if (!w)
            return;
        auto wlSurf = w->wlSurface();
        auto surface = wlSurf ? wlSurf->resource() : nullptr;
        if (surface)
            texPtr = surface->m_current.texture;
        winPos = w->m_realPosition->value();
        winSize = w->m_realSize->value();
    }

    if (!texPtr || texPtr->m_texID == 0) {
        Log::logger->log(Log::DEBUG, "[BMW] renderEffect: no texture (closing={})", m_data.closingAnim != nullptr);
        return;
    }

    // Calculate window box in monitor-local coordinates
    CBox windowBox = {winPos.x - pMonitor->m_position.x,
                      winPos.y - pMonitor->m_position.y,
                      winSize.x,
                      winSize.y};
    windowBox.scale(pMonitor->m_scale).round();

    // Compute projection matrix
    const auto TRANSFORM = Math::wlTransformToHyprutils(Math::invertTransform(WL_OUTPUT_TRANSFORM_NORMAL));
    Mat3x3 matrix = g_pHyprOpenGL->m_renderData.monitorProjection.projectBox(windowBox, TRANSFORM, windowBox.rot);
    Mat3x3 glMatrix = g_pHyprOpenGL->m_renderData.projection.copy().multiply(matrix);

    // Use Hyprland's program tracker to switch shader
    g_pHyprOpenGL->useProgram(shader->program);

    // Set projection matrix
    glMatrix.transpose();
    glUniformMatrix3fv(shader->proj, 1, GL_FALSE, glMatrix.getMatrix().data());

    // Bind window texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(texPtr->m_target, texPtr->m_texID);
    glTexParameteri(texPtr->m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texPtr->m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glUniform1i(shader->tex, 0);

    // Set common uniforms
    glUniform1f(shader->uProgress, progress);
    glUniform1f(shader->uDuration, duration);
    glUniform1i(shader->uForOpening, isOpening ? 1 : 0);
    glUniform2f(shader->uSize, winSize.x * pMonitor->m_scale,
                               winSize.y * pMonitor->m_scale);
    glUniform1f(shader->uPadding, 0.0f);

    // Set effect-specific uniforms
    if (effect == BMW_EFFECT_FIRE) {
        static auto* const PCOLOR1 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_color_1")->getDataStaticPtr();
        static auto* const PCOLOR2 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_color_2")->getDataStaticPtr();
        static auto* const PCOLOR3 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_color_3")->getDataStaticPtr();
        static auto* const PCOLOR4 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_color_4")->getDataStaticPtr();
        static auto* const PCOLOR5 = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_color_5")->getDataStaticPtr();
        static auto* const PSCALE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_scale")->getDataStaticPtr();
        static auto* const P3DNOISE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_3d_noise")->getDataStaticPtr();
        static auto* const PSPEED = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:fire_movement_speed")->getDataStaticPtr();

        float c1[4], c2[4], c3[4], c4[4], c5[4];
        vec4FromRGBA(colorConfigToRGBA(**PCOLOR1), c1);
        vec4FromRGBA(colorConfigToRGBA(**PCOLOR2), c2);
        vec4FromRGBA(colorConfigToRGBA(**PCOLOR3), c3);
        vec4FromRGBA(colorConfigToRGBA(**PCOLOR4), c4);
        vec4FromRGBA(colorConfigToRGBA(**PCOLOR5), c5);

        glUniform4f(shader->uGradient1, c1[0], c1[1], c1[2], c1[3]);
        glUniform4f(shader->uGradient2, c2[0], c2[1], c2[2], c2[3]);
        glUniform4f(shader->uGradient3, c3[0], c3[1], c3[2], c3[3]);
        glUniform4f(shader->uGradient4, c4[0], c4[1], c4[2], c4[3]);
        glUniform4f(shader->uGradient5, c5[0], c5[1], c5[2], c5[3]);
        glUniform1f(shader->uScale, static_cast<float>(**PSCALE));
        glUniform1i(shader->u3DNoise, **P3DNOISE ? 1 : 0);
        glUniform1f(shader->uMovementSpeed, static_cast<float>(**PSPEED));

        float seed;
        if (m_data.closingAnim) {
            seed = m_data.closingAnim->seed;
        } else if (m_data.deco) {
            auto w = m_data.deco->m_pWindow.lock();
            seed = w ? static_cast<float>(w->getPID()) : 0.0f;
        } else {
            seed = 0.0f;
        }
        glUniform1f(shader->uSeed, seed);

    } else if (effect == BMW_EFFECT_TV) {
        static auto* const PTVCOLOR = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:tv_color")->getDataStaticPtr();
        float tc[4];
        vec4FromRGBA(colorConfigToRGBA(**PTVCOLOR), tc);
        glUniform4f(shader->uColor, tc[0], tc[1], tc[2], tc[3]);

    } else if (effect == BMW_EFFECT_PIXELATE) {
        static auto* const PPIXSIZE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:pixelate_pixel_size")->getDataStaticPtr();
        static auto* const PPIXNOISE = (Hyprlang::FLOAT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:bmw:pixelate_noise")->getDataStaticPtr();
        glUniform1f(shader->uPixelSize, static_cast<float>(**PPIXSIZE));
        glUniform1f(shader->uNoise, static_cast<float>(**PPIXNOISE));
    }

    // Enable blending
    g_pHyprOpenGL->blend(true);

    // Render using VAO
    glBindVertexArray(shader->vao);

    // Update UV buffer with fullVerts (standard UVs)
    glBindBuffer(GL_ARRAY_BUFFER, shader->vboUV);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fullVerts), fullVerts);

    // Draw with damage clipping
    for (auto& RECT : g_pHyprOpenGL->m_renderData.damage.getRects()) {
        g_pHyprOpenGL->scissor(&RECT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Clean up GL state
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(texPtr->m_target, 0);
    g_pHyprOpenGL->useProgram(0);
    g_pHyprOpenGL->scissor(nullptr);
}
