#include "application.h"
#include "texture.h"
#include "renderer.h"
#include "orthographic_camera.h"
#include "perspective_camera.h"

#include <imgui.h>

class Sandbox : public Atlas::Layer {


	Atlas::OrthographicCameraController orthoCamera;
	Atlas::PerspectiveCameraController perspectiveCamera;

	bool bOrthoCamera = true;

	void on_update(Atlas::Timestep ts) override {
		using namespace Atlas;

		RenderApi::begin(Application::get_viewport_color_texture(), Application::get_viewport_depth_texture(), { 0.8, 0.8, 0.8, 1.0 });

		if (bOrthoCamera) {
			orthoCamera.on_update(ts);
			Render2D::set_camera(orthoCamera.get_camera());
		}
		else {
			perspectiveCamera.on_update(ts);
			Render2D::set_camera(perspectiveCamera.get_camera());
		}

		Render2D::draw_test_triangle();

		RenderApi::end(Application::get_viewport_color_texture());
	}

	void on_imgui() override {
		ImGui::Begin("Camera Settings");
		ImGui::Checkbox("OrthoGraphic Camera", &bOrthoCamera);
		ImGui::End();
	}

	void on_event(Atlas::Event &e) override {
		if (bOrthoCamera)
			orthoCamera.on_event(e);
		else
			perspectiveCamera.on_event(e);

	}

};

int main() {

	Atlas::Application app;

	app.push_layer(make_ref<Sandbox>());

	app.run();

	return 0;
}
