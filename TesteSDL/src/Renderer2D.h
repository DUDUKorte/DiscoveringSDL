#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "BaseMesh.h"
#include "Mesh2D.h"

namespace Renderer2D {
	void DrawLinesWithVertices(SDL_Renderer* renderer, std::vector<glm::vec3>& vertices, bool closed);
	
	void FindIntersections(std::vector<glm::vec3>& vertices, std::vector<std::vector<float>>& intersecoes, int height, glm::vec2 yMinMax);
	
	void FillPoly(SDL_Renderer* renderer, BaseMesh* mesh, int height);

	void DrawGizmo(BaseMesh* activeMesh, int mode, int currentGizmoState, int activeGizmoState);
}
