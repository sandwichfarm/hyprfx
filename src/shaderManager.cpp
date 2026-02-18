#include "shaderManager.hpp"
#include "shaders.hpp"

#include <hyprland/src/render/OpenGL.hpp>
#include <stdexcept>

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
        throw std::runtime_error("BMW shader compile failed: " + log);
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
        throw std::runtime_error("BMW shader link failed: " + log);
    }
    return prog;
}

static void setupVAO(SBMWShader& shader) {
    glGenVertexArrays(1, &shader.vao);
    glBindVertexArray(shader.vao);

    // Position VBO
    if (shader.posAttrib != -1) {
        glGenBuffers(1, &shader.vboPos);
        glBindBuffer(GL_ARRAY_BUFFER, shader.vboPos);
        glBufferData(GL_ARRAY_BUFFER, sizeof(fullVerts), fullVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(shader.posAttrib);
        glVertexAttribPointer(shader.posAttrib, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // UV VBO
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

static void lookupCommonUniforms(SBMWShader& shader) {
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

static void compileFireShader(SBMWShader& shader) {
    std::string fragSrc = BMW_COMMON + BMW_FRAG_FIRE;
    shader.program = createProgram(BMW_VERT, fragSrc);

    lookupCommonUniforms(shader);

    shader.uGradient1    = glGetUniformLocation(shader.program, "uGradient1");
    shader.uGradient2    = glGetUniformLocation(shader.program, "uGradient2");
    shader.uGradient3    = glGetUniformLocation(shader.program, "uGradient3");
    shader.uGradient4    = glGetUniformLocation(shader.program, "uGradient4");
    shader.uGradient5    = glGetUniformLocation(shader.program, "uGradient5");
    shader.u3DNoise      = glGetUniformLocation(shader.program, "u3DNoise");
    shader.uScale        = glGetUniformLocation(shader.program, "uScale");
    shader.uMovementSpeed = glGetUniformLocation(shader.program, "uMovementSpeed");
    shader.uSeed         = glGetUniformLocation(shader.program, "uSeed");

    setupVAO(shader);
}

static void compileTVShader(SBMWShader& shader) {
    std::string fragSrc = BMW_COMMON + BMW_FRAG_TV;
    shader.program = createProgram(BMW_VERT, fragSrc);

    lookupCommonUniforms(shader);

    shader.uColor = glGetUniformLocation(shader.program, "uColor");

    setupVAO(shader);
}

static void compilePixelateShader(SBMWShader& shader) {
    std::string fragSrc = BMW_COMMON + BMW_FRAG_PIXELATE;
    shader.program = createProgram(BMW_VERT, fragSrc);

    lookupCommonUniforms(shader);

    shader.uPixelSize = glGetUniformLocation(shader.program, "uPixelSize");
    shader.uNoise     = glGetUniformLocation(shader.program, "uNoise");

    setupVAO(shader);
}

static void destroyShader(SBMWShader& shader) {
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

namespace ShaderManager {

void compileAllShaders() {
    compileFireShader(g_pGlobalState->fireShader);
    compileTVShader(g_pGlobalState->tvShader);
    compilePixelateShader(g_pGlobalState->pixelateShader);
}

void destroyAllShaders() {
    destroyShader(g_pGlobalState->fireShader);
    destroyShader(g_pGlobalState->tvShader);
    destroyShader(g_pGlobalState->pixelateShader);
}

SBMWShader* getShader(eBMWEffect effect) {
    switch (effect) {
        case BMW_EFFECT_FIRE: return &g_pGlobalState->fireShader;
        case BMW_EFFECT_TV: return &g_pGlobalState->tvShader;
        case BMW_EFFECT_PIXELATE: return &g_pGlobalState->pixelateShader;
        default: return nullptr;
    }
}

} // namespace ShaderManager
