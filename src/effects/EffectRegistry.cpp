#include "EffectRegistry.hpp"
#include "../shaders.hpp"

#include <hyprland/src/render/OpenGL.hpp>
#include <stdexcept>

EffectRegistry& EffectRegistry::instance() {
    static EffectRegistry reg;
    return reg;
}

void EffectRegistry::registerEffect(std::unique_ptr<IEffect> effect) {
    auto name = effect->name();
    m_effects[name].effect = std::move(effect);
}

void EffectRegistry::registerAllConfigs(HANDLE handle) {
    for (auto& [name, entry] : m_effects) {
        entry.effect->registerConfig(handle);
    }
}

bool EffectRegistry::hasEffect(const std::string& name) const {
    return m_effects.contains(name);
}

IEffect* EffectRegistry::getEffect(const std::string& name) {
    auto it = m_effects.find(name);
    return it != m_effects.end() ? it->second.effect.get() : nullptr;
}

SHFXShader* EffectRegistry::getShader(const std::string& name) {
    auto it = m_effects.find(name);
    return it != m_effects.end() ? &it->second.shader : nullptr;
}

// --- Shader compilation helpers (moved from shaderManager.cpp) ---

static GLuint compileShader(GLenum type, const std::string& src) {
    auto shader = glCreateShader(type);
    auto source = src.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("HFX shader compile failed: " + log);
    }
    return shader;
}

static GLuint createProgram(const std::string& vert, const std::string& frag) {
    auto vertShader = compileShader(GL_VERTEX_SHADER, vert);
    auto fragShader = compileShader(GL_FRAGMENT_SHADER, frag);

    auto prog = glCreateProgram();
    glAttachShader(prog, vertShader);
    glAttachShader(prog, fragShader);
    glLinkProgram(prog);

    glDetachShader(prog, vertShader);
    glDetachShader(prog, fragShader);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
        GLint logLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, ' ');
        glGetProgramInfoLog(prog, logLen, nullptr, log.data());
        glDeleteProgram(prog);
        throw std::runtime_error("HFX shader link failed: " + log);
    }
    return prog;
}

static void setupVAO(SHFXShader& shader) {
    glGenVertexArrays(1, &shader.vao);
    glBindVertexArray(shader.vao);

    if (shader.posAttrib != -1) {
        glGenBuffers(1, &shader.vboPos);
        glBindBuffer(GL_ARRAY_BUFFER, shader.vboPos);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullVerts), fullVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(shader.posAttrib);
        glVertexAttribPointer(shader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    if (shader.texAttrib != -1) {
        glGenBuffers(1, &shader.vboUV);
        glBindBuffer(GL_ARRAY_BUFFER, shader.vboUV);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullVerts), fullVerts, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(shader.texAttrib);
        glVertexAttribPointer(shader.texAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void lookupCommonUniforms(SHFXShader& shader) {
    shader.proj        = glGetUniformLocation(shader.program, "proj");
    shader.tex         = glGetUniformLocation(shader.program, "tex");
    shader.uProgress   = glGetUniformLocation(shader.program, "uProgress");
    shader.uDuration   = glGetUniformLocation(shader.program, "uDuration");
    shader.uForOpening = glGetUniformLocation(shader.program, "uForOpening");
    shader.uSize       = glGetUniformLocation(shader.program, "uSize");
    shader.uPadding    = glGetUniformLocation(shader.program, "uPadding");
    shader.posAttrib   = glGetAttribLocation(shader.program, "pos");
    shader.texAttrib   = glGetAttribLocation(shader.program, "texcoord");
}

static void destroyShader(SHFXShader& shader) {
    if (shader.vao) {
        glDeleteVertexArrays(1, &shader.vao);
        shader.vao = 0;
    }
    if (shader.vboPos) {
        glDeleteBuffers(1, &shader.vboPos);
        shader.vboPos = 0;
    }
    if (shader.vboUV) {
        glDeleteBuffers(1, &shader.vboUV);
        shader.vboUV = 0;
    }
    if (shader.program) {
        glDeleteProgram(shader.program);
        shader.program = 0;
    }
}

void EffectRegistry::compileAllShaders() {
    for (auto& [name, entry] : m_effects) {
        auto& shader = entry.shader;
        std::string fragSrc = HFX_COMMON + entry.effect->fragmentSource();
        shader.program = createProgram(HFX_VERT, fragSrc);

        lookupCommonUniforms(shader);

        // Look up effect-specific uniforms
        for (const auto& uName : entry.effect->uniformNames()) {
            shader.uniforms[uName] = glGetUniformLocation(shader.program, uName.c_str());
        }

        setupVAO(shader);
    }
}

void EffectRegistry::destroyAllShaders() {
    for (auto& [name, entry] : m_effects) {
        destroyShader(entry.shader);
    }
}
