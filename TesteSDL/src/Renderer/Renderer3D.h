#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include "EditorCore/Mesh.h"
#include "EditorApp.h"
#include "Utils/Log.h"

#ifndef LeandroColor
#define LeandroColor(r, g, b, a) \
	( ((uint32_t) ((r) * 255.0f + 0.5f) << 24) | \
	((uint32_t) ((g) * 255.0f + 0.5f) << 16) | \
	((uint32_t) ((b) * 255.0f + 0.5f) << 8) | \
	((uint32_t) ((a) * 255.0f + 0.5f)))
#endif

// @Mattsu TODO: Depois de fazer a triangulação das meshes tenta implementar um FastPhong (precisa ser com mesh triangulada)
// Talvez seja melhor tentar fazer o fastphong depois de tu refatorar/reescrever o fillpoly, já q ele vai mudar p krl...
enum LightAlgorithms { Constant = 0, Gouraud = 1, Phong };

class Renderer3D {

public:
	static void Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights);

	static void UpdateResize();

	static void DrawLine(glm::vec3 source, glm::vec3 target);

	static void Draw3DLinesWithVertices(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices);

	static void DrawMesh(Mesh& meshTeste);

	static glm::vec3 CalculateConstantLightning(glm::vec3 normal, glm::vec3 centroide, MeshMaterial& material);

	static void FindIntersections(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 yMinMax);

	// @Mattsu TODO: Precisamos de um FillPoly pica, novo, primeiro tem que fazer a triangularização das meshes importadas
	// aí da pra fazer o fill poly determinístico, basicamente não precisa ordenar lista, nem rodar um y que vá pra y+1
	// resumindo: sendo tudo triangulo da pra pegar qualquer ponto e descobrir a interpolação, o x1 e x2, etc...
	// O lance é que não vamos usar a CPU, até poderia (talvez usar o OpenMP pra multithread)
	// Mas a ideia é usar compute shading, cada núcleo da GPU cuida de 1 linha do fillpoly em simultâneo
	// então vai ficar rápido p krll, como toda GPU, até as mais básicas tem mais de 720 núcleos pra rodar isso
	// vai ser bem rápido, o tempo de 1 calcular termina o fill inteiro dql poly
	static void FillPoly(const std::vector<std::pair<glm::vec3, glm::vec3>>& vertices, glm::vec2 minMaxY, bool interpolate = false, Mesh* mesh = nullptr);

	static std::vector<uint32_t>* Buffer;
	static std::vector<float>* ZBuffer;
	static std::vector<MeshLight*>* SceneLights;
	static WindowProperties* MainWindow;
	static uint32_t DrawColor;
	static std::vector<std::vector<std::pair<glm::vec3, glm::vec3>>> intersecoes;
	static glm::vec3 AmbientLight;
	static LightAlgorithms LightAlgorithm;
};
