#pragma once
#include "Mesh2D.h"

class Pipe : public Mesh2D {
public:
	Pipe(std::string name, vertex firstVertex = { 0,0 }, std::vector<vertex> v = {});

	void EventTick(float deltaTime) override;
	void EventInput(SDL_Event* event) override;
};