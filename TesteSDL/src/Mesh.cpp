#include "Mesh.h"
#include "EditorApp.h"
#include <cmath>

Mesh::Mesh(std::string name, vertex firstVertex, std::vector<vertex> v) {
	
	m_Name = name;
	m_Velocity = { 0,0 };
	GetTransform().Position = { firstVertex.x, firstVertex.y };

	if (!(v.empty())) {
		m_LocalVertices = v;
		// Aqui calcularia o centroide
		CalcLocalCentroide();
	}
	else{
		addVertex(firstVertex);
	}

	CalcLocalCentroide();
}

Mesh::~Mesh(){
	for (Mesh* m : m_SubMeshes) {
		delete m;
	}
	m_SubMeshes.clear();
	//delete &m_Vertices;
}

// Recebe o world posisiton e converte pra local
void Mesh::addVertex(vertex worldPosition){
	vertex localPosition = worldPosition;

	// Se for uma submesh
	if (m_Parent) {
		// defaz a translação, rotação e escala da mesh pai
		localPosition.x -= (m_Parent->m_LocalCentroide.x + m_Parent->GetTransform().Position.x);
		localPosition.y -= (m_Parent->m_LocalCentroide.y + m_Parent->GetTransform().Position.y);
		
		if (m_Parent->GetTransform().Rotation != 0.0f) {
			float invRot = -m_Parent->GetTransform().Rotation;
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

	m_CentroideXSum += localPosition.x;
	m_CentroideYSum += localPosition.y;
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

void Mesh::addSubMesh(Mesh* mesh){
	mesh->m_Parent = this;

	vertex parentPivot;
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
	GetTransform().Position = {0.0f + x_offset, 0.0f + y_offset};
	if (m_Parent != nullptr) {
		GetTransform().Position = { 0.0f, 0.0f};
	}
	GetTransform().Rotation = 0.0f;
	GetTransform().Scale = { 1.0f, 1.0f };
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

void Mesh::SetEditMode(bool editMode)
{
	MarkDirty();
	for (Mesh* m : m_SubMeshes) {
		m->MarkDirty();
	}

	m_EditMode = editMode;
}

void Mesh::CalcLocalCentroide(){
	if (m_LocalVertices.empty()) { return; }

	m_LocalCentroide.x = m_CentroideXSum / m_LocalVertices.size();
	m_LocalCentroide.y = m_CentroideYSum / m_LocalVertices.size();
}

vertex Mesh::GetCentroide() {
	float px = m_LocalCentroide.x + GetTransform().Position.x;
	float py = m_LocalCentroide.y + GetTransform().Position.y;
	
	// Se for submesh, arrumar a posição
	if (m_Parent) {
		px *= m_Parent->GetTransform().Scale.x;
		py *= m_Parent->GetTransform().Scale.y;

		if (m_Parent->GetTransform().Rotation != 0.0f) {
			float pCosA = std::cos(m_Parent->GetTransform().Rotation);
			float pSinA = std::sin(m_Parent->GetTransform().Rotation);
			float tx = px * pCosA - py * pSinA;
			float ty = px * pSinA + py * pCosA;
			px = tx;
			py = ty;
		}

		px += m_Parent->GetLocalCentroide().x + m_Parent->GetTransform().Position.x;
		py += m_Parent->GetLocalCentroide().y + m_Parent->GetTransform().Position.y;
	}

	return {px, py};
}

Transform2D& Mesh::GetTransform()
{
	if (!m_EditMode) {
		return m_Transform;
	}
	else {
		Transform2D defaultTransform;
		defaultTransform.Position = { WIDTH / 2, HEIGHT / 2 };
		defaultTransform.Rotation = 0.0f;
		defaultTransform.Scale = { 1.0f, 1.0f };

		return defaultTransform;
	}
}

std::vector<vertex>& Mesh::GetVertices() {
	if (m_IsDirty) {
		UpdateWorldVertices();
	}
	return m_Vertices;
}

void Mesh::UpdateWorldVertices() {
	m_Vertices.clear();
	m_Vertices.reserve(m_LocalVertices.size());

	float cosA = std::cos(GetTransform().Rotation);
	float sinA = std::sin(GetTransform().Rotation);

	// Se for submesh, pega a rotação do pai
	float pCosA = 1.0f, pSinA = 0.0f;
	if (m_Parent) {
		pCosA = std::cos(m_Parent->GetTransform().Rotation);
		pSinA = std::sin(m_Parent->GetTransform().Rotation);
	}

	for (const vertex& localVertex : m_LocalVertices) {
		// Apply sacale
		float px = localVertex.x - m_LocalCentroide.x;
		float py = localVertex.y - m_LocalCentroide.y;

		px *= GetTransform().Scale.x;
		py *= GetTransform().Scale.y;

		// Apply rotation
		if (GetTransform().Rotation != 0.0f) {
			float tx = px * cosA - py * sinA;
			float ty = px * sinA + py * cosA;
			px = tx;
			py = ty;
		}

		// Applt transform
		px += m_LocalCentroide.x + GetTransform().Position.x;
		py += m_LocalCentroide.y + GetTransform().Position.y;

		// Herdar o transform do pai
		if (m_Parent) {
			px *= m_Parent->GetTransform().Scale.x;
			py *= m_Parent->GetTransform().Scale.y;

			if (m_Parent->GetTransform().Rotation != 0.0f) {
				float tx = px * pCosA - py * pSinA;
				float ty = px * pSinA + py * pCosA;
				px = tx;
				py = ty;
			}

			px += m_Parent->GetLocalCentroide().x + m_Parent->GetTransform().Position.x;
			py += m_Parent->GetLocalCentroide().y + m_Parent->GetTransform().Position.y;
		}

		m_Vertices.push_back({ px, py });
	}
	m_IsDirty = false;
}