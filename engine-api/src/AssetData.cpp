module;

#include <glm/glm.hpp>
#include <cstdint>
#include <cstddef>

module AssetData;
import allocator;
import buffers;

/*

YOU CAN PREVENT SOURCE BUFFER FROM CLOSING.
IT CAN BE USED FOR STORING ASSET DATA WITHOUT COPY.
IN THIS CASE, YOU MUST SET SOURCE TO NULLPTR AND FREE BUFFER YOURSELF.

TODO: make universal binary format, think about flatbuffers or msgpacks

*/

ee_assets::AssetBinary::AssetBinary(std::uint8_t* &source, const std::size_t sourceSize)
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



ee_assets::AssetMapMesh::AssetMapMesh(std::uint8_t* &source, const std::size_t sourceSize)
{
    std::uint8_t* buffer = source;
    std::uint8_t* pointer = source;
    std::size_t leftoverSize = sourceSize;
    source = nullptr;

    std::uint32_t statesCount = 0;
    ee_utils::readNext(&statesCount, pointer, leftoverSize, sizeof(std::uint32_t));

    for (std::uint32_t state = 0; state < statesCount; state++)
    {
        std::uint32_t id = 0;
        glm::u8vec3 color {};
        if (!ee_utils::readNext(&color, pointer, leftoverSize, sizeof(glm::u8vec3)))
            break;
        id = static_cast<std::uint32_t>(color.r) << 16 |
             static_cast<std::uint32_t>(color.g) << 8 |
             static_cast<std::uint32_t>(color.b);

        std::uint32_t provincesCount = 0;
        ee_utils::readNext(&provincesCount, pointer, leftoverSize, sizeof(std::uint32_t));

        for (std::uint32_t pi = 0; pi < provincesCount; pi++)
        {
            ee_engine::MapMesh::Province prov;

            std::uint32_t verticesCount = 0;
            if (!ee_utils::readNext(&verticesCount, pointer, leftoverSize, sizeof(std::uint32_t)))
                break;

            prov.vertices.reserve(verticesCount);
            for (std::uint32_t vi = 0; vi < verticesCount; vi++)
            {
                glm::vec2 vertex;
                if (!ee_utils::readNext(&vertex, pointer, leftoverSize, sizeof(glm::vec2)))
                    break;

                prov.vertices.push_back(vertex);
            }

            std::uint32_t indicesCount = 0;
            if (!ee_utils::readNext(&indicesCount, pointer, leftoverSize, sizeof(std::uint32_t)))
                break;

            prov.indices.reserve(indicesCount);
            for(std::uint32_t ii = 0; ii < indicesCount; ii++)
            {
                std::uint16_t index = 0;
                if (!ee_utils::readNext(&index, pointer, leftoverSize, sizeof(std::uint16_t)))
                    break;

                prov.indices.push_back(index);
            }

            content.regions[id].push_back(prov);
        }
    }

    DeallocateArray(buffer);
}


ee_assets::AssetSave::AssetSave(std::uint8_t* &source, const std::size_t sourceSize)
{
    std::uint8_t* buffer = source;
    std::uint8_t* pointer = source;
    std::size_t leftoverSize = sourceSize;
    source = nullptr;

    std::uint16_t subjectsCount = static_cast<std::uint16_t>(content.subjects.size());
    ee_utils::readNext(&subjectsCount, pointer, leftoverSize, sizeof(std::uint16_t));

    for (std::uint16_t i = 0; i < subjectsCount; i++)
    {
        ee_data::Subject subject;

        ee_utils::readNext(&subject.tag, pointer, leftoverSize, sizeof(subject.tag));
        ee_utils::readNext(&subject.color, pointer, leftoverSize, sizeof(subject.color));

        std::uint16_t regionsCount = 0;
        ee_utils::readNext(&regionsCount, pointer, leftoverSize, sizeof(std::uint16_t));
        subject.regions.resize(regionsCount);
        ee_utils::readNext(
                subject.regions.data(),
                pointer,
                leftoverSize,
                subject.regions.size() * sizeof(std::uint32_t)
        );

        content.subjects.push_back(subject);
    }

    DeallocateArray(buffer);
}





// SERIALIZERS

std::uint8_t* ee_assets::AssetSave::Serialize(std::size_t &size)
{
    size = sizeof(std::uint16_t);
    for (ee_data::Subject &subject : content.subjects)
    {
        size += sizeof(subject.tag);
        size += sizeof(subject.color);
        size += sizeof(std::uint16_t) + subject.regions.size() * sizeof(std::uint32_t);
    }

    std::size_t leftover = size;
    std::uint8_t *result = AllocateArray<std::uint8_t>(size);
    std::uint8_t *pointer = result;

    std::uint16_t subjectsCount = static_cast<std::uint16_t>(content.subjects.size());
    ee_utils::writeNext(pointer, &subjectsCount, leftover, sizeof(std::uint16_t));

    for (ee_data::Subject &subject : content.subjects)
    {
        ee_utils::writeNext(pointer, &subject.tag, leftover, sizeof(subject.tag));
        ee_utils::writeNext(pointer, &subject.color, leftover, sizeof(subject.color));

        std::uint16_t regionsCount = static_cast<std::uint16_t>(subject.regions.size());
        ee_utils::writeNext(pointer, &regionsCount, leftover, sizeof(std::uint16_t));
        ee_utils::writeNext(
                pointer,
                subject.regions.data(),
                leftover,
                subject.regions.size() * sizeof(std::uint32_t)
        );
    }

    return result;
}