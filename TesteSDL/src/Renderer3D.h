#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "Mesh.h"

namespace Renderer3D {
	void Draw3DLinesWithVertices(SDL_Renderer* renderer, std::vector<vertex3D>& vertices, bool closed);

	void DrawMesh(SDL_Renderer* renderer, Mesh& meshTeste);

	void FindIntersections(std::vector<vertex3D>& vertices, std::vector<std::vector<float>>& intersecoes, int height);

	void FillPoly(SDL_Renderer* renderer, std::vector<vertex3D> vertices, int height);
}
