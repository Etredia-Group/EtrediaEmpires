module;
#include <glm/glm.hpp>
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