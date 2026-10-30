#include "BaseMesh.h"

BaseMesh::~BaseMesh()
{
	for (BaseMesh* m : m_SubMeshes) {
		delete m;
	}
	m_SubMeshes.clear();
}

void BaseMesh::removeVertex()
{
	if (m_LocalVertices.empty()) { return; }

	m_CentroideSum -= m_LocalVertices.back();

	m_LocalVertices.pop_back();

	CalcLocalCentroide();
	m_IsDirty = true;
}

void BaseMesh::addSubMesh(BaseMesh* mesh)
{
	mesh->m_Parent = this;

	glm::vec3 parentPivot = m_LocalCentroide + GetTransform().Position;
	mesh->GetTransform().Position -= parentPivot;
	
	m_SubMeshes.push_back(mesh);
}

void BaseMesh::removeSubMesh(int index)
{
	BaseMesh* markForDelte = GetSubMesh(index);
	m_SubMeshes.erase(m_SubMeshes.begin() + index);
	delete markForDelte;
}

void BaseMesh::ResetTransform(int x_offset /*= 0*/, int y_offset /*= 0*/, int z_offset /*= 0*/)
{
	// Save old transform
	m_CacheTransform = m_Transform;
	m_TransformMatrix = glm::mat4(1.0f);
	// Reset transform
	GetTransform().Position = {0.0f + x_offset, 0.0f + y_offset, 0.0f + z_offset};
	if (m_Parent != nullptr) {
		GetTransform().Position = { 0.0f, 0.0f, 0.0f };
	}
	GetTransform().Rotation = {0.0f, 0.0f, 0.0f};
	GetTransform().Scale = { 1.0f, 1.0f, 1.0f };

	MarkDirty();
	for (BaseMesh* subMesh : GetSubMeshes()) {
		subMesh->MarkDirty();
	}
}

void BaseMesh::RestoreTransform()
{
	m_Transform = m_CacheTransform;

	MarkDirty();
	for (BaseMesh* subMesh : GetSubMeshes()) {
		subMesh->MarkDirty();
	}
}

std::vector<glm::vec3>& BaseMesh::GetVertices()
{
	if (m_IsDirty) {
		UpdateWorldVertices();
	}
	return m_Vertices;
}

BaseMesh* BaseMesh::GetSubMesh(int index)
{
	return m_SubMeshes[index];
}

glm::vec3 BaseMesh::GetCentroide()
{
	if (m_IsDirty) { UpdateWorldVertices(); }

	glm::vec4 pv = { m_LocalCentroide.x, m_LocalCentroide.y, m_LocalCentroide.z, 1.0f };
	pv = m_TransformMatrix * pv;

	// Se for submesh, arrumar a posição
	if (m_Parent) {
		pv = m_Parent->GetTransformMatrix() * pv;
	}

	return { pv.x, pv.y, pv.z };
}

Transform& BaseMesh::GetTransform()
{
	return m_Transform;
}

void BaseMesh::CalcLocalCentroide()
{
	if (m_LocalVertices.empty()) { return; }

	// deu erro fazer m_LocalCentroide = m_CentroideSum / m_LocalVertices.size();

	m_LocalCentroide.x = m_CentroideSum.x / m_LocalVertices.size();
	m_LocalCentroide.y = m_CentroideSum.y / m_LocalVertices.size();
	m_LocalCentroide.z = m_CentroideSum.z / m_LocalVertices.size();
}

void BaseMesh::UpdateWorldVertices()
{
	m_Vertices.clear();
	m_Vertices.reserve(m_LocalVertices.size());
	m_TransformMatrix = glm::mat4(1.0f);
	m_MinMaxY = { 10000.0f, -1.0f };

	// Move pra postição de volta
	m_TransformMatrix = glm::translate(m_TransformMatrix, m_LocalCentroide + m_Transform.Position);
	// rotaciona
	m_TransformMatrix = glm::rotate(m_TransformMatrix, (float)m_Transform.Rotation.x, { 1.0f, 0.0f, 0.0f });
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

	for (const glm::vec3& localVertex : m_LocalVertices) {
		glm::vec4 adjustVertex = { localVertex.x, localVertex.y, localVertex.z, 1.0f };
		adjustVertex = m_TransformMatrix * adjustVertex;
		m_Vertices.push_back({ adjustVertex.x, adjustVertex.y, adjustVertex.z });

		// Atualiza o minmax
		if (adjustVertex.y < m_MinMaxY.x) {
			m_MinMaxY.x = adjustVertex.y;
		}
		if (adjustVertex.y > m_MinMaxY.y) {
			m_MinMaxY.y = adjustVertex.y;
		}
	}

	m_IsDirty = false;
}
