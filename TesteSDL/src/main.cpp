#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <Random>
#include <numbers>

// Testando o imgui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>

// Melhorando o Mesh
#include "Mesh.h"

#define WIDTH 1280
#define HEIGHT 720

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

// Our state
bool show_demo_window = false; // teste do imgui
bool show_another_window = false; // teste do imgui
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f); // cor do background

// Modos possíveis do editor
enum EEditMode
{
	Selection = 0, CreatePoly = 1, MovePoly = 2, ScalePoly = 3, RotatePoly = 4
};

// Modos do Gizmo
enum EGizmoState { GIZMO_NONE, GIZMO_HOVER_X, GIZMO_HOVER_Y, GIZMO_HOVER_CENTER, GIZMO_HOVER_ROT };
EGizmoState currentGizmoState = GIZMO_NONE;
EGizmoState activeGizmoState = GIZMO_NONE;

// Current Editor mode
EEditMode ECurrentMode = Selection;

// Controles de edição pra cada moddo
float fMovePolySpeed = 0.1f;
float fscalePolySpeed = 0.01f;
float frotatePolySpeed = 0.01f;

// Render data structures
std::vector<Mesh*> GlobalMeshes;

int iCurrentMesh = -1;
int iCurrentSubMesh = -1;

// Input
bool bLMB = false;
bool bRMB = false;

bool findLess(const vertex& v1, const vertex& v2) {
	return v1.x < v2.x;
}

void DrawLinesWithVertices(std::vector<vertex>& vertices, bool closed) {
	if (vertices.size() < 2) { return; } // só desenha se tiver 2 ou mais vertices

	for (int i = 0; i < vertices.size() - 1; i++) {
		SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i + 1].x, vertices[i + 1].y);

		if (closed && vertices.size() - 2 == i) {
			SDL_RenderLine(renderer, vertices[i + 1].x, vertices[i + 1].y, vertices[0].x, vertices[0].y);
		}
	}
}

void FindIntersections(std::vector<vertex>& vertices, std::vector<std::vector<float>>& intersecoes, std::vector<std::vector<vertex>>& intersecoes_v) {
	for (int i = 0; i < vertices.size(); i++) {
		//std::cout << "Source ( " << source.x << " , " << source.y << " ) " << std::endl;
		//std::cout << "Point ( " << point.x << " , " << point.y << " ) " << std::endl;
		vertex source = vertices[i];
		vertex point = vertices[(i + 1) % vertices.size()];

		if (point.y == source.y) { continue; }

		// Aqui eu tenho os dois pontos
		if (point.y < source.y) { // point é mais alto que source; faz troca
			std::swap(source, point);
		}

		int ymin = (int)source.y;
		int ymax = (int)point.y;

		// Proteger contra índices negativos
		if (ymin < 0) ymin = 0;
		if (ymax < 0) return;
		if (ymin >= HEIGHT) return;

		float dY = (point.y - source.y);

		float Tx = (point.x - source.x) / dY;
		//float Tr = (point.color.x - source.color.x) / dY;
		//float Tg = (point.color.y - source.color.y) / dY;
		//float Tb = (point.color.z - source.color.z) / dY;

		float new_x = source.x;
		//float new_r = source.color.x;
		//float new_g = source.color.y;
		//float new_b = source.color.z;

		//std::cout << "ymin: " << ymin << " | ymax: " << ymax << " | Ns: " << Ns << " | Tx: " << Tx << " | new_y: " << new_y << " | new_x: " << new_x << std::endl;

		// Pegar as interseções
		for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
			//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
			//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
			//SDL_RenderPoint(renderer, new_x, ymin + i);
			if (y > 0 && y < HEIGHT) {
				intersecoes[y].push_back(new_x);
				vertex v = { new_x, (float)y };
				intersecoes_v[y].push_back(v);
			}
			new_x += Tx;
			//new_r += Tr;
			//new_g += Tg;
			//new_b += Tb;
		}
	}
}

void FillPoly(Mesh* mesh) {
	std::vector<vertex> vertices = mesh->GetVertices();
	if (vertices.size() < 3) { return; }
	std::vector<std::vector<float>> intersecoes;
	std::vector<std::vector<vertex>> intersecoes_v;
	intersecoes.resize(HEIGHT);
	intersecoes_v.resize(HEIGHT);

	FindIntersections(vertices, intersecoes, intersecoes_v);

	/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
	for (auto submesh : mesh->GetSubMeshes()) {
		std::vector<vertex> sub_vertices = submesh->GetVertices();
		if (sub_vertices.size() < 3) { continue; }
		FindIntersections(sub_vertices, intersecoes, intersecoes_v);
	}

	// Ordenar todas as interceções
	for (auto& lista : intersecoes) {
		std::sort(lista.begin(), lista.end());
	}
	// Ordernar interseções com cor
	for (auto& lista : intersecoes_v) {
		std::sort(lista.begin(), lista.end(), findLess);
	}

	// Fazer o fill no poly com cor agr
	for (int y = 0; y < intersecoes_v.size(); y++) {
		auto& lista = intersecoes_v[y];

		for (int i = 0; i + 1 < lista.size(); i += 2) {
			//if (i + 1 < lista.size()) {
			//	SDL_RenderPoint(renderer, (int)lista[i].x, y);
			//	SDL_RenderPoint(renderer, (int)lista[i + 1].x, y);
			//}
			SDL_RenderLine(renderer, lista[i].x, (float)y, lista[i + 1].x, (float)y);
		}
	}

}

// Retorna a mesh válida selecionada (se é a main mesh ou uma submesh)
Mesh* GetActiveMesh() {
	if (GlobalMeshes.empty() || iCurrentMesh < 0 || iCurrentMesh >= GlobalMeshes.size()) {
		return nullptr;
	}
	Mesh* mainMesh = GlobalMeshes[iCurrentMesh];

	if (iCurrentSubMesh >= 0 && iCurrentSubMesh < mainMesh->GetSubMeshes().size()) {
		return mainMesh->GetSubMesh(iCurrentSubMesh);
	}

	return mainMesh;
}

// atualizar Gizmo de transformação de acotrdo com o mouse
void UpdateGizmoHover(float mouseX, float mouseY, Mesh* activeMesh, EEditMode mode) {
	currentGizmoState = GIZMO_NONE;
	if (activeMesh == nullptr || (mode == Selection || mode == CreatePoly)) { return; }

	vertex centroide = activeMesh->GetCentroide();
	float cx = centroide.x;
	float cy = centroide.y;
	float size = 60.0f;

	// Pegar rotação do pai pro gizmo local, da submesh
	float gizmoRot = 0.0f;
	if (activeMesh->m_Parent != nullptr) {
		gizmoRot = activeMesh->m_Parent->GetTransform().Rotation;
	}

	//desfazer rotação do pai
	float dx = mouseX - cx;
	float dy = mouseY - cy;
	float localX = dx * std::cos(-gizmoRot) - dy * std::sin(-gizmoRot);
	float localY = dx * std::sin(-gizmoRot) + dy * std::cos(-gizmoRot);

	// colisão com o centro
	if (std::abs(localX) < 12 && std::abs(localY) < 12) {
		currentGizmoState = GIZMO_HOVER_CENTER;
		return;
	}

	if (mode == MovePoly || mode == ScalePoly) {
		if (localX > 0 && localX < size + 15 && std::abs(localY) < 10) {
			currentGizmoState = GIZMO_HOVER_X;
		}
		else if (localY < 0 && localY > -size - 15 && std::abs(localX) < 10) {
			currentGizmoState = GIZMO_HOVER_Y;
		}
	}
	else if (mode == RotatePoly) {
		float distance = std::sqrt(localX * localX + localY * localY);
		if (std::abs(distance - size) < 10.0f) {
			currentGizmoState = GIZMO_HOVER_ROT;
		}
	}
}

// Desenhar gizmo
void DrawGizmo(Mesh* activeMesh, EEditMode mode) {
	if (activeMesh == nullptr || activeMesh->GetVertices().empty()) { return; }
	if (mode == Selection || mode == CreatePoly) { return; }

	ImDrawList* draw_list = ImGui::GetBackgroundDrawList();

	vertex centroide = activeMesh->GetCentroide();
	ImVec2 center(centroide.x, centroide.y);

	float size = 60.0f;
	float thickness = 3.0f;

	// pega rotação do pa pra aplicar no gizmo lcoal (da submesh)
	float gizmoRot = 0.0f;
	if (activeMesh->m_Parent != nullptr) {
		gizmoRot = activeMesh->m_Parent->GetTransform().Rotation;
	}

	// Vetores direcinaoais rotacionados
	float cosR = std::cos(gizmoRot);
	float sinR = std::sin(gizmoRot);
	ImVec2 dirX(cosR, sinR);
	ImVec2 dirY(sinR, -cosR);

	ImU32 colorX = IM_COL32(220, 50, 50, 255);
	ImU32 colorY = IM_COL32(50, 220, 50, 255);
	ImU32 colorRot = IM_COL32(50, 50, 220, 255);
	ImU32 colorCenter = IM_COL32(20, 200, 200, 255);

	// Colocar hover/click nos gizmos
	if (currentGizmoState == GIZMO_HOVER_X || activeGizmoState == GIZMO_HOVER_X) colorX = IM_COL32(255, 120, 120, 255);
	if (currentGizmoState == GIZMO_HOVER_Y || activeGizmoState == GIZMO_HOVER_Y) colorY = IM_COL32(120, 255, 120, 255);
	if (currentGizmoState == GIZMO_HOVER_ROT || activeGizmoState == GIZMO_HOVER_ROT) colorRot = IM_COL32(120, 200, 255, 255);
	if (currentGizmoState == GIZMO_HOVER_CENTER || activeGizmoState == GIZMO_HOVER_CENTER) colorCenter = IM_COL32(255, 255, 100, 255);

	// FUnção pra criar quadrados com rotação mais fácil
	auto AddRotQuad = [&](ImVec2 p, float s, ImU32 col) {
		draw_list->AddQuadFilled(
			ImVec2(p.x + dirX.x * s - dirY.x * s, p.y + dirX.y * s - dirY.y * s),
			ImVec2(p.x + dirX.x * s + dirY.x * s, p.y + dirX.y * s + dirY.y * s),
			ImVec2(p.x - dirX.x * s + dirY.x * s, p.y - dirX.y * s + dirY.y * s),
			ImVec2(p.x - dirX.x * s - dirY.x * s, p.y - dirX.y * s - dirY.y * s),
			col);
		};

	if (mode == MovePoly) {
		// linha e seta od x
		ImVec2 endX(center.x + dirX.x * size, center.y + dirX.y * size);
		draw_list->AddLine(center, endX, colorX, thickness);
		draw_list->AddTriangleFilled(
			ImVec2(endX.x + dirX.x * 12, endX.y + dirX.y * 12),
			ImVec2(endX.x + dirY.x * 6, endX.y + dirY.y * 6),
			ImVec2(endX.x - dirY.x * 6, endX.y - dirY.y * 6),
			colorX);

		// linha e seta do 
		ImVec2 endY(center.x + dirY.x * size, center.y + dirY.y * size);
		draw_list->AddLine(center, endY, colorY, thickness);
		draw_list->AddTriangleFilled(
			ImVec2(endY.x + dirY.x * 12, endY.y + dirY.y * 12),
			ImVec2(endY.x + dirX.x * 6, endY.y + dirX.y * 6),
			ImVec2(endY.x - dirX.x * 6, endY.y - dirX.y * 6),
			colorY);

		// centro
		AddRotQuad(center, 6.0f, colorCenter);
	}
	else if (mode == ScalePoly) {
		// eixo x
		draw_list->AddLine(center, ImVec2(center.x + size, center.y), colorX, thickness);
		draw_list->AddRectFilled(ImVec2(center.x + size, center.y - 5), ImVec2(center.x + size + 10, center.y + 5), colorX);
		// eixo y
		draw_list->AddLine(center, ImVec2(center.x, center.y - size), colorY, thickness);
		draw_list->AddRectFilled(ImVec2(center.x + 5, center.y - size), ImVec2(center.x - 5, center.y - size - 10), colorY);
		// centro
		draw_list->AddRectFilled(ImVec2(center.x - 6, center.y - 6), ImVec2(center.x + 6, center.y + 6), colorCenter);
	}
	else if (mode == RotatePoly) {
		// circulo
		draw_list->AddCircle(center, size, colorRot, 0, thickness);
		draw_list->AddCircleFilled(ImVec2(center.x + size, center.y), 5.0f, colorCenter);
	}
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	std::cout << "COMANDOS:" << std::endl;
	std::cout << "LMB: Desenha/cria pontos" << std::endl << "RMB: Desenha/cria pontos internos" << std::endl;
	std::cout << "Ctrl: Ativa/Desativa Fill Poly" << std::endl << "Space: Alterna poligono fechado/aberto" << std::endl;
	std::cout << "Plus: Adiciona/Seleciona proximo poligono" << "Minus: Seleciona poligono anterior" << std::endl;
	std::cout << "Shift+Plus: Adiciona/Seleciona proximo poligono interno" << std::endl << "Shift+Minus: Seleciona poligono interno anterior" << std::endl;
	std::cout << "===TRANSFORMACOES===" << std::endl;
	std::cout << "G: Transladar" << std::endl << "S: Escalonar" << std::endl << "R: Rotacionar" << std::endl;
	std::cout << "Setinhas: Ajustar posicao no modo G" << std::endl << "Scroll: Regula velocidade" << std::endl << "LMB: Confirma | RMB: Cancela" << std::endl;
	std::cout << "9: Espelhamento no eixo X" << std::endl << "8: Espelhamento no eixo Y" << std::endl;

	/*IMGUI FUNCIONA PFV*/
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	// Create window with SDL_Renderer graphics context
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(WIDTH * main_scale), (int)(HEIGHT * main_scale), window_flags);
	if (window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	renderer = SDL_CreateRenderer(window, nullptr);
	SDL_SetRenderVSync(renderer, 1);
	if (renderer == nullptr)
	{
		SDL_Log("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	//io.WantCaptureMouse = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

	// Setup Platform/Renderer backends
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	//SDL_SetAppMetadata("Example render", "1", "exampleidentifier");
	//SDL_Init(SDL_INIT_VIDEO);
	//SDL_CreateWindowAndRenderer("WindowTitle", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);
	//SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return SDL_APP_CONTINUE;
}

// Event callbacks
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	if (ImGui_ImplSDL3_ProcessEvent(event)) {
		//return SDL_APP_CONTINUE;
	}
	if (event->type == SDL_EVENT_QUIT) {
		std::cout << "Encerrando..." << std::endl;
		return SDL_APP_SUCCESS;
	}

	// Se o comando for absorvido pela UI, ignora os outros comandos
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
		return SDL_APP_CONTINUE;
	}

	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;

		// MODE SWITCH
		switch (e.key) {
		case SDLK_W: // Move polygons mode
			ECurrentMode = MovePoly;
			break;
		case SDLK_E: // Rotate polygons mode
			ECurrentMode = RotatePoly;
			break;
		case SDLK_R: // Scale polygons mode
			ECurrentMode = ScalePoly;
			break;
		case SDLK_Q:
			ECurrentMode = Selection;
			break;
		}

		if (e.key == SDLK_SPACE) {
			if (!GlobalMeshes.empty() && GlobalMeshes[iCurrentMesh] != nullptr) {
				GlobalMeshes[iCurrentMesh]->SetClosed(!GlobalMeshes[iCurrentMesh]->IsClosed());
			}
		}

		if (e.key == SDLK_LCTRL) {
			if (!GlobalMeshes.empty() && GlobalMeshes[iCurrentMesh] != nullptr) {
				GlobalMeshes[iCurrentMesh]->SetFill(!GlobalMeshes[iCurrentMesh]->GetFill());
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) { // LMB
			if (ECurrentMode == CreatePoly) {
				//AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
				if (GlobalMeshes.empty()) {
					//std::string mesh_name = "Mesh_" + std::to_string(iCurrentMesh);
					//GlobalMeshes.push_back(new Mesh(mesh_name, {e.x, e.y}));
				}
				else {
					Mesh* ActiveMesh = GetActiveMesh();
					if (ActiveMesh != nullptr) {
						ActiveMesh->addVertex({ e.x, e.y });
					}
				}
			}
			else {
				activeGizmoState = currentGizmoState;
			}
			bLMB = true;
		}
		else if (e.button == 3) { // RMB
			if (ECurrentMode != CreatePoly) {
				// Cancel transformation and return to CreatePoly mode
				//ECurrentMode = CreatePoly;
			}
			else {
				bRMB = true;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) {
			activeGizmoState = GIZMO_NONE;
			bLMB = false;
		}
		else if (e.button == 3) {
			bRMB = false;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION) {
		SDL_MouseButtonEvent e = event->button;
		if (e.x > WIDTH || e.y > HEIGHT || e.x < 0 || e.y < 0) {
			return SDL_APP_CONTINUE;
		}
		std::pair<float, float> mouse_delta;

		if (!bLMB) {
			UpdateGizmoHover(e.x, e.y, GetActiveMesh(), ECurrentMode);
		}

		// Aqui é quando tá segurando o botão esquerdo do mouse
		if (bLMB) {
			Mesh* activeMesh = GetActiveMesh();
			if (activeMesh != nullptr) {
				Transform2D& MeshTransform = activeMesh->GetTransform();

				// Pega rotação do pai e tira da mesh filha
				float gizmoRot = 0.0f;
				if (activeMesh->m_Parent != nullptr) {
					gizmoRot = activeMesh->m_Parent->GetTransform().Rotation;
				}

				// Pega o movimento cm a rotação do pai
				float local_dx = (event->motion.xrel * std::cos(gizmoRot)) + (event->motion.yrel * std::sin(gizmoRot));
				float local_dy = (event->motion.xrel * std::sin(gizmoRot)) + (event->motion.yrel * -std::cos(gizmoRot));

				if (activeMesh->m_Parent != nullptr) {
					local_dx /= activeMesh->m_Parent->GetTransform().Scale.x;
					local_dy /= activeMesh->m_Parent->GetTransform().Scale.y;
				}

				// Pegar o movimento do mouse no mundo e transformar em um movimento local pra arrumar as submeshes com mesh pai rotacionada
				if (ECurrentMode == MovePoly) {

					if (activeGizmoState == GIZMO_HOVER_X || activeGizmoState == GIZMO_NONE || activeGizmoState == GIZMO_HOVER_CENTER) {
						MeshTransform.Position.x += local_dx;
					}
					if (activeGizmoState == GIZMO_HOVER_Y || activeGizmoState == GIZMO_NONE || activeGizmoState == GIZMO_HOVER_CENTER) {
						MeshTransform.Position.y -= local_dy;
					}
				}
				else if (ECurrentMode == ScalePoly) {
					if (activeGizmoState == GIZMO_HOVER_X) {
						MeshTransform.Scale.x += event->motion.xrel * fscalePolySpeed;
					}
					else if (activeGizmoState == GIZMO_HOVER_Y) {
						MeshTransform.Scale.y -= event->motion.yrel * fscalePolySpeed;
					}
					else if (activeGizmoState == GIZMO_HOVER_CENTER || activeGizmoState == GIZMO_NONE) {
						float uniformScale = (event->motion.xrel - event->motion.yrel) * fscalePolySpeed;
						MeshTransform.Scale.x += uniformScale;
						MeshTransform.Scale.y += uniformScale;
					}
				}
				else if (ECurrentMode == RotatePoly) {
					MeshTransform.Rotation += event->motion.xrel * frotatePolySpeed;
				}

				activeMesh->MarkDirty();

				if (iCurrentSubMesh == -1) {
					for (Mesh* subMesh : activeMesh->GetSubMeshes()) {
						subMesh->MarkDirty();
					}
				}
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
		SDL_MouseWheelEvent e = event->wheel;
		if (ECurrentMode == MovePoly) {
			fMovePolySpeed += fMovePolySpeed + 0.01f * e.y >= 0 && fMovePolySpeed + 0.01f * e.y <= 1.0f ? 0.01f * e.y : 0.0f;
		}
		else if (ECurrentMode == ScalePoly) {
			fscalePolySpeed += fscalePolySpeed + 0.0005f * e.y >= 0 && fscalePolySpeed + 0.0005f * e.y <= 0.05f ? 0.0005f * e.y : 0.0f;
			std::cout << fscalePolySpeed << std::endl;
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	/* IMGUI */
	if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
	{
		SDL_Delay(10);
		return SDL_APP_CONTINUE;
	}

	// Start the Dear ImGui frame
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	DrawGizmo(GetActiveMesh(), ECurrentMode);

	/* IMGUI */
	SDL_SetRenderDrawColorFloat(renderer, 0.2f, 0.2f, 0.2f, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
	SDL_RenderClear(renderer);

	const double now = ((double)SDL_GetTicks() * 0.001);  /* convert from milliseconds to seconds. */

	/* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
	const float red = (float)(0.5 + 0.5 * SDL_sin(now));
	const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
	const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

	// Draw Control pannelk
	ImGui::Begin("Control pannel");
	if (ImGui::BeginChild("Mesh Tree", ImVec2(0, 200), true)) {
		for (int i = 0; i < GlobalMeshes.size(); i++) {
			Mesh* m = GlobalMeshes[i];
			if (m->GetVertices().empty()) { continue; }

			ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			bool isSelected = (iCurrentMesh == i && iCurrentSubMesh == -1);

			if (isSelected) {
				node_flags |= ImGuiTreeNodeFlags_Selected;
			}

			bool nodeOpen = ImGui::TreeNodeEx(m, node_flags, "%s", m->GetName().c_str());

			if (ImGui::IsItemClicked()) {
				iCurrentMesh = i;
				iCurrentSubMesh = -1;
			}

			if (nodeOpen) {
				auto& subMeshes = m->GetSubMeshes();
				for (int j = 0; j < subMeshes.size(); j++) {
					Mesh* subMesh = subMeshes[j];

					ImGuiTreeNodeFlags sub_node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
					bool isSubSelected = (iCurrentMesh == i && iCurrentSubMesh == j);

					if (isSubSelected) {
						sub_node_flags |= ImGuiTreeNodeFlags_Selected;
					}

					ImGui::TreeNodeEx(subMesh, sub_node_flags, "%s", subMesh->GetName().c_str());
					if (ImGui::IsItemClicked()) {
						iCurrentMesh = i;
						iCurrentSubMesh = j;
					}
				}
				ImGui::TreePop();
			}
		}
	}
	ImGui::EndChild();

	ImGui::Separator();
	if (ImGui::Button("Create New Mesh")) {
		iCurrentMesh++;
		iCurrentSubMesh = -1;

		std::string mesh_name = "Mesh_" + std::to_string(iCurrentMesh);
		GlobalMeshes.push_back(new Mesh(mesh_name, { WIDTH / 2, HEIGHT / 2 }));

		ECurrentMode = CreatePoly;
	}
	ImGui::SameLine();
	if (ImGui::Button("Create SubMesh")) {
		if (iCurrentMesh >= 0 && iCurrentMesh < GlobalMeshes.size()) {
			Mesh* parent = GlobalMeshes[iCurrentMesh];

			std::string mesh_name = "Mesh_" + std::to_string(iCurrentMesh) + "_Child_" + std::to_string(parent->GetSubMeshes().size());
			parent->addSubMesh(new Mesh(mesh_name, { parent->GetCentroide().x, parent->GetCentroide().y }));
			iCurrentSubMesh = (int)parent->GetSubMeshes().size() - 1;

			ECurrentMode = CreatePoly;
		}
	}
	ImGui::Separator();

	if (ImGui::Button("Selection Mode")) {
		ECurrentMode = Selection;
	}
	if (ImGui::Button("Edit Mode")) {
		ECurrentMode = CreatePoly;
	}
	if (ImGui::Button("Translation Mode")) {
		ECurrentMode = MovePoly;
	}
	if (ImGui::Button("Rotation Mode")) {
		ECurrentMode = RotatePoly;
	}
	if (ImGui::Button("Scale Mode")) {
		ECurrentMode = ScalePoly;
	}

	ImGui::Separator();
	ImGui::Text("Scale Speed: %f", fscalePolySpeed);
	ImGui::Text("Move Speed: %f", fMovePolySpeed);

	ImGui::End();

	// Inspector winodw
	ImGui::Begin("Inspecor");
	Mesh* activeMesh = GetActiveMesh();
	if (activeMesh != nullptr) {
		Mesh* selectedMesh = activeMesh;

		ImGui::Text("Editting: %s", selectedMesh->GetName().c_str());
		ImGui::Separator();

		// Tarnsformações da mesj
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			Transform2D& MeshTransform = selectedMesh->GetTransform();
			bool transformChanged = false;

			// posicao
			float pos[2] = { MeshTransform.Position.x, MeshTransform.Position.y };
			if (ImGui::DragFloat2("Position", pos, 1.0f)) {
				MeshTransform.Position.x = pos[0];
				MeshTransform.Position.y = pos[1];
				transformChanged = true;
			}

			// rotacao
			float rotDeg = MeshTransform.Rotation * (180.0f / std::numbers::pi);
			if (ImGui::DragFloat("Rotation", &rotDeg, 1.0f)) {
				MeshTransform.Rotation = rotDeg * (std::numbers::pi / 180.0f);
				transformChanged = true;
			}

			// escala
			float scale[2] = { MeshTransform.Scale.x, MeshTransform.Scale.y };
			if (ImGui::DragFloat2("Scale", scale, 1.0f)) {
				MeshTransform.Scale.x = scale[0];
				MeshTransform.Scale.y = scale[1];
				transformChanged = true;
			}

			if (ImGui::Button("Reset Rotation and Scale")) {
				MeshTransform.Rotation = 0.0f;
				MeshTransform.Scale = { 1.0f, 1.0f };
				transformChanged = true;
			}

			if (transformChanged) {
				selectedMesh->MarkDirty();

				if (iCurrentSubMesh == -1) {
					for (Mesh* submesh : selectedMesh->GetSubMeshes()) {
						submesh->MarkDirty();
					}
				}
			}
		}

		ImGui::Checkbox("Fillpoly", &selectedMesh->GetFill());
		ImGui::Checkbox("Closed", &selectedMesh->IsClosed());
		ImGui::Checkbox("Active", &selectedMesh->IsActive());

		ImGui::ColorEdit3("Clear Color", (float*)&clear_color);
		ImGui::ColorEdit3("Mesh Color", (float*)&selectedMesh->GetColor());

		if (ImGui::Button("Delete Vertex")) {
			selectedMesh->removeVertex();
		}

	}
	else {
		ImGui::Text("None Selected");
	}
	ImGui::Separator();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();

	// Draw polygons
	for (Mesh* m : GlobalMeshes) {
		std::vector<vertex>& vertices = m->GetVertices();
		if (vertices.empty()) { continue; }


		if (m->IsActive()) {
			ImVec4 color = m->GetColor();
			SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, SDL_ALPHA_OPAQUE_FLOAT);
		}
		else {
			SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
		}

		DrawLinesWithVertices(vertices, m->IsClosed());

		if (m->GetFill()) {
			FillPoly(m);
		}
	}

	// Rendering
	ImGui::Render();
	SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
	//SDL_SetRenderDrawColorFloat(renderer, clear_color.x, clear_color.y, clear_color.z, clear_color.w);
	//SDL_RenderClear(renderer);
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
	SDL_RenderPresent(renderer);

	//SDL_RenderPresent(renderer);
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
	// Cleanup
	// [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}