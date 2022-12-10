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
	}

	void on_update(Atlas::Timestep ts) override {
		using namespace Atlas;

		orthoCamera.on_update(ts);
		Render2D::set_camera(orthoCamera.get_camera());

		Render2D::rect({ -1, -1 }, { 1, 1 }, { 0, 0, 200 });
		Render2D::rect({ 0, 0 }, { 5, 8 }, tex);

		uint32_t size = 100;
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				bool b = (i + j) % 2;
				Color c;
				if (b) c = Color(255, 0, 0);
				else c = Color(0);

				Render2D::circle({ i, j }, 0.5, c);
			}
		}
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
