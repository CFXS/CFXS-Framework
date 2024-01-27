// [CFXS] //
#pragma once

#include "ByteOrder.hpp"
#include "Debug.hpp"

namespace CFXS {

    class IPv4 {
    public:
        constexpr IPv4() : m_data(0, 0, 0, 0) {}
        constexpr IPv4(uint32_t val) : m_data(val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF) {}
        constexpr IPv4(uint8_t oct1, uint8_t oct2, uint8_t oct3, uint8_t oct4) : m_data(oct1, oct2, oct3, oct4) {
        }
        template<size_t N>
        constexpr IPv4(const char (&ip_string)[N]) : m_data(0, 0, 0, 0) {
            size_t octet    = 0;
            size_t num      = 0;
            uint8_t nums[3] = {0, 0, 0};
            for (size_t i = 0; i <= N; i++) {
                if (i == N || ip_string[i] == '.') {
                    if (num == 1) {
                        m_data[octet] += nums[0];
                    } else if (num == 2) {
                        m_data[octet] += nums[1] + nums[0] * 10;
                    } else if (num == 3) {
                        m_data[octet] += nums[2] + nums[1] * 10 + nums[0] * 100;
                    }

                    octet++;

                    if (i == N || octet == 4) {
                        return;
                    }

                    num     = 0;
                    nums[0] = 0;
                    nums[1] = 0;
                    nums[2] = 0;
                } else {
                    if (ip_string[i] >= '0' && ip_string[i] <= '9') {
                        nums[num] = ip_string[i] - '0';
                        num++;
                        if (num > 3)
                            num = 3;
                    }
                }
            }
        }
        constexpr IPv4(const IPv4& other) : m_data{other.m_data[0], other.m_data[1], other.m_data[2], other.m_data[3]} {}

        constexpr uint32_t get_value_reverse() const { return swap_byte_order_32(get_value()); }
        constexpr uint32_t get_value() const { return *get_pointer_cast<uint32_t>(); }

        template<typename T>
        constexpr const T* get_pointer_cast() const {
            return reinterpret_cast<const T*>(m_data);
        }

        bool is_valid_subnet_mask() const {
            uint32_t mask = get_value();
            if (mask == 0)
                return false;
            if (mask & (~mask >> 1))
                return false; // NOLINT

            return true;
        }

        bool is_valid_host_address() const {
            if (m_data[0] == 0xFF || m_data[0] == 0)
                return false;
            if (m_data[3] == 0xFF || m_data[3] == 0)
                return false;
            if (m_data[1] == 0xFF || m_data[2] == 0xFF)
                return false; // NOLINT

            return true;
        }

        inline bool is_limited_broadcast() const { return get_value() == 0xFFFFFFFF; }

        constexpr bool operator==(const IPv4& other) const { return get_value() == other.get_value(); }
        constexpr bool operator!=(const IPv4& other) const { return get_value() != other.get_value(); }

        inline uint8_t& operator[](uint8_t index) {
            CFXS_ASSERT(index < 4, "Index (%u) out of range", index);
            return m_data[index];
        }

        inline const uint8_t& operator[](uint8_t index) const {
            CFXS_ASSERT(index < 4, "Index (%u) out of range", index);
            return m_data[index];
        }

        inline IPv4& operator=(const IPv4& other) {
            memcpy(m_data, other.m_data, sizeof(m_data));
            return *this;
        }

        int print_to(char* dest, int max_len) const;

    private:
        uint8_t m_data[4];
    };

} // namespace CFXS