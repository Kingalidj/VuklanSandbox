#pragma once

#include <glm/glm.hpp>

class Camera {
public:

	virtual void set_position(const glm::vec3 &pos) = 0;

	virtual glm::vec3 &get_position() = 0;
	virtual glm::mat4 &get_projection() = 0;
	virtual glm::mat4 &get_view() = 0;
	virtual glm::mat4 &get_view_projection() = 0;

};