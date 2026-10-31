#pragma once
#include "EditorCore/Mesh.h"

enum LightType { DirectionalLight = 0, PointLight = 1};

class MeshLight : public Mesh {
public:
	MeshLight(std::string name, glm::vec3 firstVertex = { 0.0f,0.0f,0.0f }, std::vector<glm::vec3> v = {});
	~MeshLight();

	virtual void BeginPlay() {};
	virtual void EventTick(float deltaTime) {};
	virtual void EventInput(SDL_Event* event) {};

	inline glm::vec3& GetLightColor() { return m_LightColor; }
	inline float& GetLightIntensity() { return m_LightIntensity; }
	inline float& GetMaxDistance() { return m_MaxDistance; }
	inline LightType& GetType() { return m_LightType; }

private:
	glm::vec3 m_LightColor = {0.2f, 0.2f, 0.2f};
	float m_LightIntensity = 1.0f;
	float m_MaxDistance = 300.0f;
	LightType m_LightType = PointLight;
};