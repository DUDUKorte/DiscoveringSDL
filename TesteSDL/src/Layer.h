#pragma once
#include <string>

class Layer {
public:
	Layer(const std::string& name = "Layer");
	virtual ~Layer();

	virtual void OnAttatch() {}
	virtual void OnDetach() {}
	virtual void OnUpdate() {}
	//virtual void OnEvent(Event& event) {}
	virtual void OnImGuiRender() {}

	inline const std::string& GetName() const { return m_DebugName; }
protected:
	std::string m_DebugName;
};