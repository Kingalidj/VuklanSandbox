#pragma once

#include <glm/glm.hpp>

#include "camera.h"

#include "event.h"
#include "layer.h"

namespace Atlas {

	class OrthographicCamera : public Camera
	{
	private:
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		float m_Rotation = 0.0f;

		float m_Near = -20.0f;
		float m_Far = 20.0;

		void recalculate_view();

	public:
		OrthographicCamera(float left, float right, float bottom, float top);
		OrthographicCamera(float left, float right, float bottom, float top, float near, float far);

		inline glm::vec3 &get_position() override { return m_Position; }
		void set_position(const glm::vec3 &position) override { m_Position = position; recalculate_view(); }

		inline const float get_rotation() const { return m_Rotation; }
		void set_rotation(float rotation) { m_Rotation = rotation; recalculate_view(); }

		void set_projection(float left, float right, float bottom, float top);

		inline glm::mat4 &get_projection() override { return m_ProjectionMatrix; }
		inline glm::mat4 &get_view() override { return m_ViewMatrix; }
		inline glm::mat4 &get_view_projection() override { return m_ViewProjectionMatrix; }
	};

	class OrthographicCameraController : public CameraController
	{
	private:
		float m_AspectRatio = 0.5f;
		float m_ZoomLevel = 1.0f;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		bool m_Rotation;
		float m_CameraRotation = 0.0f;
		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 180.0f;

		OrthographicCamera m_Camera;

		bool on_mouse_scrolled(MouseScrolledEvent &e);

	public:
		OrthographicCameraController(bool roation = false);

		void on_update(Timestep ts) override;
		void on_event(Event &e) override;

		OrthographicCamera &get_camera() override { return m_Camera; }
	};

}
