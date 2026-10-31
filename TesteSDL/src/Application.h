#pragma once

#include <iostream>
#include "EditorApp.h"
#include "EditorCore/Mesh2D.h"
#include "Utils/Log.h"

// roubei da minha IC
class LeandroApplication {
public:
	LeandroApplication();
	virtual ~LeandroApplication();

	void Run();

	virtual void OnStart() = 0;

	void OnEvent(SDL_Event* event);

	static inline LeandroApplication& Get() { return *s_Instance; }
	static inline EditorApp* GetEditor() { return s_Editor->Get(); }
	inline SDL_Window& GetWindow() { return *m_Window; }

private:
	bool OnWindowClose();

private:
	SDL_Window* m_Window;
	bool m_Running = true;
	float m_LastFrameTime = 0.0f;

	static LeandroApplication* s_Instance;
	static EditorApp* s_Editor;
};

LeandroApplication* CreateApplication();


