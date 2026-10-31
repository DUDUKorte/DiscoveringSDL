#pragma once

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "Application.h"

extern LeandroApplication* CreateApplication();

inline SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	auto app = CreateApplication();

	// Inicialiar o log
	LeandroEditor::Log::Init();
	LE_CORE_TRACE("INITIALIZING EDITOR...");

	LE_CORE_ASSERT(app->GetEditor()->Init(), "Falha ao inicializar o Editor!");
	*appstate = app;
	app->OnStart();

	LE_CORE_INFO("EDITOR INITIALIZED!");
	return SDL_APP_CONTINUE;
}

// Event callbacks
inline SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	EditorApp* app = static_cast<LeandroApplication*>(appstate)->GetEditor();
	LE_CORE_ASSERT(app, "Falha ao resgatar appstate em AppEvent!");
	static_cast<LeandroApplication*>(appstate)->OnEvent(event);
	return app->ProcessEvent(event);
}

inline SDL_AppResult SDL_AppIterate(void* appstate) {
	EditorApp* app = static_cast<LeandroApplication*>(appstate)->GetEditor();
	LE_CORE_ASSERT(app, "Falha ao resgatar appstate em AppIterate!");
	return app->UpdateAndRender();
}

inline void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	//quitar
	// Cleanup
	LE_CORE_TRACE("CLOSING EDITOR...");
	LeandroApplication* app = static_cast<LeandroApplication*>(appstate);
	EditorApp* editor = app->GetEditor();
	if (editor) {
		editor->Shutdown();
		delete editor;
	}

	if (app) {
		delete app;
	}
}
