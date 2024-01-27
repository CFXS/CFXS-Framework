// [CFXS] //
#pragma once
#include <CFXS/IPv4.hpp>

namespace CFXS::sACN {

    static constexpr uint16_t UDP_PORT      = 5568; // Default sACN UDP Port
    static constexpr IPv4 MULTICAST_BASE_IP = IPv4("236.255.0.0");

} // namespace CFXS::sACN