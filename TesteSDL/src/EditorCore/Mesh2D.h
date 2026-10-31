#pragma once
#include "EditorCore/BaseMesh.h"

class Mesh2D : public BaseMesh {
public:
	Mesh2D(std::string name, glm::vec3 firstVertex = { 0.0f, 0.0f, 0.0f }, std::vector<glm::vec3> v = {});
	virtual ~Mesh2D();

	virtual void BeginPlay() {};
	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);

	virtual void addVertex(glm::vec3 newVertex = {0.0f, 0.0f, 0.0f});
};