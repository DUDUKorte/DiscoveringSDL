#include "Mesh.h"
#include "EditorApp.h"
#include "Log.h"
#include <cmath>

Mesh::Mesh(std::string name, glm::vec3 firstVertex, std::vector<glm::vec3> v) {
	
	m_Name = name;
	m_Velocity = { 0.0f, 0.0f, 0.0f};
	GetTransform().Position = { firstVertex.x, firstVertex.y, firstVertex.z };
	meshType = MeshTypes::Type3D;
}

Mesh::~Mesh()
{

}

void Mesh::EventTick(float deltaTime)
{
	// Calcula a cada frame, seria a gravidade, colisões, etc...
	if (!GetTickEnabled()) { return; }
	// gravidadeeeeee
	GetTransform().Position.x += GetVelocity().x * deltaTime;
	GetTransform().Position.y += GetVelocity().y * deltaTime;
	
	// Aceleração
	SetVelocity({GetVelocity().x, GetVelocity().y + 0.001f * deltaTime, 0.0f});

	//Tem q verificar colisão
	//Nem ideia de como fazer isso aqui

	// Última linha sempre, pra desenhar o polígono com o transform novo
	MarkDirty();
}

void Mesh::EventInput(SDL_Event* event)
{
	if (!GetTickEnabled()) { return; }

	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;

		// MODE SWITCH
		switch (e.key) {
		case SDLK_SPACE:
			SetVelocity({ 0.0f, -0.3f, 0.0f });
			break;
		}
	}
}

// Recebe o world posisiton e converte pra local
void Mesh::addVertex(glm::vec3 newVertex){
	m_LocalVertices.push_back(newVertex);

	m_CentroideSum += newVertex;

	CalcLocalCentroide();
	m_IsDirty = true;
}

void Mesh::addFace(std::vector<int> faceIndices)
{
	if (faceIndices.empty()) { return; }

	m_Faces.push_back({faceIndices});
}