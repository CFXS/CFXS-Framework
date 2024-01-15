// [CFXS] //
#pragma once

#include <stl/utility>

namespace CFXS {

    template<class R, class ContextType, bool IsVoid, class... Args>
    class FunctionBase;

    // function with no context
    template<class R, class ContextType, class... Args>
    class FunctionBase<R, ContextType, true, Args...> {
    protected:
        using Signature = R (*)(Args...);

        constexpr FunctionBase() = default;
        constexpr FunctionBase(const Signature& func) : m_function(func) { // NOLINT
        }

        Signature m_function = nullptr;

    public:
        constexpr Signature get_function_pointer() const {
            return m_function;
        }

        constexpr R operator()(Args... args) const {
            return m_function(stl::forward<Args>(args)...);
        }

        constexpr bool operator==(const FunctionBase& other) const {
            if (this == &other)
                return true;

            return m_function == other.m_function;
        }

        constexpr explicit operator bool() const {
            return m_function;
        }
    };

    // function with context
    template<class R, class ContextType, class... Args>
    class FunctionBase<R, ContextType, false, Args...> {
    protected:
        using Signature = R (*)(ContextType, Args...);

        constexpr FunctionBase() = default;
        constexpr FunctionBase(const Signature& func) : m_function(func) { // NOLINT
        }
        constexpr FunctionBase(const Signature& func, const ContextType& context) : m_function(func), m_context(context) {
        }

        Signature m_function = nullptr;
        ContextType m_context{};

    public:
        constexpr R operator()(Args... args) const {
            return m_function(m_context, stl::forward<Args>(args)...);
        }

        constexpr bool operator==(const FunctionBase& other) const {
            if (this == &other)
                return true;

            return (m_function == other.m_function) && (m_context == other.m_context);
        }

        constexpr Signature get_function_pointer() const {
            return m_function;
        }

        constexpr explicit operator bool() const {
            return m_function;
        }
    };

    template<class T>
    class Function;

    class NoFunctionContext {
    public:
        NoFunctionContext() = delete;
    };

    template<class R, class ContextType, class... Args>
    class Function<R(ContextType, Args...)> : public FunctionBase<R, ContextType, stl::is_same_v<ContextType, NoFunctionContext>, Args...> {
        using FunctionBaseType = FunctionBase<R, ContextType, stl::is_same_v<ContextType, NoFunctionContext>, Args...>;

    public:
        constexpr Function() : FunctionBaseType() {
        }

        constexpr Function(nullptr_t) : FunctionBaseType() { // NOLINT
        }

        template<typename F>
        constexpr Function(F func, ContextType user_data) : FunctionBaseType(func, user_data) {
        }

        template<typename F>
        constexpr Function(F func) : FunctionBaseType(func) { // NOLINT
        }
    };

} // namespace CFXS