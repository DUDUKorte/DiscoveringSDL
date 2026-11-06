#pragma once
#include "EditorApp.h"

// roubei da minha IC
class LeandroApplication {
public:
	LeandroApplication();
	virtual ~LeandroApplication();

	virtual void OnStart() = 0;

	virtual void OnTick(double deltaTime) = 0;
	virtual void OnEvent(SDL_Event* event) = 0;

	static inline LeandroApplication& Get() { return *s_Instance; }
	static inline EditorApp* GetEditor() { return s_Editor->Get(); }
	inline SDL_Window& GetWindow() const { return *m_Window; }

private:
	SDL_Window* m_Window;
	bool m_Running = true;
	float m_LastFrameTime = 0.0f;

	static LeandroApplication* s_Instance;
	static EditorApp* s_Editor;
};

LeandroApplication* CreateApplication();


