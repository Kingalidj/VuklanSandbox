#pragma once

#include <glm/glm.hpp>

namespace vkutil {
	struct Texture;
}

namespace Atlas {

	class Color {
	public:
		Color();
		Color(uint8_t value);
		Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
		Color(uint8_t r, uint8_t g, uint8_t b);

		inline uint8_t red() const;
		inline uint8_t green() const;
		inline uint8_t blue() const;
		inline uint8_t alpha() const;

		glm::vec4 normalized_vec();

		explicit operator uint32_t() const;
		//operator uint32_t() const { return m_Data; }
		//operator glm::vec4() const;


	private:
		uint32_t m_Data;
	};

	std::ostream &operator<<(std::ostream &os, const Color &c);

	enum class FilterOptions {
		LINEAR,
		NEAREST,
	};

	enum class TextureFormat {
		R8G8B8A8,
		D32,
	};

	class Texture {
	public:
		Texture() = default;

		Texture(const char *path, FilterOptions options = FilterOptions::LINEAR);
		Texture(uint32_t width, uint32_t height, FilterOptions options = FilterOptions::LINEAR);
		Texture(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::R8G8B8A8, FilterOptions options = FilterOptions::LINEAR);
		Texture(const Texture &other) = delete;
		~Texture();

		Texture &operator=(Texture other);

		uint32_t width();
		uint32_t height();

		void set_data(Color *data, uint32_t count);
		void set_data(uint32_t *data, uint32_t count);

		void *get_id();

		vkutil::Texture *get_native_texture();
		inline bool is_init() { return m_Initialized; }

	private:

		WeakRef<vkutil::Texture> m_Texture;
		bool m_Initialized{ false };
	};

}
