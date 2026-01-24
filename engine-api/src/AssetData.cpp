#include <ee_engine_api/AssetData.hpp>
#include <ee_utils/allocator.hpp>
#include <ee_utils/buffers.hpp>
#include <glm/glm.hpp>

/*

YOU CAN PREVENT SOURCE BUFFER FROM CLOSING.
IT CAN BE USED FOR STORING ASSET DATA WITHOUT COPY.
IN THIS CASE, YOU MUST SET SOURCE TO NULLPTR AND FREE BUFFER YOURSELF.

*/

AssetBinary::AssetBinary(uint8_t* &source, const size_t &sourceSize)
{
    content = source;
    size = sourceSize;
    source = nullptr;
}

AssetBinary::~AssetBinary()
{
    if (content != nullptr)
        DeallocateArray(content);
}



AssetMapMesh::AssetMapMesh(uint8_t* &source, const size_t &sourceSize)
{
    uint8_t* buffer = source;
    uint8_t* pointer = source;
    size_t leftoverSize = sourceSize;
    source = nullptr;

    uint32_t statesCount = 0;
    readNext(&statesCount, pointer, leftoverSize, sizeof(uint32_t));

    for (uint32_t state = 0; state < statesCount; state++)
    {
        uint32_t id = 0;
        glm::u8vec3 color {};
        if (!readNext(&color, pointer, leftoverSize, sizeof(glm::u8vec3)))
            break;
        id = static_cast<uint32_t>(color.r) << 16 | 
            static_cast<uint32_t>(color.g) << 8 | 
            static_cast<uint32_t>(color.b);
        
        uint32_t provincesCount = 0;
        readNext(&provincesCount, pointer, leftoverSize, sizeof(uint32_t));

        for (uint32_t pi = 0; pi < provincesCount; pi++)
        {
            MapMesh::Province prov;

            uint32_t verticesCount = 0;
            if (!readNext(&verticesCount, pointer, leftoverSize, sizeof(uint32_t)))
                break;

            prov.vertices.reserve(verticesCount);
            for (uint32_t vi = 0; vi < verticesCount; vi++)
            {
                glm::vec2 vertex;
                if (!readNext(&vertex, pointer, leftoverSize, sizeof(glm::vec2)))
                    break;
                
                prov.vertices.push_back(vertex);
            }

            uint32_t indicesCount = 0;
            if (!readNext(&indicesCount, pointer, leftoverSize, sizeof(uint32_t)))
                break;

            prov.indices.reserve(indicesCount);
            for(uint32_t ii = 0; ii < indicesCount; ii++)
            {
                uint16_t index = 0;
                if (!readNext(&index, pointer, leftoverSize, sizeof(uint16_t)))
                    break;
                
                prov.indices.push_back(index);
            }

            content.states[id].push_back(prov);
        }
    }

    DeallocateArray(buffer);
}