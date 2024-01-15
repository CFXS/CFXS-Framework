// [CFXS] //
#pragma once

#include <utility>

namespace CFXS {

    template<class R, class ContextType, bool IsVoid, class... Args>
    class FunctionBase;

    // function with no userdata
    template<class R, class ContextType, class... Args>
    class FunctionBase<R, ContextType, true, Args...> {
    protected:
        using Signature = R (*)(Args...);

        constexpr FunctionBase() {
        }
        constexpr FunctionBase(const Signature& func) : m_Function(std::move(func)) {
        }
        constexpr FunctionBase(const Signature& func, const ContextType& userData) : m_Function(std::move(func)) {
        }

        Signature m_Function = nullptr;

    public:
        constexpr Signature GetFunctionPointer() const {
            return m_Function;
        }

        constexpr R operator()(Args... args) const {
            return m_Function(std::forward<Args>(args)...);
        }

        constexpr bool operator==(const FunctionBase& other) const {
            if (this == &other)
                return true;

            return m_Function == other.m_Function;
        }

        constexpr explicit operator bool() const noexcept {
            return m_Function;
        }
    };

    // function with userdata
    template<class R, class ContextType, class... Args>
    class FunctionBase<R, ContextType, false, Args...> {
    protected:
        using Signature = R (*)(ContextType, Args...);

        constexpr FunctionBase() {
        }
        constexpr FunctionBase(const Signature& func) : m_Function(func) {
        }
        constexpr FunctionBase(const Signature& func, const ContextType& userData) : m_Function(std::move(func)), m_UserData(userData) {
        }

        Signature m_Function = nullptr;
        ContextType m_UserData{};

    public:
        constexpr R operator()(Args... args) const {
            return m_Function(m_UserData, std::forward<Args>(args)...);
        }

        constexpr bool operator==(const FunctionBase& other) const {
            if (this == &other)
                return true;

            return (m_Function == other.m_Function) && (m_UserData == other.m_UserData);
        }

        constexpr Signature GetFunctionPointer() const {
            return m_Function;
        }

        constexpr explicit operator bool() const noexcept {
            return m_Function;
        }
    };

    template<class T>
    class Function;

    class NoFunctionUserData {
    public:
        NoFunctionUserData() = delete;
    };

    template<class R, class ContextType, class... Args>
    class Function<R(ContextType, Args...)>
        : public FunctionBase<R, ContextType, std::is_same<ContextType, NoFunctionUserData>::value, Args...> {
        using FunctionBaseType = FunctionBase<R, ContextType, std::is_same<ContextType, NoFunctionUserData>::value, Args...>;

    public:
        constexpr Function() noexcept : FunctionBaseType() {
        }

        constexpr Function(nullptr_t) noexcept : FunctionBaseType() {
        }

        template<typename F>
        constexpr Function(F func, ContextType userData) : FunctionBaseType(func, userData) {
        }

        template<typename F>
        constexpr Function(F func) : FunctionBaseType(func) {
        }
    };

} // namespace CFXS