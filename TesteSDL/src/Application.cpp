#include "Application.h"

LeandroApplication* LeandroApplication::s_Instance = nullptr;
EditorApp* LeandroApplication::s_Editor = nullptr;

LeandroApplication::LeandroApplication()
{
	s_Instance = this;
	s_Editor = new EditorApp();
}

LeandroApplication::~LeandroApplication()
{

}

void LeandroApplication::OnEvent(SDL_Event* event)
{
	// Lógica de eventos da aplicação
}