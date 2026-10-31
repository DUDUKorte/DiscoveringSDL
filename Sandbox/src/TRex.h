#pragma once
#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <EditorCore/MeshLight.h>

class TRex: public Mesh {
public:
	TRex(std::string name = "Player");
	~TRex();

	virtual void BeginPlay();
	virtual void EventTick(float deltaTime);
	virtual void EventInput(SDL_Event* event);

private:
	float m_Time;
	float m_Speed;
	glm::vec3 scaleAnim;
	glm::vec3 initialScale;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;

	glm::vec3 moveAndSlide = { 0.0f, 0.0f, 0.0f };

	int Direction = 0;
	float Speed = 0.5f;
	float Jump = 1.2f;

	bool canJump = true;
};