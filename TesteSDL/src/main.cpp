#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <Random>

#define WIDTH 1280
#define HEIGHT 720
#define GRAVITY 0.01
#define POLY_MIN 5
#define MARGIN 100


Uint64 new_deltaTime;
Uint64 old_deltaTime = 0;
double deltaTime = 0;

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

struct Point {
	float x;
	float y;
};

struct Vec2 {
	float x;
	float y;
};

enum EEditMode
{
	CreatePoly = 0, MovePoly = 1, ScalePoly, RotatePoly
};

float fMovePolySpeed = 0.1;
float fscalePolySpeed = 0.01;
float frotatePolySpeed = 0.01;

// Editor mode
EEditMode ECurrentMode = CreatePoly;

// Render data structures
std::vector<std::vector<Point>> curves;
std::vector<std::vector<Point>> polygons;
std::vector<Vec2> polygons_velocity;

std::vector<std::vector<std::vector<Point>>> sub_polygons;
int iCurrentCurve = 0;
int iCurrentPoly = POLY_MIN;
int iCurrentSubPoly = 0;
Point CurrentPolyCenter = {0,0};

// Render control
bool bUseSubPoly = false;
bool bIsCurrent = false;
bool bFill = false;
bool bClosed = false;
bool bGravity = false;

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

void ApplyTransform(std::vector<Point>* vertices) {
	// Update Center point
	CurrentPolyCenter.x = 0;
	CurrentPolyCenter.y = 0;
	for (Point& p : *vertices) {
		CurrentPolyCenter.x += p.x;
		CurrentPolyCenter.y += p.y;
	}
	CurrentPolyCenter.x /= vertices->size();
	CurrentPolyCenter.y /= vertices->size();

	Point Pm = CurrentPolyCenter;
	SDL_RenderPoint(renderer, Pm.x, Pm.y);

	
	for (Point& p : *vertices) {
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

void DrawLinesWithVertices(std::vector<Point> vertices, bool closed) {
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

void FindIntersections(std::vector<Point>& vertices, std::vector<std::vector<float>>& intersecoes) {
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

		//if (ymax >= intersecoes.size() && ymin > 0) {
		//	intersecoes.resize(ymax + 1);
		//}

		float Tx = (point.x - source.x) / (point.y - source.y);
		float new_x = source.x;

		//std::cout << "ymin: " << ymin << " | ymax: " << ymax << " | Ns: " << Ns << " | Tx: " << Tx << " | new_y: " << new_y << " | new_x: " << new_x << std::endl;

		// Pegar as interseções
		for (int y = ymin; y < ymax; y++) { // Scanlines de 0 à Ns-1
			//SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
			//SDL_RenderLine(renderer, new_x, ymin + i, new_x, ymin+i);
			//SDL_RenderPoint(renderer, new_x, ymin + i);
			if (y > 0 && y < HEIGHT) {
				intersecoes[y].push_back(new_x);
			}
			new_x += Tx;
		}
	}
}

void FillPoly(std::vector<Point>& vertices, int iPoly) {
	if (vertices.size() < 3) { return; }
	std::vector<std::vector<float>> intersecoes;
	intersecoes.resize(HEIGHT);

	FindIntersections(vertices, intersecoes);

	/*Sub Polygons adicionados às interseções (talvez não seja a melhor forma de fazer, mas é uma forma)*/
	for (std::vector<Point> sub_vertices : sub_polygons[iPoly]) {
		if(sub_vertices.size() < 3) {break;}

		ApplyTransform(&sub_vertices);
		FindIntersections(sub_vertices, intersecoes);
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


// Funciona +-

void CheckCollisions() {
	float y_offset = 0.0f;

	for (int i = 0; i < POLY_MIN-1; i+=2) {
		if (polygons[i].size() <= 0) { continue; }

		std::pair<Point, Point> minMaxPoints = getMinMaxPoint(i);
		Point minP = minMaxPoints.first;
		Point maxP = minMaxPoints.second;
		if (maxP.x < 0) {
			translation_matrix[0][2] = WIDTH;
			y_offset = (rand() % 400) * ((rand() % 100) > 50 ? -1 : 1);
			if (maxP.y + y_offset + MARGIN > 1000) {
				y_offset = 0;
			}
			else if (maxP.y + y_offset < 100) {
				y_offset = 0;
			}
			translation_matrix[1][2] = y_offset;
			SaveTransform(i);
			SaveTransform(i+1);
			ResetTransform();
		}
	}
}

void ApplyPhysics(int polyIndex) {
	if (polygons[polyIndex].size() <= 0 || polyIndex == iCurrentPoly) { return; }
	bool isCollision = polyIndex < POLY_MIN;
	CheckCollisions();

	//translation_matrix[0][2] += GRAVITY;
	//translation_matrix[1][2] += GRAVITY * deltaTime;
	if (!isCollision) {
		polygons_velocity[polyIndex].y += GRAVITY * deltaTime;
	}
	//Direction vector based on gravity
//	float y_velocity = (polygons[polyIndex][0].y + translation_matrix[1][2]) - polygons[polyIndex][0].y;
//	float x_velocity = (polygons[polyIndex][0].x + translation_matrix[1][2]) - polygons[polyIndex][0].x;
	//std::cout << y_velocity << std::endl;
	Point Pmax = getMinMaxPoint(polyIndex).second;
	float ymax = Pmax.y;
	// Verify Collision
	if (ymax + polygons_velocity[polyIndex].y > HEIGHT && !isCollision) {
		while (polygons_velocity[polyIndex].y > 0) {
			polygons_velocity[polyIndex].y -= 0.1;
		}
		if (polygons_velocity[polyIndex].y <= 0) {
			polygons_velocity[polyIndex].y = 0;
		}
	}

	translation_matrix[1][2] = polygons_velocity[polyIndex].y * deltaTime;
	translation_matrix[0][2] = polygons_velocity[polyIndex].x * deltaTime;
	SaveTransform(polyIndex);
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

	SDL_SetAppMetadata("Example render", "1", "exampleidentifier");

	SDL_Init(SDL_INIT_VIDEO);

	SDL_CreateWindowAndRenderer("WindowTitle", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer);

	SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	float collisions_velocity = 0.8;
	// Add initial collisions
	polygons.push_back({ Point{ WIDTH - 100, -HEIGHT}, Point{ WIDTH - 10, -HEIGHT }, Point{ WIDTH - 10, HEIGHT / 2 -MARGIN}, Point{ WIDTH - 100, HEIGHT / 2 - MARGIN} });
	polygons_velocity.push_back({ Vec2{-collisions_velocity, 0} });
	sub_polygons.push_back({ {} });

	polygons.push_back({ Point{ WIDTH - 100, HEIGHT*2 }, Point{ WIDTH - 10, HEIGHT*2 }, Point{ WIDTH - 10, HEIGHT / 2 + MARGIN }, Point{ WIDTH - 100, HEIGHT / 2 + MARGIN} });
	polygons_velocity.push_back({ Vec2{-collisions_velocity, 0} });
	sub_polygons.push_back({ {} });

	while (polygons.size() < POLY_MIN) {
		polygons.push_back({});
		polygons_velocity.push_back({ Vec2{0,0} });
		sub_polygons.push_back({ {} });
	}

	polygons.push_back({});
	polygons_velocity.push_back({ Vec2{0,0} });
	sub_polygons.push_back({ {} });
	curves.push_back({});

	new_deltaTime = SDL_GetPerformanceCounter();
	old_deltaTime = 0;
	deltaTime = 0;

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
		case SDLK_P:
			bGravity = !bGravity;
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
		}

		if (e.key == SDLK_LCTRL) {
			bFill = !bFill;
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
			if (e.key == SDLK_J) {
				if (polygons.size() == POLY_MIN + 2) {
					polygons_velocity[POLY_MIN].y -= 3;
				}
			}
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

					if (polygons.size() < iCurrentPoly + 1) {
						polygons.push_back({});
						polygons_velocity.push_back({ Vec2{0,0} });
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
					AddVerticeToPoly({ e.x, e.y }, &polygons[iCurrentPoly]);
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
	old_deltaTime = new_deltaTime;
	new_deltaTime = SDL_GetPerformanceCounter();
	deltaTime = (double)((new_deltaTime - old_deltaTime) * 1000 / (double)SDL_GetPerformanceFrequency());

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
		bIsCurrent = (i == iCurrentPoly);

		if (bIsCurrent) {
			if (ECurrentMode != CreatePoly) { ApplyTransform(&poly); }
			
			SDL_SetRenderDrawColorFloat(renderer, 0, 1, 0, SDL_ALPHA_OPAQUE_FLOAT);
		}
		else {
			SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);  /* new color, full alpha. */
		}
		
		if (bGravity && ECurrentMode == CreatePoly) {
			ApplyPhysics(i);
		}

		DrawLinesWithVertices(poly, bClosed);
		if (bFill && ECurrentMode == CreatePoly) {
			FillPoly(poly, i);
		}
	}

	SDL_RenderPresent(renderer);
	
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
}