#pragma once

template<typename T> T* Allocate()
{
    return new T();
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