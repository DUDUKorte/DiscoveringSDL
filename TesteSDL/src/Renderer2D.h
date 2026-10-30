#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "Mesh2D.h"
#include "Mesh.h"

namespace Renderer2D {
	void DrawLinesWithVertices(SDL_Renderer* renderer, std::vector<vertex>& vertices, bool closed);
	
	void FindIntersections(std::vector<vertex>& vertices, std::vector<std::vector<float>>& intersecoes, int height);
	
	void FillPoly(SDL_Renderer* renderer, Mesh2D* mesh, int height);

	void DrawGizmo(Mesh2D* activeMesh, int mode, int currentGizmoState, int activeGizmoState);
}
