#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <imgui.h>

#include "Mesh.h"

#define WIDTH 1280
#define HEIGHT 720

enum EEditMode {Selection = 0, CreatePoly, MovePoly, ScalePoly, RotatePoly};
enum EGizmoState { GIZMO_NONE, GIZMO_HOVER_X, GIZMO_HOVER_Y, GIZMO_HOVER_CENTER, GIZMO_HOVER_ROT };

class EditorApp {
public:
	EditorApp();
	~EditorApp();

	bool Init();
	SDL_AppResult ProcessEvent(SDL_Event* event);
	SDL_AppResult UpdateAndRender();
	void Shutdown();

	inline int GetWidth() { return m_Width; }
	inline int GetHeight() { return m_Height; }

private:
	void RenderUI();
	void ProcessMouseMotion(SDL_Event* event);
	Mesh* GetActiveMesh();
	void UpdateGizmoHover(float mouseX, float mouseY, Mesh* activeMesh, EEditMode);
	void OnChangeMode(EEditMode new_mode);

private:
	// Estado do SDL e ImGUi
	SDL_Window* m_Window = nullptr;
	SDL_Renderer* m_Renderer = nullptr;
	int m_Width = 1280;
	int m_Height = 720;


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

	// Meshes
	std::vector<Mesh*> m_GlobalMeshes;
	int m_CurrentMesh = -1;
	int m_CurrentSubMesh = -1;

	int m_CustomPolyFaces = 4;
};