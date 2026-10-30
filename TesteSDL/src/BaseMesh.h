#pragma once
#include <vector>
#include <imgui.h>
#include <string>
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum MeshTypes { Type3D = 0, Type2D = 1};

struct Transform {
	glm::vec3 Position = glm::vec3(0.0f);
	glm::vec3 Rotation = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);
};

class BaseMesh {
public:
	virtual ~BaseMesh();

	virtual void EventTick(float deltaTime) = 0;
	virtual void EventInput(SDL_Event* event) = 0;

	virtual void addVertex(glm::vec3 newVertex) = 0;
	virtual void removeVertex();

	virtual void addSubMesh(BaseMesh* mesh);
	virtual void removeSubMesh(int index);

	virtual void ResetTransform(int x_offset = 0, int y_offset = 0, int z_offset = 0);
	virtual void RestoreTransform();

	virtual std::vector<glm::vec3>& GetVertices();

	inline std::vector<glm::vec3>& GetLocalVertices() { return m_LocalVertices; }
	inline std::vector<BaseMesh*>& GetSubMeshes() { return m_SubMeshes; }
	virtual BaseMesh* GetSubMesh(int index) = 0;

	inline glm::vec3 Velocity() { return m_Velocity; }
	glm::vec3 GetCentroide();
	Transform& GetTransform();
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
	inline MeshTypes GetType() { return meshType; }

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
	BaseMesh* m_Parent = nullptr;

protected:
	virtual void CalcLocalCentroide();
	virtual void UpdateWorldVertices();

protected:
	// principais atributos
	std::string m_Name = "New Mesh";

	// transforms
	Transform m_Transform;
	Transform m_CacheTransform;
	glm::mat4 m_TransformMatrix = glm::mat4(1.0f);

	//Transform2D m_Transform;
	//Transform2D m_CacheTransform;
	bool m_IsDirty = true;

	std::vector<glm::vec3> m_Vertices = {}; // Vértices Globais (usa como cache)
	std::vector<glm::vec3> m_LocalVertices = {}; // Vértices Locais

	std::vector<BaseMesh*> m_SubMeshes = {};
	glm::vec3 m_Velocity = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_LocalCentroide;

	// do centroide
	glm::vec3 m_CentroideSum;

	// atributos de aparencia
	bool m_IsActive = false; // se está selecionado ou nn
	bool m_Fill = false; // se preenche o meio dele
	bool m_Closed = false; // se o último vértice é conectado cm o primeiro
	bool m_Gravity = false; // se é afetado pela gravidade ou nn
	bool m_Outline = true;
	bool m_TickEnabled = false;
	bool m_ShouldDestroy = false; // Define se deveria ser detruido no próximo frame

	MeshTypes meshType;

	//Cores
	ImVec4 m_Color = { 0, 1, 0 , 1 };
	ImVec4 m_OutlineColor = { 1, 1 ,1 , 1 };
};