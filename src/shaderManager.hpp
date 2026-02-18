#pragma once

#include "globals.hpp"

namespace ShaderManager {
    void compileAllShaders();
    void destroyAllShaders();
    SBMWShader* getShader(eBMWEffect effect);
}
