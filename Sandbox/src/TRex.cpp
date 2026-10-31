#include "TRex.h"

TRex::TRex(std::string name) : Mesh(name)
{
	GetObj() = "./Assets/dino.obj";

	GetTickEnabled() = true;
	GetInputEnabled() = true;
	// configurações do nosso sol
	GetColor() = { 0.4f, 0.4f, 0.4f, 1.0f }; // cinza
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
	m_Speed = 0.005f;
	m_Time = 0.0f;

	//GetTransform().Rotation.y = glm::radians(glm::linearRand(-180.0f, 180.0f));
	//GetTransform().Rotation.x = glm::radians(glm::linearRand(-10.0f, 10.0f));
	//GetTransform().Rotation.z = glm::radians(180.0f + glm::linearRand(-10.0f, 10.0f));

	//GetTransform().Position.y = GetTransform().Position.y + glm::linearRand(-15.0f, 25.0f);

	SetVelocity({ 0.0f, 0.01f, 0.0f });
}

void TRex::EventTick(float deltaTime)
{
	m_Time += m_Speed * deltaTime;
	//LE_INFO("TICK CHAMADO ; VELOCITY: " + std::to_string(Velocity().x) + ", " + std::to_string(Velocity().y) + ", " + std::to_string(Velocity().z));
	//m_Time += deltaTime * m_Speed;
	//float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	//float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);

	//GetTransform().Position = glm::mix(m_StartPos, m_EndPos, smoothFactor);
	//GetTransform().Position.y -= 80.0f * glm::sin(smoothFactor * glm::pi<float>());
	//GetTransform().Position += Velocity() * deltaTime;

	//float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	//float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);
	//GetTransform().Scale = glm::mix(initialScale, scaleAnim, smoothFactor);

	SetVelocity({glm::mix(Velocity().x, Direction * Speed, glm::clamp(0.01f * deltaTime, 0.0f, 1.0f)), Velocity().y, Velocity().z });

	GetTransform().Position += Velocity() * deltaTime;
	GetTransform().Position.x = glm::clamp(GetTransform().Position.x, 50.0f, 400.0f);
	
	if (GetTransform().Position.y + Velocity().y > m_StartPos.y) {
		canJump = true;

		while (GetTransform().Position.y < m_StartPos.y) {
			GetTransform().Position.y += 0.001f;
		}
		SetVelocity({Velocity().x, 0.0f, Velocity().z });
		m_Time = 0.0f;
	}
	else {
		SetVelocity({ Velocity().x, Velocity().y + 0.004f * deltaTime, Velocity().z });
		canJump = false;
	}

	// Sempre colocar isso no final de cada tick
	MarkDirty();
}

void TRex::EventInput(SDL_Event* event)
{
	LE_INFO("INPUT PROCESSES NO TREX");

	if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
		SDL_KeyboardEvent e = event->key;
		if (e.key == SDLK_D || e.key == SDLK_A) {
			Direction = (e.down && (e.key == SDLK_D)) - (e.down && (e.key == SDLK_A));
		}
		
		if ((e.key == SDLK_SPACE || e.key == SDLK_W) && canJump && e.down) {
//			GetTransform().Position.y -= Jump;
			SetVelocity({ Velocity().x, -Jump, Velocity().z });
		}
	}
	// gamepad
	if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
		 SDL_GamepadButtonEvent e = event->gbutton;
		if (e.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT || e.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) {
			Direction = (e.down && (e.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT)) - (e.down && (e.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT));
		}

		if (e.button == SDL_GAMEPAD_BUTTON_SOUTH && canJump && e.down) {
			//GetTransform().Position.y -= Jump;
			SetVelocity({ Velocity().x, -Jump, Velocity().z });
		}
	}

	if (event->type == SDL_EVENT_KEY_DOWN) {
		SDL_KeyboardEvent e = event->key;
		
	}

	if (event->type == SDL_EVENT_KEY_UP) {
		SDL_KeyboardEvent e = event->key;
		
	}
}
