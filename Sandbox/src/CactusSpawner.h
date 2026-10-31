#pragma once

#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <Utils/CoreEditor.h>
#include <EditorCore/MeshLight.h>
#include "Cactus.h"

class CactusSpawner : public Mesh {
public:
	CactusSpawner(std::string name = "cactusSpawner");
	~CactusSpawner();

	virtual void BeginPlay();
	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);
private:
	float m_Quanity;
	std::vector<Cactus*> m_SpawnedCactus;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;
};