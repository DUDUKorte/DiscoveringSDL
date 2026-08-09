#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace LeandroEditor {
	class Log {
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#define LE_CORE_TRACE(...) ::LeandroEditor::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LE_CORE_INFO(...)  ::LeandroEditor::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LE_CORE_WARN(...)  ::LeandroEditor::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LE_CORE_ERROR(...) ::LeandroEditor::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LE_CORE_CRITICAL(...) ::LeandroEditor::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define LE_TRACE(...)      ::LeandroEditor::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LE_INFO(...)       ::LeandroEditor::Log::GetClientLogger()->info(__VA_ARGS__)
#define LE_WARN(...)       ::LeandroEditor::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LE_ERROR(...)      ::LeandroEditor::Log::GetClientLogger()->error(__VA_ARGS__)
#define LE_FATAL(...)      ::LeandroEditor::Log::GetClientLogger()->fatal(__VA_ARGS__)

//ASSERTS
#define LE_ASSERT(x, ...) { if(!(x)) { LE_ERROR("Assertion Failed: {0}", __VA_ARGS__);__debugbreak(); } }
#define LE_CORE_ASSERT(x, ...) { if(!(x)) { LE_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__);__debugbreak(); } }