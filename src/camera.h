#pragma once

#include <glm/glm.hpp>

#include "layer.h"
#include "event.h"

namespace Atlas {

	class Camera {
	public:

		virtual void set_position(const glm::vec3 &pos) = 0;

		virtual glm::vec3 &get_position() = 0;
		virtual glm::mat4 &get_projection() = 0;
		virtual glm::mat4 &get_view() = 0;
		virtual glm::mat4 &get_view_projection() = 0;
	};

	class CameraController {
	public:
		virtual void on_update(Timestep ts) = 0;
		virtual void on_event(Event &e) = 0;

		virtual Camera &get_camera() = 0;

	};

}