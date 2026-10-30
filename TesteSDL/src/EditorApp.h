#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <imgui.h>

#include "BaseMesh.h"
#include "Mesh.h"
#include "Pipe.h"

#ifndef LeandroColor
#define LeandroColor(r, g, b, a) \
	( ((uint32_t) ((r) * 255.0f + 0.5f) << 24) | \
	((uint32_t) ((g) * 255.0f + 0.5f) << 16) | \
	((uint32_t) ((b) * 255.0f + 0.5f) << 8) | \
	((uint32_t) ((a) * 255.0f + 0.5f)))
#endif

enum EEditMode {Selection = 0, CreatePoly, MovePoly, ScalePoly, RotatePoly};
enum EGizmoState { GIZMO_NONE, GIZMO_HOVER_X, GIZMO_HOVER_Y, GIZMO_HOVER_CENTER, GIZMO_HOVER_ROT };

typedef struct WindowProperties {
	int Width = 1280;
	int Height = 720;
	SDL_Window* Window = nullptr;
	SDL_Renderer* Renderer = nullptr;
} WindowProperties;

class EditorApp {
public:
	EditorApp();
	~EditorApp();

	bool Init();
	SDL_AppResult ProcessEvent(SDL_Event* event);
	SDL_AppResult UpdateAndRender();
	void Shutdown();

	inline int GetWidth() { return m_WindowProperties.Width; }
	inline int GetHeight() { return m_WindowProperties.Height; }

	// VARIÁVEL GLOBAL APENAS PARA TESTES
	Mesh* cubinho = new Mesh("cubinho");
	Mesh* macaquinho = new Mesh("Monkey");

private:
	void RenderUI();
	void ProcessMouseMotion(SDL_Event* event);
	BaseMesh* GetActiveMesh();
	void UpdateGizmoHover(float mouseX, float mouseY, BaseMesh* activeMesh, EEditMode);
	void OnChangeMode(EEditMode new_mode);
	void OnDestroySelectedMesh();

	void LoadObjFile(std::string path, Mesh* mesh);

	// flappy pipe
	void EventBeginPlay();
	void CreatePipes();
	void CreatePlayer();


	std::vector<std::string> split(const std::string& str, char delim);

private:
	// Estado do SDL e ImGUi
	//SDL_Window* m_Window = nullptr;
	//SDL_Renderer* m_Renderer = nullptr;

	WindowProperties m_WindowProperties;
	
	SDL_Texture* m_Buffer = nullptr;
	std::vector<uint32_t>* m_VertexBuffer = new std::vector<uint32_t>(m_WindowProperties.Width * m_WindowProperties.Height);


	// Estado do editor
	EEditMode m_CurrentMode = Selection;
	EGizmoState m_CurrentGizmoState = GIZMO_NONE;
	EGizmoState m_ActiveGizmoState = GIZMO_NONE;

	float m_MoveMeshSpeed = 0.1f;
	float m_ScaleMeshSpeed = 0.01f;
	float m_RotateMeshSpeed = 0.01f;

	ImVec4 m_ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool m_bLMB = false;
	bool m_bRMB = false;
	bool m_GameStarted = false;

	// Delta time
	Uint64 new_deltaTime;
	Uint64 old_deltaTime = 0;
	double deltaTime = 0;
	double timeAcc = 0;

	// Meshes
	std::vector<BaseMesh*> m_GlobalMeshes;
	int m_CurrentMesh = -1;
	int m_CurrentSubMesh = -1;
	int m_CustomPolyFaces = 4;
	int m_CustomPolySize = 10;
};