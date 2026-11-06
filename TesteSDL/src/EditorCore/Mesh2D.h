#pragma once
#include "EditorCore/BaseMesh.h"

class Mesh2D : public BaseMesh {
public:
	Mesh2D(const std::string& name, glm::vec3 firstVertex = { 0.0f, 0.0f, 0.0f }, const std::vector<glm::vec3>& v = {});
	virtual ~Mesh2D() override;

	virtual void BeginPlay() override {};
	virtual void EventTick(float deltaTime) override;
	virtual void EventInput(SDL_Event* event) override;

	virtual void addVertex(glm::vec3 newVertex = {0.0f, 0.0f, 0.0f}) override;
};