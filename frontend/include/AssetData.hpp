#pragma once
#include <concepts>
#include <cstdint>

struct AssetData {};
template<typename T> concept Asset = std::derived_from<T, AssetData>;

struct AssetBinary : AssetData
{
    uint8_t* content = nullptr;
    size_t size= 0;

    AssetBinary(uint8_t* &source, size_t sourceSize);
    ~AssetBinary();
};