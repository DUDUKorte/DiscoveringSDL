#include "Mesh.h"

Mesh::Mesh(vertex firstVertex, std::vector<vertex> v) {
	if (!(v.empty())) {
		m_Vertices = v;
	}
	else{
		addVertex(firstVertex);
	}
	m_Velocity = { 0,0 };
	m_Centroide = CalcCentroide();
}

Mesh::~Mesh(){
	for (Mesh m : m_SubMeshes) {
		m.~Mesh();
	}
	delete &m_Vertices;
}

void Mesh::addVertex(vertex v){
	m_Vertices.push_back(v);

	m_CentroideXSum += v.x;
	m_CentroideYSum += v.y;
	CalcCentroide();
}

void Mesh::removeVertex(){
	m_CentroideXSum -= m_Vertices.back().x;
	m_CentroideYSum -= m_Vertices.back().y;
	
	m_Vertices.pop_back();
	
	CalcCentroide();
}

void Mesh::addSubMesh(Mesh mesh){
	m_SubMeshes.push_back(mesh);
}

Mesh Mesh::GetSubMesh(int index){
	return m_SubMeshes.at(index);
}

vertex Mesh::CalcCentroide(){
	m_Centroide.x = m_CentroideXSum / m_Vertices.size();
	m_Centroide.y = m_CentroideYSum / m_Vertices.size();
	return m_Centroide;
}
