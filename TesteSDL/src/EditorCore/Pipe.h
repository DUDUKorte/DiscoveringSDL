#pragma once
#include "EditorCore/Mesh2D.h"

class Pipe : public Mesh2D {
public:
	Pipe(const std::string& name, glm::vec3 firstVertex = { 0.0f, 0.0f, 0.0f }, const std::vector<glm::vec3>& v = {});

	void EventTick(float deltaTime) override;
	void EventInput(SDL_Event* event) override;
};