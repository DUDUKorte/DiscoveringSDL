#pragma once

#include <sol/sol.hpp>
#include <string>

class BaseMesh;
union SDL_Event;

class ScriptEngine {
public:
	static void Init();
	static void Shutdown();

	static bool LoadMeshScript(BaseMesh* mesh, const std::string& filepath);
	static void OnBeginPlay(BaseMesh* mesh);
	static void OnTick(BaseMesh* mesh, float deltaTime);
	static void OnInput(BaseMesh* mesh, SDL_Event* event);

	static sol::state& GetLuaState() { return m_Lua; }

private:
	static void InitLuaBindings();
	static void PushSelf(sol::protected_function& func, BaseMesh* mesh);

	static sol::state m_Lua;
};