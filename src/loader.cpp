#include "loader.hpp"

namespace loader
{
	void ReadMesh(const aiScene* &scene, void* &data);

	void ReadFile(const std::string& pfile, ImportedInfo* &modelInfo, ImportFlag flag)
	{
		std::bitset<8> flagBitset {flag};

		if (flagBitset.count() == 0)
		{
			return;
		}

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(pfile, aiProcess_CalcTangentSpace |
														aiProcess_Triangulate |
														aiProcess_JoinIdenticalVertices |
														aiProcess_SortByPType);

		if (scene == nullptr)
		{
			throw std::runtime_error("Failed to open file!");
		}

		modelInfo = new ImportedInfo(flag, flagBitset.count());
		uint8_t index {0};

		if (flagBitset[0])
		{
			ReadMesh(scene, modelInfo->components[index]);
			index++;
		}
	}

	void ReadMesh(const aiScene* &scene, void* &data)
	{
		uint32_t arrayMeshLength {scene->mNumMeshes};
		uint32_t meshIndices {0};
		uint32_t meshVertices;

		Mesh* accessor = new Mesh;
		data = accessor;
		
		Vertex v;
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		for (int i = 0; i < arrayMeshLength; i++)
		{
			meshVertices = scene->mMeshes[i]->mNumVertices;
			for (int j = 0; j < meshVertices; j++)
			{
				pos.x = static_cast<float_t>(scene->mMeshes[i]->mVertices[j].x);
				pos.y = static_cast<float_t>(scene->mMeshes[i]->mVertices[j].y);
				pos.z =static_cast<float_t>(scene->mMeshes[i]->mVertices[j].z);

				normal.x = static_cast<float_t>(scene->mMeshes[i]->mNormals[j].x);
				normal.y = static_cast<float_t>(scene->mMeshes[i]->mNormals[j].y);
				normal.z = static_cast<float_t>(scene->mMeshes[i]->mNormals[j].z);

				uv.x = static_cast<float_t>(scene->mMeshes[i]->mTextureCoords[0][j].x);
				uv.y = static_cast<float_t>(scene->mMeshes[i]->mTextureCoords[0][j].y);
				v =
				{
					.pos = pos,
					.normal = normal,
					.uv = uv
				};
				accessor->vertices.push_back(v);
			}

			for (int k = 0; k < scene->mMeshes[i]->mNumFaces; k++)
			{
				accessor->indices.push_back(static_cast<uint32_t>(scene->mMeshes[i]->mFaces[k].mIndices[0]) + meshIndices);
				accessor->indices.push_back(static_cast<uint32_t>(scene->mMeshes[i]->mFaces[k].mIndices[1]) + meshIndices);
				accessor->indices.push_back(static_cast<uint32_t>(scene->mMeshes[i]->mFaces[k].mIndices[2]) + meshIndices);
			}
			meshIndices += scene->mMeshes[i]->mNumFaces * 3;
		}

		return;
	}
}
