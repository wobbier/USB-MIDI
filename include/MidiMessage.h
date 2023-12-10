#pragma once

#include "Common.h"

using DeviceHandle = HMIDIIN;
using DeviceID = uint32_t;

struct MidiMessage
{
    DeviceID m_sourceDevice = 0;
    std::uint8_t m_status = 0;
    std::uint8_t m_data1 = 0;
    std::uint8_t m_data2 = 0;

    MidiMessage() = default;

    MidiMessage( DeviceID inSource, uint32_t inData )
        : m_sourceDevice( inSource )
        , m_status( inData )
        , m_data1( inData >> 8 )
        , m_data2( inData >> 16 )
    {
    }

    MidiMessage( std::uint64_t inPackedData )
        : MidiMessage( inPackedData, inPackedData >> 32 )
    {
    }

    std::uint64_t Encode()
    {
        std::uint64_t encoded = m_sourceDevice;
        encoded |= (std::uint64_t)m_status << 32;
        encoded |= (std::uint64_t)m_data1 << 40;
        encoded |= (std::uint64_t)m_data2 << 48;
        return encoded;
    }

    std::string ToString()
    {
        char temp[256];
        std::snprintf( temp, sizeof( temp ), "Device(%X) Channel/Status: %02X Key: %02X Velocity: %02X", m_sourceDevice, m_status, m_data1, m_data2 );
        return temp;
    }
};