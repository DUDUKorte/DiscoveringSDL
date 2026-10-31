#pragma once
#include "EditorCore/BaseMesh.h"

struct face {
	std::vector<int> vertices;
	int normal;
};

enum LightAlgorithm { Constant = 0, Phong = 1, Gouraud };

class Mesh : public BaseMesh {
public:
	Mesh(std::string name, glm::vec3 firstVertex = { 0.0f,0.0f,0.0f }, std::vector<glm::vec3> v = {});
	virtual ~Mesh();

	virtual void BeginPlay() {};
	virtual void EventTick(float deltaTime) {};
	virtual void EventInput(SDL_Event* event) {};


	virtual void addVertex(glm::vec3 newVertex);
	
	void addFace(std::vector<int> vertexIndices, int normalIndice);
	void addNormal(glm::vec3 newNormal);

	inline std::vector<face>& GetFaces() { return m_Faces; }
	inline std::vector<glm::vec3>& GetNormals() { return m_Normals; }
	inline bool& GetAfectedByLights() { return m_AfectedByLights; }
	inline bool& GetUseZBuffer() { return m_UseZBuffer; }
	inline bool& GetUseBackFaceCulling() { return m_UseBackFaceCulling; }
	inline LightAlgorithm& GetLightAlgorithm() { return m_LightAlgorithm; }
	inline bool& GetUsePerspective() { return m_UsePerspective; }

private:
	std::vector<face> m_Faces = {}; // Faces da mesh
	std::vector<glm::vec3> m_Normals = {}; // os vetores normais das faces

protected:
	// Atributos personalizáveis
	bool m_AfectedByLights = true;
	LightAlgorithm m_LightAlgorithm = LightAlgorithm::Constant;
	bool m_UseZBuffer = true;
	bool m_UseBackFaceCulling = true;
	bool m_UsePerspective = false;
};
