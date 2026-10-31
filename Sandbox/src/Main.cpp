#include <iostream>

#include <LeandroEditor.h>

#include "Sol.h"
#include "Cactus.h"
#include "CactusSpawner.h"
#include "TRex.h"
#include "PointLight.h"

class Sandbox : public LeandroApplication {
public:
	Sandbox() {

	}

	~Sandbox() {

	}

	void OnStart() {
		GetEditor()->GetBGColor() = {0.3f, 0.8f, 0.9f};

		Mesh* cubo = new Mesh("Cubin");
		cubo->GetColor() = { 1.0f, 0.7f, 0.5f, 1.0f };
		cubo->GetTransform().Position = { 640.0f, 620.0f, 1000.0f };
		cubo->GetTransform().Scale = { 800.0f, 100.0f, 350.0f };
		cubo->GetTransform().Rotation = { -25.0f, 0.0f, 0.0f };
		cubo->GetMaterial().diffuse = 1.0f;
		cubo->GetMaterial().ambientReflection = 1.0f;
		cubo->GetMaterial().specular = 1.0f;

		GetEditor()->LoadObjFile("./Assets/cubo.obj", cubo);
		GetEditor()->SubmitMesh(cubo);

		Sol* sol = new Sol();
		GetEditor()->SubmitMesh(sol);

		CactusSpawner* cactusSpawner = new CactusSpawner();
		GetEditor()->SubmitMesh(cactusSpawner);

		TRex* player = new TRex();
		GetEditor()->SubmitMesh(player);

	}

	void OnEvent(SDL_Event* event) {

	}

};

LeandroApplication* CreateApplication() {
	return new Sandbox();
}