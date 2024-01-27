// [CFXS] //
#pragma once

#include <CFXS/Debug.hpp>

namespace CFXS {

    class MAC_Address {
    public:
        constexpr MAC_Address() : m_data(0, 0, 0, 0, 0, 0) {}
        constexpr explicit MAC_Address(uint8_t b) : m_data(b, b, b, b, b, b) {
        }
        constexpr explicit MAC_Address(const uint8_t data[6]) : m_data(data[0], data[1], data[2], data[3], data[4], data[5]) {
        }
        constexpr explicit MAC_Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f) : m_data(a, b, c, d, e, f) {
        }
        constexpr MAC_Address(const MAC_Address& other) {
            for (size_t i = 0; i < sizeof(m_data); i++)
                m_data[i] = other.m_data[i];
        }

        /// @brief Construct MAC address from string
        /// @param mac_string "12:34:56:AB:CD:EF"
        template<size_t N>
        constexpr explicit MAC_Address(const char (&mac_string)[N]) : m_data(0, 0, 0, 0, 0, 0) {
            int idx = 0;
            int loc = 0;
            for (size_t i = 0; i < N; i++) {
                char ch = mac_string[i];

                if (ch == ':') {
                    idx++;
                    loc = 0;
                    continue;
                }

                if (loc < 2) {
                    if (ch >= '0' && ch <= '9') {
                        int val = ch - '0';
                        m_data[idx] += loc & 1 ? val : (val << 4);
                    } else if (ch >= 'a' && ch <= 'f') {
                        int val = (ch - 'a') + 10;
                        m_data[idx] += loc & 1 ? val : (val << 4);
                    } else if (ch >= 'A' && ch <= 'F') {
                        int val = (ch - 'A') + 10;
                        m_data[idx] += loc & 1 ? val : (val << 4);
                    }
                }

                loc++;
            }
        }

        /// @brief Get raw data pointer
        /// @return Raw data pointer
        constexpr uint8_t* get_raw_data() {
            return m_data;
        }

        /// @brief Get raw data pointer
        /// @return Raw data pointer
        constexpr const uint8_t* get_raw_data() const {
            return m_data;
        }

        inline bool operator==(const MAC_Address& other) const {
            return memcmp(get_raw_data(), other.get_raw_data(), 6) == 0;
        }

        inline bool operator!=(const MAC_Address& other) const {
            return memcmp(get_raw_data(), other.get_raw_data(), 6) != 0;
        }

        inline uint8_t& operator[](uint8_t index) {
            CFXS_ASSERT(index < sizeof(m_data), "Index (%u) out of range", index);
            return m_data[index];
        }

        inline const uint8_t& operator[](uint8_t index) const {
            CFXS_ASSERT(index < sizeof(m_data), "Index (%u) out of range", index);
            return m_data[index];
        }

        inline MAC_Address& operator=(const MAC_Address& other) {
            memcpy(m_data, other.m_data, sizeof(m_data));
            return *this;
        }

        int print_to(char* dest, int max_len) const;

    private:
        uint8_t m_data[6];
    };

} // namespace CFXS