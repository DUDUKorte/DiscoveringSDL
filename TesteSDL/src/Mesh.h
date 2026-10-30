#pragma once
#include <vector>
#include <imgui.h>
#include <string>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct vertex3D {
	float x;
	float y;
	float z = 0.0f;
};

struct face {
	std::vector<int> vertices;
};

struct Transform3D {
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
};

class Mesh {
public:
	Mesh(std::string name, vertex3D firstVertex = { 0,0 }, std::vector<vertex3D> v = {});
	virtual ~Mesh();

	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);

	void addVertex(vertex3D worldPosition);
	void removeVertex();

	void addFace(std::vector<int> faceIndices);

	void addSubMesh(Mesh* mesh);
	void removeSubMesh(int index);

	void ResetTransform(int x_offset = 0, int y_offset = 0);
	void RestoreTransform();

	std::vector<vertex3D>& GetVertices();

	inline std::vector<vertex3D>& GetLocalVertices() { return m_LocalVertices; }
	inline std::vector<Mesh*>& GetSubMeshes() { return m_SubMeshes; }
	inline std::vector<face>& GetFaces() { return m_Faces; }
	Mesh* GetSubMesh(int index);

	inline glm::vec3 Velocity() { return m_Velocity; }
	vertex3D GetCentroide();
	Transform3D& GetTransform();
	inline glm::mat4& GetTransformMatrix() { return m_TransformMatrix; }

	// getters
	inline glm::vec3& GetLocalCentroide() { return m_LocalCentroide; }
	inline bool& IsActive() { return m_IsActive; }
	inline bool& GetFill() { return m_Fill; }
	inline bool& GetOutline() { return m_Outline; }
	inline bool& IsClosed() { return m_Closed; }
	inline bool& GetGravity() { return m_Gravity; }
	inline ImVec4& GetColor() { return m_Color; }
	inline ImVec4& GetOutlineColor() { return m_OutlineColor; }
	inline std::string& GetName() { return m_Name; }
	inline bool& GetTickEnabled() { return m_TickEnabled; }
	inline glm::vec3& GetVelocity() { return m_Velocity; }
	inline bool& GetShouldDestroy() { return m_ShouldDestroy; }

	// setters
	inline void MarkDirty() { m_IsDirty = true; }
	inline void SetActive(bool active) { m_IsActive = active; }
	inline void SetFill(bool fill) { m_Fill = fill; }
	inline void SetClosed(bool closed) { m_Closed = closed; }
	inline void SetGravity(bool gravity) { m_Gravity = gravity; }
	inline void SetVelocity(glm::vec3 velocity) { m_Velocity = velocity; }
	inline void SetColor(ImVec4 color) { m_Color = color; }
	inline void SetName(std::string name) { m_Name = name; }
	inline void SetTickEnabled(bool enabled) { m_TickEnabled = enabled; }
	inline void SetShouldDestroy(bool should) { m_ShouldDestroy = should; }

public:
	Mesh* m_Parent = nullptr;

private:
	void CalcLocalCentroide();
	void UpdateWorldVertices();

private:
	// principais atributos
	std::string m_Name = "New Mesh";

	// 3D
	Transform3D m_Transform;
	Transform3D m_CacheTransform;
	glm::mat4 m_TransformMatrix = glm::mat4(1.0f);

	//Transform2D m_Transform;
	//Transform2D m_CacheTransform;
	bool m_IsDirty = true;

	std::vector<vertex3D> m_Vertices = {}; // Vértices Globais (usa como cache)
	std::vector<vertex3D> m_LocalVertices = {}; // Vértices Locais

	std::vector<face> m_Faces = {}; // Faces da mesh

	std::vector<Mesh*> m_SubMeshes = {};
	glm::vec3 m_Velocity = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_LocalCentroide;

	// do centroide
	float m_CentroideXSum = 0;
	float m_CentroideYSum = 0;
	float m_CentroideZSum = 0;

	// atributos de aparencia
	bool m_IsActive = false; // se está selecionado ou nn
	bool m_Fill = false; // se preenche o meio dele
	bool m_Closed = false; // se o último vértice é conectado cm o primeiro
	bool m_Gravity = false; // se é afetado pela gravidade ou nn
	bool m_Outline = true;
	bool m_TickEnabled = false;
	bool m_ShouldDestroy = false; // Define se deveria ser detruido no próximo frame

	//Cores
	ImVec4 m_Color = { 0, 1, 0 , 1 };
	ImVec4 m_OutlineColor = { 1, 1 ,1 , 1 };
};
