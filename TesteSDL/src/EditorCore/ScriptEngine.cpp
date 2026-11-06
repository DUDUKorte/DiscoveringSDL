#include "ScriptEngine.h"
#include "BaseMesh.h"
#include "Mesh.h"
#include "MeshLight.h"
#include "EditorApp.h"
#include <Utils/Log.h>

sol::state ScriptEngine::m_Lua;

void ScriptEngine::Init() {
	InitLuaBindings();
}

void ScriptEngine::Shutdown() {
	// Limpa estados se necessário
}

bool ScriptEngine::LoadMeshScript(BaseMesh* mesh, const std::string& filepath) {
	if (filepath.empty()) return false;


	// Cria um ambiente isolado para essa mesh que herda as funções globais (vec3, math, etc.)
	sol::environment env(m_Lua, sol::create, m_Lua.globals());
	
	// Atribui as variáveis primeiro pra n...
	mesh->GetLuaEnv() = env;
	mesh->GetScriptPath() = filepath;

	// Compila e carrega o arquivo .lua de forma segura // aka: n vai crashar
	auto result = m_Lua.safe_script_file(filepath, env, sol::script_pass_on_error);
	if (!result.valid()) {
		sol::error err = result;
		LE_CORE_ERROR(std::string("Erro ao compilar Lua (") + filepath + "): " + err.what());
		return false;
	}

	LE_CORE_INFO("Script Lua compilado com sucesso: " + filepath);
	return true;
}

void ScriptEngine::OnBeginPlay(BaseMesh* mesh) {
	if (!mesh->GetLuaEnv().valid()) return;

	sol::protected_function func = mesh->GetLuaEnv()["BeginPlay"];
	if (func.valid()) {
		sol::protected_function_result res;
		if (mesh->GetType() == MeshTypes::TypeLight)
			res = func(static_cast<MeshLight*>(mesh));
		else if (mesh->GetType() == MeshTypes::Type3D)
			res = func(static_cast<Mesh*>(mesh));
		else
			res = func(mesh);

		if (!res.valid()) {
			sol::error err = res;
			LE_CORE_ERROR(std::string("Erro Lua no BeginPlay: ") + err.what());
		}
	}
}

void ScriptEngine::OnTick(BaseMesh* mesh, float deltaTime) {
	if (!mesh->GetLuaEnv().valid()) return;

	sol::protected_function func = mesh->GetLuaEnv()["EventTick"];
	if (func.valid()) {
		sol::protected_function_result res;
		if (mesh->GetType() == MeshTypes::TypeLight)
			res = func(static_cast<MeshLight*>(mesh), deltaTime);
		else if (mesh->GetType() == MeshTypes::Type3D)
			res = func(static_cast<Mesh*>(mesh), deltaTime);
		else
			res = func(mesh, deltaTime);

		if (!res.valid()) {
			sol::error err = res;
			LE_CORE_ERROR(std::string("Erro Lua no EventTick: ") + err.what());
		}
	}
}

void ScriptEngine::OnInput(BaseMesh* mesh, SDL_Event* event) {
	if (!mesh->GetLuaEnv().valid()) return;

	sol::protected_function func = mesh->GetLuaEnv()["EventInput"];
	if (func.valid() && event->type == SDL_EVENT_KEY_DOWN) {
		// Passa o nome da tecla (ex: "W", "Space") ou o keycode pro Lua
		const char* keyName = SDL_GetKeyName(event->key.key);
		auto res = func(mesh, std::string(keyName));
		if (!res.valid()) {
			sol::error err = res;
			LE_CORE_ERROR(std::string("Erro Lua no EventInput: ") + err.what());
		}
	}
}

void ScriptEngine::InitLuaBindings()
{
	LE_CORE_TRACE("Binding Lua Classes....");
	m_Lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

	// 1. Bind do glm::vec3 (com construtores e operadores matemáticos + - *)
	m_Lua.new_usertype<glm::vec3>("vec3",
		sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
		"x", &glm::vec3::x,
		"y", &glm::vec3::y,
		"z", &glm::vec3::z,
		sol::meta_function::addition, [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
		sol::meta_function::subtraction, [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
		sol::meta_function::multiplication, [](const glm::vec3& a, float b) { return a * b; }
	);

	// 2. Funções auxiliares de GLM (mix e smoothstep que você usou no Sol.cpp)
	m_Lua.set_function("lerp_vec3", [](const glm::vec3& a, const glm::vec3& b, float t) {
		return glm::mix(a, b, t);
		});
	m_Lua.set_function("smoothstep", [](float edge0, float edge1, float x) {
		return glm::smoothstep(edge0, edge1, x);
		});

	// 3. Bind do Transform
	m_Lua.new_usertype<Transform>("Transform",
		"Position", &Transform::Position,
		"Rotation", &Transform::Rotation,
		"Scale", &Transform::Scale
	);

	// 4. Bind da BaseMesh / Mesh / MeshLight
	m_Lua.new_usertype<BaseMesh>("BaseMesh",
		"GetTransform", &BaseMesh::GetTransform,
		"GetVelocity", [](BaseMesh& b) {return b.GetVelocity(); },
		"SetVelocity", [](BaseMesh& b, const glm::vec3& c) { b.GetVelocity() = c; },
		"MarkDirty", &BaseMesh::MarkDirty,
		"SetColor", [](BaseMesh& m, const glm::vec3& c) { m.GetColor() = glm::vec3(c.x, c.y, c.z); },
		"SetObj", [](BaseMesh& l, const std::string& c) { l.GetObj() = c; }
	);

	//// 4. Bind da BaseMesh / Mesh / MeshLight
	m_Lua.new_usertype<Mesh>("Mesh",
		sol::base_classes, sol::bases<BaseMesh>() // Permite usar tudo de BaseMesh!
	);

	m_Lua.new_usertype<MeshLight>("MeshLight",
		sol::base_classes, sol::bases<Mesh, BaseMesh>(), // Permite usar tudo de BaseMesh!
		"SetLightColor", [](MeshLight& l, const glm::vec3& c) { l.GetLightColor() = c; },
		"GetLightColor", [](MeshLight& l) { return l.GetLightColor(); },
		"SetIntensity", [](MeshLight& l, float i) { l.GetLightIntensity() = i; }
	);

	// 5. Funções globais do Editor (ex: mudar cor de fundo ou esconder cursor)
	m_Lua.set_function("SetBGColor", [](const glm::vec3& c) { EditorApp::Get()->GetBGColor() = c; });
	m_Lua.set_function("SetHideCursor", [](bool hide) { EditorApp::Get()->SetHideCursor(hide); });
	m_Lua.set_function("IsKeyPressed", [](int scancode) { return EditorApp::Get()->GetIsKeyPressed((SDL_Scancode)scancode); });

	LE_CORE_INFO("Script Engine Setup Complete!");
}