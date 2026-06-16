#pragma once
#include "Layer.h"

class ImGuiLayer : public Layer {
public:
	ImGuiLayer();
	~ImGuiLayer();

	virtual void OnAttatch() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;

	void Begin();
	void End();

	//int MyGlfwKeyToImGuiKey(int keycode);

	float m_Time = 0.0f;
};