#include <ee_engine_api/AssetData.hpp>
#include <ee_utils/allocator.hpp>
#include <ee_utils/buffers.hpp>
#include <glm/glm.hpp>

/*

YOU CAN PREVENT SOURCE BUFFER FROM CLOSING.
IT CAN BE USED FOR STORING ASSET DATA WITHOUT COPY.
IN THIS CASE, YOU MUST SET SOURCE TO NULLPTR AND FREE BUFFER YOURSELF.

TODO: make binary format describing for common functions instead of individual serialize/deserialize functions
TODO: parallelize convertion loops

*/

ee_assets::AssetBinary::AssetBinary(uint8_t* &source, const size_t sourceSize)
{
    content = source;
    size = sourceSize;
    source = nullptr;
}

ee_assets::AssetBinary::~AssetBinary()
{
    if (content != nullptr)
        DeallocateArray(content);
}



ee_assets::AssetMapMesh::AssetMapMesh(uint8_t* &source, const size_t sourceSize)
{
    uint8_t* buffer = source;
    uint8_t* pointer = source;
    size_t leftoverSize = sourceSize;
    source = nullptr;

    uint32_t statesCount = 0;
    ee_utils::readNext(&statesCount, pointer, leftoverSize, sizeof(uint32_t));

    for (uint32_t state = 0; state < statesCount; state++)
    {
        uint32_t id = 0;
        glm::u8vec3 color {};
        if (!ee_utils::readNext(&color, pointer, leftoverSize, sizeof(glm::u8vec3)))
            break;
        id = static_cast<uint32_t>(color.r) << 16 | 
            static_cast<uint32_t>(color.g) << 8 | 
            static_cast<uint32_t>(color.b);
        
        uint32_t provincesCount = 0;
        ee_utils::readNext(&provincesCount, pointer, leftoverSize, sizeof(uint32_t));

        for (uint32_t pi = 0; pi < provincesCount; pi++)
        {
            ee_engine::MapMesh::Province prov;

            uint32_t verticesCount = 0;
            if (!ee_utils::readNext(&verticesCount, pointer, leftoverSize, sizeof(uint32_t)))
                break;

            prov.vertices.reserve(verticesCount);
            for (uint32_t vi = 0; vi < verticesCount; vi++)
            {
                glm::vec2 vertex;
                if (!ee_utils::readNext(&vertex, pointer, leftoverSize, sizeof(glm::vec2)))
                    break;
                
                prov.vertices.push_back(vertex);
            }

            uint32_t indicesCount = 0;
            if (!ee_utils::readNext(&indicesCount, pointer, leftoverSize, sizeof(uint32_t)))
                break;

            prov.indices.reserve(indicesCount);
            for(uint32_t ii = 0; ii < indicesCount; ii++)
            {
                uint16_t index = 0;
                if (!ee_utils::readNext(&index, pointer, leftoverSize, sizeof(uint16_t)))
                    break;
                
                prov.indices.push_back(index);
            }

            content.states[id].push_back(prov);
        }
    }

    DeallocateArray(buffer);
}


ee_assets::AssetSave::AssetSave(uint8_t* &source, const size_t sourceSize)
{
    uint8_t* buffer = source;
    uint8_t* pointer = source;
    size_t leftoverSize = sourceSize;
    source = nullptr;

    uint16_t subjectsCount = static_cast<uint16_t>(subjects.size());
    ee_utils::readNext(&subjectsCount, pointer, leftoverSize, sizeof(uint16_t));

    for (uint16_t i = 0; i < subjectsCount; i++)
    {
        ee_data::Subject subject;

        ee_utils::readNext(&subject.tag, pointer, leftoverSize, sizeof(subject.tag));
        ee_utils::readNext(&subject.color, pointer, leftoverSize, sizeof(subject.color));
        
        uint16_t regionsCount = 0;
        ee_utils::readNext(&regionsCount, pointer, leftoverSize, sizeof(uint16_t));
        subject.regions.resize(regionsCount);
        ee_utils::readNext(
            subject.regions.data(),
            pointer,
            leftoverSize,
            subject.regions.size() * sizeof(uint32_t)
        );

        subjects.push_back(subject);
    }

    DeallocateArray(buffer);
}





// SERIALIZERS

uint8_t* ee_assets::AssetSave::Serialize(size_t &size)
{
    size = sizeof(uint16_t);
    for (ee_data::Subject &subject : subjects)
    {
        size += sizeof(subject.tag);
        size += sizeof(subject.color);
        size += sizeof(uint16_t) + subject.regions.size() * sizeof(uint32_t);
    }

    size_t leftover = size;
    uint8_t *result = AllocateArray<uint8_t>(size);
    uint8_t *pointer = result;

    uint16_t subjectsCount = static_cast<uint16_t>(subjects.size());
    ee_utils::writeNext(pointer, &subjectsCount, leftover, sizeof(uint16_t));

    for (ee_data::Subject &subject : subjects)
    {
        ee_utils::writeNext(pointer, &subject.tag, leftover, sizeof(subject.tag));
        ee_utils::writeNext(pointer, &subject.color, leftover, sizeof(subject.color));
        
        uint16_t regionsCount = static_cast<uint16_t>(subject.regions.size());
        ee_utils::writeNext(pointer, &regionsCount, leftover, sizeof(uint16_t));
        ee_utils::writeNext(
            pointer,
            subject.regions.data(),
            leftover,
            subject.regions.size() * sizeof(uint32_t)
        );
    }

    return result;
}