#pragma once
#include <glm/glm.hpp>
#include "texture.h"
#include "render_api.h"

namespace Atlas {

	class Camera;

	namespace Render2D {

		struct Vertex {
			glm::vec2 position;
			glm::vec4 color;
			glm::vec2 uv;
			int texID;
			float sqrRadius;
		};

		void rect(const glm::vec2 &pos, const glm::vec2 &size, Color color, uint32_t textureIndx, float radius);
		void rect(const glm::vec2 &pos, const glm::vec2 &size, Color color);
		void rect(const glm::vec2 &pos, const glm::vec2 &size, Ref<Texture> texture, Color tint = { 255 });

		void circle(const glm::vec2 &pos, const float radius, Color color);

		void flush();
		void set_camera(Camera &camera);

		void begin(Ref<Texture> color, Ref<Texture> depth);
		void begin(Ref<Texture> color);
		void end();

		void clear_color(Color color);

		void init();
		void cleanup();

		void test_render(Ref<Texture> color);

	}

}
