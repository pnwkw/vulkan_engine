#ifndef DISPLAY_APP_VK_H
#define DISPLAY_APP_VK_H

#include <app_com.h>
#include "engine_vk.h"
#include "triangle_renderer.h"

class app_vk : public app_com {
private:
	std::unique_ptr<engine_vk> engine;
	std::unique_ptr<triangle_renderer> field;

public:
	app_vk(int width, int height);
	~app_vk();

	void startFrame() noexcept override;
	void drawFrame() noexcept override;
	void endFrame() noexcept override;

};

using app = app_vk;

#endif //DISPLAY_APP_VK_H
