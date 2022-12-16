#include "application.h"
#include "texture.h"
#include "buffer.h"
#include "renderer.h"
#include "orthographic_camera.h"
#include "perspective_camera.h"

#include <imgui.h>

class Sandbox : public Atlas::Layer {


	Atlas::OrthographicCameraController orthoCamera;

	Ref<Atlas::Texture> tex;

	void on_attach() override {
		tex = make_ref<Atlas::Texture>("res/images/uv_checker_v2.png", Atlas::FilterOptions::NEAREST);
	}

	void on_update(Atlas::Timestep ts) override {
		using namespace Atlas;

		orthoCamera.on_update(ts);

		Render2D::set_camera(orthoCamera.get_camera());


		Render2D::clear_color(Atlas::Color(200, 255, 255));
		Render2D::begin(Application::get_viewport_color_texture());
		Render2D::end();

		Render2D::clear_color(Atlas::Color(0, 0, 0, 0));
		Render2D::begin(Application::get_viewport_color_texture());
		Render2D::circle({ 0 , 1 }, 0.5, Color(0, 0, 200));
		Render2D::circle({ 0 , 2 }, 0.5, Color(200, 0, 0));
		Render2D::circle({ 0 , 3 }, 0.5, Color(200, 0, 0));
		Render2D::end();

		Render2D::clear_color(Atlas::Color(0, 0, 0, 0));
		Render2D::begin(Application::get_viewport_color_texture());
		Render2D::circle({ 1 , 0 }, 0.5, Color(0, 200, 0));
		Render2D::circle({ 2 , 0 }, 0.5, Color(0, 200, 0));
		Render2D::end();

		Render2D::clear_color(Atlas::Color(0, 0, 0, 0));
		Render2D::begin(Application::get_viewport_color_texture());
		Render2D::circle({ 1 , 1 }, 0.5, Color(0, 0, 200));
		Render2D::end();
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
