#ifndef DISPLAY_APP_COM_H
#define DISPLAY_APP_COM_H

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <fstream>
#include <memory>
#include <vector>

using DestroyglfwWin = struct DestroyglfwWin {

	void operator()(GLFWwindow* ptr){
		glfwDestroyWindow(ptr);
	}

};

using UniqueGLFWWindow = std::unique_ptr<GLFWwindow, DestroyglfwWin>;

class app_com {
protected:
	UniqueGLFWWindow window{};
public:
	[[nodiscard]] bool windowShouldClose() const noexcept { return glfwWindowShouldClose(this->window.get()); };

	virtual void startFrame() noexcept { glfwPollEvents(); };
	virtual void drawFrame() noexcept = 0;
	virtual void endFrame() noexcept = 0;

	static std::vector<std::uint8_t> loadShader(const std::string& shaderFilename) {
		std::fstream file;

		std::ifstream input("shaders/" + shaderFilename + ".spv", std::ios::binary);

		if(input.good()) {
			std::vector<std::uint8_t> buffer(std::istreambuf_iterator<char>(input), {});

			return buffer;
		}

		throw std::runtime_error("Cannot load shader!");
	};
};


#endif //DISPLAY_APP_COM_H
