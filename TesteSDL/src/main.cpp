#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>

#include "EditorApp.h"
#include "Mesh.h"
#include "Log.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	// Inicialiar o log
	LeandroEditor::Log::Init();
	LE_CORE_TRACE("INITIALIZING EDITOR...");

	EditorApp* app = new EditorApp();
	LE_CORE_ASSERT(app->Init(), "Falha ao inicializar o Editor!");
	*appstate = app;

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