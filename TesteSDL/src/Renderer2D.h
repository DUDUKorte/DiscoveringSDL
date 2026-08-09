#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "Mesh.h"

namespace Renderer2D {
	void DrawLinesWithVertices(SDL_Renderer* renderer, std::vector<vertex>& vertices, bool closed);
	
	void FindIntersections(std::vector<vertex>& vertices, std::vector<std::vector<float>>& intersecoes, std::vector<std::vector<vertex>>& intersecoes_v);
	
	void FillPoly(SDL_Renderer* renderer, Mesh* mesh);

	void DrawGizmo(Mesh* activeMesh, int mode, int currentGizmoState, int activeGizmoState);
}