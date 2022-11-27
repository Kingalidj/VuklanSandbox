#pragma once

namespace vkutil {
	struct Texture;
}

namespace Atlas {

	enum class FilterOptions {
		LINEAR,
		NEAREST,
	};

	enum class ColorFormat {
		R8G8B8A8,
		D32,
	};

	class Texture {
	public:
		Texture() = default;

		Texture(const char *path, FilterOptions options = FilterOptions::LINEAR);
		Texture(uint32_t width, uint32_t height, ColorFormat format = ColorFormat::R8G8B8A8, FilterOptions options = FilterOptions::LINEAR);
		Texture(const Texture &other) = delete;
		~Texture();

		Texture &operator=(Texture other);

		uint32_t width();
		uint32_t height();

		void *get_id();

		vkutil::Texture *get_native();

	private:

		//Scope<vkutil::Texture> m_Texture;
		WeakRef<vkutil::Texture> m_Texture;
	};

}
