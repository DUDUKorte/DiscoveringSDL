#pragma once

#include "PointLight.h"
#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <Utils/CoreEditor.h>

class Cactus : public Mesh {
public:
	Cactus(std::string name = "cactus");
	~Cactus() override;

	virtual void BeginPlay() override;
	virtual void EventTick(float deltaTime) override;
	virtual void EventInput(SDL_Event* event) override;
private:
	float m_Time;
	float m_Speed;
	glm::vec3 scaleAnim;
	glm::vec3 initialScale;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;

	PointLight* myLight = nullptr;
};