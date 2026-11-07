#include "EditorApp.h"
#include "Renderer/Renderer3D.h"
#include "Utils/Log.h"
#include <Physics/PhysicsEngine.h>

#include <iostream>
#include <string>
#include <numbers>
#include <random>
#include <algorithm>
#include <complex>
#include <fstream>

#include "ImGuizmo.h"
#include "imgui_impl_sdl3.h"

// Pra usar o bglh de carregar o .obj
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>
#endif

#include <filesystem>

EditorApp* EditorApp::m_Instance = nullptr;
glm::vec3 EditorApp::m_BgColor = {0.2f, 0.2f, 0.2f};
double EditorApp::deltaTime = 0.0f;

EditorApp::EditorApp()
{
	m_Instance = this;
}

EditorApp::~EditorApp()
{
	for (auto m : m_GlobalMeshes) {
		delete m;
	}
	m_GlobalMeshes.clear();
}

///////////////////////////////////////////////////////////
//////////INITIALIZE SDL IMGUI AND EDITOR//////////////////
///////////////////////////////////////////////////////////
SDL_AppResult EditorApp::Init()
{
	/*IMGUI FUNCIONA PFV*/
	LE_CORE_TRACE("Initializing SDL...");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// Create window with SDL_Renderer graphics context
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	LE_CORE_TRACE("Creating SDL Window...");
	m_WindowProperties.Window = SDL_CreateWindow("LeandroEditor 2D", static_cast<int>(m_WindowProperties.Width) * main_scale, static_cast<int>(m_WindowProperties.Height) * main_scale, window_flags);
	if (m_WindowProperties.Window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	LE_CORE_TRACE("Creating SDL Renderer...");
	m_WindowProperties.GPUDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, false, "vulkan");
	if (!m_WindowProperties.GPUDevice) {
		SDL_Log("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	
	if (!SDL_ClaimWindowForGPUDevice(m_WindowProperties.GPUDevice, m_WindowProperties.Window)) {
		SDL_Log("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetGPUSwapchainParameters(m_WindowProperties.GPUDevice, m_WindowProperties.Window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_IMMEDIATE);

	SDL_SetWindowPosition(m_WindowProperties.Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(m_WindowProperties.Window);

	LE_CORE_TRACE("Setting up Dear ImGui context...");
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;


	//style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	//style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

	// Setup Dear ImGui style
	SetupCustomEditorStyle(main_scale);
	io.Fonts->AddFontFromFileTTF("./Assets/fonts/RobotoMono-Bold.ttf", 16.0f);

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLGPU(m_WindowProperties.Window);
	ImGui_ImplSDLGPU3_InitInfo init_info = {};
	init_info.Device = m_WindowProperties.GPUDevice;
	init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(m_WindowProperties.GPUDevice, m_WindowProperties.Window);
	init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
	init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
	init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
	ImGui_ImplSDLGPU3_Init(&init_info);

	ImGuizmo::Style& gizmoStyle = ImGuizmo::GetStyle();

	//// 1. Espessuras e tamanhos (em pixels)
	gizmoStyle.TranslationLineThickness = 4.0f; // Grossura das linhas de mover
	gizmoStyle.TranslationLineArrowSize = 7.0f; // Tamanho da ponta da seta
	gizmoStyle.RotationLineThickness = 3.0f; // Grossura do anel de rotação
	gizmoStyle.RotationOuterLineThickness = 4.0f; // Grossura do anel externo de rotação
	gizmoStyle.ScaleLineThickness = 4.0f; // Grossura das linhas de escala
	gizmoStyle.ScaleLineCircleSize = 7.0f; // Tamanho do quadradinho/bolinha na ponta da escala
	gizmoStyle.CenterCircleSize = 6.0f; // Tamanho do círculo central

	// Setup Platform/Renderer backends
	//ImGui_ImplSDL3_InitForSDLRenderer(m_WindowProperties.Window, m_WindowProperties.Renderer);
	//ImGui_ImplSDLRenderer3_Init(m_WindowProperties.Renderer);

	//Update Width/Height values
	LE_CORE_TRACE("Updating Window size...");
	SDL_GetWindowSizeInPixels(m_WindowProperties.Window, &m_WindowProperties.Width, &m_WindowProperties.Height);
	LE_CORE_TRACE("WIDTH: " + std::to_string(m_WindowProperties.Width) + " | HEIGHT: " + std::to_string(m_WindowProperties.Height));

	m_FrameBuffer = new std::vector<uint32_t>(m_WindowProperties.Width * m_WindowProperties.Height);
	m_ZBuffer = new std::vector<float>(m_WindowProperties.Height * m_WindowProperties.Width);


	// Inicializa o buffer
	SDL_GPUTextureCreateInfo textInfo = {};
	textInfo.height = m_WindowProperties.Height;
	textInfo.width = m_WindowProperties.Width;
	textInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	textInfo.type = SDL_GPU_TEXTURETYPE_2D;
	textInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	textInfo.layer_count_or_depth = 1;
	textInfo.num_levels = 1;
	m_Buffer = SDL_CreateGPUTexture(m_WindowProperties.GPUDevice, &textInfo);
	//m_Buffer = SDL_CreateTexture(m_WindowProperties.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, m_WindowProperties.Width, m_WindowProperties.Height);
	
	SDL_GPUTransferBufferCreateInfo transferInfo = {};
	transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transferInfo.size = static_cast<size_t>(m_WindowProperties.Width) * m_WindowProperties.Height * sizeof(uint32_t);
	m_TransferBuffer = SDL_CreateGPUTransferBuffer(m_WindowProperties.GPUDevice, &transferInfo);

	// Seta as variáveis do renderer
	Renderer3D::Init(m_FrameBuffer, m_ZBuffer, &m_WindowProperties, &m_GlobalMeshLights);
	if (Renderer3D::RendererMode == RendererModes::Hardware) {
		Renderer3D::InitGPU(&m_WindowProperties);
	}

	m_BgColor = {0.2f, 0.2f, 0.2f};

	// Initialize scripting
	ScriptEngine::Init();

	return SDL_APP_CONTINUE;
}
///////////////////////////////////////////////////////////
//////////INITIALIZE SDL IMGUI AND EDITOR//////////////////
///////////////////////////////////////////////////////////


void EditorApp::SetupCustomEditorStyle(float main_scale) {
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();

	// 1. Paddings e Espaçamentos (10 e 5)
	style.WindowPadding = ImVec2(10.0f, 10.0f);
	style.FramePadding = ImVec2(10.0f, 5.0f);
	style.ItemSpacing = ImVec2(10.0f, 5.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
	style.IndentSpacing = 10.0f;
	style.GrabMinSize = 14.0f;
	style.ScrollbarSize = 10.0f;

	// 2. Bordas e Arredondamentos (4 e 2)
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 2.0f;
	style.PopupBorderSize = 0.0f;
	style.FrameBorderSize = 1.0f;
	style.TabBorderSize = 0.0f;

	style.WindowRounding = 5.0f;
	style.ChildRounding = 4.0f;
	style.FrameRounding = 2.0f;
	style.PopupRounding = 2.0f;
	style.ScrollbarRounding = 2.0f;
	style.GrabRounding = 2.0f;
	style.TabRounding = 2.0f;

	style.ScrollbarSize = 10.0f;
	style.ScrollbarPadding = 4.0f;

	style.TabBarBorderSize = 0.0f;
	style.TabBarOverlineSize = 0.0f;

	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowBorderHoverPadding = 7.0f;
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SeparatorSize = 5.0f;
	style.SeparatorTextBorderSize = 5.0f;
	style.SeparatorTextAlign = ImVec2(0.5f, 0.5f);
	style.DockingSeparatorSize = 0.0f;

	// 3. Paleta de Cores extraída direto da sua print
	ImVec4* colors = style.Colors;

	// Fundos principais (Cinza-ardósia escuro: ~34, 42, 45)
	colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.16f, 0.18f, 0.6f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.16f, 0.18f, 0.7f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.13f, 0.15f, 0.6f);

	// Bordas e Separadores (~80, 82, 87)
	colors[ImGuiCol_Border] = ImVec4(0.31f, 0.32f, 0.34f, 0.65f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.31f, 0.32f, 0.34f, 0.65f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.24f, 0.41f, 0.59f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.24f, 0.41f, 0.59f, 1.00f);

	// Inputs, DragFloats e Checkboxes (Quase preto azulado: ~17, 22, 26)
	colors[ImGuiCol_FrameBg] = ImVec4(0.07f, 0.09f, 0.10f, 0.8f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.15f, 0.18f, 0.9f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.21f, 0.25f, 0.9f);

	// Barra de Título das Janelas / Dock (~22, 39, 60)
	colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.15f, 0.24f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.22f, 0.35f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.15f, 0.24f, 0.75f);

	// Botões, Headers ("Transform") e Seleções (Azul ~60, 105, 150)
	colors[ImGuiCol_Button] = ImVec4(0.24f, 0.41f, 0.59f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.29f, 0.51f, 0.75f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.35f, 0.53f, 1.00f);

	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.34f, 0.49f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.45f, 0.65f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.29f, 0.51f, 0.75f, 1.00f);

	// Abas do Docking ("Control pannel", "Inspector")
	colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.24f, 0.36f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.29f, 0.51f, 0.75f, 1.00f);
	colors[ImGuiCol_TabSelected] = ImVec4(0.25f, 0.46f, 0.70f, 1.00f);
	colors[ImGuiCol_TabDimmed] = ImVec4(0.09f, 0.15f, 0.24f, 1.00f);
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.32f, 0.48f, 1.00f);

	// Checkmark, Sliders e Destaques (Azul vivo ~58, 128, 224)
	colors[ImGuiCol_CheckMark] = ImVec4(0.23f, 0.50f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.45f, 0.68f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.29f, 0.55f, 0.85f, 1.00f);
	colors[ImGuiCol_DockingPreview] = ImVec4(0.24f, 0.45f, 0.68f, 0.70f);

	// Ajuste para Multi-Viewports (mantém cantos retos quando a janela sai do app)
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 0.6f;
	}

	// Aplica o DPI scale por cima dos tamanhos configurados
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;
}


///////////////////////////////////////////////////////////
//////////////////////SHUTDOWN////////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::Shutdown() const
{
	// [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
	ImGui_ImplSDLGPU3_Shutdown();
	//ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	if (m_MusicStream) {
		SDL_DestroyAudioStream(m_MusicStream);
	}
	if (m_WavBuffer) {
		SDL_free(m_WavBuffer);
	}
	if (m_Buffer) {
		SDL_ReleaseGPUTexture(m_WindowProperties.GPUDevice, m_Buffer);
	}
	if (m_TransferBuffer) {
		SDL_ReleaseGPUTransferBuffer(m_WindowProperties.GPUDevice, m_TransferBuffer);
	}

	//SDL_DestroyRenderer(m_WindowProperties.Renderer);
	SDL_ReleaseWindowFromGPUDevice(m_WindowProperties.GPUDevice, m_WindowProperties.Window);
	SDL_DestroyGPUDevice(m_WindowProperties.GPUDevice);
	SDL_Quit();
}
///////////////////////////////////////////////////////////
//////////////////////SHUTDOWN////////////////////////////
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
///////////////////GET ACTIVE MESH/////////////////////////
///////////////////////////////////////////////////////////
BaseMesh* EditorApp::GetActiveMesh() const
{
	if (m_GlobalMeshes.empty() || m_CurrentMesh < 0 || m_CurrentMesh >= m_GlobalMeshes.size()) {
		return nullptr;
	}
	LE_CORE_ASSERT(m_CurrentMesh <= m_GlobalMeshes.size() && m_CurrentMesh >= 0, "Tentando acessar Mesh INVALIDA!");
	BaseMesh* mainMesh = m_GlobalMeshes[m_CurrentMesh];

	if (m_CurrentSubMesh >= 0 && m_CurrentSubMesh < mainMesh->GetSubMeshes().size()) {
		return mainMesh->GetSubMesh(m_CurrentSubMesh);
	}
	return mainMesh;
}
///////////////////////////////////////////////////////////
///////////////////GET ACTIVE MESH/////////////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
/////////////////////PROCESS EVENTS////////////////////////
///////////////////////////////////////////////////////////
SDL_AppResult EditorApp::ProcessEvent(SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	// Todas as meshes agora devem receber um event
	for (BaseMesh* m : m_GlobalMeshes) {
		if (m->GetInputEnabled() && m_GameStarted) {
			m->EventInput(event);
		}
	}

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
		SDL_GetWindowSize(m_WindowProperties.Window, &m_WindowProperties.Width, &m_WindowProperties.Height);
		m_FrameBuffer->resize(m_WindowProperties.Width * m_WindowProperties.Height);
		m_ZBuffer->resize(m_WindowProperties.Width * m_WindowProperties.Height);
		//SDL_GPUTexture* markForDelete = m_Buffer;
		//m_Buffer = SDL_CreateTexture(m_WindowProperties.Renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, m_WindowProperties.Width, m_WindowProperties.Height);
		
		SDL_ReleaseGPUTexture(m_WindowProperties.GPUDevice, m_Buffer);
		SDL_ReleaseGPUTransferBuffer(m_WindowProperties.GPUDevice, m_TransferBuffer);
		
		// Inicializa o buffer
		SDL_GPUTextureCreateInfo textInfo = {};
		textInfo.height = m_WindowProperties.Height;
		textInfo.width = m_WindowProperties.Width;
		textInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		textInfo.type = SDL_GPU_TEXTURETYPE_2D;
		textInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		textInfo.layer_count_or_depth = 1;
		textInfo.num_levels = 1;
		m_Buffer = SDL_CreateGPUTexture(m_WindowProperties.GPUDevice, &textInfo);

		SDL_GPUTransferBufferCreateInfo transferInfo = {};
		transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		transferInfo.size = m_WindowProperties.Width * m_WindowProperties.Height * sizeof(uint32_t);
		m_TransferBuffer = SDL_CreateGPUTransferBuffer(m_WindowProperties.GPUDevice, &transferInfo);
		
		Renderer3D::UpdateResize();
		//SDL_DestroyGPUTexture(markForDelete);
	}

	// Se o comando for absorvido pela UI, ignora os outros comandos
	ImGuiIO& io = ImGui::GetIO();

	/// EVENTOS DE TECLA PRESSIONADA ///////////////////////////////////////////////////////
	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;

		// MODE SWITCH
		switch (e.key) {
		case SDLK_W: // Move polygons mode
			OnChangeMode(MovePoly);
			break;
		case SDLK_E: // Rotate polygons mode
			OnChangeMode(RotatePoly);
			break;
		case SDLK_R: // Scale polygons mode
			OnChangeMode(ScalePoly);
			break;
		case SDLK_Q: // Selection polygons mode
			OnChangeMode(Selection);
			break;
		case SDLK_DELETE:
			OnDestroySelectedMesh();
			break;
		case SDLK_ESCAPE:
			m_GameStarted = false;
			SetHideCursor(false);
			OnChangeMode(Selection);
			break;
		case SDLK_F8: {
			if (m_GameStarted && m_GameSimulate) {
				SetHideCursor(false);
			}
			break;
		}
		case SDLK_F7: {
			if (m_GameStarted && m_GameSimulate) {
				SetHideCursor(true);
			}
			break;
		}
		default:
			break;
		}
	}
	else if (event->type == SDL_EVENT_KEY_UP) {
		SDL_KeyboardEvent e = event->key;
		switch (e.key) {
		default:
			break;
		}
	}
	// EVENTOS DE MOUSE PRESSIONADO ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		// Se o comando for absorvido pela UI, ignora os outros comandos
		if (io.WantCaptureMouse) {
			return SDL_APP_CONTINUE;
		}

		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) { // LMB
			m_bLMB = true;
		}
		else if (e.button == 3) { // RMB
			if (!m_GameStarted) {
				m_bRMB = true;
				SetHideCursor(true);
				m_LastMode = m_CurrentMode;
				OnChangeMode(Navigation);
			}
		}
	}
	/// EVENTOS DE MOUSE "DESPRESSIONADO"? ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {	
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) {
			m_bLMB = false;

		}
		else if (e.button == 3) { // RMB
			if (!m_GameStarted) {
				m_bRMB = false;
				SetHideCursor(false);
				// reseta o mouse pro centro da tela
				SDL_WarpMouseInWindow(m_WindowProperties.Window, m_WindowProperties.Width / 2.0f, m_WindowProperties.Height / 2.0f);
				OnChangeMode(m_LastMode);
			}
		}
	}
	/// EVENTOS DO MOUSE SE MEXENDA ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_MOTION) {
		ProcessMouseMotion(event);
	}
	/// EVENTOS DO SCROLL DO MOUSE ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
		SDL_MouseWheelEvent e = event->wheel;
		if (m_bRMB) {
			m_CameraSpeed += 0.1f * e.y;
			m_CameraSpeed = glm::clamp(m_CameraSpeed, 0.1f, 10.0f);
		}
	}

	return SDL_APP_CONTINUE;
}
///////////////////////////////////////////////////////////
/////////////////////PROCESS EVENTS////////////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
//////////////////////MOUSE MOTION/////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::ProcessMouseMotion(const SDL_Event* event)
{
	SDL_MouseButtonEvent e = event->button;
	if (m_bRMB) {
		m_WindowProperties.CameraRot += glm::vec2({-event->motion.yrel * 0.1f, event->motion.xrel * 0.1f});
		m_WindowProperties.CameraRot.x = glm::clamp(m_WindowProperties.CameraRot.x, -90.0f, 90.0f);
	}

	// Aqui é quando tá segurando o botão esquerdo do mouse
	if (m_bLMB) {

	}
}
///////////////////////////////////////////////////////////
//////////////////////MOUSE MOTION/////////////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
///////////////////UPDATE AND RENDER///////////////////////
///////////////////////////////////////////////////////////
SDL_AppResult EditorApp::UpdateAndRender()
{
	//Calc delta Time
	old_deltaTime = new_deltaTime;
	new_deltaTime = SDL_GetPerformanceCounter();
	deltaTime = (double)((new_deltaTime - old_deltaTime) * 1000 / (double)SDL_GetPerformanceFrequency());

	// movimento de câmera (gambiarra, isso deve não existir no futuro)
	if (m_bRMB) {
		glm::vec3 addMovement = {0.0f, 0.0f, 0.0f};
		addMovement.x += m_CameraSpeed * (float)deltaTime * (GetIsKeyPressed(SDL_SCANCODE_D) - GetIsKeyPressed(SDL_SCANCODE_A));
		addMovement.z += m_CameraSpeed * (float)deltaTime * (GetIsKeyPressed(SDL_SCANCODE_W) - GetIsKeyPressed(SDL_SCANCODE_S));
		addMovement.y += m_CameraSpeed * (float)deltaTime * (GetIsKeyPressed(SDL_SCANCODE_Q) - GetIsKeyPressed(SDL_SCANCODE_E));

		glm::vec3 moveDir = { addMovement.x, 0.0f, addMovement.z };
		moveDir = glm::rotateX(moveDir, glm::radians(m_WindowProperties.CameraRot.x));
		moveDir = glm::rotateY(moveDir, glm::radians(m_WindowProperties.CameraRot.y));

		m_WindowProperties.CameraPos += moveDir;
		m_WindowProperties.CameraPos.y += addMovement.y;
	}

	/* IMGUI */
	if (SDL_GetWindowFlags(m_WindowProperties.Window) & SDL_WINDOW_MINIMIZED)
	{
		SDL_Delay(10);
		return SDL_APP_CONTINUE;
	}

	// Start the Dear ImGui frame
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSDLGPU3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	ImGuizmo::BeginFrame();

	// Desenhar o gizmo aqui
	BaseMesh* activeMesh = GetActiveMesh();
	if (activeMesh && activeMesh->GetType() != MeshTypes::Type2D && !(m_GameStarted || m_CurrentMode == EEditMode::Navigation || m_CurrentMode == EEditMode::Selection)) {
		ImGuizmo::SetOrthographic(!Renderer3D::UsePerspective);

		ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

		ImGuiViewport* mainViewport = ImGui::GetMainViewport();
		ImGuizmo::SetRect(mainViewport->Pos.x, mainViewport->Pos.y, mainViewport->Size.x, mainViewport->Size.y);

		// NÃO DEVE CALCUALR ISSO TODO FRAME (TEMPORÁRIO) TODO 
		glm::mat4 cameraTransform = glm::translate(glm::mat4(1.0f), m_WindowProperties.CameraPos);
		cameraTransform = glm::rotate(cameraTransform, glm::radians(m_WindowProperties.CameraRot.y), { 0.0f, 1.0f, 0.0f });
		cameraTransform = glm::rotate(cameraTransform, glm::radians(m_WindowProperties.CameraRot.x), { 1.0f, 0.0f, 0.0f });
		glm::mat4 viewMatrix = glm::inverse(cameraTransform);

		glm::mat4 projMatrix;
		if (Renderer3D::UsePerspective) {
			projMatrix = glm::perspectiveLH_ZO(glm::radians(60.0f), (float)m_WindowProperties.Width / m_WindowProperties.Height, 0.1f, 100000.0f);
			projMatrix[1][1] *= -1.0f;
		}
		else {
			projMatrix = glm::ortho(0.0f, (float)m_WindowProperties.Width, (float)m_WindowProperties.Height, 0.0f, 10000.0f, -10000.0f);
		}

		ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
		if (m_CurrentMode == EEditMode::RotatePoly) { op = ImGuizmo::ROTATE; }
		else if (m_CurrentMode == EEditMode::ScalePoly) { op = ImGuizmo::SCALE; }

		glm::mat4 modelMatrix;
		Transform& t = activeMesh->GetTransform();
		ImGuizmo::RecomposeMatrixFromComponents(
			glm::value_ptr(t.Position),
			glm::value_ptr(t.Rotation),
			glm::value_ptr(t.Scale),
			glm::value_ptr(modelMatrix));

		ImGuizmo::Manipulate(
			glm::value_ptr(viewMatrix),
			glm::value_ptr(projMatrix),
			op,
			ImGuizmo::WORLD,
			glm::value_ptr(modelMatrix));

		if (ImGuizmo::IsUsing()) {
			if (GetIsKeyPressed(SDL_SCANCODE_LALT) && !m_AltDuplicated) {
				m_AltDuplicated = true;
				if (activeMesh->GetType() == MeshTypes::Type3D) {
					Mesh* copyMesh = new Mesh(*(static_cast<Mesh*>(activeMesh)));
					SubmitMesh(copyMesh);
				}
				else if (activeMesh->GetType() == MeshTypes::TypeLight) {
					MeshLight* copyMesh = new MeshLight(*(static_cast<MeshLight*>(activeMesh)));
					SubmitMesh(copyMesh);
				}

				m_CurrentMesh = (int)m_GlobalMeshes.size() - 1;
				activeMesh = GetActiveMesh();

				t = activeMesh->GetTransform();
				ImGuizmo::RecomposeMatrixFromComponents(
					glm::value_ptr(t.Position),
					glm::value_ptr(t.Rotation),
					glm::value_ptr(t.Scale),
					glm::value_ptr(modelMatrix));

				ImGuizmo::Manipulate(
					glm::value_ptr(viewMatrix),
					glm::value_ptr(projMatrix),
					op,
					ImGuizmo::LOCAL,
					glm::value_ptr(modelMatrix));
			}
			ImGuizmo::DecomposeMatrixToComponents(
				glm::value_ptr(modelMatrix),
				glm::value_ptr(t.Position),
				glm::value_ptr(t.Rotation),
				glm::value_ptr(t.Scale));
			activeMesh->MarkDirty();
		}
		else {
			m_AltDuplicated = false;
		}

	}
	
	for (BaseMesh* m : m_GlobalMeshes) {
		// Run tick SEMPRE FICA PRIMEIRO (Calcula física/lógica -> cálcula desenho -> Mostra na tela)
		if (m_GameStarted && m->GetTickEnabled()) {
			m->EventTick((float)deltaTime);
		}
	}
	
	// Atualiza a collisionEngine
	PhysicsEngine::UpdateAndCheckCollision(m_GlobalMeshes);
	if (!m_GameStarted || (m_GameSimulate && m_GameStarted)) {
		PhysicsEngine::DrawDebugBoxes(m_GlobalMeshes, &m_WindowProperties, Renderer3D::UsePerspective);
	}
	
	// Roda a física
	if (m_GameStarted) {
		PhysicsEngine::ResolvePhysics(m_GlobalMeshes);
	}

	if (!m_GameStarted || m_GameSimulate) {
		RenderUI();
	}

	ImGui::Render();

	// isso aqui é bascimanete como se fosse o ImGui NewFrame, mas aseria tipo o Renderer3D::NewFrame()
	// mas sme fazer essa função NewFrame()
	SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_WindowProperties.GPUDevice);
	SDL_GPUTexture* swapChainTexture = nullptr;
	SDL_GPURenderPass* renderPass3D = nullptr; // tem que ser atribuido se for hardware renderer!!!
	SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, m_WindowProperties.Window, &swapChainTexture, nullptr, nullptr);

	// se tiver minimizada (ou só dar erro aklsdhjksdfjgh)
	if (swapChainTexture == nullptr) {
		SDL_SubmitGPUCommandBuffer(commandBuffer);
		return SDL_APP_CONTINUE;
	}

	// Preenche o buffer com a cor neutra do fundo
	if (Renderer3D::RendererMode == RendererModes::Software) {
		std::ranges::fill(*m_FrameBuffer, LeandroColor(m_BgColor.x, m_BgColor.y, m_BgColor.z, 1.0f)); // Limpa a tela na cor que queremos
		std::ranges::fill(*m_ZBuffer, 9999999.0f); // preenche o zbuffer
	}
	else if (Renderer3D::RendererMode == RendererModes::Hardware) {
		// limpa a tela com bgColor
		SDL_GPUColorTargetInfo colorTarget = {};
		colorTarget.texture = swapChainTexture;
		colorTarget.clear_color = { m_BgColor.x, m_BgColor.y, m_BgColor.z, 1.0f };
		colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTarget.store_op = SDL_GPU_STOREOP_STORE;

		// limpa o zbuffer
		SDL_GPUDepthStencilTargetInfo depthTarget = {};
		depthTarget.texture = Renderer3D::DepthTexture;
		depthTarget.clear_depth = 1.0f;
		depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
		depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

		// abre o renderPass, vincula o pipelline e aí desenha oq quiser (no caso, as meshes)
		renderPass3D = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, &depthTarget);
		SDL_BindGPUGraphicsPipeline(renderPass3D, Renderer3D::DefaultPipeline);
	}

	// Draw polygons
	for (BaseMesh* m : m_GlobalMeshes) {
		// Desnha 3D!!!!
		if (m->GetType() != MeshTypes::Type2D && !(m->GetHiddenInGame() && m_GameStarted && !m_GameSimulate)) {
			if (Renderer3D::RendererMode == RendererModes::Hardware) {
				Renderer3D::DrawMeshGPU(static_cast<Mesh&>(*(m)), commandBuffer, renderPass3D);
			}
			else if (Renderer3D::RendererMode == RendererModes::Software) {
				Renderer3D::DrawMesh(static_cast<Mesh&>(*(m)));
			}
			continue;
		}
	}
	
	// SEMPRE Fechar o render pass 3D aqui
	if (Renderer3D::RendererMode == RendererModes::Hardware) {
		SDL_EndGPURenderPass(renderPass3D);
	}

	// Software Rendering usando a SDLGPUtexture lá
	if (Renderer3D::RendererMode == RendererModes::Software) {
		// copia tudo de FrameBuffer pra vram da gpu
		void* mapeamento = SDL_MapGPUTransferBuffer(m_WindowProperties.GPUDevice, m_TransferBuffer, true);
		memcpy(mapeamento, m_FrameBuffer->data(), m_WindowProperties.Width * m_WindowProperties.Height * sizeof(uint32_t));
		SDL_UnmapGPUTransferBuffer(m_WindowProperties.GPUDevice, m_TransferBuffer);

		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
		SDL_GPUTextureTransferInfo transferInfo = {};
		transferInfo.transfer_buffer = m_TransferBuffer;
		transferInfo.offset = 0;

		SDL_GPUTextureRegion textRegion = {};
		textRegion.texture = m_Buffer;
		textRegion.w = m_WindowProperties.Width;
		textRegion.h = m_WindowProperties.Height;
		textRegion.d = 1;

		SDL_UploadToGPUTexture(copyPass, &transferInfo, &textRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		// copia a textura/pixels do m_Buffer da vram pra swapchain
		SDL_GPUBlitInfo blitInfo = {};
		blitInfo.source.texture = m_Buffer;
		blitInfo.source.w = m_WindowProperties.Width;
		blitInfo.source.h = m_WindowProperties.Height;
	
		blitInfo.destination.texture = swapChainTexture;
		blitInfo.destination.w = m_WindowProperties.Width;
		blitInfo.destination.h = m_WindowProperties.Height;

		blitInfo.load_op = SDL_GPU_LOADOP_DONT_CARE;

		SDL_BlitGPUTexture(commandBuffer, &blitInfo);
	}
	// último render pass pra desenhar a imgui por cima
	ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), commandBuffer);
	SDL_GPUColorTargetInfo colorTargInfo = {};
	colorTargInfo.texture = swapChainTexture;
	colorTargInfo.load_op = SDL_GPU_LOADOP_LOAD; // deixa esse como LAOD, se colcoar CLEAR ele vai apagar o fundo pra desenhar o ImGui
	colorTargInfo.store_op = SDL_GPU_STOREOP_STORE;

	// renderiza o imgui por cima de tudo
	SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargInfo, 1, nullptr);
	ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderPass);
	SDL_EndGPURenderPass(renderPass);

	// Manda a GPU executar tudo
	SDL_SubmitGPUCommandBuffer(commandBuffer);

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	return SDL_APP_CONTINUE;
}
///////////////////////////////////////////////////////////
///////////////////UPDATE AND RENDER///////////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
//////////////////////RENDER UI////////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::RenderUI()
{
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

	// Draw Control pannelk
	ImGui::Begin("Control pannel");
	ImGui::SeparatorText("Outliner");
	if (ImGui::BeginChild("Mesh Tree", ImVec2(0, 200), true)) {
		for (int i = 0; i < m_GlobalMeshes.size(); i++) {
			BaseMesh* m = m_GlobalMeshes[i];
			if (m->GetVertices().empty() && m->GetType()==MeshTypes::Type2D) { continue; }

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

			if (m_CurrentMesh == i && m_CurrentSubMesh == -1) {
				node_flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool nodeOpen = ImGui::TreeNodeEx(m, node_flags, "%s", m->GetName().c_str());

			if (ImGui::IsItemClicked()) {
				if (m_CurrentMode == CreatePoly) {
					OnChangeMode(MovePoly);
				}

				m_CurrentMesh = i;
				m_CurrentSubMesh = -1;
			}

			if (nodeOpen) {
				auto& subMeshes = m->GetSubMeshes();
				for (int j = 0; j < subMeshes.size(); j++) {
					BaseMesh* subMesh = subMeshes[j];

					ImGuiTreeNodeFlags sub_node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;

					if (m_CurrentMesh == i && m_CurrentSubMesh == j) {
						sub_node_flags |= ImGuiTreeNodeFlags_Selected;
					}

					ImGui::TreeNodeEx(subMesh, sub_node_flags, "%s", subMesh->GetName().c_str());
					if (ImGui::IsItemClicked()) {
						if (m_CurrentMode == CreatePoly) {
							OnChangeMode(MovePoly);
						}

						m_CurrentMesh = i;
						m_CurrentSubMesh = j;

					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	if (ImGui::Button("Create New Mesh")) {
		std::string meshName = "Mesh_" + std::to_string(m_GlobalMeshes.size());
		Mesh* newMesh = new Mesh(meshName);
		SubmitMesh(newMesh);
		m_CurrentMesh = (int)m_GlobalMeshes.size() - 1;
	}
	ImGui::SameLine();
	if (ImGui::Button("Create SubMesh")) { // TODO: Fazer Sistema de SubMeshes!!!!!
		if (m_CurrentMesh >= 0 && m_CurrentMesh < m_GlobalMeshes.size()) {
			BaseMesh* parent = m_GlobalMeshes[m_CurrentMesh];

			std::string mesh_name = "Mesh_" + std::to_string(m_CurrentMesh) + "_Child_" + std::to_string(parent->GetSubMeshes().size());
			parent->addSubMesh(new Mesh(mesh_name, { parent->GetCentroide().x, parent->GetCentroide().y, 1.0f }));
			m_CurrentSubMesh = (int)parent->GetSubMeshes().size() - 1;
			SubmitMesh(parent->GetSubMesh(m_CurrentSubMesh));

			OnChangeMode(CreatePoly);
			LE_TRACE("SubMesh: " + mesh_name + " Created!");
		}
		else {
			LE_ERROR("You Cannot create a SubMesh!");
		}
	}

	if (ImGui::Button("Import .OBJ")) {
		std::string selectedPath = OpenObjFileDialog(m_WindowProperties.Window);

		if (!selectedPath.empty()) {
			std::string meshName = std::filesystem::path(selectedPath).stem().string();

			Mesh* importedMesh = new Mesh(meshName);

			LoadObjFile(selectedPath, importedMesh);

			importedMesh->GetTransform().Position = { GetWidth() / 2.0f, GetHeight() / 2.0f, 0.0f };
			importedMesh->GetTransform().Scale = { 100.0f, 100.0f, 100.0f };
			importedMesh->MarkDirty();

			m_GlobalMeshes.push_back(importedMesh);
			m_CurrentMesh = (int)m_GlobalMeshes.size() - 1;
			m_CurrentSubMesh = -1;

			OnChangeMode(MovePoly);
			LE_CORE_INFO("OBJ importado com sucesso: " + selectedPath);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Create New Light")) {
		std::string lightName = "Light_" + std::to_string(m_GlobalMeshes.size());
		MeshLight* newLight = new MeshLight(lightName);
		SubmitMesh(newLight);
		m_CurrentMesh = (int)m_GlobalMeshes.size() - 1;
	}

	ImGui::SeparatorText("Editor Modes");

	std::string currentModeText;
	switch (m_CurrentMode)
	{
	case Selection:
		currentModeText = "Selection";
		break;
	case CreatePoly:
		currentModeText = "Editing";
		break;
	case MovePoly:
		currentModeText = "Translation";
		break;
	case ScalePoly:
		currentModeText = "Scale";
		break;
	case RotatePoly:
		currentModeText = "Rotation";
		break;
	default:
		currentModeText = "None";
		break;
	}
	ImGui::Text(("Mode: " + currentModeText).c_str());

	if (ImGui::Button("Selection Mode")) {
		OnChangeMode(Selection);
	}
	if (ImGui::Button("Translation Mode")) {
		OnChangeMode(MovePoly);
	}
	if (ImGui::Button("Rotation Mode")) {
		OnChangeMode(RotatePoly);
	}
	if (ImGui::Button("Scale Mode")) {
		OnChangeMode(ScalePoly);
	}
	
	ImGui::SeparatorText("Game Options");
	if (ImGui::Button("PLAY!") && !m_GameStarted) {
		LE_TRACE("Thank you for playing!");
		EventBeginPlay();
	}
	ImGui::Checkbox("Simulate", &m_GameSimulate);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	ImGui::SeparatorText("Renderer Options");
	const char* algorithms[] = { "Constant", "Gouraud", "Phong" };
	int currentAlgo = static_cast<int>(Renderer3D::LightAlgorithm);

	if (ImGui::Combo("Light Algorithm", &currentAlgo, algorithms, IM_ARRAYSIZE(algorithms))) {
		Renderer3D::LightAlgorithm = static_cast<LightAlgorithms>(currentAlgo);
	}

	const char* rendererModes[] = { "Software", "Hardware" };
	int currentRenderer = static_cast<int>(Renderer3D::RendererMode);


#ifdef USE_HARDWARE_RENDERER
	if (ImGui::Combo("Renderer Mode", &currentRenderer, rendererModes, IM_ARRAYSIZE(rendererModes))) {
		Renderer3D::RendererMode = static_cast<RendererModes>(currentRenderer);
		if (Renderer3D::RendererMode == RendererModes::Software) {
			SDL_ReleaseGPUGraphicsPipeline(m_WindowProperties.GPUDevice, Renderer3D::DefaultPipeline);
			Renderer3D::Init(m_FrameBuffer, m_ZBuffer, &m_WindowProperties, &m_GlobalMeshLights);
		}
		else {
			Renderer3D::InitGPU(&m_WindowProperties);
		}
	}

#endif

	ImGui::Checkbox("Wireframe Mode", &Renderer3D::WireframeMode);
	ImGui::Checkbox("Perspective", &Renderer3D::UsePerspective);
	ImGui::End();

	// Inspector window
	ImGui::Begin("Inspector");
	BaseMesh* activeMesh = GetActiveMesh();
	if (activeMesh != nullptr) {
		BaseMesh* selectedMesh = activeMesh;

		// FINALMENTE CONSEGUI TROCAR O NOME NESA DESGRAÇA
		char nameBuffer[256];
		strncpy_s(nameBuffer, sizeof(nameBuffer), selectedMesh->GetName().c_str(), _TRUNCATE);
		ImGui::Text("Editing: %s", nameBuffer);

		if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
			selectedMesh->GetName() = std::string(nameBuffer); // (ou selectedMesh->SetName(nameBuffer))
		}

		const char* typeLabel = "Unknown";
		if (selectedMesh->GetType() == MeshTypes::Type2D) typeLabel = "2D Polygon";
		else if (selectedMesh->GetType() == MeshTypes::Type3D) typeLabel = "3D Mesh";
		else if (selectedMesh->GetType() == MeshTypes::TypeLight) typeLabel = "Light";
		ImGui::TextDisabled("Type: %s", typeLabel);
		ImGui::Separator();

		// TRANSFORM
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			Transform& MeshTransform = selectedMesh->GetTransform();
			bool transformChanged = false;

			if (ImGui::DragFloat3("Position", glm::value_ptr(MeshTransform.Position), 1.0f)) {
				transformChanged = true;
			}
			if (ImGui::DragFloat3("Rotation", glm::value_ptr(MeshTransform.Rotation), 1.0f)) {
				transformChanged = true;
			}
			if (ImGui::DragFloat3("Scale", glm::value_ptr(MeshTransform.Scale), 0.1f)) {
				transformChanged = true;
			}

			if (transformChanged) {
				selectedMesh->MarkDirty();
				if (m_CurrentSubMesh == -1) {
					for (BaseMesh* submesh : selectedMesh->GetSubMeshes()) {
						submesh->MarkDirty();
					}
				}
			}
		}

		// PROPEIDADES GERAIS E CORES
		if (ImGui::CollapsingHeader("General Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (selectedMesh->m_Parent == nullptr) {
				ImGui::Checkbox("Fill Poly", &selectedMesh->GetFill());
				ImGui::SameLine();
				ImGui::Checkbox("Use Outline", &selectedMesh->GetOutline());

				if (selectedMesh->GetType() == MeshTypes::Type2D) {
					ImGui::Checkbox("Closed Poly", &selectedMesh->IsClosed());
					ImGui::SameLine();
					ImGui::Checkbox("Fill Color Active", &selectedMesh->IsActive());
				}

				ImGui::Checkbox("Tick Enabled", &selectedMesh->GetTickEnabled());
				ImGui::SameLine();
				ImGui::Checkbox("Input Enabled", &selectedMesh->GetInputEnabled());
			}

			ImGui::Spacing();
			ImGui::ColorEdit3("Viewport Clear Color", glm::value_ptr(m_BgColor));
			ImGui::ColorEdit3("Mesh Base Color", reinterpret_cast<float*>(&selectedMesh->GetColor()));
			if (selectedMesh->GetOutline()) {
				ImGui::ColorEdit3("Outline Color", reinterpret_cast<float*>(&selectedMesh->GetOutlineColor()));
			}
		}

		// 3D E MATERIAIS
		if (selectedMesh->GetType() != MeshTypes::Type2D) {
			Mesh* mesh3D = static_cast<Mesh*>(selectedMesh);

			if (ImGui::CollapsingHeader("3D Mesh & Material", ImGuiTreeNodeFlags_DefaultOpen)) {
				if (selectedMesh->GetType() == MeshTypes::Type3D) {
					ImGui::Text("OBJ Path: %s", mesh3D->GetObj().c_str());
					if (ImGui::Button("Change .OBJ Model")) {
						std::string newObj = OpenObjFileDialog(m_WindowProperties.Window);
						if (!newObj.empty()) {
							// é mais fácil criar uma mesh nova e substituir do q tentar limpar a bagunça q virou essa Mesh3D
							Mesh* newMesh3D = new Mesh(mesh3D->GetName());

							newMesh3D->GetTransform() = mesh3D->GetTransform();
							newMesh3D->GetObj() = newObj;
							newMesh3D->GetMaterial() = mesh3D->GetMaterial();
							LoadObjFile(newObj, newMesh3D);

							// Substitui a mesh na lista
							auto it = std::ranges::find(m_GlobalMeshes, selectedMesh);
							m_GlobalMeshes.erase(it);
							m_GlobalMeshes.emplace(it, newMesh3D);
						}
					}
				}

				ImGui::Spacing();
				ImGui::Checkbox("Affected By Lights", &mesh3D->GetAfectedByLights());
				ImGui::Checkbox("Hidden In Game", &mesh3D->GetHiddenInGame());

				ImGui::SeparatorText("Mesh Material");
				MeshMaterial& mat = mesh3D->GetMaterial();
				ImGui::ColorEdit3("Material Albedo", glm::value_ptr(mat.baseColor));
				ImGui::SliderFloat("Ambient Reflection (Ka)", &mat.ambientReflection, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Diffuse Reflection (Kd)", &mat.diffuse, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Specular Reflection (Ks)", &mat.specular, 0.0f, 1.0f, "%.3f");
				ImGui::DragFloat("Shininess (n)", &mat.specular_brightness, 0.5f, 1.0f, 256.0f, "%.1f");
			}

			// PROPRIEDADES DA LUZ
			if (selectedMesh->GetType() == MeshTypes::TypeLight) {
				MeshLight* light = static_cast<MeshLight*>(selectedMesh);

				if (ImGui::CollapsingHeader("Light Emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
					const char* lightTypes[] = { "Directional Light", "Point Light" };
					int currentLightType = (light->GetLightType() == LightType::DirectionalLight) ? 0 : 1;
					if (ImGui::Combo("Light Type", &currentLightType, lightTypes, IM_ARRAYSIZE(lightTypes))) {
						light->GetLightType() = (currentLightType == 0)
							? LightType::DirectionalLight : LightType::SceneLight;
					}

					ImGui::ColorEdit3("Light Color", glm::value_ptr(light->GetLightColor()));
					ImGui::DragFloat("Intensity", &light->GetLightIntensity(), 0.05f, 0.0f, 500.0f, "%.2f");

					if (light->GetLightType() != LightType::DirectionalLight) {
						ImGui::DragFloat("Attenuation Radius (Max Dist)", &light->GetMaxDistance(), 10.0f, 1.0f, 100000.0f, "%.1f");
					}
				}
			}
		}

		if (ImGui::CollapsingHeader("Collision (AABB)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Enable Collision", &selectedMesh->GetCollision());

			if (selectedMesh->GetCollision()) {
				BoundingBox& bb = selectedMesh->GetBoundingBox();
				ImGui::SameLine();
				ImGui::Checkbox("Show Debug Box", &bb.ShowDebug);

				if (ImGui::DragFloat3("Box Offset", glm::value_ptr(bb.offset), 0.05f)) {
					selectedMesh->MarkDirty();
				}
				if (ImGui::DragFloat3("Box Scale", glm::value_ptr(bb.sizeScale), 0.02f, 0.01f, 100.0f)) {
					selectedMesh->MarkDirty();
				}

				if (selectedMesh->IsColliding()) {
					ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Collision: (%d)", (int)bb.OutHits.size());
					for (CollisionHit& outHit : bb.OutHits) {
						ImGui::BulletText("Hit: %s | Pen: %.2f | Normal: (%.2f, %.2f, %.2f)", outHit.other->GetName().c_str(), outHit.penetration, outHit.normal.x, outHit.normal.y, outHit.normal.z);
					}
				}
				else {
					ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "Collision: Clear");
				}
			}
			
			ImGui::Checkbox("Simulate Physics", &selectedMesh->GetSimulatePhysics());
			//ImGui::DragFloat("Gravity", &selectedMesh->GetGravity(), 0.05f)
		}
		

		// SCRIPTING LUA
		if (ImGui::CollapsingHeader("Scripting (Lua)", ImGuiTreeNodeFlags_DefaultOpen)) {
			std::string scriptName = selectedMesh->GetScriptPath().empty()
				? "None"
				: std::filesystem::path(selectedMesh->GetScriptPath()).filename().string();

			ImGui::Text("Active Script: %s", scriptName.c_str());
			if (!selectedMesh->GetScriptPath().empty()) {
				ImGui::TextDisabled("%s", selectedMesh->GetScriptPath().c_str());
			}

			if (ImGui::Button("Assign .LUA")) {
				std::string luaPath = OpenLuaFileDialog(m_WindowProperties.Window);
				if (!luaPath.empty()) {
					if (ScriptEngine::LoadMeshScript(selectedMesh, luaPath)) {
						selectedMesh->GetTickEnabled() = true;
						selectedMesh->GetInputEnabled() = true;
					}
				}
			}

			if (!selectedMesh->GetScriptPath().empty()) {
				ImGui::SameLine();
				if (ImGui::Button("Recompile / Hot-Reload")) {
					if (ScriptEngine::LoadMeshScript(selectedMesh, selectedMesh->GetScriptPath())) {
						if (m_GameStarted) {
							ScriptEngine::OnBeginPlay(selectedMesh);
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear Script")) {
					selectedMesh->GetScriptPath().clear();
					selectedMesh->GetLuaEnv() = sol::nil;
				}
			}
		}

		// DELETE
		ImGui::SeparatorText("Danger Zone");
		if (selectedMesh->GetType() == MeshTypes::Type2D && ImGui::Button("Delete Last Vertex")) {
			selectedMesh->removeVertex();
		}
		if (ImGui::Button("Destroy Mesh")) {
			OnDestroySelectedMesh();
		}
	}
	else {
		ImGui::TextDisabled("Select an actor in the Outliner to inspect its properties.");
	}
	ImGui::End();
}
///////////////////////////////////////////////////////////
//////////////////////RENDER UI///////////////////////////
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
//////////////////////CHANGE MODE///////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::OnChangeMode(EEditMode new_mode)
{
	if (BaseMesh* activeMesh = GetActiveMesh()) {
		switch (new_mode) {
		case CreatePoly:
			activeMesh->ResetTransform(GetWidth()/2, GetHeight()/2);
			break;
		default:
			if(m_CurrentMode == CreatePoly){ activeMesh->RestoreTransform(); }
			break;
		}
	}
	
	if (m_bRMB || m_GameStarted) {
		m_CurrentMode = Navigation;
		return;
	}

	m_CurrentMode = new_mode;
}

///////////////////////////////////////////////////////////
//////////////////////DETROY MESH///////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::OnDestroySelectedMesh()
{
	if (m_CurrentSubMesh == -1) { // se for mesh pai
		BaseMesh* markForDelete = m_GlobalMeshes[m_CurrentMesh];
		
		if (markForDelete->GetType() == MeshTypes::TypeLight) {
			auto it = std::ranges::find(m_GlobalMeshLights, markForDelete);
			m_GlobalMeshLights.erase(it);
		}

		m_GlobalMeshes.erase(m_GlobalMeshes.begin() + m_CurrentMesh);

		delete markForDelete;

		if (m_GlobalMeshes.empty()) {
			m_CurrentMesh = -1;
		}
		else {
			m_CurrentMesh = std::max(0, m_CurrentMesh - 1);
		}
		m_CurrentSubMesh = -1;
		LE_INFO("Mesh destroyed!");
	}
	else { // se for mesh filha
		BaseMesh* parent = m_GlobalMeshes[m_CurrentMesh];
		parent->removeSubMesh(m_CurrentSubMesh);

		m_CurrentSubMesh = -1;
		m_ActiveGizmoState = GIZMO_NONE;
		LE_INFO("Submesh destroyed!");
	}
}


void EditorApp::LoadObjFile(std::string path, Mesh* mesh)
{
	// CARREGANDO UM ARQUIVO OBJ
	std::ifstream file;

	file.open(path, std::ifstream::in);
	if (!file.is_open()) { 
		LE_CORE_ERROR("FALHA AO ABRIR O ARQUIVO OBJ: " + path);
		return; 
	}

	std::string line;

	LE_CORE_INFO("Loading obj...");
	while (std::getline(file, line))
	{
		std::stringstream sstream(line);

		std::string prefix;
		sstream >> prefix;

		if (prefix.size() > 0)
		{
			if ("mtlib" == prefix)
			{
				// Carregar mtlib
				//LE_CORE_TRACE("LOAD MTLIB");
			}
			else if ("v" == prefix)
			{
				// vértices
				//LE_CORE_TRACE("LOAD VERTICES");
				// pegar os vértices
				glm::vec3 loadedVertex;
				sstream >> loadedVertex.x >> loadedVertex.y >> loadedVertex.z;

				mesh->addVertex(loadedVertex);

			}
			else if ("vn" == prefix)
			{
				// normals
				//LE_CORE_TRACE("LOAD NORMALS");
				glm::vec3 loadedNormal;
				sstream >> loadedNormal.x >> loadedNormal.y >> loadedNormal.z;

				mesh->addNormal(loadedNormal);
			}
			else if ("vt" == prefix)
			{
				// texture coordenadas
				//LE_CORE_TRACE("LOAD TEXTURE COORDS");

			}
			else if ("usemtl" == prefix)
			{
				//LE_CORE_TRACE("USE MTL");
				// mtl pelo jeito é tipo uma textura, vai indicar o nome do arquivo mtl na mesma pasta (ou o caminho)
				// aí lá no mtl vai ter o endereço/caminho/nome das texturas pra carregar

			}
			else if ("f" == prefix)
			{
				// @Mattsu TODO: Verificar aqui se a face f tem 3 vértices ou mais, se tiver 3, ok
				// se tiver 4 ou mais, aí marca sla um booleano ou algo assim pra fazer a triangulação da mesh aqui
				// assim que terminar o while, o problema é q provavelmente vai ter q descartar todo o vetor
				// de faces que foi salvo, os normais e etc... mas acontce nas melhores famílias né

				std::string indexGroup;
				std::vector<int> facesVertexIndices = {};
				int faceNormalIndice = -1;

				while (sstream >> indexGroup)
				{
					std::vector<std::string> indices = split(indexGroup, '/');
					facesVertexIndices.push_back(std::stoi(indices[0]) - 1);
					faceNormalIndice = std::stoi(indices[2]) - 1;

					if (1 == indices.size())
					{

					}
					else if (2 == indices.size())
					{

					}
					else if (3 == indices.size())
					{

					}
					else
					{
						// ERROR
					}
				}

				mesh->addFace(facesVertexIndices, faceNormalIndice);
			}
			else
			{
				continue;
			}
		}
	}

	mesh->GetObj() = path;

	// calcular os vertexNormals
	mesh->GetVertexNormals() = std::vector<glm::vec3>(mesh->GetLocalVertices().size());
	for (face& f : mesh->GetFaces()) {
		glm::vec3 vertexA = mesh->GetLocalVertices()[f.vertices[0]];
		glm::vec3 vertexB = mesh->GetLocalVertices()[f.vertices[1]];
		glm::vec3 vertexC = mesh->GetLocalVertices()[f.vertices[2]];

		glm::vec3 faceNormal = glm::normalize(glm::cross((vertexC - vertexA), (vertexB - vertexA)));
		faceNormal = mesh->GetNormals()[f.normal];
		for (int i : f.vertices) {
			mesh->GetVertexNormals()[i] += faceNormal;
		}
	}
	// normalizar os normais das vértices
	for (glm::vec3& vn : mesh->GetVertexNormals()) {
		if (glm::length(vn) <= 0) { continue; }
		vn = glm::normalize(vn);
	}

	// Calcular o minmax da BoundingBox dessa mesh
	glm::vec3 minV(FLT_MAX);
	glm::vec3 maxV(-FLT_MAX);
	for (const glm::vec3& v: mesh->GetLocalVertices()) {
		minV = glm::min(minV, v);
		maxV = glm::max(maxV, v);
	}
	mesh->GetBoundingBox().localMin = minV;
	mesh->GetBoundingBox().localMax = maxV;

	// Para poder utilizar a mesh no renderer de hardware
	Renderer3D::UploadMeshToGPU(*mesh);

	file.close();
}

void EditorApp::SubmitMesh(BaseMesh* newMesh)
{
	if (newMesh) {
		if (!newMesh->m_Parent) {
			m_GlobalMeshes.push_back(newMesh);
		}

		switch (newMesh->GetType())
		{
		case MeshTypes::Type2D:
			break;
		case MeshTypes::Type3D:
			if (newMesh->GetObj() != "None") {
				LoadObjFile(newMesh->GetObj(), static_cast<Mesh*>(newMesh));
			}
			break;
		case MeshTypes::TypeLight:
			if (newMesh->GetObj() != "None") {
				LoadObjFile(newMesh->GetObj(), static_cast<Mesh*>(newMesh));
			}
			m_GlobalMeshLights.push_back(static_cast<MeshLight*>(newMesh));
			break;
		case MeshTypes::TypeText:
			break;
		default:
			break;
		}
	}
}

void EditorApp::PlaySound(const char* filepath)
{
	SDL_AudioSpec spec;

	if (!SDL_LoadWAV(filepath, &spec, &m_WavBuffer, &m_WavLength)) {
		LE_CORE_ERROR("Falha ao carregar som %s", SDL_GetError());
		return;
	}

	m_MusicStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);

	if (!m_MusicStream) {
		LE_CORE_ERROR("Falaha ao abrir stream de audio %s", SDL_GetError());
		SDL_free(m_WavBuffer);
		return;
	}

	SDL_ResumeAudioStreamDevice(m_MusicStream);
	SDL_PutAudioStreamData(m_MusicStream, m_WavBuffer, m_WavLength);
}

void EditorApp::SetHideCursor(bool hide) const
{
	SDL_SetWindowRelativeMouseMode(m_WindowProperties.Window, hide);
}

bool EditorApp::GetIsKeyPressed(SDL_Scancode ScanCode)
{
	int numkeys = 0;
	const bool* keystate = SDL_GetKeyboardState(&numkeys);
	return keystate[ScanCode];
}

std::string EditorApp::OpenObjFileDialog(SDL_Window* window)
{
#ifdef _WIN32
	char filePath[MAX_PATH] = { };

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = sizeof(filePath);
	// Filtro: Nome exibido \0 Extensão \0
	ofn.lpstrFilter = "Wavefront OBJ (*.obj)\0*.obj\0Todos os Arquivos (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		return std::string(ofn.lpstrFile);
	}
#endif
	return ""; // se cancelar
}

std::string EditorApp::OpenLuaFileDialog(SDL_Window* window)
{
#ifdef _WIN32
	char filePath[MAX_PATH] = { };

	OPENFILENAMEA ofn;
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFile = filePath;
	ofn.nMaxFile = sizeof(filePath);
	// Filtro: Nome exibido \0 Extensão \0
	ofn.lpstrFilter = "Lua Script (*.lua)\0*.lua\0Todos os Arquivos (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		return std::string(ofn.lpstrFile);
	}
#endif
	return ""; // se cancelar
}

///////////////////////////////////////////////////////////
//////////////////////BEGIN PLAY///////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::EventBeginPlay()
{
	for (BaseMesh* m : m_GlobalMeshes) {
		m->GetOutline() = false;
		m->GetFill() = true;
		m->BeginPlay();
	}
	OnChangeMode(EEditMode::Selection);
	m_GameStarted = true;
}

std::vector<std::string> EditorApp::split(const std::string& str, char delim) {
	std::vector<std::string> items;
	std::stringstream sstream(str);
	std::string item;

	while (std::getline(sstream, item, delim))
		items.push_back(item);

	return items;
}
