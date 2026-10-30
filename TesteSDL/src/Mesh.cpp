#include "Mesh.h"
#include "EditorApp.h"
#include "Log.h"
#include <cmath>

Mesh::Mesh(std::string name, vertex3D firstVertex, std::vector<vertex3D> v) {
	
	m_Name = name;
	m_Velocity = { 0.0f, 0.0f, 0.0f};
	GetTransform().Position = { firstVertex.x, firstVertex.y, firstVertex.z };

	//if (!(v.empty())) {
	//	m_LocalVertices = v;
	//	// Aqui calcularia o centroide
	//	CalcLocalCentroide();
	//}
	//else{
	//	//addVertex(firstVertex);
	//}

	//CalcLocalCentroide();
}

Mesh::~Mesh(){
	for (Mesh* m : m_SubMeshes) {
		delete m;
	}
	m_SubMeshes.clear();
	//delete &m_Vertices;
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
void Mesh::addVertex(vertex3D newVertex){
	m_LocalVertices.push_back(newVertex);

	m_CentroideXSum += newVertex.x;
	m_CentroideYSum += newVertex.y;
	m_CentroideZSum += newVertex.z;

	CalcLocalCentroide();
	m_IsDirty = true;
}

void Mesh::removeVertex(){
	if (m_LocalVertices.empty()) { return; }
	
	m_CentroideXSum -= m_LocalVertices.back().x;
	m_CentroideYSum -= m_LocalVertices.back().y;
	
	m_LocalVertices.pop_back();
	
	CalcLocalCentroide();
	m_IsDirty = true;
}

void Mesh::addFace(std::vector<int> faceIndices)
{
	if (faceIndices.empty()) { return; }

	m_Faces.push_back({faceIndices});
}

void Mesh::addSubMesh(Mesh* mesh){
	mesh->m_Parent = this;

	vertex3D parentPivot;
	parentPivot.x = m_LocalCentroide.x + GetTransform().Position.x;
	parentPivot.y = m_LocalCentroide.y + GetTransform().Position.y;

	mesh->GetTransform().Position.x -= parentPivot.x;
	mesh->GetTransform().Position.y -= parentPivot.y;

	m_SubMeshes.push_back(mesh);
}

void Mesh::removeSubMesh(int index)
{
	Mesh* markForDelte = GetSubMesh(index);
	m_SubMeshes.erase(m_SubMeshes.begin() + index);
	delete markForDelte;
}

void Mesh::ResetTransform(int x_offset, int y_offset)
{
	// Save old transform
	m_CacheTransform = m_Transform;
	// Reset transform
	//GetTransform().Position = {0.0f + x_offset, 0.0f + y_offset};
	//if (m_Parent != nullptr) {
	//	GetTransform().Position = { 0.0f, 0.0f};
	//}
	//GetTransform().Rotation = 0.0f;
	//GetTransform().Scale = { 1.0f, 1.0f };
	MarkDirty();
	for (Mesh* subMesh : GetSubMeshes()) {
		subMesh->MarkDirty();
	}
}

void Mesh::RestoreTransform()
{
	m_Transform = m_CacheTransform;

	MarkDirty();
	for (Mesh* subMesh : GetSubMeshes()) {
		subMesh->MarkDirty();
	}
}



Mesh* Mesh::GetSubMesh(int index){
	return m_SubMeshes[index];
}



void Mesh::CalcLocalCentroide(){
	if (m_LocalVertices.empty()) { return; }

	m_LocalCentroide.x = m_CentroideXSum / m_LocalVertices.size();
	m_LocalCentroide.y = m_CentroideYSum / m_LocalVertices.size();
	m_LocalCentroide.z = m_CentroideZSum / m_LocalVertices.size();
}

vertex3D Mesh::GetCentroide() {
	float px = m_LocalCentroide.x + GetTransform().Position.x;
	float py = m_LocalCentroide.y + GetTransform().Position.y;
	float pz = m_LocalCentroide.z + GetTransform().Position.z;
	
	// Se for submesh, arrumar a posição
	if (m_Parent) {
		px *= m_Parent->GetTransform().Scale.x;
		py *= m_Parent->GetTransform().Scale.y;
		pz *= m_Parent->GetTransform().Scale.z;

		//if (m_Parent->GetTransform().Rotation != 0.0f) {
		//	float pCosA = std::cos(m_Parent->GetTransform().Rotation);
		//	float pSinA = std::sin(m_Parent->GetTransform().Rotation);
		//	float tx = px * pCosA - py * pSinA;
		//	float ty = px * pSinA + py * pCosA;
		//	px = tx;
		//	py = ty;
		//}

		px += m_Parent->GetLocalCentroide().x + m_Parent->GetTransform().Position.x;
		py += m_Parent->GetLocalCentroide().y + m_Parent->GetTransform().Position.y;
		pz += m_Parent->GetLocalCentroide().z + m_Parent->GetTransform().Position.z;
	}

	return {px, py, pz};
}

Transform3D& Mesh::GetTransform()
{
	return m_Transform;
}

std::vector<vertex3D>& Mesh::GetVertices() {
	if (m_IsDirty) {
		UpdateWorldVertices();
	}
	return m_Vertices;
}

void Mesh::UpdateWorldVertices() {
	m_Vertices.clear();
	m_Vertices.reserve(m_LocalVertices.size());
	m_TransformMatrix = glm::mat4(1.0f);

	// Move pra postição de volta
	m_TransformMatrix = glm::translate(m_TransformMatrix, m_LocalCentroide + m_Transform.Position);
	// rotaciona
	m_TransformMatrix = glm::rotate(m_TransformMatrix, (float)m_Transform.Rotation.x, { 1.0f, 0.0f, 0.0f});
	m_TransformMatrix = glm::rotate(m_TransformMatrix, (float)m_Transform.Rotation.y, { 0.0f, 1.0f, 0.0f });
	m_TransformMatrix = glm::rotate(m_TransformMatrix, (float)m_Transform.Rotation.z, { 0.0f, 0.0f, 1.0f });
	//escala
	m_TransformMatrix = glm::scale(m_TransformMatrix, m_Transform.Scale);
	// Move para o ponto central
	m_TransformMatrix = glm::translate(m_TransformMatrix, -m_LocalCentroide);

	// Se for submesh, pega a rotação do pai
	if (m_Parent) {
		m_TransformMatrix = m_Parent->GetTransformMatrix() * m_TransformMatrix;
	}

	for (const vertex3D& localVertex : m_LocalVertices) {
		glm::vec4 adjustVertex = { localVertex.x, localVertex.y, localVertex.z, 1.0f };
		adjustVertex = m_TransformMatrix * adjustVertex;
		m_Vertices.push_back({ adjustVertex.x, adjustVertex.y, adjustVertex.z });
	}
	m_IsDirty = false;
}