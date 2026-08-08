#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <Random>

// Testando o imgui
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>

// Melhorando o Mesh
#include "Mesh.h"

#define WIDTH 1280
#define HEIGHT 720
#define POLY_MIN 1

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

// Our state
bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

struct Point {
	float x;
	float y;
};

enum EEditMode
{
	Selection = 0, CreatePoly = 1, MovePoly = 2, ScalePoly = 3, RotatePoly = 4
};

float fMovePolySpeed = 0.1;
float fscalePolySpeed = 0.01;
float frotatePolySpeed = 0.01;

// Editor mode
EEditMode ECurrentMode = Selection;

// Render data structures
std::vector<Mesh*> GlobalMeshes;

std::vector<std::vector<Point>> curves;
std::vector<std::vector<Point>> polygons;

std::vector<std::vector<std::vector<Point>>> sub_polygons;
int iCurrentCurve = 0;
int iCurrentPoly = 0;
int iCurrentSubPoly = 0;
Point CurrentPolyCenter = {0,0};

// Render control
bool bUseSubPoly = false;
bool bIsCurrent = false;
bool bFill = false;
bool bClosed = false;

// Input
bool bLMB = false;
bool bRMB = false;


// Curva de Bézier
std::vector<std::vector<int>> hermite_matrix = {
	{-1, 3, -3,	1},
	{ 3,-6,	 3,	0},
	{-3, 3,  0,	0},
	{ 1, 0,	 0,	0}
};

// Matrizes de transformação

// Transladar
float mouse_oX, mouse_oY;

std::vector<std::vector<float>> translation_matrix = {
	{1,  0,  0},
	{0,  1,  0},
	{0,  0,  1}
};

// Escalonar
float scaleX = 1;
float scaleY = 1;

// Rotacionar
std::vector<std::vector<float>> rotation_matrix = {
	{1,  0,  0},
	{0,  1,  0},
	{0,  0,  1}
};

bool findLess(const vertex& v1, const vertex& v2) {
	return v1.x < v2.x;
}

void AddVerticeToPoly(Point p, std::vector<Point> *vertices) {
	vertices->push_back(p);
}

void ResetTransform() {
	// Reset translation values
	translation_matrix[0][2] = 0;
	translation_matrix[1][2] = 0;
	// Reset scale values
	scaleX = 1.0;
	scaleY = 1.0;
	// Reset rotation values
	rotation_matrix[0][0] = 1;
	rotation_matrix[0][1] = 0;
	rotation_matrix[1][0] = 0;
	rotation_matrix[1][1] = 1;
}

void ApplyTransform(std::vector<vertex>* vertices) {
	// Update Center point
	CurrentPolyCenter.x = 0;
	CurrentPolyCenter.y = 0;
	for (vertex& p : *vertices) {
		CurrentPolyCenter.x += p.x;
		CurrentPolyCenter.y += p.y;
	}
	CurrentPolyCenter.x /= vertices->size();
	CurrentPolyCenter.y /= vertices->size();

	Point Pm = CurrentPolyCenter;
	SDL_RenderPoint(renderer, Pm.x, Pm.y);

	
	for (vertex& p : *vertices) {
		p.x -= Pm.x;
		p.y -= Pm.y;

		// Apply scale locally
		p.x *= scaleX;
		p.y *= scaleY;

		// Apply Rotation locally
		if (ECurrentMode == RotatePoly) {
			float tx = p.x;
			float ty = p.y;
			p.x = rotation_matrix[0][0] * tx + rotation_matrix[0][1] * ty;
			p.y = rotation_matrix[1][0] * tx + rotation_matrix[1][1] * ty;
		}

		p.x += Pm.x;
		p.y += Pm.y;
		
		// Apply Transform
		p.x += translation_matrix[0][2];
		p.y += translation_matrix[1][2];
	}

}

void SaveTransform(int polyIndex) {
	Point Pm = CurrentPolyCenter;
	for (Point& p : polygons[polyIndex]) {
		p.x -= Pm.x;
		p.y -= Pm.y;

		// Apply scale locally
		p.x *= scaleX;
		p.y *= scaleY;

		// Apply Rotation locally
		if (ECurrentMode == RotatePoly) {
			float tx = p.x;
			float ty = p.y;
			p.x = rotation_matrix[0][0] * tx + rotation_matrix[0][1] * ty;
			p.y = rotation_matrix[1][0] * tx + rotation_matrix[1][1] * ty;
		}

		p.x += Pm.x;
		p.y += Pm.y;

		// Apply Transform
		p.x += translation_matrix[0][2];
		p.y += translation_matrix[1][2];
	}
	// Apply for subpolygons
	for (std::vector<Point>& subPoly : sub_polygons[polyIndex]) {
		for (Point& sp : subPoly) {
			sp.x -= Pm.x;
			sp.y -= Pm.y;

			// Apply scale locally
			sp.x *= scaleX;
			sp.y *= scaleY;

			// Apply Rotation locally
			if (ECurrentMode == RotatePoly) {
				float tx = sp.x;
				float ty = sp.y;
				sp.x = rotation_matrix[0][0] * tx + rotation_matrix[0][1] * ty;
				sp.y = rotation_matrix[1][0] * tx + rotation_matrix[1][1] * ty;
			}

			sp.x += Pm.x;
			sp.y += Pm.y;

			// Apply Transform
			sp.x += translation_matrix[0][2];
			sp.y += translation_matrix[1][2];
		}
	}
	//translation_matrix[0][2] = 0;
	//translation_matrix[1][2] = 0;
}

void DrawLinesWithVertices(std::vector<vertex>& vertices, bool closed) {
	//Draw meshes
	//for (Mesh* m : GlobalMeshes) {
	//	std::vector<vertex> vertices = m->GetVertices();
	//	for (int i = 0; i < vertices.size() - 1; i++) {
	//		SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i + 1].x, vertices[i + 1].y);

	//		if (vertices.size() - 2 == i && m->IsClosed()) {
	//			SDL_RenderLine(renderer, vertices[i + 1].x, vertices[i + 1].y, vertices[0].x, vertices[0].y);
	//		}
	//	}
	//}

	if (vertices.size() < 2) { return; }

	// Apply translation to vertices apenas aqui
	//ApplyTranslation(&vertices);

	for (int i = 0; i < vertices.size() - 1; i++) {
		SDL_RenderLine(renderer, vertices[i].x, vertices[i].y, vertices[i+1].x, vertices[i+1].y);

		if (closed && vertices.size() - 2 == i) {
			SDL_RenderLine(renderer, vertices[i+1].x, vertices[i+1].y, vertices[0].x, vertices[0].y);
		}
	}
}

void DrawCurves(std::vector<Point> vertices, double level_of_detail) {
	if (vertices.size() < 2) { return; }
	
	for (int pi = 0; pi < vertices.size() - 1; pi++) {
		Point p0 = vertices[pi];
		Point p1 = vertices[pi + 1];
		Point t0 = {p0.x + 10, p0.y + 10};
		Point t1 = {p1.x + 50, p0.y - 50 };

		double last_x = -1;
		double last_y = -1;

		for (double i = 0; i <= 1.0; i+=level_of_detail) {
			// Gerar os Ts
			double ti = i;
			std::vector<double> T = {pow(ti, 3.0), pow(ti, 2.0), ti, 1};
			// Multiplicar as matrizes Q(t) = T.M.G
			double x = (
				( T[0] * hermite_matrix[0][0] + T[1] * hermite_matrix[1][0] + T[2] * hermite_matrix[2][0] + T[3] * hermite_matrix[3][0]) * p0.x)+
				((T[0] * hermite_matrix[0][1] + T[1] * hermite_matrix[1][1] + T[2] * hermite_matrix[2][1] + T[3] * hermite_matrix[3][1]) * t0.x)+
				((T[0] * hermite_matrix[0][2] + T[1] * hermite_matrix[1][2] + T[2] * hermite_matrix[2][2] + T[3] * hermite_matrix[3][2]) * p1.x)+
				((T[0] * hermite_matrix[0][3] + T[1] * hermite_matrix[1][3] + T[2] * hermite_matrix[2][3] + T[3] * hermite_matrix[3][3]) * t1.x);
			double y = (
				(T[0] * hermite_matrix[0][0] + T[1] * hermite_matrix[1][0] + T[2] * hermite_matrix[2][0] + T[3] * hermite_matrix[3][0]) * p0.y) +
				((T[0] * hermite_matrix[0][1] + T[1] * hermite_matrix[1][1] + T[2] * hermite_matrix[2][1] + T[3] * hermite_matrix[3][1]) * t0.y) +
				((T[0] * hermite_matrix[0][2] + T[1] * hermite_matrix[1][2] + T[2] * hermite_matrix[2][2] + T[3] * hermite_matrix[3][2]) * p1.y) +
				((T[0] * hermite_matrix[0][3] + T[1] * hermite_matrix[1][3] + T[2] * hermite_matrix[2][3] + T[3] * hermite_matrix[3][3]) * t1.y);

			//SDL_RenderPoint(renderer, x, y);
			if (last_x != -1 && last_y != -1) {
				SDL_RenderLine(renderer, x, y, last_x, last_y);
			}
			last_x = x;
			last_y = y;
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
				vertex v = {new_x, (float)y};
				intersecoes_v[y].push_back(v);
			}
			new_x += Tx;
			//new_r += Tr;
			//new_g += Tg;
			//new_b += Tb;
		}
	}
}

void FillPoly(std::vector<vertex>& vertices, int iPoly) {
	if (vertices.size() < 3) { return; }
	std::vector<std::vector<float>> intersecoes;
	std::vector<std::vector<vertex>> intersecoes_v;
	intersecoes.resize(HEIGHT);
	intersecoes_v.resize(HEIGHT);

	FindIntersections(vertices, intersecoes, intersecoes_v);

	/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
	//for (std::vector<Point> sub_vertices : sub_polygons[iPoly]) {
	//	if(sub_vertices.size() < 3) {break;}

	//	ApplyTransform(&sub_vertices);
	//	FindIntersections(sub_vertices, intersecoes);
	//}

	// Ordenar todas as interceções
	for (auto& lista : intersecoes) {
		std::sort(lista.begin(), lista.end());
	}
	// Ordernar interseções com cor
	for (auto& lista : intersecoes_v) {
		std::sort(lista.begin(), lista.end(), findLess);
	}

	// Fazer o fill no poly
	//for (int y = 0; y < intersecoes.size(); y++) {
	//	auto& lista = intersecoes[y];

	//	for (int i = 0; i+1 < lista.size(); i+=2) {

	//		SDL_RenderLine(renderer, lista[i], y, lista[i+1], y);
	//	}
	//}

	// Fazer o fill no poly com cor agr
	for (int y = 0; y < intersecoes_v.size(); y++) {
		auto& lista = intersecoes_v[y];

		for (int i = 0; i + 1 < lista.size(); i += 2) {
			//if (i + 1 < lista.size()) {
			//	SDL_RenderPoint(renderer, (int)lista[i].x, y);
			//	SDL_RenderPoint(renderer, (int)lista[i + 1].x, y);
			//}
			SDL_RenderLine(renderer, lista[i].x, y, lista[i + 1].x, y);
		}
	}

}

std::pair<Point, Point> getMinMaxPoint(int polyIndex) {
	if (polygons[polyIndex].size() <= 0) { return{ {0,0},{0,0} }; }
	float ymin = HEIGHT + 1;
	float xmin = WIDTH + 1;
	float ymax = -1;
	float xmax = -1;
	for (Point& p : polygons[polyIndex]) {
		if (p.y < ymin) {
			ymin = p.y;
		}
		if (p.x < xmin) {
			xmin = p.x;
		}
		if (p.y > ymax) {
			ymax = p.y;
		}
		if (p.x > xmax) {
			xmax = p.x;
		}
	}
	return { { xmin, ymin }, {xmax, ymax } };
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

	polygons.push_back({});
	sub_polygons.push_back({ {} });
	curves.push_back({});

	return SDL_APP_CONTINUE;
}

// Event callbacks
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){
	if (ImGui_ImplSDL3_ProcessEvent(event)) {
		//return SDL_APP_CONTINUE;
	}

	std::vector<vertex>* vertices = nullptr;
	if (iCurrentPoly < GlobalMeshes.size() && GlobalMeshes[iCurrentPoly] != nullptr) {
		vertices = &GlobalMeshes[iCurrentPoly]->GetVertices();
	}

	if (event->type == SDL_EVENT_QUIT) {
		std::cout << "Encerrando..." << std::endl;
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;

		// MODE SWITCH
		switch (e.key) {
		case SDLK_G: // Move polygons mode
			ECurrentMode = MovePoly;
			mouse_oX = 0;
			mouse_oY = 0;
			break;
		case SDLK_R: // Rotate polygons mode
			ECurrentMode = RotatePoly;
			mouse_oX = 0;
			break;
		case SDLK_S: // Scale polygons mode
			ECurrentMode = ScalePoly;
			mouse_oY = 1;
			mouse_oX = 1;
			break;
		case SDLK_9:
			if (ECurrentMode == RotatePoly) {
				scaleX = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
			}
			break;
		case SDLK_8:
			if (ECurrentMode == RotatePoly) {
				scaleY = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
			}
			break;
		}

		if (e.key == SDLK_SPACE) {
			bClosed = !bClosed;
			if (!GlobalMeshes.empty() && GlobalMeshes[0] != nullptr) {
				GlobalMeshes[0]->SetClosed(!GlobalMeshes[0]->IsClosed());
			}
		}

		if (e.key == SDLK_LCTRL) {
			bFill = !bFill;
			if (!GlobalMeshes.empty() && GlobalMeshes[0] != nullptr) {
				GlobalMeshes[0]->SetFill(!GlobalMeshes[0]->GetFill());
			}
		}

		if (ECurrentMode == MovePoly) {
			switch (e.key) {
			case SDLK_RIGHT:
				translation_matrix[0][2] += 10;
				mouse_oX = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
				break;
			case SDLK_LEFT:
				translation_matrix[0][2] -= 10;
				mouse_oX = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
				break;
			case SDLK_UP:
				translation_matrix[1][2] -= 10;
				mouse_oX = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
				break;
			case SDLK_DOWN:
				translation_matrix[1][2] += 10;
				mouse_oX = -1;
				SaveTransform(iCurrentPoly);
				ResetTransform();
				break;
			}
		}

		if (ECurrentMode == CreatePoly) {
			if (e.key == SDLK_KP_PLUS) {
				if (e.mod & SDL_KMOD_SHIFT) {
					iCurrentSubPoly++;
					if (sub_polygons[iCurrentPoly].size() < iCurrentSubPoly + 1) {
						sub_polygons[iCurrentPoly].push_back({});
					}
				}
				else if (e.mod & SDL_KMOD_ALT) {
					iCurrentCurve++;
					if (curves.size() < iCurrentCurve + 1) {
						curves.push_back({});
					}
				}
				else {
					iCurrentPoly++;
					iCurrentSubPoly = 0;

					if (GlobalMeshes.size() < iCurrentPoly + 1) {
						GlobalMeshes.push_back(new Mesh({ event->button.x, event->button.y }));
						sub_polygons.push_back({ {} });
					}
				}
			}

			if (e.key == SDLK_KP_MINUS) {
				if (e.mod & SDL_KMOD_SHIFT) {
					iCurrentSubPoly > 0 ? iCurrentSubPoly-- : iCurrentSubPoly;
				}
				else if (e.mod & SDL_KMOD_ALT) {
					iCurrentCurve > 0 ? iCurrentCurve-- : iCurrentCurve;
				}
				else {
					iCurrentPoly > POLY_MIN ? iCurrentPoly-- : iCurrentPoly;
					iCurrentSubPoly = 0;
				}
			}

			if (e.key == SDLK_BACKSPACE) {
				if (e.mod & SDL_KMOD_SHIFT) {
					if (sub_polygons[iCurrentPoly][iCurrentSubPoly].size() > 0) {
						sub_polygons[iCurrentPoly][iCurrentSubPoly].pop_back();
					}
				}
				else if (e.mod & SDL_KMOD_ALT) {
					if (curves[iCurrentCurve].size() > 0) {
						curves[iCurrentCurve].pop_back();
					}
				}
				else {
					if (polygons[iCurrentPoly].size() > 0) {
						polygons[iCurrentPoly].pop_back();
					}
				}

			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) { // LMB
			if (ECurrentMode != CreatePoly) {
				// Apply transformation and return to CreatePoly mode
				SaveTransform(iCurrentPoly);
				ResetTransform();

				ECurrentMode = CreatePoly;
			}
			else {
				if (SDL_GetModState() & SDL_KMOD_ALT) {
					AddVerticeToPoly({ e.x, e.y }, &curves[iCurrentCurve]);
				}
				else {
					//AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
					if (GlobalMeshes.empty()) {
						GlobalMeshes.push_back(new Mesh({e.x, e.y}));
					}
					else {
						GlobalMeshes[iCurrentPoly]->addVertex({e.x, e.y});
					}
					bLMB = true;
				}
			}

		}
		else if(e.button == 3){ // RMB
			if (ECurrentMode != CreatePoly) {
				// Cancel transformation and return to CreatePoly mode
				ResetTransform();
				ECurrentMode = CreatePoly;
			}
			else {
				AddVerticeToPoly({ e.x, e.y }, &sub_polygons[iCurrentPoly][iCurrentSubPoly]);
				bRMB = true;
			}
		}
	}else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) {
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

		// LOCK MOUSE INSIDE WINDOW
		if (ECurrentMode != CreatePoly) {
			int threshold = 20;
			if (e.y < threshold) {
				SDL_WarpMouseInWindow(window, e.x, HEIGHT - threshold);
			}
			else if (e.y > HEIGHT - threshold) {
				SDL_WarpMouseInWindow(window, e.x, threshold);
			}
			else if (e.x < threshold) {
				SDL_WarpMouseInWindow(window, WIDTH - threshold, e.y);
			}
			else if (e.x > WIDTH - threshold) {
				SDL_WarpMouseInWindow(window, threshold, e.y);
			}
		}

		if (ECurrentMode == MovePoly) {

			mouse_oX += event->motion.xrel * fMovePolySpeed;
			mouse_oY += event->motion.yrel * fMovePolySpeed;

			translation_matrix[0][2] = mouse_oX;
			translation_matrix[1][2] = mouse_oY;
		}
		
		else if (ECurrentMode == ScalePoly) {
			mouse_oY -= event->motion.yrel * fscalePolySpeed;

			scaleX = mouse_oY;
			scaleY = mouse_oY;
		}

		else if (ECurrentMode == RotatePoly) {
			mouse_oX += event->motion.xrel * frotatePolySpeed;

			rotation_matrix[0][0] = std::cos(mouse_oX);
			rotation_matrix[0][1] = -std::sin(mouse_oX);
			rotation_matrix[1][0] = std::sin(mouse_oX);
			rotation_matrix[1][1] = std::cos(mouse_oX);
		}

		else if (ECurrentMode == CreatePoly) {
			mouse_oX = -1;
			if (bLMB) {
				AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
			}
			else if (bRMB) {
				AddVerticeToPoly({ e.x, e.y }, &sub_polygons[iCurrentPoly][iCurrentSubPoly]);
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL) {
		SDL_MouseWheelEvent e = event->wheel;
		if (ECurrentMode == MovePoly) {
			fMovePolySpeed += fMovePolySpeed+ 0.01 * e.y >= 0 && fMovePolySpeed+ 0.01 * e.y <= 1.0 ? 0.01 * e.y : 0.0;
		}else if (ECurrentMode == ScalePoly) {
			fscalePolySpeed += fscalePolySpeed + 0.0005 * e.y >= 0 && fscalePolySpeed + 0.0005 * e.y <= 0.05 ? 0.0005 * e.y : 0.0;
			std::cout << fscalePolySpeed << std::endl;
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate){
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

	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);


	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
	/* IMGUI */
	
	SDL_SetRenderDrawColorFloat(renderer, 0.2, 0.2, 0.2, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
	SDL_RenderClear(renderer);
	
	const double now = ((double)SDL_GetTicks() * 0.001);  /* convert from milliseconds to seconds. */

	/* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
	const float red = (float)(0.5 + 0.5 * SDL_sin(now));
	const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
	const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

	// Draw Curves
	//for (int i = 0; i < curves.size(); i++) {
	//	auto curve = curves[i];
	//	if (i == iCurrentCurve) {
	//		SDL_SetRenderDrawColorFloat(renderer, 0, 1, 0, SDL_ALPHA_OPAQUE_FLOAT);
	//	}
	//	else {
	//		SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
	//	}

	//	DrawCurves(curve, 0.01);
	//}
	
	if (ImGui::BeginListBox("Meshes")) {
		// Draw polygons
		for (Mesh* m : GlobalMeshes) {
			std::vector<vertex>& vertices = m->GetVertices();
			if (vertices.empty()) { continue; }

			if (ImGui::Selectable("Mesh tal"), m->IsActive()) {

			}

			// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
			{
				ImGui::PushID(m);
				static float f = 0.0f;
				static int counter = 0;

				ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

				ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
				ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
				ImGui::Checkbox("Another Window", &show_another_window);
				ImGui::Checkbox("Fillpoly", &m->GetFill());
				ImGui::Checkbox("Closed", &m->IsClosed());
				ImGui::Checkbox("Active", &m->IsActive());

				ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
				ImGui::ColorEdit3("Mesh color", (float*)&m->GetColor()); // Edit 3 floats representing a color

				if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
					counter++;
				ImGui::SameLine();
				ImGui::Text("counter = %d", counter);

				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

				ImGui::End();
				ImGui::PopID();
			}

			if (m->IsActive()) {
				if (ECurrentMode != CreatePoly) { ApplyTransform(&vertices); }
				ImVec4 color = m->GetColor();
				SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, SDL_ALPHA_OPAQUE_FLOAT);
			}
			else {
				SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
			}

			DrawLinesWithVertices(vertices, m->IsClosed());

			if (m->GetFill() && ECurrentMode == CreatePoly) {
				FillPoly(vertices, 0);
			}
		}
		ImGui::EndListBox();
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