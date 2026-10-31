#include "CactusSpawner.h"

CactusSpawner::CactusSpawner(std::string name) : Mesh(name)
{
	GetTickEnabled() = true;

	m_Quanity = 10;
	glm::vec3 initialPos = { 1380.0f, 481.0f, 1250.0f };

	for (int i = 0; i < m_Quanity; i++) {
		Cactus* newCactus = new Cactus("Cactus Gerado n" + std::to_string(i));
		glm::vec3 randFactor = { glm::linearRand(-80.0f, 80.0f), 0.0f, glm::linearRand(-80.0f, 80.0f) };
		newCactus->GetTransform().Position = initialPos + randFactor;
		initialPos.x += glm::linearRand(80.0f, 250.0f);
		EditorApp::Get()->SubmitMesh(newCactus);

		newCactus->GetOutline() = false;
		newCactus->GetFill() = true;
		newCactus->BeginPlay();
		m_SpawnedCactus.push_back(newCactus);
	}
}

CactusSpawner::~CactusSpawner()
{

}

void CactusSpawner::BeginPlay()
{
	//m_Speed = 0.0005f;
	//m_Time = 0.0f;
	//SetVelocity({ -0.01f, 0.0f, 0.0f });
}

void CactusSpawner::EventTick(float deltaTime)
{
	//LE_INFO("TICK CHAMADO ; VELOCITY: " + std::to_string(Velocity().x) + ", " + std::to_string(Velocity().y) + ", " + std::to_string(Velocity().z));
	//m_Time += deltaTime * m_Speed;
	//float rawFactor = (glm::sin(m_Time) + 1.0f) * 0.5f;
	//float smoothFactor = glm::smoothstep(0.0f, 1.0f, rawFactor);

	//GetTransform().Position = glm::mix(m_StartPos, m_EndPos, smoothFactor);
	//GetTransform().Position.y -= 80.0f * glm::sin(smoothFactor * glm::pi<float>());

	// Sempre colocar isso no final de cada tick
	for (Cactus* cactu : m_SpawnedCactus) {
		if (cactu->GetTransform().Position.x < -20.0f) {
			cactu->GetTransform().Position.x = 1380.0f;
		}
	}

	MarkDirty();
}

void CactusSpawner::EventInput(SDL_Event* event)
{

}
