#include "EditorApp.h"
#include "Renderer2D.h"
#include "Renderer3D.h"
#include "Log.h"

#include <iostream>
#include <string>
#include <numbers>
#include <cmath>
#include <random>

#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

EditorApp::EditorApp()
{
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
bool EditorApp::Init()
{
	/*IMGUI FUNCIONA PFV*/
	LE_CORE_TRACE("Initializing SDL...");
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return false;
	}

	// Create window with SDL_Renderer graphics context
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;

	LE_CORE_TRACE("Creating SDL Window...");
	m_Window = SDL_CreateWindow("LeandroEditor 2D", (int)(WIDTH * main_scale), (int)(HEIGHT * main_scale), window_flags);
	if (m_Window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	LE_CORE_TRACE("Creating SDL Renderer...");
	m_Renderer = SDL_CreateRenderer(m_Window, nullptr);
	SDL_SetRenderVSync(m_Renderer, 1);
	if (m_Renderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(m_Window);

	LE_CORE_TRACE("Setting up Dear ImGui context...");
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(m_Window, m_Renderer);
	ImGui_ImplSDLRenderer3_Init(m_Renderer);

	LE_CORE_TRACE("Updating Window size...");
	//Update Width/Height values
	SDL_GetWindowSize(m_Window, &m_Width, &m_Height);

	LoadObjFile("./cubo.obj", cubinho);
	LoadObjFile("./zelda.obj", macaquinho);

	cubinho->GetTransform().Scale = { 100.0f, 100.0f, 100.0f };
	cubinho->GetTransform().Position = { 740.0f, 480.0f, 0.0f };
	cubinho->GetTransform().Rotation = { 40.0f, 30.0f, 10.0f };

	macaquinho->GetTransform().Scale = { 100.0f, 100.0f, 100.0f };
	macaquinho->GetTransform().Position = { 340.0f, 280.0f, 0.0f };
	macaquinho->GetTransform().Rotation = { 0.2f, 0.0f, 2.0f };

	m_GlobalMeshes.push_back(cubinho);
	m_GlobalMeshes.push_back(macaquinho);

	return true;
}
///////////////////////////////////////////////////////////
//////////INITIALIZE SDL IMGUI AND EDITOR//////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
//////////////////////SHUTDOWN////////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::Shutdown()
{
	// [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(m_Renderer);
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}
///////////////////////////////////////////////////////////
//////////////////////SHUTDOWN////////////////////////////
///////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////
///////////////////GET ACTIVE MESH/////////////////////////
///////////////////////////////////////////////////////////
BaseMesh* EditorApp::GetActiveMesh()
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
////////////////UPDATE GIZMO HOVER/////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::UpdateGizmoHover(float mouseX, float mouseY, BaseMesh* activeMesh, EEditMode mode)
{
	m_CurrentGizmoState = GIZMO_NONE;
	if (activeMesh == nullptr || (mode == Selection || mode == CreatePoly)) { return; }

	glm::vec3 centroide = activeMesh->GetCentroide();
	float cx = centroide.x;
	float cy = centroide.y;
	float size = 60.0f;

	// Pegar rotação do pai pro gizmo local, da submesh
	float gizmoRot = 0.0f;
	if (activeMesh->m_Parent != nullptr) {
		gizmoRot = activeMesh->m_Parent->GetTransform().Rotation.z;
	}

	//desfazer rotação do pai
	float dx = mouseX - cx;
	float dy = mouseY - cy;
	float localX = dx * std::cos(-gizmoRot) - dy * std::sin(-gizmoRot);
	float localY = dx * std::sin(-gizmoRot) + dy * std::cos(-gizmoRot);

	// colisão com o centro
	if (std::abs(localX) < 12 && std::abs(localY) < 12) {
		m_CurrentGizmoState = GIZMO_HOVER_CENTER;
		return;
	}

	if (mode == MovePoly || mode == ScalePoly) {
		if (localX > 0 && localX < size + 15 && std::abs(localY) < 10) {
			m_CurrentGizmoState = GIZMO_HOVER_X;
		}
		else if (localY < 0 && localY > -size - 15 && std::abs(localX) < 10) {
			m_CurrentGizmoState = GIZMO_HOVER_Y;
		}
	}
	else if (mode == RotatePoly) {
		float distance = std::sqrt(localX * localX + localY * localY);
		if (std::abs(distance - size) < 10.0f) {
			m_CurrentGizmoState = GIZMO_HOVER_ROT;
		}
	}
}
///////////////////////////////////////////////////////////
////////////////UPDATE GIZMO HOVER/////////////////////////
///////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////
/////////////////////PROCESS EVENTS////////////////////////
///////////////////////////////////////////////////////////
SDL_AppResult EditorApp::ProcessEvent(SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED) {
		SDL_GetWindowSize(m_Window, &m_Width, &m_Height);
	}

	// Se o comando for absorvido pela UI, ignora os outros comandos
	ImGuiIO& io = ImGui::GetIO();

	/// EVENTOS DE TECLA PRESSIONADA ///////////////////////////////////////////////////////
	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;
		BaseMesh* activeMesh = GetActiveMesh();
		if (activeMesh != nullptr) { activeMesh->EventInput(event); }

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
		case SDLK_BACKSPACE:
		case SDLK_DELETE:
			OnDestroySelectedMesh();
			break;
		case SDLK_ESCAPE:
			m_GameStarted = false;
			break;
		}
		// Atalho pra alternar o fillpoly 
		if (e.key == SDLK_LCTRL) {
			if (!m_GlobalMeshes.empty() && m_GlobalMeshes[m_CurrentMesh] != nullptr) {
				m_GlobalMeshes[m_CurrentMesh]->SetFill(!m_GlobalMeshes[m_CurrentMesh]->GetFill());
			}
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
			if (m_CurrentMode == CreatePoly){
				BaseMesh* ActiveMesh = GetActiveMesh();
				if (ActiveMesh != nullptr) {
					ActiveMesh->addVertex({ e.x, e.y, 1.0f });
				}
			}
			else {
				m_ActiveGizmoState = m_CurrentGizmoState;
			}
			m_bLMB = true;
		}
		else if (e.button == 3) { // RMB
			m_bRMB = true;
		}
	}
	/// EVENTOS DE MOUSE "DESPRESSIONADO"? ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {	
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) {
			m_ActiveGizmoState = GIZMO_NONE;
			m_bLMB = false;
		}
		else if (e.button == 3) {
			m_bRMB = false;
		}
	}
	/// EVENTOS DO MOUSE SE MEXENDA ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_MOTION) {
		ProcessMouseMotion(event);
	}
	/// EVENTOS DO SCROLL DO MOUSE ///////////////////////////////////////////////////////
	else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
		SDL_MouseWheelEvent e = event->wheel;
		if (m_CurrentMode == MovePoly) {
			m_MoveMeshSpeed += m_MoveMeshSpeed + 0.01f * e.y >= 0 && m_MoveMeshSpeed + 0.01f * e.y <= 1.0f ? 0.01f * e.y : 0.0f;
		}
		else if (m_CurrentMode == ScalePoly) {
			m_ScaleMeshSpeed += m_ScaleMeshSpeed + 0.0005f * e.y >= 0 && m_ScaleMeshSpeed + 0.0005f * e.y <= 0.05f ? 0.0005f * e.y : 0.0f;
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
void EditorApp::ProcessMouseMotion(SDL_Event* event)
{
	SDL_MouseButtonEvent e = event->button;
	if (e.x > GetWidth() || e.y > GetHeight() || e.x < 0 || e.y < 0) {
		return;
	}

	if (!m_bLMB) {
		UpdateGizmoHover(e.x, e.y, GetActiveMesh(), m_CurrentMode);
	}

	// Aqui é quando tá segurando o botão esquerdo do mouse
	if (m_bLMB) {
		BaseMesh* activeMesh = GetActiveMesh();
		if (activeMesh != nullptr) {
			Transform& MeshTransform = activeMesh->GetTransform();

			// Pega rotação do pai e tira da mesh filha
			float gizmoRot = 0.0f;
			if (activeMesh->m_Parent != nullptr) {
				gizmoRot = activeMesh->m_Parent->GetTransform().Rotation.z;
			}

			// Pega o movimento cm a rotação do pai
			float local_dx = (event->motion.xrel * std::cos(gizmoRot)) + (event->motion.yrel * std::sin(gizmoRot));
			float local_dy = (event->motion.xrel * std::sin(gizmoRot)) + (event->motion.yrel * -std::cos(gizmoRot));

			if (activeMesh->m_Parent != nullptr) {
				local_dx /= activeMesh->m_Parent->GetTransform().Scale.x;
				local_dy /= activeMesh->m_Parent->GetTransform().Scale.y;
			}

			// Pegar o movimento do mouse no mundo e transformar em um movimento local pra arrumar as submeshes com mesh pai rotacionada
			if (m_CurrentMode == MovePoly) {

				if (m_ActiveGizmoState == GIZMO_HOVER_X || m_ActiveGizmoState == GIZMO_NONE || m_ActiveGizmoState == GIZMO_HOVER_CENTER) {
					MeshTransform.Position.x += local_dx;
				}
				if (m_ActiveGizmoState == GIZMO_HOVER_Y || m_ActiveGizmoState == GIZMO_NONE || m_ActiveGizmoState == GIZMO_HOVER_CENTER) {
					MeshTransform.Position.y -= local_dy;
				}
			}
			else if (m_CurrentMode == ScalePoly) {
				if (m_ActiveGizmoState == GIZMO_HOVER_X) {
					MeshTransform.Scale.x += event->motion.xrel * m_ScaleMeshSpeed;
				}
				else if (m_ActiveGizmoState == GIZMO_HOVER_Y) {
					MeshTransform.Scale.y -= event->motion.yrel * m_ScaleMeshSpeed;
				}
				else if (m_ActiveGizmoState == GIZMO_HOVER_CENTER || m_ActiveGizmoState == GIZMO_NONE) {
					float uniformScale = (event->motion.xrel - event->motion.yrel) * m_ScaleMeshSpeed;
					MeshTransform.Scale.x += uniformScale;
					MeshTransform.Scale.y += uniformScale;
				}
			}
			else if (m_CurrentMode == RotatePoly) {
				MeshTransform.Rotation.z += event->motion.xrel * m_RotateMeshSpeed;
			}
			else if (m_CurrentMode == CreatePoly) {
				activeMesh->addVertex({e.x, e.y, 1.0f});
			}

			activeMesh->MarkDirty();

			if (m_CurrentSubMesh == -1) {
				for (BaseMesh* subMesh : activeMesh->GetSubMeshes()) {
					subMesh->MarkDirty();
				}
			}
		}
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

	// Cria novos pipes após X tempo
	if (m_GameStarted) {
		if(timeAcc > 5000.0){
			CreatePipes();
			timeAcc = 0;
		}
		else {
			timeAcc += deltaTime;
		}
	}

	/* IMGUI */
	if (SDL_GetWindowFlags(m_Window) & SDL_WINDOW_MINIMIZED)
	{
		SDL_Delay(10);
		return SDL_APP_CONTINUE;
	}

	// Start the Dear ImGui frame
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	// BUG TODO
	Renderer2D::DrawGizmo(GetActiveMesh(), m_CurrentMode, m_CurrentGizmoState, m_ActiveGizmoState);

	if (!m_GameStarted) {
		RenderUI();
	}

	SDL_SetRenderDrawColorFloat(m_Renderer, 0.2f, 0.2f, 0.2f, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
	SDL_RenderClear(m_Renderer);

	// Draw polygons
	for (BaseMesh* m : m_GlobalMeshes) {
		std::vector<glm::vec3>& vertices = m->GetVertices();
		if (vertices.empty()) { continue; }
		
		// Verifica se tusa fill color e seta a cor
		if (m->IsActive()) {
			ImVec4 color = m->GetColor();
			SDL_SetRenderDrawColorFloat(m_Renderer, color.x, color.y, color.z, SDL_ALPHA_OPAQUE_FLOAT);
		}
		else{
			const double now = ((double)SDL_GetTicks() * 0.001);  /* convert from milliseconds to seconds. */
			/* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
			const float red = (float)(0.5 + 0.5 * SDL_sin(now));
			const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
			const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
			SDL_SetRenderDrawColorFloat(m_Renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
		}

		if (m->GetType() == MeshTypes::Type3D) {
			Renderer3D::DrawMesh(m_Renderer, static_cast<Mesh&>(*(m)));
			continue;
		}

		// Fill poly
		if (m->GetFill()) {
			// BUG TODO
			Renderer2D::FillPoly(m_Renderer, m, GetHeight());
		}
		
		// Pega a cor do outline e desenha ela
		if (m->GetOutline()) {
			ImVec4 color = m->GetOutlineColor();
			SDL_SetRenderDrawColorFloat(m_Renderer, color.x, color.y, color.z, SDL_ALPHA_OPAQUE_FLOAT);

			// Se tiver filhos, desenha o outline de cada filho
			for (BaseMesh* subMesh : m->GetSubMeshes()) {
				Renderer2D::DrawLinesWithVertices(m_Renderer, subMesh->GetVertices(), true);
			}
		}

		Renderer2D::DrawLinesWithVertices(m_Renderer, vertices, m->IsClosed());

		// Run tick
		m->EventTick(deltaTime);
		// Talvez verificar a colisão aqui?
		if (m->GetTransform().Position.y >= GetHeight()) {
			m->SetVelocity({ 0.0f, 0.0f, 0.0f });
		}
	}

	SDL_SetRenderDrawColorFloat(m_Renderer, 1.0f, 1.0f, 1.0f, SDL_ALPHA_OPAQUE_FLOAT);
	//// Draw Test Mesh
	//Renderer3D::DrawMesh(m_Renderer, *(cubinho));
	//Renderer3D::DrawMesh(m_Renderer, *(macaquinho));

	// Rendering
	ImGui::Render();
	SDL_SetRenderScale(m_Renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_Renderer);
	SDL_RenderPresent(m_Renderer);

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

	// Draw Control pannelk
	ImGui::Begin("Control pannel");
	ImGui::SeparatorText("Outliner");
	if (ImGui::BeginChild("Mesh Tree", ImVec2(0, 200), true)) {
		for (int i = 0; i < m_GlobalMeshes.size(); i++) {
			BaseMesh* m = m_GlobalMeshes[i];
			if (m->GetVertices().empty()) { continue; }

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool isSelected = (m_CurrentMesh == i && m_CurrentSubMesh == -1);

			if (isSelected) {
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
					bool isSubSelected = (m_CurrentMesh == i && m_CurrentSubMesh == j);

					if (isSubSelected) {
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
		m_CurrentMesh = m_GlobalMeshes.size();
		m_CurrentSubMesh = -1;

		std::string mesh_name = "Mesh_" + std::to_string(m_CurrentMesh);
		m_GlobalMeshes.push_back(new Mesh2D(mesh_name, { GetWidth() / 2.0f, GetHeight() / 2.0f, 1.0f }));
		
		LE_TRACE("Mesh: " + mesh_name + " Created!");
		OnChangeMode(CreatePoly);
	}
	ImGui::SameLine();
	if (ImGui::Button("Create SubMesh")) {
		if (m_CurrentMesh >= 0 && m_CurrentMesh < m_GlobalMeshes.size()) {
			BaseMesh* parent = m_GlobalMeshes[m_CurrentMesh];

			std::string mesh_name = "Mesh_" + std::to_string(m_CurrentMesh) + "_Child_" + std::to_string(parent->GetSubMeshes().size());
			parent->addSubMesh(new Mesh2D(mesh_name, { parent->GetCentroide().x, parent->GetCentroide().y, 1.0f }));
			m_CurrentSubMesh = (int)parent->GetSubMeshes().size() - 1;

			OnChangeMode(CreatePoly);
			LE_TRACE("SubMesh: " + mesh_name + " Created!");
		}
		LE_ERROR("You Cannot create a SubMesh!");
	}

	ImGui::SeparatorText("Custom Mesh");

	// Criar polígono personalizado
	ImGui::SliderInt("Num. Faces", &m_CustomPolyFaces, 3, 100);
	ImGui::SliderInt("Size", &m_CustomPolySize, 10, 100);
	if (ImGui::Button("Create Cusotm Poly")) {
		m_CurrentMesh = m_GlobalMeshes.size();
		m_CurrentSubMesh = -1;

		std::string mesh_name = "Mesh_Poly" + std::to_string(m_CurrentMesh);
		std::vector<glm::vec3> custom_poly;

		glm::vec3 start_vertex = {-10 * m_CustomPolySize, -10 * m_CustomPolySize, 1.0f }; // Ponto inicial
		float angle = (360.0f / (float)m_CustomPolyFaces) * std::numbers::pi / 180.0f;

		// Rotaciona o ponto
		for (int i = 0; i < m_CustomPolyFaces; i++) {
			custom_poly.push_back(start_vertex);
			float tx = start_vertex.x * std::cos(angle) - start_vertex.y * std::sin(angle);
			float ty = start_vertex.x * std::sin(angle) + start_vertex.y * std::cos(angle);
			start_vertex.x = tx;
			start_vertex.y = ty;
		}

		m_GlobalMeshes.push_back(new Mesh2D(mesh_name, { GetWidth() / 2.0f, GetHeight() / 2.0f, 1.0f }, custom_poly));
		m_GlobalMeshes[m_CurrentMesh]->SetClosed(true);

		LE_TRACE("Custom Mesh: " + mesh_name + " Created!");
		OnChangeMode(CreatePoly);
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
	if (ImGui::Button("Edit Mode")) {
		OnChangeMode(CreatePoly);
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

	ImGui::SeparatorText("Info.");
	ImGui::Text("Scale Speed: %f", m_ScaleMeshSpeed);
	
	ImGui::SeparatorText("BÔNUS");
	if (ImGui::Button("Flappy Fill!")) {
		LE_TRACE("Thank you for playing!");
		EventBeginPlay();
	}

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	ImGui::End();

	// Inspector window
	ImGui::Begin("Inspector");
	BaseMesh* activeMesh = GetActiveMesh();
	if (activeMesh != nullptr) {
		BaseMesh* selectedMesh = activeMesh;

		ImGui::Text("Editing: %s", selectedMesh->GetName().c_str());
		ImGui::Separator();

		// Tarnsformações da mesj
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			Transform& MeshTransform = selectedMesh->GetTransform();
			bool transformChanged = false;

			// posicao
			float pos[3] = { MeshTransform.Position.x, MeshTransform.Position.y, MeshTransform.Position.z };
			if (ImGui::DragFloat3("Position", pos, 1.0f)) {
				MeshTransform.Position.x = pos[0];
				MeshTransform.Position.y = pos[1];
				MeshTransform.Position.z = pos[2];
				transformChanged = true;
			}

			// rotacao
			float rotDegX = MeshTransform.Rotation.x * (180.0f / std::numbers::pi);
			float rotDegY = MeshTransform.Rotation.y * (180.0f / std::numbers::pi);
			float rotDegZ = MeshTransform.Rotation.z * (180.0f / std::numbers::pi);
			float rotDeg[3] = { rotDegX, rotDegY, rotDegZ };
			if (ImGui::DragFloat3("Rotation", rotDeg, 1.0f)) {
				MeshTransform.Rotation.x = rotDeg[0] * (std::numbers::pi / 180.0f);
				MeshTransform.Rotation.y = rotDeg[1] * (std::numbers::pi / 180.0f);
				MeshTransform.Rotation.z = rotDeg[2] * (std::numbers::pi / 180.0f);
				transformChanged = true;
			}

			// escala
			float scale[3] = { MeshTransform.Scale.x, MeshTransform.Scale.y, MeshTransform.Scale.z };
			if (ImGui::DragFloat3("Scale", scale, 0.1f)) {
				MeshTransform.Scale.x = scale[0];
				MeshTransform.Scale.y = scale[1];
				MeshTransform.Scale.z = scale[2];
				transformChanged = true;
			}

			if (ImGui::Button("Reset Rotation and Scale")) {
				MeshTransform.Rotation = {0.0f, 0.0f, 0.0f};
				MeshTransform.Scale = { 1.0f, 1.0f, 1.0f };
				transformChanged = true;
			}

			if (transformChanged) {
				selectedMesh->MarkDirty();
				OnChangeMode(MovePoly);

				if (m_CurrentSubMesh == -1) {
					for (BaseMesh* submesh : selectedMesh->GetSubMeshes()) {
						submesh->MarkDirty();
					}
				}
			}
		}

		ImGui::SeparatorText("Properties");

		if (selectedMesh->m_Parent == nullptr) {
			ImGui::Checkbox("Fillpoly", &selectedMesh->GetFill());

			ImGui::Checkbox("Closed", &selectedMesh->IsClosed());
			ImGui::Checkbox("Outline Color", &selectedMesh->GetOutline());

			ImGui::Checkbox("Tick Enabled", &selectedMesh->GetTickEnabled());
		}
		ImGui::Checkbox("Fill Color", &selectedMesh->IsActive());

		ImGui::ColorEdit3("Clear Color", (float*)&m_ClearColor);
		ImGui::ColorEdit3("Mesh Color", (float*)&selectedMesh->GetColor());
		ImGui::ColorEdit3("Outline Color", (float*)&selectedMesh->GetOutlineColor());

		ImGui::SeparatorText("Deleting");

		if (ImGui::Button("Delete Vertex")) {
			LE_TRACE("Deleted last vertex on mesh: " + selectedMesh->GetName());
			selectedMesh->removeVertex();
		}

		if (ImGui::Button("Delete Mesh")) { // deleta as meshes
			LE_TRACE("Destroying Mesh... " + selectedMesh->GetName());
			OnDestroySelectedMesh();
		}
	}
	else {
		ImGui::Text("Select a mesh to edit it's properties");
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
	BaseMesh* activeMesh = GetActiveMesh();
	if (activeMesh) {
		switch (new_mode) {
		case CreatePoly:
			activeMesh->ResetTransform(GetWidth()/2, GetHeight()/2);
			break;
		default:
			if(m_CurrentMode == CreatePoly){ activeMesh->RestoreTransform(); }
			break;
		}
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
		m_GlobalMeshes.erase(m_GlobalMeshes.begin() + m_CurrentMesh);

		delete markForDelete;

		if (m_GlobalMeshes.empty()) {
			m_CurrentMesh = -1;
		}
		else {
			m_CurrentMesh = std::max(0, m_CurrentMesh - 1);
		}
		m_CurrentSubMesh = -1;

		m_ActiveGizmoState = GIZMO_NONE;
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
	// TESTE
	// CARREGANDO UM ARQUIVO OBJ
	std::ifstream file;

	file.open(path, std::ifstream::in);
	LE_CORE_ASSERT(file.is_open(), "FALHA AO ABRIR O ARQUIVO OBJ!!!");

	std::string line;

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
				LE_CORE_TRACE("LOAD MTLIB");
			}
			else if ("v" == prefix)
			{
				// vértices
				LE_CORE_TRACE("LOAD VERTICES");
				// pegar os vértices
				glm::vec3 loadedVertex;
				sstream >> loadedVertex.x >> loadedVertex.y >> loadedVertex.z;

				mesh->addVertex(loadedVertex);

			}
			else if ("vn" == prefix)
			{
				// normals
				LE_CORE_TRACE("LOAD NORMALS");

			}
			else if ("vt" == prefix)
			{
				// texture coordenadas
				LE_CORE_TRACE("LOAD TEXTURE COORDS");

			}
			else if ("usemtl" == prefix)
			{
				LE_CORE_TRACE("USE MTL");

			}
			else if ("f" == prefix)
			{
				// Faces
				LE_CORE_TRACE("LOAD FACES");
				std::string indexGroup;
				std::vector<int> facesIndices = {};

				while (sstream >> indexGroup)
				{
					std::vector<std::string> indices = split(indexGroup, '/');
					facesIndices.push_back(std::stoi(indices[0]) - 1);

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

				mesh->addFace(facesIndices);

			}
			else
			{
				continue;
			}
		}
	}

	file.close();

	// CARREGANDO UM ARQUIVO OBJ
}

///////////////////////////////////////////////////////////
//////////////////////FLAPPY POLY///////////////////////////
///////////////////////////////////////////////////////////
void EditorApp::EventBeginPlay()
{
	CreatePipes();
	CreatePlayer();
	m_GameStarted = true;
}

void EditorApp::CreatePipes()
{
	float maxHeight = GetHeight() - 100;

	// Create first pipes
	std::string up_pipe_name = "UpPipe_" + std::to_string(m_CurrentMesh);
	std::vector<glm::vec3> UpPipeVertexes = {
		{-20, 100, 1.0f},
		{-20, maxHeight, 1.0f},
		{ 20, maxHeight, 1.0f},
		{ 20, 100, 1.0f}
	};
	std::string down_pipe_name = "DownPipe_" + std::to_string(m_CurrentMesh);
	std::vector<glm::vec3> DownPipeVertexes = {
		{-20, -100, 1.0f},
		{-20, -maxHeight, 1.0f},
		{ 20, -maxHeight, 1.0f},
		{ 20, -100, 1.0f}
	};
	std::random_device rd;
	std::mt19937 gen(rd());
	float min = 120.0f;
	float max = GetHeight() - 100.0f;
	std::uniform_real_distribution<float> distr(min, max);
	float random_pos = distr(gen);

	m_GlobalMeshes.push_back(new Pipe(up_pipe_name, { (float)GetWidth(), (float)random_pos, 1.0f }, UpPipeVertexes));
	m_GlobalMeshes.push_back(new Pipe(down_pipe_name, { (float)GetWidth(), (float)random_pos, 1.0f }, DownPipeVertexes));
}

void EditorApp::CreatePlayer()
{
	OnChangeMode(Selection);

	m_CurrentMesh = m_GlobalMeshes.size();
	m_CurrentSubMesh = -1;

	std::string mesh_name = "Player" + std::to_string(m_CurrentMesh);
	std::vector<glm::vec3> custom_poly;

	glm::vec3 start_vertex = { -10, -10, 1.0f }; // Ponto inicial
	float angle = (360.0f / (float)m_CustomPolyFaces) * std::numbers::pi / 180.0f;

	// Rotaciona o ponto
	for (int i = 0; i < m_CustomPolyFaces; i++) {
		custom_poly.push_back(start_vertex);
		float tx = start_vertex.x * std::cos(angle) - start_vertex.y * std::sin(angle);
		float ty = start_vertex.x * std::sin(angle) + start_vertex.y * std::cos(angle);
		start_vertex.x = tx;
		start_vertex.y = ty;
	}

	m_GlobalMeshes.push_back(new Mesh2D(mesh_name, { GetWidth() / 2.0f, GetHeight() / 2.0f, 1.0f }, custom_poly));
	m_GlobalMeshes[m_CurrentMesh]->SetClosed(true);
	m_GlobalMeshes[m_CurrentMesh]->SetFill(true);
	m_GlobalMeshes[m_CurrentMesh]->SetTickEnabled(true);
}

std::vector<std::string> EditorApp::split(const std::string& str, char delim) {
	std::vector<std::string> items;
	std::stringstream sstream(str);
	std::string item;

	while (std::getline(sstream, item, delim))
		items.push_back(item);

	return items;
}
