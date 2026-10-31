#include "MeshLight.h"

MeshLight::MeshLight(std::string name)
{
	m_Name = name;
	meshType = MeshTypes::TypeLight;
	addVertex({ 0.0f, 0.0f, 0.0f });
}

MeshLight::~MeshLight() 
{

}

void MeshLight::EventTick(float deltaTime)
{
	// Tick events goes here
}

void MeshLight::EventInput(SDL_Event* event)
{
	// Input for lights goes here
}

void MeshLight::addVertex(glm::vec3 newVertex)
{
	// Light need to have 1 vertex
	m_LocalVertices.push_back(newVertex);

	m_CentroideSum += newVertex;

	CalcLocalCentroide();
	m_IsDirty = true;
}
