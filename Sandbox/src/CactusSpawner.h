#pragma once

#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <Utils/CoreEditor.h>
#include <EditorCore/MeshLight.h>
#include "Cactus.h"

class CactusSpawner : public Mesh {
public:
	CactusSpawner(std::string name = "cactusSpawner");
	~CactusSpawner() override;

	virtual void BeginPlay() override;
	virtual void EventTick(float deltaTime) override;
	virtual void EventInput(SDL_Event* event) override;
private:
	int m_Quanity;
	std::vector<Cactus*> m_SpawnedCactus;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;
};