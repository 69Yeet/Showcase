#ifndef LOADER_HPP
#define LOADER_HPP

#include <bitset>
#include <array>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "vertex.hpp"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <stdexcept>

enum ImportFlag
{
	LOADER_MESH = 0b1,
	LOADER_ANIM = 0b10,
};

namespace loader
{
	struct ImportedInfo;
	void ReadFile(const std::string& pfile, ImportedInfo* &modelInfo, ImportFlag flag);

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		~Mesh()
		{
			vertices.clear();
			indices.clear();
			delete(vertices.data());
			delete(indices.data());
		}
	};

	struct ImportedInfo
	{
		std::bitset<8> flags;
		std::vector<void*> components;
		ImportedInfo(uint8_t a, uint8_t elementAmount)
		{
			flags = a;
			components.resize(elementAmount);
		}
		~ImportedInfo()
		{
			
		}
	};
}

#endif
