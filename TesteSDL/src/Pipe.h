#pragma once
#include "Mesh.h"

class Pipe : public Mesh {
public:
	Pipe(std::string name, vertex firstVertex = { 0,0 }, std::vector<vertex> v = {});

	void EventTick(float deltaTime) override;
	void EventInput(SDL_Event* event) override;
};