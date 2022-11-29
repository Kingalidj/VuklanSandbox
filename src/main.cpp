#include "application.h"
#include "texture.h"
#include "renderer.h"
#include "orthographic_camera.h"
#include "perspective_camera.h"

class Sandbox : public Atlas::Layer {

	Atlas::OrthographicCameraController orthoCamera{ true };
	Atlas::PerspectiveCameraController perspectiveCamera;

	void on_update(Atlas::Timestep ts) override {

		orthoCamera.on_update(ts);
		Atlas::Render2D::set_camera(orthoCamera.get_camera().get_view_projection());

		Atlas::Render2D::draw_test_triangle();
	}

	void on_imgui() override {
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
