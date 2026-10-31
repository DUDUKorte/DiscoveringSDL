#include "Cactus.h"

Cactus::Cactus(std::string name) : Mesh(name)
{
	GetObj() = "./Assets/cactus.obj";

	GetTickEnabled() = true;
	// configurações do nosso sol
	GetColor() = { 0.2f, 0.9f, 0.2f, 1.0f }; // amarello
	GetTransform().Position = { 640.0f, 481.0f, 0.0f }; // essa posição ficou massa
	GetTransform().Scale = { 280.0f, 280.0f, 280.0f }; // por algm motivo tudo é minúsculo aqui
	GetTransform().Rotation = { 0.0f, 0.0f, glm::radians(180.0f) };

	scaleAnim = { 260.0f, 320.0f, 260.0f };
	initialScale = GetTransform().Scale;

	m_StartPos = GetTransform().Position;
	m_EndPos = { 100.0f, 230.0f, -500.f };

	myLight = nullptr;

	// Criar uma point light com uma chance
	if (glm::linearRand(0.0f, 1.0f) <= 0.3) {
		myLight = new PointLightL();
		myLight->GetTransform().Position = GetTransform().Position;
		myLight->GetTransform().Position += glm::vec3({0.0f, -15.0f, 0.0f});

		EditorApp::Get()->SubmitMesh(myLight);
		myLight->BeginPlay();
	}
}

Cactus::~Cactus()
{

}

void Cactus::BeginPlay()
{
	m_Speed = 0.005f;
	m_Time = 0.0f;

	GetTransform().Rotation.y = glm::radians(glm::linearRand(-180.0f, 180.0f));
	GetTransform().Rotation.x = glm::radians(glm::linearRand(-10.0f, 10.0f));
	GetTransform().Rotation.z = glm::radians(180.0f + glm::linearRand(-10.0f, 10.0f));

	GetTransform().Position.y = GetTransform().Position.y + glm::linearRand(-15.0f, 25.0f);

	SetVelocity({ -0.1f, 0.0f, 0.0f });
}

void Cactus::EventTick(float deltaTime)
{
	m_Time += m_Speed * deltaTime;
	//LE_INFO("TICK CHAMADO ; VELOCITY: " + std::to_string(Velocity().x) + ", " + std::to_string(Velocity().y) + ", " + std::to_string(Velocity().z));
	//m_Time += deltaTime * m_Speed;
	//float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	//float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);

	//GetTransform().Position = glm::mix(m_StartPos, m_EndPos, smoothFactor);
	//GetTransform().Position.y -= 80.0f * glm::sin(smoothFactor * glm::pi<float>());
	GetTransform().Position += Velocity() * deltaTime;

	float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);
	GetTransform().Scale = glm::mix(initialScale, scaleAnim, smoothFactor);

	if (myLight) {
		myLight->GetTransform().Position = GetTransform().Position + glm::vec3({0.0f, -50.0f, 0.0f});
	}

	// Sempre colocar isso no final de cada tick
	MarkDirty();
}

void Cactus::EventInput(SDL_Event* event)
{

}
