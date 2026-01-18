#pragma once
#include <utility>

template<typename T, typename... Args> T* Allocate(Args&&... args)
{
    return new T(std::forward<Args>(args)...);
}

template<typename T> void Deallocate(T* obj)
{
    delete obj;
}

template<typename T> T* AllocateArray(size_t size)
{
    return new T[size];
}

template<typename T> void DeallocateArray(T* obj)
{
    delete[] obj;
}