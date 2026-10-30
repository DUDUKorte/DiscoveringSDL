#include "Pipe.h"

Pipe::Pipe(std::string name, vertex firstVertex, std::vector<vertex> v)
    : Mesh2D(name, firstVertex, v)
{
    SetVelocity({-0.1f, 0.0f});
	SetTickEnabled(true);
	SetClosed(true);
	SetFill(false);
}

void Pipe::EventTick(float deltaTime)
{
	// Calcula a cada frame, seria a gravidade, colisões, etc...
	if (!GetTickEnabled()) { return; }
	// gravidadeeeeee
	GetTransform().Position.x += GetVelocity().x * deltaTime;
	GetTransform().Position.y += GetVelocity().y * deltaTime;

	if (GetTransform().Position.x < 1000) {
		// Auto deletar a mesh
		SetShouldDestroy(true);
	}

	//Tem q verificar colisão
	//Nem ideia de como fazer isso aqui

	// Última linha sempre, pra desenhar o polígono com o transform novo
	MarkDirty();
}

void Pipe::EventInput(SDL_Event* event)
{

}
