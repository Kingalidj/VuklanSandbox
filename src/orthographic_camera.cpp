#include "orthographic_camera.h"

#include "event.h"
#include "layer.h"
#include  "application.h"

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace Atlas {
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_ViewMatrix(1.0f)
	{
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::set_projection(float left, float right, float bottom, float top)
	{
		m_ProjectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::recalculate_view()
	{
		glm::mat4 transform = glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1)) * glm::translate(glm::mat4(1.0f), m_Position);

		m_ViewMatrix = glm::inverse(transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	}

	bool OrthographicCameraController::on_mouse_scrolled(MouseScrolledEvent &e)
	{
		m_ZoomLevel -= e.offsetY * 0.15f * m_ZoomLevel;
		m_Camera.set_projection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		return false;
	}

	OrthographicCameraController::OrthographicCameraController(bool rotation)
		: m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio *m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel), m_Rotation(rotation) {
		auto size = Application::get_viewport_size();
		m_AspectRatio = (float)size.x / (float)size.y;
	}

	void OrthographicCameraController::on_update(Timestep timestep)
	{
		//if (Input::IsKeyPressed(ATL_KEY_D))
		if (Application::is_key_pressed(KeyCode::D))
		{
			m_CameraPosition.x += m_CameraTranslationSpeed * timestep;
		}
		else if (Application::is_key_pressed(KeyCode::A))
		{
			m_CameraPosition.x -= m_CameraTranslationSpeed * timestep;
		}

		if (Application::is_key_pressed(KeyCode::S))
		{
			m_CameraPosition.y += m_CameraTranslationSpeed * timestep;
		}
		else if (Application::is_key_pressed(KeyCode::W))
		{
			m_CameraPosition.y -= m_CameraTranslationSpeed * timestep;
		}

		if (m_Rotation)
		{
			if (Application::is_key_pressed(KeyCode::Q))
			{
				m_CameraRotation -= m_CameraRotationSpeed * timestep;
			}
			else if (Application::is_key_pressed(KeyCode::E))
			{
				m_CameraRotation += m_CameraRotationSpeed * timestep;
			}

			if (m_CameraRotation > 180.0f)
			{
				m_CameraRotation -= 360.0f;
			}
			else if (m_CameraRotation <= -180.0f)
			{
				m_CameraRotation += 360.0f;
			}

			m_Camera.set_rotation(m_CameraRotation);
		}

		m_Camera.set_position(m_CameraPosition);

		m_CameraTranslationSpeed = m_ZoomLevel * 2;

		glm::vec2 &viewportSize = Application::get_viewport_size();

		float aspecRatio = (float)viewportSize.x / (float)viewportSize.y;
		if (aspecRatio != m_AspectRatio)
		{
			m_AspectRatio = aspecRatio;
			m_Camera.set_projection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		}
	}

	void OrthographicCameraController::on_event(Event &e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.dispatch<MouseScrolledEvent>(BIND_EVENT_FN(OrthographicCameraController::on_mouse_scrolled));
	}
}