#include "PointLight.h"

PointLightL::PointLightL(std::string name) : MeshLight(name)
{
	GetObj() = "./Assets/sphere.obj";

	GetTickEnabled() = true;
	// configurações do nosso sol
	GetColor() = { 0.9f, 0.7f, 0.5f, 1.0f }; // amarello
	//GetTransform().Position = { 1000.0f, 200.0f, 1000.0f }; // essa posição ficou massa
	GetTransform().Scale = { 5.0f, 5.0f, 5.0f }; // por algm motivo tudo é minúsculo aqui

	GetLightColor() = { 1.0f, 0.8f, 0.5f }; // cor branca
	GetLightIntensity() = 3.0f;
	GetMaxDistance() = 600.0f;

	m_StartPos = GetTransform().Position;
	m_EndPos = { 100.0f, 230.0f, -500.f };
}

PointLightL::~PointLightL()
{

}

void PointLightL::BeginPlay()
{
	m_Speed = 0.0005f;
	m_Time = 0.0f;

	SetVelocity({ -0.1f, 0.0f, 0.0f });
}

void PointLightL::EventTick(float deltaTime)
{
	//LE_INFO("TICK CHAMADO ; VELOCITY: " + std::to_string(Velocity().x) + ", " + std::to_string(Velocity().y) + ", " + std::to_string(Velocity().z));
	//m_Time += deltaTime * m_Speed;
	//float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	//float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);

	//GetTransform().Position = glm::mix(m_StartPos, m_EndPos, smoothFactor);
	//GetTransform().Position.y -= 80.0f * glm::sin(smoothFactor * glm::pi<float>());

	//EditorApp::Get()->GetBGColor() = glm::mix(bgColorStart, bgColorEnd, smoothFactor);
	//GetLightColor() = glm::mix(lightColorStart, lightColorEnd, smoothFactor);

	//GetTransform().Position += Velocity() * deltaTime;

	// Sempre colocar isso no final de cada tick
	MarkDirty();
}

void PointLightL::EventInput(SDL_Event* event)
{

}
