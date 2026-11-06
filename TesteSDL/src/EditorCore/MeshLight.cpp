#include "EditorCore/MeshLight.h"
#include "Utils/Log.h"

MeshLight::MeshLight(const std::string& name, glm::vec3 firstVertex, const std::vector<glm::vec3>& v) : Mesh(name, firstVertex, v)
{
	m_Name = name;
	meshType = MeshTypes::TypeLight;
	m_AfectedByLights = false;

	GetObj() = "./Assets/sphere_tri.obj"; // Mesh padrão da luz agora pq sim
	m_Transform.Scale = { 50.0f, 50.0f, 50.0f };
	GetLightType() = LightType::SceneLight;
	GetHiddenInGame() = true;
}

MeshLight::~MeshLight()
= default;
