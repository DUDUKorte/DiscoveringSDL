#pragma once

#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <EditorCore/MeshLight.h>
#include <Utils/CoreEditor.h>

class PointLightL : public MeshLight {
public:
	PointLightL(std::string name = "Poinbt Light");
	~PointLightL();

	virtual void BeginPlay();
	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);
private:
	float m_Time;
	float m_Speed;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;

	glm::vec3 lightColorStart = { 1.0f, 1.0f, 1.0f };
	glm::vec3 lightColorEnd = { 0.9f, 0.6f, 0.3f };

	glm::vec3 bgColorStart = { 0.3f, 0.8f, 0.9f };
	glm::vec3 bgColorEnd = { 0.9f, 0.6f, 0.3f };
};