#include "texture.h"
#include "application.h"

#include "atl_vk_utils.h"
#include "vk_textures.h"
#include "vk_initializers.h"
#include "vk_engine.h"

namespace Atlas {
	Texture::Texture(const char *path, FilterOptions options)
		: m_Initialized(true)
	{
		if (!std::filesystem::exists(path)) {
			CORE_WARN("Could not find file: {}", path);
			return;
		}

		VkFilter filter = atlas_to_vk_filter(options);

		vkutil::Texture texture;
		VkSamplerCreateInfo info = vkinit::sampler_create_info(filter);
		vkutil::load_texture(path, Application::get_engine().manager(), info, &texture);

		m_Texture = Application::get_engine().asset_manager().register_texture(texture);
	}

	Texture::Texture(uint32_t width, uint32_t height, ColorFormat f, FilterOptions options)
		: m_Initialized(true)
	{
		vkutil::TextureCreateInfo info = color_format_to_texture_info(f, width, height);
		info.filter = atlas_to_vk_filter(options);

		//m_Texture = make_scope<vkutil::Texture>();
		vkutil::Texture texture;
		vkutil::alloc_texture(Application::get_engine().manager(), info, &texture);

		m_Texture = Application::get_engine().asset_manager().register_texture(texture);
	}

	Texture::~Texture()
	{
		if (auto shared = m_Texture.lock()) {
			Application::get_engine().asset_manager().deregister_texture(shared);
			Application::get_engine().wait_idle(); //TODO: is this right??
			vkutil::destroy_texture(Application::get_engine().manager(), *shared.get());
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
	vkutil::Texture *Texture::get_native_texture()
	{
		if (auto texture = m_Texture.lock()) {
			return texture.get();
		}

		CORE_WARN("This texture was never created / or deleted!");
		return nullptr;
	}
}
