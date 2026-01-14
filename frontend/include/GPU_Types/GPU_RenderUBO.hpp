#pragma once

#include "GPU_BaseTypes.hpp"
#include <glm/glm.hpp>

struct GPU_GlobalUBO
{
    glm::mat4 view;
    glm::mat4 proj;
    float time;
};

struct GPU_TranformUBO
{
    glm::mat4 model;

    void Update(const GPU_Transform &tranform);
};