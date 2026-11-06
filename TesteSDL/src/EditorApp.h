#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <imgui.h>
#include <ImGuizmo.h>

#include "EditorCore/BaseMesh.h"
#include "EditorCore/Mesh.h"
#include "EditorCore/MeshLight.h"
#include "EditorCore/Pipe.h"
#include "EditorCore/ScriptEngine.h"
#include "Physics/CollisionEngine.h"
#include "Utils/LeandroMath.h"

#include "imgui_impl_sdl3.h"

#include "imgui_impl_sdlgpu3.h"
#include "imgui_impl_sdlrenderer3.h"

#ifndef LeandroColor
#define LeandroColor(r, g, b, a) \
	( ((uint32_t) ((a) * 255.0f + 0.5f) << 24) | \
	((uint32_t) ((b) * 255.0f + 0.5f) << 16) | \
	((uint32_t) ((g) * 255.0f + 0.5f) << 8) | \
	((uint32_t) ((r) * 255.0f + 0.5f)))
#endif

enum EEditMode { Selection = 0, CreatePoly, MovePoly, ScalePoly, RotatePoly, Navigation };
enum EGizmoState { GIZMO_NONE, GIZMO_HOVER_X, GIZMO_HOVER_Y, GIZMO_HOVER_CENTER, GIZMO_HOVER_ROT };

typedef struct WindowProperties {
	int Width = 1280;
	int Height = 720;
	SDL_Window* Window = nullptr;
	SDL_Renderer* Renderer = nullptr;
	SDL_GPUDevice* GPUDevice = nullptr;
	glm::vec3 CameraPos = { 0.0f, 0.0f, 0.0f };
	glm::vec2 CameraRot = { 0.0f, 0.0f };
} WindowProperties;

class EditorApp {
public:
	EditorApp();
	~EditorApp();

	SDL_AppResult Init();
	SDL_AppResult ProcessEvent(SDL_Event* event);
	SDL_AppResult UpdateAndRender();
	void Shutdown();

	inline int GetWidth() { return m_WindowProperties.Width; }
	inline int GetHeight() { return m_WindowProperties.Height; }
	inline bool GetGameStarted() { return m_GameStarted; }
	inline WindowProperties& GetWindowProperties() { return m_WindowProperties; }
	
	inline static glm::vec3& GetBGColor() { return m_BgColor; }
	inline static EditorApp* Get() { return m_Instance; }
	inline static double GetDeltaTime() { return deltaTime; }

	void LoadObjFile(std::string path, Mesh* mesh);
	void SubmitMesh(BaseMesh* newMesh);
	void PlaySound(const char* filepath);
	void SetHideCursor(bool hide);

	bool GetIsKeyPressed(SDL_Scancode ScanCode);

	// VARIÁVEL GLOBAL APENAS PARA TESTES
	//Mesh* cubinho = new Mesh("cubinho");
	//Mesh* macaquinho = new Mesh("Monkey");
	//MeshLight* sun = new MeshLight("sun");

	static EditorApp* m_Instance;
	static glm::vec3 m_BgColor;

private:
	void RenderUI();
	void SetupCustomEditorStyle(float main_scale);
	void ProcessMouseMotion(SDL_Event* event);
	BaseMesh* GetActiveMesh();
	void UpdateGizmoHover(float mouseX, float mouseY, BaseMesh* activeMesh, EEditMode);
	void OnChangeMode(EEditMode new_mode);
	void OnDestroySelectedMesh();

	std::string OpenObjFileDialog(SDL_Window* window);
	std::string OpenLuaFileDialog(SDL_Window* window);

	// Game related 
	void EventBeginPlay();

	std::vector<std::string> split(const std::string& str, char delim);

private:
	// Estado do SDL e ImGUi
	//SDL_Window* m_Window = nullptr;
	//SDL_Renderer* m_Renderer = nullptr;

	WindowProperties m_WindowProperties;
	
	SDL_GPUTransferBuffer* m_TransferBuffer = nullptr;
	SDL_GPUTexture* m_Buffer = nullptr;
	std::vector<uint32_t>* m_FrameBuffer = new std::vector<uint32_t>(m_WindowProperties.Width * m_WindowProperties.Height);
	std::vector<float>* m_ZBuffer = new std::vector<float>(m_WindowProperties.Width * m_WindowProperties.Height);

	// Estado do editor
	EEditMode m_CurrentMode = Selection;
	EEditMode m_LastMode = Selection;
	EGizmoState m_CurrentGizmoState = GIZMO_NONE;
	EGizmoState m_ActiveGizmoState = GIZMO_NONE;

	float m_MoveMeshSpeed = 0.1f;
	float m_ScaleMeshSpeed = 0.01f;
	float m_RotateMeshSpeed = 0.01f;

	ImVec4 m_ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	float m_CameraSpeed = 4.0f;
	bool m_bLMB = false;
	bool m_bRMB = false;
	bool m_GameStarted = false;
	bool m_GameSimulate = false;

	// Delta time
	Uint64 new_deltaTime;
	Uint64 old_deltaTime = 0;
	static double deltaTime;
	double timeAcc = 0;

	// Meshes
	std::vector<BaseMesh*> m_GlobalMeshes;
	std::vector<MeshLight*> m_GlobalMeshLights;
	int m_CurrentMesh = -1;
	int m_CurrentSubMesh = -1;
	int m_CustomPolyFaces = 4;
	int m_CustomPolySize = 10;
	bool m_AltDuplicated = false;

	// Audio
	SDL_AudioStream* m_MusicStream = nullptr;
	Uint8* m_WavBuffer = nullptr;
	Uint32 m_WavLength = 0;
};