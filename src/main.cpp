#include "vk_engine.h"

int main() {

	VulkanEngine app;

	app.init();
	app.run();
	app.cleanup();

	return 0;
}
