#pragma once

#include "IEffect.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class EffectRegistry {
  public:
    static EffectRegistry& instance();

    void registerEffect(std::unique_ptr<IEffect> effect);
    void registerAllConfigs(HANDLE handle);
    void compileAllShaders();
    void destroyAllShaders();

    IEffect* getEffect(const std::string& name);
    SHFXShader* getShader(const std::string& name);

    bool hasEffect(const std::string& name) const;

  private:
    EffectRegistry() = default;

    struct EffectEntry {
        std::unique_ptr<IEffect> effect;
        SHFXShader shader;
    };

    std::unordered_map<std::string, EffectEntry> m_effects;
};
