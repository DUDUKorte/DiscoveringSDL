#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

#include "EditorApp.h"
#include "Mesh.h"
#include "Log.h"

std::vector<std::string> split(const std::string& str, char delim) {
	std::vector<std::string> items;
	std::stringstream sstream(str);
	std::string item;

	while (std::getline(sstream, item, delim))
		items.push_back(item);

	return items;
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	// Inicialiar o log
	LeandroEditor::Log::Init();
	LE_CORE_TRACE("INITIALIZING EDITOR...");

	EditorApp* app = new EditorApp();
	LE_CORE_ASSERT(app->Init(), "Falha ao inicializar o Editor!");
	*appstate = app;

	// TESTE
	// CARREGANDO UM ARQUIVO OBJ

	std::string path = "./cubo.obj";
	std::ifstream file;

	file.open(path, std::ifstream::in);
	LE_CORE_ASSERT(file.is_open(), "FALHA AO ABRIR O ARQUIVO OBJ!!!");

	std::string line;

	while(std::getline(file, line))
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
			}else if ("v" == prefix)
			{
				// vértices
				LE_CORE_TRACE("LOAD VERTICES");
				// pegar os vértices
				float x, y, z;
				sstream >> x >> y >> z;
				
				LE_CORE_INFO(std::to_string(x) + " ; " + std::to_string(y) + " ; " + std::to_string(z));

			}else if ("vn" == prefix)
			{
				// normals
				LE_CORE_TRACE("LOAD NORMALS");

			}else if ("vt" == prefix)
			{
				// texture coordenadas
				LE_CORE_TRACE("LOAD TEXTURE COORDS");

			}else if ("usemtl" == prefix)
			{
				LE_CORE_TRACE("USE MTL");

			}else if ("f" == prefix)
			{
				// Faces
				LE_CORE_TRACE("LOAD FACES");
				std::string indexGroup;

				while (sstream >> indexGroup)
				{
					std::vector<std::string> indices = split(indexGroup, '/');

					for (auto index : indices)
					{
						LE_CORE_INFO("FACE VERTEX: " + index);
					}

					if (1 == indices.size())
					{
						
					}else if (2 == indices.size())
					{
						
					}else if (3 == indices.size())
					{
						
					}else
					{
						// ERROR
					}
				}

			}else
			{
				continue;
			}
		}
	}

	file.close();

	// CARREGANDO UM ARQUIVO OBJ


	LE_CORE_INFO("EDITOR INITIALIZED!");
	return SDL_APP_CONTINUE;
}

// Event callbacks
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){
	EditorApp* app = static_cast<EditorApp*>(appstate);
	LE_CORE_ASSERT(app, "Falha ao resgatar appstate em AppEvent!");
	return app->ProcessEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate){
	EditorApp* app = static_cast<EditorApp*>(appstate);
	LE_CORE_ASSERT(app, "Falha ao resgatar appstate em AppIterate!");
	return app->UpdateAndRender();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
	// Cleanup
	LE_CORE_TRACE("CLOSING EDITOR...");

	EditorApp* app = static_cast<EditorApp*>(appstate);
	if (app) {
		app->Shutdown();
		delete app;
	}
}