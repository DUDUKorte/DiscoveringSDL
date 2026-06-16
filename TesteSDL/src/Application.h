#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include "Layer.h"
#include "ImGuiLayer.h"
#include "LayerStack.h"

class Application {
public:
	Application();
	virtual ~Application();

	void Run();

	void OnEvent(SDL_Event& e);

	void PushLayer(Layer* layer);
	void PushOverlay(Layer* overlay);

	static inline Application& Get() { return *s_Instance; }
	inline SDL_Window& GetWindow() { return *m_Window; }

private:

	std::unique_ptr<SDL_Window> m_Window;
	ImGuiLayer* m_ImGuiLayer;
	bool m_Running = true;
	LayerStack m_LayerStack;

	static Application* s_Instance;
};

// to be defined in client
Application* CreateApplication();