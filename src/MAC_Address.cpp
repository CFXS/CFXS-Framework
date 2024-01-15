// [CFXS] //

#include <CFXS/MAC_Address.hpp>

namespace CFXS {

    int MAC_Address::print_to(char* dest, int maxLen) const {
        return snprintf(dest, maxLen, "%02X:%02X:%02X:%02X:%02X:%02X", m_data[0], m_data[1], m_data[2], m_data[3], m_data[4], m_data[5]);
    }

} // namespace CFXS
