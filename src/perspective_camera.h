#pragma once

#include <glm/glm.hpp>

#include "camera.h"

#include "event.h"
#include "layer.h"

namespace Atlas
{
	class PerspectiveCamera : public Camera
	{
	private:
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_Front = { 0.0f, 0.0f, -1.0f };
		glm::vec3 m_Up = { 0.0f, 1.0f, 0.0f };
		glm::vec3 m_Right = glm::normalize(glm::cross(m_Front, m_Up));
		glm::vec3 m_WorldUp = { 0.0f, 1.0f, 0.0f };

		float m_NearPlane, m_FarPlane, m_Fov, m_AspectRatio;

		void recalculate_view();
		void recalculate_projection();

	public:
		PerspectiveCamera(float nearPlane, float farPlane, float fov, float aspectRatio);

		void set_position(const glm::vec3 &position) override { m_Position = position; recalculate_view(); }
		void set_front(const glm::vec3 &direction);
		void set_projection(float nearPlane, float farPlane, float fov, float aspectRatio);
		void set_fov(float fov) { m_Fov = fov; recalculate_projection(); }
		void set_aspect_ratio(float aspectRatio) { m_AspectRatio = aspectRatio; recalculate_projection(); }

		glm::mat4 &get_projection() override { return m_ProjectionMatrix; }
		glm::mat4 &get_view() override { return m_ViewMatrix; }
		glm::mat4 &get_view_projection()  override { return m_ViewProjectionMatrix; }
		glm::vec3 &get_up() { return m_Up; }
		glm::vec3 &get_front() { return m_Front; }
		glm::vec3 &get_position() override { return m_Position; }
		glm::vec3 &get_right() { return m_Right; }

		inline float get_aspect_ratio() { return m_AspectRatio; }

	};

	class PerspectiveCameraController
	{
	private:
		float m_CameraMoveSpeed = 5.0f, m_CamearSensitivity = 0.2f;
		float m_Yaw = -90.0f, m_Pitch = 0.0f;

		float m_PMouseX = 0.0f, m_PMouseY = 0.0f;
		bool firstMouseMove = true;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_CameraDirection = { 0.0f, 0.0f, 0.0f };

		PerspectiveCamera m_Camera;

		bool on_mouse_moved(MouseMovedEvent &e);
		bool on_mouse_pressed(MouseButtonPressedEvent &e);
		bool on_mouse_released(MouseButtonReleasedEvent &e);
		bool on_window_resized(WindowResizedEvent &e);

	public:
		PerspectiveCameraController(float aspecRatio = 1.5707f);

		void on_update(Timestep ts);
		void on_event(Event &e);

		inline void set_fov(float fov) { m_Camera.set_fov(fov); }
		inline void set_aspect_ratio(float aspectRatio) { m_Camera.set_aspect_ratio(aspectRatio); }

		inline float get_aspect_ratio() { return m_Camera.get_aspect_ratio(); }

		inline const PerspectiveCamera &get_camera() { return m_Camera; }

		const glm::mat4 &get_view() { return m_Camera.get_view(); }
		const glm::mat4 &get_projection() { return m_Camera.get_projection(); }
	};

}

