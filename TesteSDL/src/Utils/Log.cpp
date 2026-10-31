#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"

namespace LeandroEditor {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger = spdlog::stdout_color_mt("LEANDRO EDITOR");
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = spdlog::stdout_color_mt("EDITOR APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

}