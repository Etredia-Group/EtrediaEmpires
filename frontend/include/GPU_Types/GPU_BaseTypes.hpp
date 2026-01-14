#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

#define ATTRIBUTE_FORMAT_VEC2 VK_FORMAT_R32G32_SFLOAT
#define ATTRIBUTE_FORMAT_VEC3 VK_FORMAT_R32G32B32_SFLOAT
#define ATTRIBUTE_FORMAT_VEC4 VK_FORMAT_R32G32B32A32_SFLOAT

struct GPU_Transform
{
    glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
};

struct GPU_Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributes();
};

struct GPU_BufferSlot
{
    size_t position;
    size_t size;
};

template<typename T> struct GPU_Buffer
{
    T *data;
    size_t size;
    const size_t ELEMENT_SIZE;
    std::vector<GPU_BufferSlot> freeSlots;

    GPU_Buffer();
    GPU_BufferSlot* FindFreeSlot(size_t size);
    size_t Write(const T *elements, size_t elementsCount);
    // void Clear(GPU_BufferSlot range);
};




// Template definitions

template<typename T> GPU_Buffer<T>::GPU_Buffer() : ELEMENT_SIZE(sizeof(T))
{
    freeSlots.push_back({.position = 0, .size = size});
}

template<typename T> GPU_BufferSlot* GPU_Buffer<T>::FindFreeSlot(size_t size)
{
    for (auto i = freeSlots.rbegin(); i != freeSlots.rend(); i++)
    {
        if (i->size >= size)
            return &(*i);
    }
    return nullptr;
}

template<typename T> size_t GPU_Buffer<T>::Write(const T *elements, size_t elementsCount)
{
    GPU_BufferSlot *slot = FindFreeSlot(elementsCount);
    if (slot == nullptr) return 0;

    memcpy(data + slot->position, elements, elementsCount * ELEMENT_SIZE);
    
    slot->size -= elementsCount;
    slot->position += elementsCount;
    if (slot->size == 0)
        freeSlots.pop_back();

    return slot->position - elementsCount;
}

// template<typename T> void GPU_Buffer<T>::Clear(GPU_BufferSlot range)
// {
//     freeSlots.push_back(range);
//     //TODO: insert sorted by pos and combine intersections
// }