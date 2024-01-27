// [CFXS] //
#pragma once
#include <CFXS/IPv4.hpp>
#include <CFXS/UUID.hpp>

namespace CFXS::sACN {

    static constexpr uint16_t UDP_PORT              = 5568;              // Default sACN UDP Port
    static constexpr IPv4 MULTICAST_UNIVERSE_GROUP  = "239.255.0.0";     // Multicast universe base ip (base + universe)
    static constexpr IPv4 MULTICAST_DISCOVERY_GROUP = "239.255.250.214"; // Universe discovery packet group

    // 0x41 0x53 0x43 0x2D 0x45 0x31 0x2E 0x31 0x37 0x00 0x00 0x00 (ASC-E1.17\0\0\0)
    static constexpr uint8_t ASC_E117_ID[12] = {0x41, 0x53, 0x43, 0x2D, 0x45, 0x31, 0x2E, 0x31, 0x37, 0x00, 0x00, 0x00};

    // Vectors
    static constexpr uint32_t VECTOR_ROOT_E131_DATA                   = 0x00000004;
    static constexpr uint32_t VECTOR_ROOT_E131_EXTENDED               = 0x00000008;
    static constexpr uint32_t VECTOR_DMP_SET_PROPERTY                 = 0x02;
    static constexpr uint32_t VECTOR_E131_DATA_PACKET                 = 0x00000002;
    static constexpr uint32_t VECTOR_E131_EXTENDED_SYNCHRONIZATION    = 0x00000001;
    static constexpr uint32_t VECTOR_E131_EXTENDED_DISCOVERY          = 0x00000002;
    static constexpr uint32_t VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST = 0x00000001;

#pragma pack(1)
    struct ACN_ID {
        uint8_t value[sizeof(ASC_E117_ID)];
        inline void initialize() { memcpy(value, ASC_E117_ID, sizeof(ASC_E117_ID)); }
        inline bool is_valid() const { return memcmp(value, ASC_E117_ID, sizeof(ASC_E117_ID)) == 0; }
    };

    struct RootLayer {
        uint16_t preamble_size;
        uint16_t postamble_size;
        ACN_ID acn_id;
        uint16_t flags_and_length; // low 12 bits = PDU length, high 4 bits = flags
        uint32_t vector;
        UUID cid;

        template<typename T>
        inline T* cast_pdu() {
            return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + sizeof(*this));
        }

        inline uint16_t get_length() const { return flags_and_length & 0x0FFF; }
        inline uint16_t get_flags() const { return flags_and_length >> 12; }
    };

    struct PDU {
        uint16_t flags_and_length; // low 12 bits = PDU length, high 4 bits = flags
        uint32_t vector;

        template<typename T>
        inline T* cast_pdu() {
            return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + sizeof(*this));
        }

        inline uint16_t get_length() const { return flags_and_length & 0x0FFF; }
        inline uint16_t get_flags() const { return flags_and_length >> 12; }
    };

    struct DMPLayer {
        uint16_t flags_and_length; // low 12 bits = PDU length, high 4 bits = flags
        uint8_t vector;
        uint8_t address_type_and_data_type;
        uint16_t first_property_adress;
        uint16_t address_increment;
        uint16_t property_value_count;
        uint8_t property_values[513];

        inline uint16_t get_length() const { return flags_and_length & 0x0FFF; }
        inline uint16_t get_flags() const { return flags_and_length >> 12; }
    };

    struct UniverseDiscoveryLayer : PDU {
        uint8_t page;
        uint8_t last;
        uint16_t universes[512];
    };

    struct FramingLayer_Data : PDU {
        char source_name[64]; // UTF-8 null-terminated
        uint8_t priority;     // 0-200 (default = 100)
        uint16_t synchronization_address;
        uint8_t sequence_number;
        uint8_t options;
        uint16_t universe;
    };

    struct FramingLayer_Synchronization : PDU {
        uint8_t sequence_number;
        uint16_t synchronization_address;
        uint16_t reserved;
    };

    struct FramingLayer_Discovery : PDU {
        char source_name[64]; // UTF-8 null-terminated
    };
#pragma pack()

} // namespace CFXS::sACN