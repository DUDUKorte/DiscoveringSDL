#include "EditorCore/Mesh.h"
#include "EditorApp.h"
#include "Utils/Log.h"
#include <cmath>

Mesh::Mesh(std::string name, glm::vec3 firstVertex, std::vector<glm::vec3> v) {
	
	m_Name = name;
	GetTransform().Position = { firstVertex.x, firstVertex.y, firstVertex.z };
	meshType = MeshTypes::Type3D;
}

Mesh::~Mesh()
{

}

// Recebe o world posisiton e converte pra local
void Mesh::addVertex(glm::vec3 newVertex){
	m_LocalVertices.push_back(newVertex);

	m_CentroideSum += newVertex;

	CalcLocalCentroide();
	m_IsDirty = true;
}

void Mesh::addFace(std::vector<int> vertexIndices, int normalIndice)
{
	if (vertexIndices.empty()) { return; }

	m_Faces.push_back({ vertexIndices, normalIndice });
}

void Mesh::addNormal(glm::vec3 newNormal)
{
	m_Normals.push_back(newNormal);
	// talvez calcular o normal central aqui? n sei se precisa
}
