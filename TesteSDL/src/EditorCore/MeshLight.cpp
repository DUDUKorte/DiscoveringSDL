#include "EditorCore/MeshLight.h"
#include "Utils/Log.h"

MeshLight::MeshLight(std::string name, glm::vec3 firstVertex, std::vector<glm::vec3> v) : Mesh(name, firstVertex, v)
{
	m_Name = name;
	meshType = MeshTypes::TypeLight;
	m_AfectedByLights = false;
}

MeshLight::~MeshLight() 
{

}
