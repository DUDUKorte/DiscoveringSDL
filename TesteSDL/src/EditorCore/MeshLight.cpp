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


nlohmann::json MeshLight::Serialize() {
	nlohmann::json mesh = Mesh::Serialize();
	// configs específicas das luzes
	mesh["LightType"] = GetLightType();
	mesh["LightColor"] = GetLightColor();
	mesh["LightIntensity"] = GetLightIntensity();
	mesh["LightMaxDistance"] = GetMaxDistance();
	return mesh;
}

void MeshLight::Deserialize(nlohmann::json& j) {
	Mesh::Deserialize(j);
	GetLightType() = j["LightType"];
	GetLightColor() = j["LightColor"];
	GetLightIntensity() = j["LightIntensity"];
	GetMaxDistance() = j["LightMaxDistance"];
}
