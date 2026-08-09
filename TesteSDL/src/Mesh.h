#pragma once
#include <vector>
#include <imgui.h>
#include <string>

struct vertex {
	float x;
	float y;
};

struct Vec2 {
	float x;
	float y;
};

struct Transform2D {
	Vec2 Position = { 0, 0 };
	float Rotation = 0.0f;
	Vec2 Scale = { 1.0f, 1.0f };
};

class Mesh {
public:
	Mesh(std::string name, vertex firstVertex = {0,0}, std::vector<vertex> v = {});
	~Mesh();

	void addVertex(vertex worldPosition);
	void removeVertex();
	void addSubMesh(Mesh* mesh);

	std::vector<vertex>& GetVertices();
	
	inline std::vector<vertex>& GetLocalVertices() { return m_LocalVertices; }
	inline std::vector<Mesh*>& GetSubMeshes() { return m_SubMeshes; }
	Mesh* GetSubMesh(int index);

	inline Vec2 Velocity() { return m_Velocity; }
	vertex GetCentroide();

	// getters
	inline Transform2D& GetTransform() { return m_Transform; }
	inline vertex& GetLocalCentroide() { return m_LocalCentroide; }
	inline bool& IsActive() { return m_IsActive; }
	inline bool& GetFill() { return m_Fill; }
	inline bool& IsClosed() { return m_Closed; }
	inline bool& GetGravity() { return m_Gravity; }
	inline ImVec4& GetColor() { return m_Color; }
	inline std::string& GetName() { return m_Name; }

	// setters
	inline void MarkDirty() { m_IsDirty = true; }
	inline void SetActive(bool active) { m_IsActive = active; }
	inline void SetFill(bool fill) { m_Fill = fill; }
	inline void SetClosed(bool closed) { m_Closed = closed; }
	inline void SetGravity(bool gravity) { m_Gravity = gravity; }
	inline void SetVelocity(Vec2 velocity) { m_Velocity = velocity; }
	inline void SetColor(ImVec4 color) { m_Color = color; }
	inline void SetName(std::string name) { m_Name = name; }

public:
	Mesh* m_Parent = nullptr;

private:
	void CalcLocalCentroide();
	void UpdateWorldVertices();

private:
	// principais atributos
	std::string m_Name = "New Mesh";

	Transform2D m_Transform;
	bool m_IsDirty = true;

	std::vector<vertex> m_Vertices = {}; // Vértices Globais (usa como cache)
	std::vector<vertex> m_LocalVertices = {}; // Vértices Locais

	std::vector<Mesh*> m_SubMeshes = {};
	Vec2 m_Velocity;
	vertex m_LocalCentroide;
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