#pragma once
#include <glm/glm.hpp>
#include "texture.h"
#include "render_api.h"

namespace Atlas {

	class Camera;

	namespace Render2D {

		struct Vertex {
			glm::vec3 position;
			glm::vec4 color;
			glm::vec2 uv;
			int texID;
			float sqrRadius;
		};

		void rect(const glm::vec2 &pos, const glm::vec2 &size, Color color, uint32_t textureIndx, float radius);
		void rect(const glm::vec2 &pos, const glm::vec2 &size, Color color);
		void rect(const glm::vec2 &pos, const glm::vec2 &size, Ref<Texture> texture);

		void round_rect(const glm::vec2 &pos, const glm::vec2 &size, float radius, Color color);
		void round_rect(const glm::vec2 &pos, const glm::vec2 &size, float radius, Ref<Texture> texture);

		void circle(const glm::vec2 &pos, const float radius, Color color);

		void flush();
		void set_camera(Camera &camera);

		void begin(Texture &color, Texture &depth);
		void begin(Texture &color);
		void end();

		void clear_color(Color color);

		void init();
		void cleanup();

	}

}
