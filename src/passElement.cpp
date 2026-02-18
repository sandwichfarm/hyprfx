#include "passElement.hpp"
#include "decoration.hpp"
#include "effects/EffectRegistry.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/Renderer.hpp>

CHFXPassElement::CHFXPassElement(const CHFXPassElement::SHFXData& data) : m_data(data) {}

void CHFXPassElement::draw(const CRegion& damage) {
    auto pMonitor = g_pHyprOpenGL->m_renderData.pMonitor.lock();
    if (!pMonitor)
        return;

    renderEffect(pMonitor, m_data.alpha);
}

bool CHFXPassElement::needsLiveBlur() {
    return false;
}

bool CHFXPassElement::needsPrecomputeBlur() {
    return false;
}

std::optional<CBox> CHFXPassElement::boundingBox() {
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

void CHFXPassElement::renderEffect(PHLMONITOR pMonitor, float alpha) {
    std::string effectName;
    float progress;
    float duration;
    bool isOpening;
    float seed = 0.0f;

    if (m_data.closingAnim) {
        effectName = m_data.closingAnim->effectName;
        progress = m_data.closingAnim->getProgress();
        duration = m_data.closingAnim->duration;
        isOpening = false;
        seed = m_data.closingAnim->seed;
    } else if (m_data.deco) {
        auto PWINDOW = m_data.deco->m_pWindow.lock();
        if (!PWINDOW)
            return;
        effectName = m_data.deco->m_effectName;
        progress = m_data.deco->getProgress();
        duration = m_data.deco->m_fDuration;
        isOpening = !m_data.deco->m_bIsClosing;
        seed = static_cast<float>(PWINDOW->getPID());
    } else {
        return;
    }

    auto* effect = EffectRegistry::instance().getEffect(effectName);
    auto* shader = EffectRegistry::instance().getShader(effectName);
    if (!effect || !shader || !shader->program) {
        Log::logger->log(Log::DEBUG, "[HFX] renderEffect: no shader for '{}'", effectName);
        return;
    }

    // Get window texture and geometry
    SP<CTexture> texPtr;
    Vector2D winPos, winSize;

    if (m_data.closingAnim) {
        texPtr = m_data.closingAnim->snapshotTex;
        winPos = m_data.closingAnim->windowPos;
        winSize = m_data.closingAnim->windowSize;
    } else if (m_data.deco) {
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
        Log::logger->log(Log::DEBUG, "[HFX] renderEffect: no texture (closing={})", m_data.closingAnim != nullptr);
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

    // Set effect-specific uniforms via the effect object
    effect->setUniforms(*shader, PHANDLE, seed);

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
