#pragma once
#include "BaseMesh.h"

enum LightType { DirectionalLight = 0, PointLight = 1};

class MeshLight : public BaseMesh {
public:
	MeshLight(std::string name);
	~MeshLight();

	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);

	virtual void addVertex(glm::vec3 newVertex);

	inline glm::vec3& GetLightColor() { return m_LightColor; }
	inline float& GetLightIntensity() { return m_LightIntensity; }
	inline LightType& GetType() { return m_LightType; }

private:
	glm::vec3 m_LightColor = {0.2f, 0.2f, 0.2f};
	float m_LightIntensity = 1.0f;
	LightType m_LightType = PointLight;
};