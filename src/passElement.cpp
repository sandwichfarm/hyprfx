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
    const auto PWINDOW = m_data.deco->m_pWindow.lock();
    if (!PWINDOW)
        return std::nullopt;

    auto pMonitor = m_data.pMonitor.lock();
    if (!pMonitor)
        return std::nullopt;

    return CBox{PWINDOW->m_realPosition->value().x - pMonitor->m_position.x,
                PWINDOW->m_realPosition->value().y - pMonitor->m_position.y,
                PWINDOW->m_realSize->value().x,
                PWINDOW->m_realSize->value().y};
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
    const auto PWINDOW = m_data.deco->m_pWindow.lock();
    if (!PWINDOW)
        return;

    eBMWEffect effect = m_data.deco->m_effect;
    SBMWShader* shader = ShaderManager::getShader(effect);
    if (!shader || !shader->program)
        return;

    float progress = m_data.deco->getProgress();
    float duration = m_data.deco->m_fDuration;
    bool isOpening = !m_data.deco->m_bIsClosing;

    // Get the window's framebuffer texture
    PHLWINDOWREF ref{PWINDOW};
    if (!g_pHyprOpenGL->m_windowFramebuffers.contains(ref))
        return;

    auto& fb = g_pHyprOpenGL->m_windowFramebuffers.at(ref);
    auto texPtr = fb.getTexture();
    if (!texPtr || texPtr->m_texID == 0)
        return;

    // Calculate window box in monitor-local coordinates
    CBox windowBox = {PWINDOW->m_realPosition->value().x - pMonitor->m_position.x,
                      PWINDOW->m_realPosition->value().y - pMonitor->m_position.y,
                      PWINDOW->m_realSize->value().x,
                      PWINDOW->m_realSize->value().y};
    windowBox.scale(pMonitor->m_scale).round();

    // Compute projection matrix
    const auto TRANSFORM = Math::wlTransformToHyprutils(Math::invertTransform(WL_OUTPUT_TRANSFORM_NORMAL));
    Mat3x3 matrix = g_pHyprOpenGL->m_renderData.monitorProjection.projectBox(windowBox, TRANSFORM, windowBox.rot);
    Mat3x3 glMatrix = g_pHyprOpenGL->m_renderData.projection.copy().multiply(matrix);

    // Bind our shader
    glUseProgram(shader->program);

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
    glUniform2f(shader->uSize, PWINDOW->m_realSize->value().x * pMonitor->m_scale,
                               PWINDOW->m_realSize->value().y * pMonitor->m_scale);
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
        glUniform1f(shader->uSeed, static_cast<float>(PWINDOW->getPID()));

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

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(texPtr->m_target, 0);

    g_pHyprOpenGL->scissor(nullptr);
}
