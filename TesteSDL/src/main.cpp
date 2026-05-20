#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <vector>
#include <algorithm>

#define WIDTH 1280
#define HEIGHT 720

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

struct Point {
	float x;
	float y;
};

//std::vector<Point> vertices = {
//	Point(50, 25), Point(0, 0),
//	Point(50, 25), Point(30, 48),
//	Point(30, 48), Point(30, 65),
//	Point(0, 0), Point(30, 65),
//};

//std::vector<Point> vertices = {};

// Render data structures
std::vector<std::vector<Point>> curves;
std::vector<std::vector<Point>> polygons;
std::vector<std::vector<std::vector<Point>>> sub_polygons;
int iCurrentCurve = 0;
int iCurrentPoly = 0;
int iCurrentSubPoly = 0;

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




void AddVerticeToPoly(Point p, std::vector<Point> *vertices) {
	vertices->push_back(p);
}

void DrawLinesWithVertices(std::vector<Point> vertices, bool closed) {
	if (vertices.size() < 2) { return; }

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

void FillPoly(std::vector<Point>& vertices, int iPoly) {
	if (vertices.size() < 3) { return; }
	std::vector<std::vector<float>> intersecoes;
	intersecoes.resize(500);

	for (int i = 0; i < vertices.size(); i++) {
		//std::cout << "Source ( " << source.x << " , " << source.y << " ) " << std::endl;
		//std::cout << "Point ( " << point.x << " , " << point.y << " ) " << std::endl;
		Point source = vertices[i];
		Point point = vertices[(i + 1) % vertices.size()];

		if (point.y == source.y) { continue; }

		// Aqui eu tenho os dois pontos
		if (point.y < source.y) { // point é mais alto que source; faz troca
			std::swap(source, point);
		}

		int ymin = (int)source.y;
		int ymax = (int)point.y;
		
		if (ymax >= intersecoes.size()) {
			intersecoes.resize(ymax + 1);
		}

		float Tx = (point.x - source.x) / (point.y - source.y);
		float new_x = source.x;

		//std::cout << "ymin: " << ymin << " | ymax: " << ymax << " | Ns: " << Ns << " | Tx: " << Tx << " | new_y: " << new_y << " | new_x: " << new_x << std::endl;

		// Pegar as interseções
		for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
			//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
			//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
			//SDL_RenderPoint(renderer, new_x, ymin + i);

			intersecoes[y].push_back(new_x);
			new_x += Tx;
		}

	}

	/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
	for (std::vector<Point>& sub_vertices : sub_polygons[iPoly]) {
		if(sub_vertices.size() < 3) {break;}
		for (int i = 0; i < sub_vertices.size(); i++) {
			//std::cout << "Source ( " << source.x << " , " << source.y << " ) " << std::endl;
			//std::cout << "Point ( " << point.x << " , " << point.y << " ) " << std::endl;
			Point source = sub_vertices[i];
			Point point = sub_vertices[(i + 1) % sub_vertices.size()];

			if (point.y == source.y) { continue; }

			// Aqui eu tenho os dois pontos
			if (point.y < source.y) { // point é mais alto que source; faz troca
				std::swap(source, point);
			}

			int ymin = (int)source.y;
			int ymax = (int)point.y;

			if (ymax >= intersecoes.size()) {
				intersecoes.resize(ymax + 1);
			}

			float Tx = (point.x - source.x) / (point.y - source.y);
			float new_x = source.x;

			//std::cout << "ymin: " << ymin << " | ymax: " << ymax << " | Ns: " << Ns << " | Tx: " << Tx << " | new_y: " << new_y << " | new_x: " << new_x << std::endl;

			// Pegar as interseções
			for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
				//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
				//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
				//SDL_RenderPoint(renderer, new_x, ymin + i);

				intersecoes[y].push_back(new_x);
				new_x += Tx;
			}
		}
	}

	// Ordenar todas as interceções
	for (auto& lista : intersecoes) {
		std::sort(lista.begin(), lista.end());
	}

	// Fazer o fill no poly
	for (int y = 0; y < intersecoes.size(); y++) {
		auto& lista = intersecoes[y];

		for (int i = 0; i+1 < lista.size(); i+=2) {
			SDL_RenderLine(renderer, lista[i], y, lista[i+1], y);
		}
	}

}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	std::cout << "COMANDOS:" << std::endl;
	std::cout << "LMB: Desenha/cria pontos" << std::endl << "RMB: Desenha/cria pontos internos" << std::endl;
	std::cout << "Ctrl: Ativa/Desativa Fill Poly" << std::endl << "Space: Alterna poligono fechado/aberto" << std::endl;
	std::cout << "Plus: Adiciona/Seleciona proximo poligono" << "Minus: Seleciona poligono anterior" << std::endl;
	std::cout << "Shift+Plus: Adiciona/Seleciona proximo poligono interno" << std::endl << "Shift+Minus: Seleciona poligono interno anterior" << std::endl;

	SDL_SetAppMetadata("Example render", "1", "exampleidentifier");

	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindowAndRenderer("WindowTitle", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	polygons.push_back({});
	sub_polygons.push_back({ {} });
	curves.push_back({});

	return SDL_APP_CONTINUE;
}

// Event callbacks
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){
	if (event->type == SDL_EVENT_QUIT) {
		std::cout << "Encerrando..." << std::endl;
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;

		if (e.key == SDLK_SPACE) {
			bClosed = !bClosed;
		}

		if (e.key == SDLK_LCTRL) {
			bFill = !bFill;
		}
		if (e.key == SDLK_KP_PLUS) {
			if (e.mod & SDL_KMOD_SHIFT) {
				iCurrentSubPoly++;
				if (sub_polygons[iCurrentPoly].size() < iCurrentSubPoly+1) {
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

				if (polygons.size() < iCurrentPoly+1) {
					polygons.push_back({});
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
				iCurrentPoly > 0 ? iCurrentPoly-- : iCurrentPoly;
				iCurrentSubPoly = 0;
			}
		}

		if (e.key == SDLK_BACKSPACE) {
			if (e.mod & SDL_KMOD_SHIFT) {
				if (sub_polygons[iCurrentPoly][iCurrentSubPoly].size() > 0) {
					sub_polygons[iCurrentPoly][iCurrentSubPoly].pop_back();
				}
			}
			else if(e.mod & SDL_KMOD_ALT){
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
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		SDL_MouseButtonEvent e = event->button;
		if (e.button == 1) {
			if (SDL_GetModState() & SDL_KMOD_ALT) {
				AddVerticeToPoly({ e.x, e.y }, &curves[iCurrentCurve]);
			}
			else {
				AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
				bLMB = true;
			}
		}
		else if(e.button == 3){
			AddVerticeToPoly({ e.x, e.y }, &sub_polygons[iCurrentPoly][iCurrentSubPoly]);
			bRMB = true;
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

		if (bLMB) {
			AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
		}
		else if (bRMB) {
			AddVerticeToPoly({ e.x, e.y }, &sub_polygons[iCurrentPoly][iCurrentSubPoly]);
		}
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate){
	SDL_SetRenderDrawColorFloat(renderer, 0.2, 0.2, 0.2, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
	SDL_RenderClear(renderer);

	const double now = ((double)SDL_GetTicks() * 0.001);  /* convert from milliseconds to seconds. */

	/* choose the color for the frame we will draw. The sine wave trick makes it fade between colors smoothly. */
	const float red = (float)(0.5 + 0.5 * SDL_sin(now));
	const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
	const float blue = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));

	// Draw Curves
	for (int i = 0; i < curves.size(); i++) {
		auto curve = curves[i];
		if (i == iCurrentCurve) {
			SDL_SetRenderDrawColorFloat(renderer, 0, 1, 0, SDL_ALPHA_OPAQUE_FLOAT);
		}
		else {
			SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
		}

		DrawCurves(curve, 0.01);
	}
	
	// Draw polygons
	for (int i = 0; i < polygons.size(); i++) {
		auto poly = polygons[i];

		if (i == iCurrentPoly) {
			SDL_SetRenderDrawColorFloat(renderer, 0, 1, 0, SDL_ALPHA_OPAQUE_FLOAT);
		}
		else {
			SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
		}

		bIsCurrent = (i == iCurrentPoly);

		DrawLinesWithVertices(poly, bClosed);
		if (bFill) {
			FillPoly(poly, i);
		}
	}

	SDL_RenderPresent(renderer);
	
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
}