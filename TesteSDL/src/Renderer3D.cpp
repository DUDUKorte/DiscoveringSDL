#include "Renderer3D.h"
#include "EditorApp.h"
#include "Log.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <glm/gtc/random.hpp>

namespace Renderer3D {
	void Draw3DLinesWithVertices(SDL_Renderer* renderer, std::vector<vertex3D>& vertices, bool closed)
	{
		if (vertices.size() < 2) { // só desenha se tiver 2 ou mais vertices
			SDL_RenderPoint(renderer, vertices[0].x, vertices[0].y);
			return;
		}

		for (int i = 0; i < vertices.size() - 1; i++) {
			SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i + 1].x, vertices[i + 1].y);

			if (closed && vertices.size() - 2 == i) {
				SDL_RenderLine(renderer, vertices[i + 1].x, vertices[i + 1].y, vertices[0].x, vertices[0].y);
			}
		}
	}

	void DrawMesh(SDL_Renderer* renderer, Mesh& meshTeste)
	{
		std::vector<vertex3D> allVertices = meshTeste.GetVertices();
		for (face currentFace : meshTeste.GetFaces()) {

			std::vector<vertex3D> currentVertices = {};
			for (int i : currentFace.vertices) {
				currentVertices.push_back(allVertices[i]);
			}
			///* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
			//const float red = (float)glm::linearRand(0.1, 1.0);
			//const float green = (float)glm::linearRand(0.1, 1.0);
			//const float blue = (float)glm::linearRand(0.1, 1.0);
			//SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */

			Draw3DLinesWithVertices(renderer, currentVertices, true);
			//FillPoly(renderer, currentVertices, 1280);
		}
	}

	void FindIntersections(std::vector<vertex3D>& vertices, std::vector<std::vector<float>>& intersecoes, int height)
	{
		for (int i = 0; i < vertices.size(); i++) {
			vertex3D source = vertices[i];
			vertex3D point = vertices[(i + 1) % vertices.size()];

			if (point.y == source.y) { continue; }

			// Aqui eu tenho os dois pontos
			if (point.y < source.y) { // point é mais alto que source; faz troca
				std::swap(source, point);
			}
			LE_CORE_ASSERT(point.y >= source.y, "Y FINAL da aresta maior que o INICIAL!");

			int ymin = (int)source.y;
			int ymax = (int)point.y;

			float dY = (point.y - source.y);

			float Tx = (point.x - source.x) / dY;
			//float Tr = (point.color.x - source.color.x) / dY;
			//float Tg = (point.color.y - source.color.y) / dY;
			//float Tb = (point.color.z - source.color.z) / dY;

			float new_x = source.x;
			//float new_r = source.color.x;
			//float new_g = source.color.y;
			//float new_b = source.color.z;

			// Pegar as interseções
			for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
				//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
				//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
				//SDL_RenderPoint(renderer, new_x, ymin + i);
				if (y > 0 && y < height) {
					intersecoes[y].push_back(new_x);
					LE_CORE_ASSERT(y > 0 && y < height, "Coordenada Y com valor INVALIDO!");
				}
				new_x += Tx;
				//new_r += Tr;
				//new_g += Tg;
				//new_b += Tb;
			}
		}
	}

	void FillPoly(SDL_Renderer* renderer, std::vector<vertex3D> vertices, int height)
	{
		if (vertices.size() < 3) { return; }

		std::vector<std::vector<float>> intersecoes;
		intersecoes.resize(height);
		LE_CORE_ASSERT(intersecoes.size() == height, "Intersecoes com tamanho incorreto alocado!");

		FindIntersections(vertices, intersecoes, height);

		/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
		//for (Mesh* submesh : mesh->GetSubMeshes()) {
		//	std::vector<vertex3D> sub_vertices = submesh->GetVertices();
		//	if (sub_vertices.size() < 3) { continue; }
		//	if (submesh->IsActive()) {
		//		ImVec4 old_color = mesh->GetColor();
		//		ImVec4 submesh_color = submesh->GetColor();
		//		SDL_SetRenderDrawColorFloat(renderer, submesh_color.x, submesh_color.y, submesh_color.z, SDL_ALPHA_OPAQUE_FLOAT);
		//		FillPoly(renderer, submesh, height);
		//		SDL_SetRenderDrawColorFloat(renderer, old_color.x, old_color.y, old_color.z, SDL_ALPHA_OPAQUE_FLOAT);
		//	}
		//	FindIntersections(sub_vertices, intersecoes, height);
		//}

		// Ordenar todas as interceções
		for (auto& lista : intersecoes) {
			std::sort(lista.begin(), lista.end());
		}

		// Fazer o fill no poly
		for (int y = 0; y < intersecoes.size(); y++) {
			std::vector<float> lista = intersecoes[y];

			for (int i = 0; i + 1 < lista.size(); i += 2) {
				//if (i + 1 < lista.size()) {
				//	SDL_RenderPoint(renderer, (int)lista[i].x, y);
				//	SDL_RenderPoint(renderer, (int)lista[i + 1].x, y);
				//}
				SDL_RenderLine(renderer, lista[i], (float)y, lista[i + 1], (float)y);
			}
		}
	}
}