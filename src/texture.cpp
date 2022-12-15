#include "texture.h"
#include "application.h"

#include "atl_vk_utils.h"
#include "vk_initializers.h"
#include "vk_engine.h"

uint32_t to_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	uint32_t result = (a << 24) | (r << 16) | (g << 8) | b;
	return result;
}

namespace Atlas {
	Color::Color()
		: m_Data(to_rgb(255, 255, 255, 255)) {}
	Color::Color(uint8_t v)
		: m_Data(to_rgb(v, v, v, 255)) {}
	Color::Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: m_Data(to_rgb(r, g, b, a)) {}
	Color::Color(uint8_t r, uint8_t g, uint8_t b)
		: m_Data(to_rgb(r, g, b, 255)) {}

	inline uint8_t Color::red() const
	{
		return m_Data >> 16 & 0xff;
	}

	inline uint8_t Color::green() const
	{
		return m_Data >> 8 & 0xff;
	}

	inline uint8_t Color::blue() const
	{
		return m_Data & 0xff;
	}

	inline uint8_t Color::alpha() const
	{
		return m_Data >> 24;
	}

	glm::vec4 Color::normalized_vec()
	{
		float r = red() / 255.f;
		float g = green() / 255.f;
		float b = blue() / 255.f;
		float a = alpha() / 255.f;

		return { r, g, b, a };
	}

	Color::operator uint32_t() const
	{
		return m_Data;
	}

	std::ostream &operator<<(std::ostream &os, const Color &c) {
		os << "{ r: " << (uint32_t)c.red() << ", g: " << (uint32_t)c.green() << ", b: "
			<< (uint32_t)c.blue() << ", a: " << (uint32_t)c.alpha() << " }";
		return os;
	}

	Texture::Texture(const char *path, FilterOptions options)
		: m_Initialized(true)
	{
		if (!std::filesystem::exists(path)) {
			CORE_WARN("Could not find file: {}", path);
			return;
		}

		VkFilter filter = atlas_to_vk_filter(options);

		vkutil::VkTexture texture;
		VkSamplerCreateInfo info = vkinit::sampler_create_info(filter);
		vkutil::load_texture(path, Application::get_engine().manager(), info, &texture);

		m_Texture = Application::get_engine().asset_manager().register_texture(texture);
	}

	Texture::Texture(uint32_t width, uint32_t height, FilterOptions options)
		: m_Initialized(true)
	{
		vkutil::TextureCreateInfo info =
			color_format_to_texture_info(TextureFormat::R8G8B8A8, width, height);
		info.filter = atlas_to_vk_filter(options);

		vkutil::VkTexture texture;
		vkutil::alloc_texture(Application::get_engine().manager(), info, &texture);

		m_Texture = Application::get_engine().asset_manager().register_texture(texture);
	}

	Texture::Texture(uint32_t width, uint32_t height, TextureFormat format, FilterOptions options)
		: m_Initialized(true)
	{
		vkutil::TextureCreateInfo info = color_format_to_texture_info(format, width, height);
		info.filter = atlas_to_vk_filter(options);

		vkutil::VkTexture texture;
		vkutil::alloc_texture(Application::get_engine().manager(), info, &texture);

		m_Texture = Application::get_engine().asset_manager().register_texture(texture);
	}

	Texture::~Texture()
	{
		if (auto shared = m_Texture.lock()) {
			Application::get_engine().asset_manager().queue_destroy_texture(shared);
			//Application::get_engine().asset_manager().deregister_texture(shared);
			//Application::get_engine().wait_idle();
			//vkutil::destroy_texture(Application::get_engine().manager(), *shared.get());
		}
	}

	Texture &Texture::operator=(Texture other)
	{
		//m_Texture = other.m_Texture;
		m_Texture.swap(other.m_Texture);
		std::swap(m_Initialized, other.m_Initialized);
		return *this;
	}

	uint32_t Texture::width()
	{
		if (auto texture = m_Texture.lock()) {
			return texture->width;
		}

		CORE_WARN("This texture was never created / or deleted!");
		return 0;
	}

	uint32_t Texture::height()
	{
		if (auto texture = m_Texture.lock()) {
			return texture->height;
		}

		CORE_WARN("This texture was never created / or deleted!");
		return 0;
	}

	void Texture::set_data(uint32_t *data, uint32_t count)
	{
		if (auto texture = m_Texture.lock()) {

			if (count != texture->width * texture->height) CORE_WARN("Texture: {} does not match width * height!", count);

			vkutil::set_texture_data(Application::get_engine().manager(), *texture.get(), data);
		}
	}

	void Texture::set_data(Color *data, uint32_t count)
	{
		set_data((uint32_t *)data, count);
	}

	void *Texture::get_id()
	{
		if (auto texture = m_Texture.lock()) {
			if (!texture->bImguiDescriptor) {
				CORE_WARN("no imgui descriptors were created for this texture");
				return nullptr;
			}

			return texture->imguiDescriptor;
		}

		CORE_WARN("This texture was never created / or deleted!");
		return nullptr;
	}
	vkutil::VkTexture *Texture::get_native_texture()
	{
		if (auto texture = m_Texture.lock()) {
			return texture.get();
		}

		CORE_WARN("This texture was never created / or deleted!");
		return nullptr;
	}
}
