#pragma once
#include <glm/glm.hpp>

namespace Atlas {

	class Camera;

	namespace Render2D {

		struct Vertex {
			glm::vec3 position;
			glm::vec4 color;
			glm::vec2 uv;
		};

		void init();
		void cleanup();
		void set_camera(Camera &camera);

		void draw_test_triangle();

	}

}
