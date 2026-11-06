#pragma once
#include <Utils/Log.h>
#include <Utils/LeandroMath.h>
#include <EditorCore/MeshLight.h>

class TRex: public Mesh {
public:
	TRex(std::string name = "Player");
	~TRex() override;

	virtual void BeginPlay() override;
	virtual void EventTick(float deltaTime) override;
	virtual void EventInput(SDL_Event* event) override;

	int CalculateDirection();
	int CalculateJump();

private:
	float m_Time;
	float m_Speed;
	glm::vec3 scaleAnim;
	glm::vec3 initialScale;

	glm::vec3 m_StartPos;
	glm::vec3 m_EndPos;
	glm::vec3 m_EndRot;

	int Direction = 0;
	float Speed = 0.5f;
	float Score = 0.0f;

	float JumpForce = 0.001f;
	float JumpImpulse = 1.2f;
	float JumpMin = 1.2f;
	float JumpAcc = 0.0f;

	float MaxJumpHeight = 150.0f;
	float LastYPos = -1.0f;

	bool canJump = true;
	bool isGrounded = false;
	bool wasJumpPressed = false;
	bool isAttacking = false;
	
	SDL_Gamepad* gamepad = nullptr;
};