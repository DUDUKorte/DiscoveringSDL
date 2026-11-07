#include "PhysicsEngine.h"
#include <Utils/CoreEditor.h>

#include "Renderer/Renderer3D.h"
#include "Utils/Log.h"

void PhysicsEngine::UpdateAndCheckCollision(const std::vector<BaseMesh*>& meshList)
{
	for(BaseMesh* mesh : meshList) {
		if (!mesh->GetCollision()) { continue; }
		// limpa os collision hit dentro do outhits
		mesh->GetBoundingBox().OutHits.clear();
		UpdateWorldBB(mesh);
	}

	size_t count = meshList.size();
	for (size_t i = 0; i < count; i++) {
		BaseMesh* meshA = meshList[i];
		if (!meshA->GetCollision()) { continue; }

		for (size_t j = i + 1; j < count; j++) {
			BaseMesh* meshB = meshList[j];
			if (!meshB->GetCollision()) { continue; }
			
			CollisionHit* hitResult = CheckBBCollision(meshA->GetBoundingBox(), meshB->GetBoundingBox());
			if (hitResult != nullptr) { // se teve colisão!!!!!
				CollisionHit hitA;
				hitA.other = meshB;
				hitA.penetration = hitResult->penetration;
				hitA.normal = hitResult->normal;
				meshA->GetBoundingBox().OutHits.emplace_back(hitA);
				
				CollisionHit hitB;
				hitB.other = meshA;
				hitB.penetration = hitResult->penetration;
				hitB.normal = -hitResult->normal;
				meshB->GetBoundingBox().OutHits.emplace_back(hitB);
				
				delete hitResult;
			}
		}
	}
}

void PhysicsEngine::ResolvePhysics(const std::vector<BaseMesh*>& meshList) {
	// primeiro vamo rodar todas as meshes, verificar se ela simmula física
	// se simula física, agnt acessa cada mesh e dentro de cada mesh
	// roda todos os collisionHit, pra cada collision Hit agnt define
	// se tem que ajustar posição e zerar velocidade ou não
	for (BaseMesh* mesh : meshList) {
		if (!mesh->GetSimulatePhysics()){ continue; }
		if (!mesh->IsColliding()) { continue; }
		
		glm::vec3 totalPush(0.0f);
		
		// aqui roda cada hit ent
		for (CollisionHit& hit : mesh->GetBoundingBox().OutHits){
			// 50% de arrasto (arrasta o outro objeto)
			if (hit.other->GetSimulatePhysics()) {
				totalPush += hit.normal * hit.penetration * 0.5f;
			}else { //100% de arrasto pra fora da colisão
				totalPush += hit.normal * hit.penetration;
			}
			
			// Aqui realmente para o objeto
			float velAgainstOther = glm::dot(mesh->GetVelocity(), hit.normal);
			if (velAgainstOther < 0.0f) {
				mesh->GetVelocity() -= hit.normal * velAgainstOther;
			}
		}
		
		if (glm::length(totalPush) > 0.0f) {
			mesh->GetTransform().Position += totalPush;
			mesh->MarkDirty();
			UpdateWorldBB(mesh); // atualiza a colisão dele imediatamente pra evitrar bug (teria que esperar um frame inteiro pra ajustar)
		}
	}
}

RaycastHit PhysicsEngine::Raycast(glm::vec3 origin, glm::vec3 direction, float maxDist, BaseMesh* ignoreMesh, bool drawDebug) {
	direction = glm::normalize(direction);
	
	std::vector<BaseMesh*>& meshList = EditorApp::GetMeshList();
	
	RaycastHit raycastResult;
	raycastResult.distance = FLT_MAX;
	raycastResult.outHit = false;
	
	// todar todas as meshes com colisão
	for (BaseMesh* mesh : meshList) {
		if (!mesh->GetCollision() || mesh == ignoreMesh){ continue; }
		
		// testa a colisão com a obb
		BoundingBox& bb = mesh->GetBoundingBox();
		glm::vec3 axes[3] = {bb.axes.x, bb.axes.y, bb.axes.z};
		float halfExtends[3] = {bb.halfExtends.x, bb.halfExtends.y, bb.halfExtends.z};
		
		float tMin = 0.0f;
		float tMax = maxDist;
		glm::vec3 eI;
		bool hitMiss = false;
		
		glm::vec3 deltaP = bb.worldCenter - origin;
		for (int i = 0; i < 3; i++) {
			// distancia até o centro da caixa com a linha projetada na face
			float distToCenter = glm::dot(deltaP, axes[i]);
			// taxa de aproximação ( == 0 é paralelo a placa)
			float f = glm::dot(direction, axes[i]);
			
			if (glm::epsilonEqual(glm::abs(f), 0.0f, 0.00001f)) {
				if (glm::abs(distToCenter) > halfExtends[i]) { hitMiss = true; break; } // ou break tbm 
				if (glm::abs(distToCenter) <= halfExtends[i]){ continue; } // continua sem alterar nada
			}
			
			float t1 = (distToCenter - halfExtends[i]) / f;
			float t2 = (distToCenter + halfExtends[i]) / f;
			glm::vec3 normal = f > 0.0f ? -axes[i] : axes[i];
			
			if (t1 > t2) {
				std::swap(t1, t2);
				normal = -normal;
			}
			
			
			if (t1 > tMin) {
				tMin = t1;
				eI = normal;
			}
			tMax = glm::min(tMax, t2);
			
			if (tMin > tMax || tMax < 0.0f) {
				// retornaria false aqui
				hitMiss = true;
				break;
			}
		}
		// Passa pra próxima mesh se deu hitMiss
		if (hitMiss) {
			continue;
		}
		if (tMin < raycastResult.distance) {
			raycastResult.distance = tMin;
			raycastResult.impactPoint = origin + tMin * direction;
			raycastResult.normal = eI;
			raycastResult.other = mesh;
			raycastResult.outHit = true;
		}
	}
	
	if (drawDebug) {
		DrawRayDebug(origin, direction, maxDist, raycastResult, &EditorApp::Get()->GetWindowProperties(), Renderer3D::UsePerspective);
	}
	
	return raycastResult;
}

CollisionHit* PhysicsEngine::CheckBBCollision(const BoundingBox& a, const BoundingBox& b)
{
	CollisionHit* hitResult = new CollisionHit;
	hitResult->penetration = FLT_MAX;
	bool found = false;
	
	glm::vec3 lList[15] = { // sim, são 15, tem 6 nas 2 primeiras linhas
		a.axes.x, a.axes.y, a.axes.z,
		b.axes.x, b.axes.y, b.axes.z,
		glm::cross(a.axes.x, b.axes.x),
		glm::cross(a.axes.x, b.axes.y),
		glm::cross(a.axes.x, b.axes.z),
		glm::cross(a.axes.y, b.axes.x),
		glm::cross(a.axes.y, b.axes.y),
		glm::cross(a.axes.y, b.axes.z),
		glm::cross(a.axes.z, b.axes.x),
		glm::cross(a.axes.z, b.axes.y),
		glm::cross(a.axes.z, b.axes.z),
	};
	
	for(glm::vec3 l : lList) {
		if (glm::length(l) < 0.0001) { continue; }
		l = glm::normalize(l);
		
		float distance = glm::abs(glm::dot(b.worldCenter - a.worldCenter, l));
		
		// conta manual cabulosa aqui
		float raioa =	(a.halfExtends.x * glm::abs(glm::dot(l, a.axes.x))) + 
						(a.halfExtends.y * glm::abs(glm::dot(l, a.axes.y))) + 
						(a.halfExtends.z * glm::abs(glm::dot(l, a.axes.z)));
		
		float raiob =	(b.halfExtends.x * glm::abs(glm::dot(l, b.axes.x))) + 
						(b.halfExtends.y * glm::abs(glm::dot(l, b.axes.y))) + 
						(b.halfExtends.z * glm::abs(glm::dot(l, b.axes.z)));
		
		float overlap = (raioa + raiob) - distance;
		if (overlap < 0.0f) {
			delete hitResult;
			return nullptr;
		}
		// atauliza o penetration
		if (overlap < hitResult->penetration) {
			hitResult->penetration = overlap; // o mínimo que penetrou na outra mesh
			hitResult->normal = l; // o "melhor" axis
			found = true;
		}
	}
	// Garante que o normal sempre aponta de B -> A (pq lá no final vai colocar -normal pra B e normal pra A)
	if (glm::dot(a.worldCenter - b.worldCenter, hitResult->normal) < 0.0f) {
		hitResult->normal = -hitResult->normal;
	}
	
	if (!found ) {return nullptr;}
	
	return hitResult;
}

void PhysicsEngine::DrawRayDebug(glm::vec3 origin, glm::vec3 direction, float maxDist, const RaycastHit& hit, const WindowProperties* mainWindow, bool usePerspective)
{
    // Use GetForegroundDrawList() para garantir que desenhe por cima de qualquer janela/viewport
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();

    // 1. Monta a matriz View-Projection
    glm::mat4 camTransform = glm::translate(glm::mat4(1.0f), mainWindow->CameraPos);
    camTransform = glm::rotate(camTransform, glm::radians(mainWindow->CameraRot.y), { 0.0f, 1.0f, 0.0f });
    camTransform = glm::rotate(camTransform, glm::radians(mainWindow->CameraRot.x), { 1.0f, 0.0f, 0.0f });
    glm::mat4 viewMatrix = glm::inverse(camTransform);

    glm::mat4 projMatrix;
    if (usePerspective) {
       projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), static_cast<float>(mainWindow->Width) / mainWindow->Height, 0.1f, 100000.0f);
       projMatrix[1][1] *= -1.0f;
    } else {
       projMatrix = glm::ortho(0.0f, static_cast<float>(mainWindow->Width), static_cast<float>(mainWindow->Height), 0.0f, 10000.0f, -10000.0f);
    }
    glm::mat4 viewProj = projMatrix * viewMatrix;

    // Projeta coordenadas de mundo para pixels na tela
    auto WorldToScreen = [&](const glm::vec3& worldPos, ImVec2& outScreen) -> bool {
       glm::vec4 clip = viewProj * glm::vec4(worldPos, 1.0f);
       if (usePerspective && clip.w <= 0.1f) return false; // Ponto atrás ou colado na lente da câmera
       glm::vec3 ndc = glm::vec3(clip) / clip.w;
       outScreen.x = mainViewport->Pos.x + (ndc.x * 0.5f + 0.5f) * mainViewport->Size.x;
       outScreen.y = mainViewport->Pos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * mainViewport->Size.y;
       return true;
    };

    // Se o ponto de origem estiver colado na câmera (clip.w <= 0.1f), avança ao longo da direção
    glm::vec3 renderStart = origin;
    ImVec2 startScreen;
    if (!WorldToScreen(renderStart, startScreen)) {
       renderStart = origin + direction * 5.0f; // Salta 5 unidades à frente da câmera
       if (!WorldToScreen(renderStart, startScreen)) {
          // Se ainda assim estiver atrás, usa o centro da tela
          startScreen = ImVec2(mainViewport->Pos.x + mainViewport->Size.x * 0.5f,
                               mainViewport->Pos.y + mainViewport->Size.y * 0.5f);
       }
    }

    if (hit.outHit) {
       ImVec2 hitScreen;
       if (WorldToScreen(hit.impactPoint, hitScreen)) {
          // 1. Linha do tiro da origem até o impacto (Vermelha)
          drawList->AddLine(startScreen, hitScreen, IM_COL32(255, 50, 50, 255), 2.5f);

          // 2. Círculo vermelho sólido no ponto de impacto
          drawList->AddCircleFilled(hitScreen, 5.0f, IM_COL32(255, 20, 20, 255));

          // 3. Vetor Normal da face atingida (Linha Amarela de 30 unidades)
          ImVec2 normalEndScreen;
          if (WorldToScreen(hit.impactPoint + hit.normal * 30.0f, normalEndScreen)) {
             drawList->AddLine(hitScreen, normalEndScreen, IM_COL32(255, 230, 0, 255), 3.0f);
             drawList->AddCircleFilled(normalEndScreen, 3.5f, IM_COL32(255, 180, 0, 255));
          }
       }
    } else {
       // Tiro no vazio: traça a linha verde até maxDist
       ImVec2 endScreen;
       glm::vec3 endPoint = origin + direction * maxDist;
       if (WorldToScreen(endPoint, endScreen)) {
          drawList->AddLine(startScreen, endScreen, IM_COL32(50, 255, 80, 200), 1.5f);
       }
    }
}

void PhysicsEngine::DrawDebugBoxes(const std::vector<BaseMesh*>& meshList, const WindowProperties* mainWindow, bool usePerspective)
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
		projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), static_cast<float>(mainWindow->Width) / mainWindow->Height, 0.1f, 100000.0f);
		projMatrix[1][1] *= -1.0f;
	}
	else {
		projMatrix = glm::ortho(0.0f, static_cast<float>(mainWindow->Width), static_cast<float>(mainWindow->Height), 0.0f, 10000.0f, -10000.0f);
	}
	glm::mat4 viewProj = projMatrix * viewMatrix;

	// Índices das 12 arestas que formam um cubo

	for (BaseMesh* m : meshList) {
		if (!m->GetCollision() || !m->GetBoundingBox().ShowDebug) continue;

		const BoundingBox& bb = m->GetBoundingBox();

		ImVec2 screenPts[8];
		bool validCorner[8];

		for (int i = 0; i < 8; i++) {
			glm::vec4 clip = viewProj * glm::vec4(bb.obbPoints[i], 1.0f);
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
		
		// DESENHA O VETOR (normal * penetration) DE CADA HIT
		if (m->IsColliding()) {
			// Projeta o centro da mesh atual na tela (Ponto Inicial da seta)
			glm::vec4 startClip = viewProj * glm::vec4(bb.worldCenter, 1.0f);
			if (!usePerspective || startClip.w > 0.1f) {
				glm::vec3 startNdc = glm::vec3(startClip) / startClip.w;
				ImVec2 startScreen(
					mainViewport->Pos.x + (startNdc.x * 0.5f + 0.5f) * mainViewport->Size.x,
					mainViewport->Pos.y + (1.0f - (startNdc.y * 0.5f + 0.5f)) * mainViewport->Size.y
				);

				for (const CollisionHit& hit : bb.OutHits) {
					// Ponto final no mundo: Centro + (normal * penetration)
					glm::vec3 pushVector = hit.normal * hit.penetration;
					glm::vec3 endWorld = bb.worldCenter + pushVector;

					glm::vec4 endClip = viewProj * glm::vec4(endWorld, 1.0f);
					if (usePerspective && endClip.w <= 0.1f) continue;

					glm::vec3 endNdc = glm::vec3(endClip) / endClip.w;
					ImVec2 endScreen(
						mainViewport->Pos.x + (endNdc.x * 0.5f + 0.5f) * mainViewport->Size.x,
						mainViewport->Pos.y + (1.0f - (endNdc.y * 0.5f + 0.5f)) * mainViewport->Size.y
					);

					// Desenha a linha do empurrão (Amarelo vibrante) e uma bolinha na ponta
					drawList->AddLine(startScreen, endScreen, IM_COL32(255, 230, 0, 255), 3.0f);
					drawList->AddCircleFilled(endScreen, 5.0f, IM_COL32(255, 120, 0, 255));
				}
			}
		}

		// VERMELHO se estiver colidindo, VERDE se estiver livre!
		ImU32 boxColor = m->IsColliding()
			? IM_COL32(255, 45, 45, 255)
			: IM_COL32(45, 255, 85, 255);

		for (int e = 0; e < 12; e++) {
			const int edges[12][2] = {
				{ 0,1 },{ 1,3 },{ 3,2 },{ 2,0 }, // Face traseira (zMin)
				{ 4,5 },{ 5,7 },{ 7,6 },{ 6,4 }, // Face frontal (zMax)
				{ 0,4 },{ 1,5 },{ 2,6 },{ 3,7 }  // 4 conexões laterais
			};
			int i0 = edges[e][0];
			int i1 = edges[e][1];
			if (validCorner[i0] && validCorner[i1]) {
				drawList->AddLine(screenPts[i0], screenPts[i1], boxColor, 2.0f);
			}
		}
	}
}

void PhysicsEngine::UpdateWorldBB(BaseMesh* mesh)
{
	BoundingBox& bb = mesh->GetBoundingBox();
	glm::mat4 mat = mesh->GetTransformMatrix();
	
	bb.localCenter = (bb.localMin + bb.localMax) * 0.5f + bb.offset;
	glm::vec3 localHalf = (bb.localMax - bb.localMin) * 0.5f * bb.sizeScale;
	
	bb.worldCenter = glm::vec3(mat * glm::vec4(bb.localCenter, 1.0f));
	
	bb.axes.x = glm::normalize(glm::vec3(mat[0]));
	bb.axes.y = glm::normalize(glm::vec3(mat[1]));
	bb.axes.z = glm::normalize(glm::vec3(mat[2]));
	
	glm::vec3 vx = glm::vec3(mat[0]) * localHalf.x;
	glm::vec3 vy = glm::vec3(mat[1]) * localHalf.y;
	glm::vec3 vz = glm::vec3(mat[2]) * localHalf.z;
	
	bb.halfExtends = glm::vec3(glm::length(vx), glm::length(vy), glm::length(vz));
	
	// eu nunca mais quero montar essas matrizes manuais mds do céu
	bb.obbPoints[0] = bb.worldCenter - vx - vy - vz;
	bb.obbPoints[1] = bb.worldCenter + vx - vy - vz;
	bb.obbPoints[2] = bb.worldCenter - vx + vy - vz;
	bb.obbPoints[3] = bb.worldCenter + vx + vy - vz;
	bb.obbPoints[4] = bb.worldCenter - vx - vy + vz;
	bb.obbPoints[5] = bb.worldCenter + vx - vy + vz;
	bb.obbPoints[6] = bb.worldCenter - vx + vy + vz;
	bb.obbPoints[7] = bb.worldCenter + vx + vy + vz;
}

