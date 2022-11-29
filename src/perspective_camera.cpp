#include "perspective_camera.h"

#include "event.h"
#include "layer.h"
#include "application.h"
#include "window.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Atlas
{
	void PerspectiveCamera::recalculate_view()
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::recalculate_projection()
	{
		m_ProjectionMatrix = glm::perspective(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	PerspectiveCamera::PerspectiveCamera(float nearPlane, float farPlane, float fov, float aspectRatio)
		: m_NearPlane(nearPlane), m_FarPlane(farPlane), m_Fov(fov), m_AspectRatio(aspectRatio),
		m_ProjectionMatrix(glm::perspective(fov, aspectRatio, nearPlane, farPlane))
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void PerspectiveCamera::set_front(const glm::vec3 &direction)
	{
		m_Front = direction;
		m_Right = glm::normalize(glm::cross(m_Front, m_Up));
		recalculate_view();
	}

	void PerspectiveCamera::set_projection(float nearPlane, float farPlane, float fov, float aspectRatio)
	{
		m_NearPlane = nearPlane;
		m_FarPlane = farPlane;
		m_Fov = fov;
		m_AspectRatio = aspectRatio;
		m_ProjectionMatrix = glm::perspective(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
	}

	bool PerspectiveCameraController::on_mouse_moved(MouseMovedEvent &e)
	{
		if (!Application::is_mouse_pressed(1)) return false;

		if (firstMouseMove)
		{
			m_PMouseX = e.mouseX;
			m_PMouseY = e.mouseY;
			firstMouseMove = false;
		}

		float xOffset = e.mouseX - m_PMouseX;
		float yOffset = m_PMouseY - e.mouseY;
		m_PMouseX = e.mouseX;
		m_PMouseY = e.mouseY;

		m_Yaw += xOffset * m_CamearSensitivity;
		m_Pitch -= yOffset * m_CamearSensitivity;

		m_Pitch = std::clamp(m_Pitch, -89.0f, 89.0f);

		m_CameraDirection.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		m_CameraDirection.y = sin(glm::radians(m_Pitch));
		m_CameraDirection.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

		m_Camera.set_front(glm::normalize(m_CameraDirection));

		return false;
	}

	bool PerspectiveCameraController::on_mouse_pressed(MouseButtonPressedEvent &e)
	{
		Window &window = Application::get_window();
		if (e.button == 1) window.capture_mouse(true);
		return false;
	}

	bool PerspectiveCameraController::on_mouse_released(MouseButtonReleasedEvent &e)
	{
		Window &window = Application::get_window();
		window.capture_mouse(false);
		firstMouseMove = true;
		return false;
	}

	bool PerspectiveCameraController::on_window_resized(WindowResizedEvent &e)
	{
		m_Camera.set_aspect_ratio((float)e.width / (float)e.height);
		return false;
	}

	PerspectiveCameraController::PerspectiveCameraController(float aspecRatio)
		: m_Camera(0.1f, 1000.0f, glm::radians(90.0f), aspecRatio) {}

	void PerspectiveCameraController::on_update(Timestep ts)
	{
		bool updated = false;

		if (Application::is_key_pressed(KeyCode::S))
		{
			m_CameraPosition -= m_Camera.get_front() * (m_CameraMoveSpeed * ts);
			updated = true;
		}
		if (Application::is_key_pressed(KeyCode::W))
		{
			m_CameraPosition += m_Camera.get_front() * (m_CameraMoveSpeed * ts);
			updated = true;
		}
		if (Application::is_key_pressed(KeyCode::A))
		{
			m_CameraPosition -= m_Camera.get_right() * (m_CameraMoveSpeed * ts);
			updated = true;
		}
		if (Application::is_key_pressed(KeyCode::D))
		{
			m_CameraPosition += m_Camera.get_right() * (m_CameraMoveSpeed * ts);
			updated = true;
		}


		if (Application::is_key_pressed(KeyCode::LEFT_CONTROL))
		{
			m_CameraPosition.y += m_CameraMoveSpeed * ts;
			updated = true;
		}
		if (Application::is_key_pressed(KeyCode::SPACE))
		{
			m_CameraPosition.y -= m_CameraMoveSpeed * ts;
			updated = true;
		}

		if (updated) m_Camera.set_position(m_CameraPosition);

		glm::vec2 &viewportSize = Application::get_viewport_size();
		float aspecRatio = viewportSize.x / viewportSize.y;
		if (aspecRatio != m_Camera.get_aspect_ratio())
		{
			m_Camera.set_aspect_ratio(aspecRatio);
		}

	}

	void PerspectiveCameraController::on_event(Event &e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<MouseMovedEvent>(BIND_EVENT_FN(PerspectiveCameraController::on_mouse_moved))
			.dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(PerspectiveCameraController::on_mouse_pressed))
			.dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(PerspectiveCameraController::on_mouse_released));
	}
}
