module;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

module AssetData;
import utils;
import std;

/*

YOU CAN PREVENT SOURCE BUFFER FROM CLOSING.
IT CAN BE USED FOR STORING ASSET DATA WITHOUT COPY.
IN THIS CASE, YOU MUST SET SOURCE TO NULLPTR AND FREE BUFFER YOURSELF.

*/

ee_assets::Binary::Binary(std::uint8_t * & source, const std::size_t sourceSize) {
    content = source;
    size = sourceSize;
    source = nullptr;
}

ee_assets::Binary::~Binary() {
    if (content != nullptr)
        DeallocateArray(content);
}

ee_assets::Texture::Texture(std::uint8_t *&source, const std::size_t sourceSize) {
    std::uint8_t *decoded = stbi_load_from_memory(source, sourceSize, &dimentionX, &dimentionY, &channels, 4);
    DeallocateArray(source);
    source = nullptr;

    if (!decoded) {
        content = nullptr;
        size = 0;
        return;
    }

    content = decoded;
    size = dimentionX * dimentionY * channels;
}

ee_assets::Texture::~Texture() {
    if (content != nullptr)
        DeallocateArray(content);
}
