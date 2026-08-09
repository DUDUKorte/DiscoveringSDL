#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

// Melhorando o Mesh
#include "EditorApp.h"
#include "Mesh.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	EditorApp* app = new EditorApp();

	if (!app->Init()) {
		std::cerr << "Falha ao inicializar o Editor!" << std::endl;
		return SDL_APP_FAILURE;
	}

	*appstate = app;
	return SDL_APP_CONTINUE;
}

// Event callbacks
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event){
	EditorApp* app = static_cast<EditorApp*>(appstate);
	return app->ProcessEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate){
	EditorApp* app = static_cast<EditorApp*>(appstate);
	return app->UpdateAndRender();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
	// Cleanup
	EditorApp* app = static_cast<EditorApp*>(appstate);
	if (app) {
		app->Shutdown();
		delete app;
	}
}