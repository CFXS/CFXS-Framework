// [CFXS] //
#pragma once

namespace CFXS {

    constexpr inline uint16_t swap_byte_order_16(uint16_t input) {
        return __builtin_bswap16(input);
    }

    constexpr inline uint32_t swap_byte_order_32(uint32_t input) {
        return __builtin_bswap32(input);
    }

} // namespace CFXS
