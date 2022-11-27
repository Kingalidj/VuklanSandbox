#include "application.h"
#include "texture.h"
#include "renderer.h"

class Sandbox : public Atlas::Layer {

	void on_update(Atlas::Timestep ts) override {
		Atlas::Render2D::draw_test_triangle();
	}

	void on_imgui() override {

	}

	void on_event(Atlas::Event &e) override {

	}

};

int main() {

	Atlas::Application app;

	app.push_layer(make_ref<Sandbox>());

	app.run();

	return 0;
}
