#include <cstdlib>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <app_vk.h>

#include <isdebug.h>

constexpr int WIDTH = 1920;
constexpr int HEIGTH = 1080;

void error_callback(int error, const char* description)
{
	spdlog::get("glfw")->error("Error {}: {}", error, description);
}

int main() {
	auto logger_glfw = spdlog::stdout_color_mt("glfw");
	auto logger_graphics = spdlog::stdout_color_mt("graphics");

	if constexpr (com::isDebug) {
		logger_glfw->set_level(spdlog::level::debug);
		logger_graphics->set_level(spdlog::level::debug);
	}

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		std::exit(EXIT_FAILURE);
	}

	{
		app app(WIDTH, HEIGTH);

		while (!app.windowShouldClose()) {
			app.startFrame();

			app.drawFrame();

			app.endFrame();
		}
	}

	glfwTerminate();

	return EXIT_SUCCESS;
}
