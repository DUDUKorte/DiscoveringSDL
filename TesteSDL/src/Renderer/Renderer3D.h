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

class Renderer3D {

public:
	static void Init(std::vector<uint32_t>* buffer, std::vector<float>* zbuffer, WindowProperties* window, std::vector<MeshLight*>* lights);

	static void UpdateResize();

	static void DrawLine(glm::vec3 source, glm::vec3 target);

	static void Draw3DLinesWithVertices(const std::vector<glm::vec3>& vertices);

	static void DrawMesh(Mesh& meshTeste);

	static void FindIntersections(const std::vector<glm::vec3>& vertices, glm::vec2 yMinMax);

	static void FillPoly(const std::vector<glm::vec3>& vertices, glm::vec2 minMaxY);

	static std::vector<uint32_t>* Buffer;
	static std::vector<float>* ZBuffer;
	static std::vector<MeshLight*>* SceneLights;
	static WindowProperties* MainWindow;
	static uint32_t DrawColor;
	static std::vector<std::vector<std::pair<float, float>>> intersecoes;
	static glm::vec3 AmbientLight;
};
