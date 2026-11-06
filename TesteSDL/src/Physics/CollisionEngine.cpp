#include "CollisionEngine.h"

#include <Utils/LeandroMath.h>
#include <Utils/CoreEditor.h>
#include <EditorCore/Mesh.h>

void CollisionEngine::UpdateAndCheckCollision(std::vector<BaseMesh*>& meshList)
{
	for(BaseMesh* mesh : meshList) {
		if (!mesh->GetCollision()) { continue; }
		mesh->GetBoundingBox().CollidingMeshes.clear();
		UpdateWorldBB(mesh); // isso aqui nem tá fazendo mais nada na vdd, mudei pra calcular direto na mesh quando atualiza os vértices
	}

	size_t count = meshList.size();
	for (size_t i = 0; i < count; i++) {
		BaseMesh* meshA = meshList[i];
		if (!meshA->GetCollision()) { continue; }

		for (size_t j = i + 1; j < count; j++) {
			BaseMesh* meshB = meshList[j];
			if (!meshB->GetCollision()) { continue; }

			if (CheckBBCollision(meshA->GetBoundingBox(), meshB->GetBoundingBox())) {
				meshA->GetBoundingBox().CollidingMeshes.push_back(meshB);
				meshB->GetBoundingBox().CollidingMeshes.push_back(meshA);
			}
		}
	}

}

bool CollisionEngine::CheckBBCollision(const BoundingBox& a, const BoundingBox& b)
{
	return	(a.worldMin.x <= b.worldMax.x && a.worldMax.x >= b.worldMin.x) &&
			(a.worldMin.y <= b.worldMax.y && a.worldMax.y >= b.worldMin.y) &&
			(a.worldMin.z <= b.worldMax.z && a.worldMax.z >= b.worldMin.z);
}

void CollisionEngine::DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, WindowProperties* mainWindow, bool usePerspective)
{
	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	ImGuiViewport* mainViewport = ImGui::GetMainViewport();

	// Monta a mesma ViewProj do seu DrawMeshGPU
	glm::mat4 camTransform = glm::translate(glm::mat4(1.0f), mainWindow->CameraPos);
	camTransform = glm::rotate(camTransform, glm::radians(mainWindow->CameraRot.y), { 0.0f, 1.0f, 0.0f });
	camTransform = glm::rotate(camTransform, glm::radians(mainWindow->CameraRot.x), { 1.0f, 0.0f, 0.0f });
	glm::mat4 viewMatrix = glm::inverse(camTransform);

	glm::mat4 projMatrix;
	if (usePerspective) {
		projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), (float)mainWindow->Width / mainWindow->Height, 0.1f, 100000.0f);
		projMatrix[1][1] *= -1.0f;
	}
	else {
		projMatrix = glm::ortho(0.0f, (float)mainWindow->Width, (float)mainWindow->Height, 0.0f, 10000.0f, -10000.0f);
	}
	glm::mat4 viewProj = projMatrix * viewMatrix;

	// Índices das 12 arestas que formam um cubo
	const int edges[12][2] = {
		{ 0,1 },{ 1,3 },{ 3,2 },{ 2,0 }, // Face traseira (zMin)
		{ 4,5 },{ 5,7 },{ 7,6 },{ 6,4 }, // Face frontal (zMax)
		{ 0,4 },{ 1,5 },{ 2,6 },{ 3,7 }  // 4 conexões laterais
	};

	for (BaseMesh* m : meshList) {
		if (!m->GetCollision() || !m->GetBoundingBox().ShowDebug) continue;

		const BoundingBox& bb = m->GetBoundingBox();
		glm::vec3 wMin = bb.worldMin;
		glm::vec3 wMax = bb.worldMax;

		glm::vec3 worldCorners[8] = {
			{ wMin.x, wMin.y, wMin.z },{ wMax.x, wMin.y, wMin.z },
			{ wMin.x, wMax.y, wMin.z },{ wMax.x, wMax.y, wMin.z },
			{ wMin.x, wMin.y, wMax.z },{ wMax.x, wMin.y, wMax.z },
			{ wMin.x, wMax.y, wMax.z },{ wMax.x, wMax.y, wMax.z }
		};

		ImVec2 screenPts[8];
		bool validCorner[8];

		for (int i = 0; i < 8; i++) {
			glm::vec4 clip = viewProj * glm::vec4(worldCorners[i], 1.0f);
			if (usePerspective && clip.w <= 0.1f) {
				validCorner[i] = false; // Ponto atrás da câmera
				continue;
			}
			validCorner[i] = true;
			glm::vec3 ndc = glm::vec3(clip) / clip.w;

			// Converte de NDC (-1 a +1) para pixels da janela principal
			float sx = mainViewport->Pos.x + (ndc.x * 0.5f + 0.5f) * mainViewport->Size.x;
			float sy = mainViewport->Pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * mainViewport->Size.y;
			screenPts[i] = ImVec2(sx, sy);
		}

		// VERMELHO se estiver colidindo, VERDE se estiver livre!
		ImU32 boxColor = m->IsColliding()
			? IM_COL32(255, 45, 45, 255)
			: IM_COL32(45, 255, 85, 255);

		for (int e = 0; e < 12; e++) {
			int i0 = edges[e][0];
			int i1 = edges[e][1];
			if (validCorner[i0] && validCorner[i1]) {
				drawList->AddLine(screenPts[i0], screenPts[i1], boxColor, 2.0f);
			}
		}
	}
}

void CollisionEngine::UpdateWorldBB(BaseMesh* mesh)
{

}

