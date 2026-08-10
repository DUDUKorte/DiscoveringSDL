#include "Renderer2D.h"
#include "EditorApp.h"
#include "Log.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>

namespace Renderer2D {
	void DrawLinesWithVertices(SDL_Renderer* renderer, std::vector<vertex>& vertices, bool closed)
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

	void FindIntersections(std::vector<vertex>& vertices, std::vector<std::vector<float>>& intersecoes, int height)
	{
		for (int i = 0; i < vertices.size(); i++) {
			vertex source = vertices[i];
			vertex point = vertices[(i + 1) % vertices.size()];

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

	void FillPoly(SDL_Renderer* renderer, Mesh* mesh, int height)
	{
		std::vector<vertex> vertices = mesh->GetVertices();
		if (vertices.size() < 3) { return; }

		std::vector<std::vector<float>> intersecoes;
		intersecoes.resize(height);
		LE_CORE_ASSERT(intersecoes.size() == height, "Intersecoes com tamanho incorreto alocado!");

		FindIntersections(vertices, intersecoes, height);

		/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
		for (auto submesh : mesh->GetSubMeshes()) {
			std::vector<vertex> sub_vertices = submesh->GetVertices();
			if (sub_vertices.size() < 3) { continue; }
			FindIntersections(sub_vertices, intersecoes, height);
		}

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

	void DrawGizmo(Mesh* activeMesh, int mode, int currentGizmoState, int activeGizmoState)
	{
		if (activeMesh == nullptr || activeMesh->GetVertices().empty()) { return; }
		if (mode == Selection || mode == CreatePoly) { return; }

		ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

		vertex centroide = activeMesh->GetCentroide();
		ImVec2 center(centroide.x, centroide.y);

		float size = 60.0f;
		float thickness = 3.0f;

		// pega rotação do pa pra aplicar no gizmo lcoal (da submesh)
		float gizmoRot = 0.0f;
		if (activeMesh->m_Parent != nullptr) {
			gizmoRot = activeMesh->m_Parent->GetTransform().Rotation;
		}

		// Vetores direcinaoais rotacionados
		float cosR = std::cos(gizmoRot);
		float sinR = std::sin(gizmoRot);
		ImVec2 dirX(cosR, sinR);
		ImVec2 dirY(sinR, -cosR);

		ImU32 colorX = IM_COL32(220, 50, 50, 255);
		ImU32 colorY = IM_COL32(50, 220, 50, 255);
		ImU32 colorRot = IM_COL32(50, 50, 220, 255);
		ImU32 colorCenter = IM_COL32(20, 200, 200, 255);

		// Colocar hover/click nos gizmos
		if (currentGizmoState == GIZMO_HOVER_X || activeGizmoState == GIZMO_HOVER_X) colorX = IM_COL32(255, 120, 120, 255);
		if (currentGizmoState == GIZMO_HOVER_Y || activeGizmoState == GIZMO_HOVER_Y) colorY = IM_COL32(120, 255, 120, 255);
		if (currentGizmoState == GIZMO_HOVER_ROT || activeGizmoState == GIZMO_HOVER_ROT) colorRot = IM_COL32(120, 200, 255, 255);
		if (currentGizmoState == GIZMO_HOVER_CENTER || activeGizmoState == GIZMO_HOVER_CENTER) colorCenter = IM_COL32(255, 255, 100, 255);

		// FUnção pra criar quadrados com rotação mais fácil
		auto AddRotQuad = [&](ImVec2 p, float s, ImU32 col) {
			draw_list->AddQuadFilled(
				ImVec2(p.x + dirX.x * s - dirY.x * s, p.y + dirX.y * s - dirY.y * s),
				ImVec2(p.x + dirX.x * s + dirY.x * s, p.y + dirX.y * s + dirY.y * s),
				ImVec2(p.x - dirX.x * s + dirY.x * s, p.y - dirX.y * s + dirY.y * s),
				ImVec2(p.x - dirX.x * s - dirY.x * s, p.y - dirX.y * s - dirY.y * s),
				col);
			};

		if (mode == MovePoly) {
			// linha e seta od x
			ImVec2 endX(center.x + dirX.x * size, center.y + dirX.y * size);
			draw_list->AddLine(center, endX, colorX, thickness);
			draw_list->AddTriangleFilled(
				ImVec2(endX.x + dirX.x * 12, endX.y + dirX.y * 12),
				ImVec2(endX.x + dirY.x * 6, endX.y + dirY.y * 6),
				ImVec2(endX.x - dirY.x * 6, endX.y - dirY.y * 6),
				colorX);

			// linha e seta do 
			ImVec2 endY(center.x + dirY.x * size, center.y + dirY.y * size);
			draw_list->AddLine(center, endY, colorY, thickness);
			draw_list->AddTriangleFilled(
				ImVec2(endY.x + dirY.x * 12, endY.y + dirY.y * 12),
				ImVec2(endY.x + dirX.x * 6, endY.y + dirX.y * 6),
				ImVec2(endY.x - dirX.x * 6, endY.y - dirX.y * 6),
				colorY);

			// centro
			AddRotQuad(center, 6.0f, colorCenter);
		}
		else if (mode == ScalePoly) {
			// eixo x
			draw_list->AddLine(center, ImVec2(center.x + size, center.y), colorX, thickness);
			draw_list->AddRectFilled(ImVec2(center.x + size, center.y - 5), ImVec2(center.x + size + 10, center.y + 5), colorX);
			// eixo y
			draw_list->AddLine(center, ImVec2(center.x, center.y - size), colorY, thickness);
			draw_list->AddRectFilled(ImVec2(center.x + 5, center.y - size), ImVec2(center.x - 5, center.y - size - 10), colorY);
			// centro
			draw_list->AddRectFilled(ImVec2(center.x - 6, center.y - 6), ImVec2(center.x + 6, center.y + 6), colorCenter);
		}
		else if (mode == RotatePoly) {
			// circulo
			draw_list->AddCircle(center, size, colorRot, 0, thickness);
			draw_list->AddCircleFilled(ImVec2(center.x + size, center.y), 5.0f, colorCenter);
		}
	}
}