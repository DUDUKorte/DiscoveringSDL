#include "MeshLight.h"

MeshLight::MeshLight(std::string name, glm::vec3 firstVertex, std::vector<glm::vec3> v) : Mesh(name, firstVertex, v)
{
	m_Name = name;
	meshType = MeshTypes::TypeLight;
	m_AfectedByLights = false;
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
