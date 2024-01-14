// [CFXS] //
#pragma once

namespace CFXS {

    using VoidFunction = void (*)(void);

    template<typename T>
    constexpr void safe_call(T&& fn) {
        if (fn) {
            fn();
        }
    }

    constexpr auto create_bitmask(int bits) {
        return (1 << bits) - 1;
    }

} // namespace CFXS

#define CFXS_ENUM_UNDERLYING_OPERATORS(enumName)                                                                            \
    constexpr enumName operator|(enumName a, enumName b) {                                                                  \
        return static_cast<enumName>(static_cast<std::underlying_type<enumName>::type>(a) |                                 \
                                     static_cast<std::underlying_type<enumName>::type>(b));                                 \
    }                                                                                                                       \
    constexpr bool operator&(enumName a, enumName b) {                                                                      \
        return static_cast<std::underlying_type<enumName>::type>(a) & static_cast<std::underlying_type<enumName>::type>(b); \
    }
