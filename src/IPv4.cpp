// [CFXS] //

#include <CFXS/IPv4.hpp>

namespace CFXS {

    char* IPv4::print_to(char* dest, int maxLen) const {
        snprintf(dest, maxLen, "%u.%u.%u.%u", this->operator[](0), this->operator[](1), this->operator[](2), this->operator[](3));
        return dest;
    }

} // namespace CFXS