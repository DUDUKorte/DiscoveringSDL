#include "TRex.h"

TRex::TRex(std::string name) : Mesh(name)
{
	GetObj() = "./Assets/dino.obj";

	GetTickEnabled() = true;
	GetInputEnabled() = true;
	// configurações do nosso sol
	GetColor() = { 0.4f, 0.4f, 0.4f }; // cinza
	GetTransform().Position = { 240.0f, 484.0f, 920.0f }; // essa posição ficou massa
	GetTransform().Scale = { 30.0f, 30.0f, 30.0f }; // por algm motivo tudo é minúsculo aqui
	GetTransform().Rotation = { glm::radians(20.0f), glm::radians(190.0f), glm::radians(180.0f) };

	//scaleAnim = { 260.0f, 320.0f, 260.0f };
	//initialScale = GetTransform().Scale;

	m_StartPos = GetTransform().Position;
	//m_EndPos = { 100.0f, 230.0f, -500.f };
}

TRex::~TRex()
{

}

void TRex::BeginPlay()
{
	EditorApp::Get()->PlaySound("./Assets/Test.wav");

	m_Speed = 0.005f;
	m_Time = 0.0f;

	//GetTransform().Rotation.y = glm::radians(glm::linearRand(-180.0f, 180.0f));
	//GetTransform().Rotation.x = glm::radians(glm::linearRand(-10.0f, 10.0f));
	//GetTransform().Rotation.z = glm::radians(180.0f + glm::linearRand(-10.0f, 10.0f));

	//GetTransform().Position.y = GetTransform().Position.y + glm::linearRand(-15.0f, 25.0f);

	SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void TRex::EventTick(float deltaTime)
{

	ImGui::Begin("SCORE");
	ImGui::Text((std::to_string(int(Score))).c_str());
	ImGui::End();

	Score += 0.01f * deltaTime;
	m_Time += m_Speed * deltaTime;

	// Ataque do trex
	if (isAttacking) {
		GetTransform().Rotation.y = glm::mix(GetTransform().Rotation.y, m_EndRot.y, glm::clamp(0.01f * deltaTime, 0.0f, 1.0f));
		if (glm::epsilonEqual(GetTransform().Rotation.y, m_EndRot.y, 0.1f)){
			GetTransform().Rotation.y = m_EndRot.y;
			isAttacking = false;
		}
	}

	// Verifica se está no chão
	isGrounded = GetTransform().Position.y >= m_StartPos.y;
	if (isGrounded) {
		Velocity().y = 0.0f;
	}

	// Movimento horizontal
	Direction = CalculateDirection();
	SetVelocity({glm::mix(Velocity().x, int(Direction) * Speed, glm::clamp(0.01f * deltaTime, 0.0f, 1.0f)), Velocity().y, Velocity().z });

	// Pulo dinâmico (tipo mario)
	int JumpPressed = CalculateJump();
	if (JumpPressed && isGrounded) { // Começa a pular
		SetVelocity({ Velocity().x, -JumpImpulse, Velocity().z });
		wasJumpPressed = true;
	}
	else if (!JumpPressed && wasJumpPressed) { // para o pulo
		wasJumpPressed = false;
		SetVelocity({ Velocity().x, -0.3f, Velocity().z });
	}

	
	if (GetTransform().Position.y + Velocity().y > m_StartPos.y && !isGrounded) {
		while (GetTransform().Position.y < m_StartPos.y) {
			GetTransform().Position.y += 0.001f;
		}

		canJump = true;
		m_Time = 0.0f;

		SetVelocity({Velocity().x, 0.0f, Velocity().z });
	}
	else if(!isGrounded) {
		// gravidade
		SetVelocity({ Velocity().x, Velocity().y + 0.004f * deltaTime, Velocity().z });
	}

	GetTransform().Position += Velocity() * deltaTime;
	GetTransform().Position.x = glm::clamp(GetTransform().Position.x, 50.0f, 400.0f);

	// Sempre colocar isso no final de cada tick
	MarkDirty();
}

void TRex::EventInput(SDL_Event* event)
{
	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;
		if (e.key == SDLK_P) {
			EditorApp::Get()->PlaySound("./Assets/Test2.wav");
		}

		if (e.key == SDLK_F && !e.repeat && !isAttacking) {
			isAttacking = true;
			m_EndRot = GetTransform().Rotation + glm::vec3({0.0f, glm::radians(360.0f), 0.0f});
		}
	}

	// gamepad connection
	if ((event->type == SDL_EVENT_GAMEPAD_ADDED || event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION || event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) && !gamepad) {
		SDL_GamepadButtonEvent e = event->gbutton;

		int numgamepad = 0;
		SDL_JoystickID* gamepads = SDL_GetGamepads(&numgamepad);
		
		gamepad = SDL_GetGamepadFromID(gamepads[0]);
		SDL_free(gamepads);
	}
	else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
		gamepad = nullptr;
	}
}

int TRex::CalculateDirection()
{
	int right = (EditorApp::Get()->GetIsKeyPressed(SDL_SCANCODE_A) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT));
	int left = (EditorApp::Get()->GetIsKeyPressed(SDL_SCANCODE_D) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
	return left - right;
}

int TRex::CalculateJump()
{
	int jumpPressed = (EditorApp::Get()->GetIsKeyPressed(SDL_SCANCODE_SPACE) || EditorApp::Get()->GetIsKeyPressed(SDL_SCANCODE_W) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH));
	return jumpPressed;
}
