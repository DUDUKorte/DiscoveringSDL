#include "Mesh2D.h"
#include "EditorApp.h"
#include "Log.h"
#include <cmath>

Mesh2D::Mesh2D(std::string name, glm::vec3 firstVertex, std::vector<glm::vec3> v) {

	m_Name = name;
	m_Velocity = { 0.0f, 0.001f, 0.0f };
	GetTransform().Position = firstVertex;

	meshType = MeshTypes::Type2D;

	if (!(v.empty())) {
		m_LocalVertices = v;
		// Aqui calcularia o centroide
		CalcLocalCentroide();
	}
	else {
		addVertex(firstVertex);
	}

	CalcLocalCentroide();
}

Mesh2D::~Mesh2D() {

}

void Mesh2D::EventTick(float deltaTime)
{
	// Calcula a cada frame, seria a gravidade, colisões, etc...
	if (!GetTickEnabled()) { return; }
	// gravidadeeeeee
	GetTransform().Position.x += GetVelocity().x * deltaTime;
	GetTransform().Position.y += GetVelocity().y * deltaTime;

	// Aceleração
	SetVelocity({ GetVelocity().x, GetVelocity().y + 0.001f * deltaTime, 0.0f });

	//Tem q verificar colisão
	//Nem ideia de como fazer isso aqui

	// Última linha sempre, pra desenhar o polígono com o transform novo
	MarkDirty();
}

void Mesh2D::EventInput(SDL_Event* event)
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
void Mesh2D::addVertex(glm::vec3 newVertex) {
	glm::vec3 localPosition = newVertex;

	// Se for uma submesh
	if (m_Parent) {
		// defaz a translação, rotação e escala da mesh pai

		localPosition.x -= (m_Parent->GetLocalCentroide().x + m_Parent->GetTransform().Position.x);
		localPosition.y -= (m_Parent->GetLocalCentroide().y + m_Parent->GetTransform().Position.y);

		if (m_Parent->GetTransform().Rotation.z != 0.0f) {
			float invRot = -m_Parent->GetTransform().Rotation.z;
			float tx = localPosition.x * std::cos(invRot) - localPosition.y * std::sin(invRot);
			float ty = localPosition.x * std::sin(invRot) + localPosition.y * std::cos(invRot);
			localPosition.x = tx;
			localPosition.y = ty;
		}

		localPosition.x /= m_Parent->GetTransform().Scale.x;
		localPosition.y /= m_Parent->GetTransform().Scale.y;
	}

	// De world position para local position
	localPosition.x -= GetTransform().Position.x;
	localPosition.y -= GetTransform().Position.y;

	m_LocalVertices.push_back(localPosition);

	m_CentroideSum += localPosition;
	CalcLocalCentroide();
	m_IsDirty = true;
}