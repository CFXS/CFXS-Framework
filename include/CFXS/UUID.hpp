// [CFXS] //
#pragma once

namespace CFXS {

    class UUID {
    public:
        constexpr UUID() : m_data(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) {}
        constexpr UUID(const UUID& other) = default;
        constexpr UUID(UUID&& other)      = default;
        constexpr UUID(const uint8_t data[16]) {
            for (size_t i = 0; i < sizeof(m_data); i++) {
                m_data[i] = data[i];
            }
        }

        inline const uint8_t* get_data() const { return m_data; }

        inline bool operator==(const UUID& other) { return memcmp(m_data, other.m_data, sizeof(m_data)) == 0; }
        inline bool operator!=(const UUID& other) { return memcmp(m_data, other.m_data, sizeof(m_data)) != 0; }

    private:
        uint8_t m_data[16];
    };

} // namespace CFXS