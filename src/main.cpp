#include "application.h"
#include "texture.h"
#include "renderer.h"
#include "orthographic_camera.h"
#include "perspective_camera.h"

#include <imgui.h>

class Sandbox : public Atlas::Layer {


	Atlas::OrthographicCameraController orthoCamera;
	//Atlas::PerspectiveCameraController perspectiveCamera;
	Ref<Atlas::Texture> tex;

	void on_attach() override {
		tex = make_ref<Atlas::Texture>("res/images/uv_checker_v2.png");

		Atlas::Render2D::clear_color(Atlas::Color(255));
	}

	void on_update(Atlas::Timestep ts) override {
		using namespace Atlas;

		orthoCamera.on_update(ts);
		Render2D::set_camera(orthoCamera.get_camera());

		uint32_t size = 200;
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				Render2D::circle({ i, j }, 0.5, Color(i / (float)size * 255, j / (float)size * 255, 100));
			}
		}

		Render2D::rect({ -1, -1 }, { 2, 2 }, tex);

	}

	void on_event(Atlas::Event &e) override {
		orthoCamera.on_event(e);
	}

};

int main() {

	Atlas::Application app;

	app.push_layer(make_ref<Sandbox>());

	app.run();

	return 0;
}
