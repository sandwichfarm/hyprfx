#pragma once

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/OpenGL.hpp>

struct SHFXShader {
    GLuint program = 0;

    // Common uniforms
    GLint proj       = -1;
    GLint tex        = -1;
    GLint uProgress  = -1;
    GLint uDuration  = -1;
    GLint uForOpening = -1;
    GLint uSize      = -1;
    GLint uPadding   = -1;

    // Effect-specific uniforms by name
    std::unordered_map<std::string, GLint> uniforms;

    // Vertex attribs / VAO
    GLuint vao = 0;
    GLuint vboPos = 0;
    GLuint vboUV = 0;
    GLint  posAttrib = -1;
    GLint  texAttrib = -1;
};

// Config entry: a registration callback that calls HyprlandAPI::addConfigValue
using ConfigRegistrar = std::function<void(HANDLE)>;

class IEffect {
  public:
    virtual ~IEffect() = default;

    virtual std::string name() const = 0;
    virtual std::string fragmentSource() const = 0;
    virtual void registerConfig(HANDLE handle) const = 0;
    virtual std::vector<std::string> uniformNames() const = 0;
    virtual void setUniforms(const SHFXShader& shader,
                             HANDLE handle, float seed) const = 0;
};
