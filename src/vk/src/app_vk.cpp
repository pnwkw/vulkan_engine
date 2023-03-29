#include "app_vk.h"

#include <spdlog/spdlog.h>

app_vk::app_vk(int width, int height) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	this->window = UniqueGLFWWindow(glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr));

	if (!this->window) {
		spdlog::get("glfw")->error("Can not open window!");
		exit(EXIT_FAILURE);
	}

	this->engine = std::make_unique<engine_vk>(this->window.get());
	this->field = std::make_unique<triangle_renderer>(*this->engine);
}

app_vk::~app_vk() {
	this->engine->waitDeviceIdle();
}

void app_vk::startFrame() noexcept {
	app_com::startFrame();
	this->field->startFrame();
}

void app_vk::drawFrame() noexcept {
	this->field->drawFrame();
}

void app_vk::endFrame() noexcept {
	this->field->endFrame();
}
