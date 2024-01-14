// [CFXS] //
#pragma once

#include <utility>

namespace CFXS {

    template<class R, class ContextType, class... Args>
    class Function {
    public:
        using Signature = R (*)(ContextType, Args...);

        constexpr Function() {
        }
        constexpr Function(const Signature& func) : m_function(func) {
        }
        constexpr Function(const Signature& func, const ContextType& userData) : m_function(func), m_context(userData) {
        }

        constexpr R operator()(Args&&... args) const {
            return m_function(m_context, std::forward<Args>(args)...);
        }

        constexpr bool operator==(const Function& other) const {
            if (this == &other)
                return true;

            return (m_function == other.m_function) && (m_context == other.m_context);
        }

        constexpr Signature get_function_pointer() const {
            return m_function;
        }

        constexpr explicit operator bool() const noexcept {
            return m_function;
        }

    private:
        Signature m_function = nullptr;
        ContextType m_context{};
    };

} // namespace CFXS