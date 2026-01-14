#include "GPU_Types/GPU_BaseTypes.hpp"
#include "GPU_Types/GPU_RenderUBO.hpp"

#include <glm/gtc/matrix_transform.hpp>

std::array<VkVertexInputAttributeDescription, 2> GPU_Vertex::GetAttributes()
{
	VkVertexInputAttributeDescription posAttribute = {
		.location = 0,
		.binding = 0,
		.format = ATTRIBUTE_FORMAT_VEC2,
		.offset = offsetof(GPU_Vertex, pos)
	};
	VkVertexInputAttributeDescription colorAttribute = {
		.location = 1,
		.binding = 0,
		.format = ATTRIBUTE_FORMAT_VEC3,
		.offset = offsetof(GPU_Vertex, color)
	};

	return {posAttribute, colorAttribute};
}

void GPU_TranformUBO::Update(const GPU_Transform &transform)
{
	model = glm::mat4(1.0f);
	model = glm::translate(model, transform.pos);
	model = glm::scale(model, transform.scale);
}