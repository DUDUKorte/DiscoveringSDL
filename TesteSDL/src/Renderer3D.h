#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "Mesh.h"

namespace Renderer3D {
	void Draw3DLinesWithVertices(SDL_Renderer* renderer, const std::vector<glm::vec3>& vertices, bool closed);

	void DrawMesh(SDL_Renderer* renderer, Mesh& meshTeste);

	void FindIntersections(const std::vector<glm::vec3>& vertices, std::vector<std::vector<float>>& intersecoes, int height, glm::vec2 yMinMax);

	void FillPoly(SDL_Renderer* renderer, const std::vector<glm::vec3>& vertices, glm::vec2 minMaxY);
}
