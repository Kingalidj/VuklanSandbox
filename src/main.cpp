#include "vk_engine.h"

#include "Logger.h"

int main() {

	VulkanEngine app;

	app.init();
	app.run();
	app.cleanup();

	return 0;
}
