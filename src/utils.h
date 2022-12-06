#pragma once

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      CORE_ERROR("at line: {}, in file: {}\nDetected Vulkan error: {}",        \
                 __LINE__, __FILE__, err);                                     \
    }                                                                          \
  } while (0)

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

template <typename T>
class Result {
	using ResultType = std::variant<T, std::string>;

	Result(T res)
		: m_Data(res) {}

	Result(T &&res)
		: m_Data(std::forward<T>(res)) {}

	Result(const char *msg)
		: m_Data(msg) {}

	void has_value() {

	}

private:

	ResultType m_Data;

};

template<typename T>
using Scope = std::unique_ptr<T>;
template<typename T, typename ... Args>
constexpr Scope<T> make_scope(Args&& ... args) {
	return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using Ref = std::shared_ptr<T>;
template<typename T, typename ... Args>
constexpr Ref<T> make_ref(Args&& ... args) {
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using WeakRef = std::weak_ptr<T>;
