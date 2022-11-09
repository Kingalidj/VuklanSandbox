#pragma once

#include <string>
#include <sstream>

#define BIT(x) (1 << x)

#define EVENT_CLASS_TYPE(type)	static EventType get_static_type() { return EventType::type; }\
								virtual EventType get_event_type() const override { return get_static_type(); } \
								virtual const char* get_name() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int get_category_flags() const override { return category; }


namespace Atlas {

	enum class EventType : unsigned short {
		NONE = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,
		EntitySelected
	};

	enum EventCategory {
		NONE = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
	};

	class Event {
	public:
		bool handled = false;

		virtual EventType get_event_type() const = 0;
		virtual const char *get_name() const = 0;
		virtual int get_category_flags() const = 0;
		virtual std::string to_string() const { return get_name(); }

		inline bool in_category(EventCategory category) {
			return get_category_flags() & category;
		}
	};

	class EventDispatcher {
		//using EventFn = std::function<bool(T&)>;

	private:
		Event &m_Event;

	public:
		EventDispatcher(Event &event)
			: m_Event(event) {
		}

		template<typename T, typename F>
		bool dispatch(const F &func) {
			if (m_Event.get_event_type() == T::get_static_type()) {
				m_Event.handled = func(static_cast<T &> (m_Event));
				return true;
			}
			return false;
		}
	};

	inline std::ostream &operator<<(std::ostream &os, const Event &e) {
		return os << e.to_string();
	}

	/*
		WINDOW EVENT
	*/

	class WindowResizeEvent : public Event {
	private:
		unsigned int m_Width, m_Height;

	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {
		}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
			EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowCloseEvent : public Event {
	public:
		WindowCloseEvent() {}

		EVENT_CLASS_TYPE(WindowClose)
			EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppTickEvent : public Event {
	public:
		AppTickEvent() {}

		EVENT_CLASS_TYPE(AppTick)
			EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppUpdateEvent : public Event {
	public:
		AppUpdateEvent() {}

		EVENT_CLASS_TYPE(AppUpdate)
			EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class AppRenderEvent : public Event {
	public:
		AppRenderEvent() {}

		EVENT_CLASS_TYPE(AppRender)
			EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};


	/*
		KEY EVENT
	*/

	class KeyEvent : public Event {
	protected:
		int m_KeyCode;

		KeyEvent(int keyCode)
			: m_KeyCode(keyCode) {
		}

	public:
		inline int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	};

	class KeyPressedEvent : public KeyEvent {
	private:
		int m_RepeatCount;

	public:
		KeyPressedEvent(int keyCode, int repeatCount)
			: KeyEvent(keyCode), m_RepeatCount(repeatCount) {
		}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyPressEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	};

	class KeyReleasedEvent : public KeyEvent {
	public:
		KeyReleasedEvent(int keycode)
			: KeyEvent(keycode) {
		}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent {

	public:
		KeyTypedEvent(unsigned int keyCode)
			: KeyEvent(keyCode) {
		}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "KeyPressEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};

	/*
		MOUSE EVENT
	*/

	class MouseMovedEvent : public Event {
	private:
		float m_MouseX, m_MouseY;

	public:
		MouseMovedEvent(float x, float y)
			: m_MouseX(x), m_MouseY(y) {
		}

		inline float GetX() { return m_MouseX; }
		inline float GetY() { return m_MouseY; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseScrolledEvent : public Event {
	private:
		float m_XOffset, m_YOffset;

	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {
		}

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << m_XOffset << ", " << m_YOffset;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
			EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseButtonEvent : public Event {
	protected:
		int m_Button;

		MouseButtonEvent(int button)
			: m_Button(button) {
		}

	public:
		inline int GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
	};

	class MouseButtonPressedEvent : public MouseButtonEvent {
	public:
		MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button) {
		}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent {
	public:
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button) {
		}

		std::string to_string() const override {
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};


}
