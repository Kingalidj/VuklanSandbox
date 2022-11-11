#pragma once

#define BIT(x) (1 << x)



namespace Atlas {

	struct WindowClosedEvent {
		std::string to_string() const {
			return "WindowClosed";
		}
	};

	struct WindowResizedEvent {
		uint32_t width, height;

		std::string to_string() const {
			std::stringstream ss;
			ss << "WindowResized: "
				<< width << ", " << height;
			return ss.str();
		}
	};

	struct ViewportResizedEvent {
		uint32_t width, height;

		std::string to_string() const {
			std::stringstream ss;
			ss << "ViewportResized: "
				<< width << ", " << height;
			return ss.str();
		}
	};

	struct KeyPressedEvent {
		int keyCode, repeatCount;

		std::string to_string() const {
			std::stringstream ss;
			ss << "KeyPressed: "
				<< keyCode << " (repeat:  " << repeatCount << ")";
			return ss.str();
		}
	};

	struct KeyReleasedEvent {
		int keyCode;

		std::string to_string() const {
			std::stringstream ss;
			ss << "KeyReleased: "
				<< keyCode;
			return ss.str();
		}
	};

	struct KeyTypedEvent {
		int keyCode;

		std::string to_string() const {
			std::stringstream ss;
			ss << "KeyTyped: "
				<< keyCode;
			return ss.str();
		}
	};

	struct MouseMovedEvent {
		float mouseX, mouseY;

		std::string to_string() const {
			std::stringstream ss;
			ss << "MouseMoved: "
				<< mouseX << ", " << mouseY;
			return ss.str();
		}
	};

	struct MouseScrolledEvent {
		float offsetX, offsetY;

		std::string to_string() const {
			std::stringstream ss;
			ss << "MouseScrolled: "
				<< offsetX << ", " << offsetY;
			return ss.str();
		}
	};

	struct MouseButtonPressedEvent {
		int button;

		std::string to_string() const {
			std::stringstream ss;
			ss << "MousePressed: "
				<< button;
			return ss.str();
		}
	};

	struct MouseButtonReleasedEvent {
		int button;

		std::string to_string() const {
			std::stringstream ss;
			ss << "MouseReleased: "
				<< button;
			return ss.str();
		}
	};


	enum EventCategory {
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
	};

#define EVENT_LIST													\
	A(WindowClosed, EventCategoryApplication)						\
	A(WindowResized, EventCategoryApplication)						\
	A(ViewportResized, EventCategoryApplication)					\
	A(KeyPressed, EventCategoryInput | EventCategoryKeyboard)		\
	A(KeyReleased, EventCategoryInput | EventCategoryKeyboard)		\
	A(KeyTyped, EventCategoryInput | EventCategoryKeyboard)			\
	A(MouseButtonPressed, EventCategoryInput | EventCategoryMouse)	\
	A(MouseButtonReleased, EventCategoryInput | EventCategoryMouse)	\
	A(MouseMoved, EventCategoryInput | EventCategoryMouse)			\
	A(MouseScrolled, EventCategoryInput | EventCategoryMouse)		\

	//Create Enum
#define A(x, _) x,
	enum class EventType : int {
		NONE = -1,
		EVENT_LIST
	};
#undef A

	//Create variant
#define A(x, _) x##Event,
	using EventVariant = std::variant<
		EVENT_LIST
		std::monostate
	>;
#undef A

	//get_event_type<...>()
	template<typename T>
	inline EventType get_event_type() {
		CORE_ASSERT(false, "get_event_type not defined for this type");
	}
#define A(x, _) \
template<> \
inline EventType get_event_type< x##Event>() { \
	return EventType::x; \
}
	EVENT_LIST
#undef A

		//get_category_flags<...>()
		template<typename T>
	inline int get_event_category_flags() {
		CORE_ASSERT(false, "get_event_category_flags not defined for this type");
	}
#define A(x, z) \
template<> \
inline int get_event_category_flags< x##Event >() { \
	return z; \
}
	EVENT_LIST
#undef A


		class Event {
		public:
			bool handled = false;

			EventVariant event;

#define A(x, _) Event(x##Event e) :event(e) {}
			EVENT_LIST
#undef A

				EventType get_type() const {
				size_t indx = event.index();

#define A(x, _) if (indx == (int) get_event_type< x##Event >()) \
{ return EventType::x; }
				EVENT_LIST
#undef A

					assert(false);

				return EventType::NONE;
			}

			template<typename T>
			T &get() {
				if (get_event_type<T>() == get_type()) {
					return std::get<T>(event);
				}

				assert(false);
				return std::get<T>(event);
			}

			template<typename T>
			const T &get() const {
				if (get_event_type<T>() == get_type()) {
					return std::get<T>(event);
				}

				assert(false);
				return std::get<T>(event);
			}

			const char *get_name() {
				size_t indx = event.index();

#define A(x, _) if (indx == (int) get_event_type< x##Event >()) \
													 { return #x; }
				EVENT_LIST
#undef A

					assert(false);
				return "NONE";
			}

			int get_category_flags() {
				size_t indx = event.index();

#define A(x, _) if (indx == (int) get_event_type< x##Event >()) \
					{ return get_event_category_flags< x##Event >(); }
				EVENT_LIST
#undef A

					assert(false);
				return 0;
			}

			const std::string to_string() const {
				size_t indx = event.index();

#define A(x, _) if (indx == (int) get_event_type< x##Event >()) \
					{ return get< x##Event >().to_string(); }
				EVENT_LIST
#undef A
					assert(false);
				return "NONE";
			}

			inline bool in_category(EventCategory category) {
				return get_category_flags() & category;
			}
	};

	inline std::ostream &operator<<(std::ostream &os, const Event &e) {
		return os << e.to_string();
	}


	class EventDispatcher {

	private:
		Event &m_Event;

	public:
		EventDispatcher(Event &event)
			: m_Event(event) {
		}

		template<typename T, typename F>
		bool dispatch(const F &func) {
			if (m_Event.get_type() == get_event_type<T>()) {
				m_Event.handled = func(m_Event.get<T>());
				return true;
			}
			return false;
		}
	};

}
