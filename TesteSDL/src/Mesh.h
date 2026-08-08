#pragma once
#include <vector>
#include <imgui.h>

struct vertex {
	float x;
	float y;
	ImVec4 color = {1,1,1, 1};
};

struct Vec2 {
	float x;
	float y;
};

struct Vec3 {
	float r;
	float g;
	float b;
};

class Mesh {
public:
	Mesh(vertex firstVertex = {0,0}, std::vector<vertex> v = {});
	~Mesh();

	void addVertex(vertex v);
	void removeVertex();
	void addSubMesh(Mesh mesh);

	inline std::vector<vertex>& GetVertices() { return m_Vertices; }
	inline std::vector<Mesh> GetSubMeshes() { return m_SubMeshes; }
	Mesh GetSubMesh(int index);

	inline Vec2 Velocity() { return m_Velocity; }
	inline vertex GetCentroide() { return m_Centroide; }

	// getters
	inline bool& IsActive() { return m_IsActive; }
	inline bool& GetFill() { return m_Fill; }
	inline bool& IsClosed() { return m_Closed; }
	inline bool& GetGravity() { return m_Gravity; }
	inline ImVec4& GetColor() { return m_Color; }
	// setters
	inline void SetActive(bool active) { m_IsActive = active; }
	inline void SetFill(bool fill) { m_Fill = fill; }
	inline void SetClosed(bool closed) { m_Closed = closed; }
	inline void SetGravity(bool gravity) { m_Gravity = gravity; }
	inline void SetVelocity(Vec2 velocity) { m_Velocity = velocity; }
	inline void SetColor(ImVec4 color) { m_Color = color; }

private:
	vertex CalcCentroide();

public:
	// principais atributos
	std::vector<vertex> m_Vertices = {};
	std::vector<Mesh> m_SubMeshes = {};
	Vec2 m_Velocity;
	vertex m_Centroide;
	// do centroide
	float m_CentroideXSum = 0;
	float m_CentroideYSum = 0;
	// atributos de aparencia
	bool m_IsActive = false; // se está selecionado ou nn
	bool m_Fill = false; // se preenche o meio dele
	bool m_Closed = false; // se o último vértice é conectado cm o primeiro
	bool m_Gravity = false; // se é afetado pela gravidade ou nn
	//
	ImVec4 m_Color = { 0, 1, 0 , 1 };
};