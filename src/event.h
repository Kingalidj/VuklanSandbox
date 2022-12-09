#pragma once

namespace Atlas {

	enum class KeyCode : int {
		UNKNOWN = -1,

		/* Printable keys */
		SPACE = 32,
		APOSTROPHE = 39,
		COMMA = 44,
		MINUS = 45,
		PERIOD = 46,
		SLASH = 47,
		ZERO = 48,
		ONE = 49,
		TWO = 50,
		THREE = 51,
		FOUR = 52,
		FIVE = 53,
		SIX = 54,
		SEVEN = 55,
		EIGHT = 56,
		NINE = 57,
		SEMICOLON = 59,
		EQUAL = 61,
		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,
		LEFT_BRACKET = 91,
		BACKSLASH = 92,
		RIGHT_BRACKET = 93,
		GRAVE_ACCENT = 96,
		WORLD_1 = 161,
		WORLD_2 = 162,

		/* Function keys */
		ESCAPE = 256,
		ENTER = 257,
		TAB = 258,
		BACKSPACE = 259,
		INSERT = 260,
		DELETE = 261,
		RIGHT = 262,
		LEFT = 263,
		DOWN = 264,
		UP = 265,
		PAGE_UP = 266,
		PAGE_DOWN = 267,
		HOME = 268,
		END = 269,
		CAPS_LOCK = 280,
		SCROLL_LOCK = 281,
		NUM_LOCK = 282,
		PRINT_SCREEN = 283,
		PAUSE = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,
		KP_0 = 320,
		KP_1 = 321,
		KP_2 = 322,
		KP_3 = 323,
		KP_4 = 324,
		KP_5 = 325,
		KP_6 = 326,
		KP_7 = 327,
		KP_8 = 328,
		KP_9 = 329,
		KP_DECIMAL = 330,
		KP_DIVIDE = 331,
		KP_MULTIPLY = 332,
		KP_SUBTRACT = 333,
		KP_ADD = 334,
		KP_ENTER = 335,
		KP_EQUAL = 336,
		LEFT_SHIFT = 340,
		LEFT_CONTROL = 341,
		LEFT_ALT = 342,
		LEFT_SUPER = 343,
		RIGHT_SHIFT = 344,
		RIGHT_CONTROL = 345,
		RIGHT_ALT = 346,
		RIGHT_SUPER = 347,
		MENU = 348,
	};

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
		int keyCode;
		bool repeat;

		std::string to_string() const {
			std::stringstream ss;
			ss << "KeyPressed: "
				<< keyCode << " (repeat:  " << (repeat ? "True" : "False") << ")";
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
		char key;

		std::string to_string() const {
			std::stringstream ss;
			ss << "KeyTyped: "
				<< key;
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
		EventCategoryApplication = 1 << 0,
		EventCategoryInput = 1 << 1,
		EventCategoryKeyboard = 1 << 2,
		EventCategoryMouse = 1 << 3,
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
		return EventType::NONE;
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
		return 0;
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
		EventDispatcher &dispatch(const F &func) {
			if (m_Event.get_type() == get_event_type<T>()) {
				m_Event.handled = func(m_Event.get<T>());
			}

			return *this;
		}
	};

}
