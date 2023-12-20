#pragma once

#include "Common.h"
#include "MidiMessage.h"

class MidiDeviceManager
{
public:
    MidiDeviceManager() = default;

    std::string GetDeviceName( std::size_t inIndex )
    {
        DeviceHandle deviceHandle = DeviceIDToHandle( inIndex );
        return GetDeviceName( deviceHandle );
    }

    std::string GetDeviceName(DeviceHandle inDeviceHandle)
    {
        UINT_PTR handleID = reinterpret_cast<UINT_PTR>( inDeviceHandle );
        MIDIINCAPS caps;
        if( midiInGetDevCaps( handleID, &caps, sizeof( caps ) ) == MMSYSERR_NOERROR )
        {
            std::wstring outName( caps.szPname );
            return std::string( outName.begin(), outName.end() );
        }
        return "Unknown Device";
    }

    bool OpenDevice( std::uint32_t inIndex )
    {
        static const DWORD_PTR deviceCallback = reinterpret_cast<DWORD_PTR>( MidiMessageProc );
        DeviceHandle deviceHandle;
        if( midiInOpen( &deviceHandle, inIndex, deviceCallback, reinterpret_cast<DWORD_PTR>( this ), CALLBACK_FUNCTION ) == MMSYSERR_NOERROR )
        {
            if( midiInStart( deviceHandle ) == MMSYSERR_NOERROR )
            {
                m_resourceLock.lock();
                m_activeDevices.push_back( deviceHandle );
                m_resourceLock.unlock();
                return true;
            }
            else
            {
                midiInClose( deviceHandle );
                return false;
            }
        }
        return false;
    }

    void CloseDevice( DeviceHandle inHandle )
    {
        midiInClose( inHandle );

        m_resourceLock.lock();
        auto it = std::find( m_activeDevices.begin(), m_activeDevices.end(), inHandle );
        if( it != m_activeDevices.end() )
        {
            m_activeDevices.erase( it );
        }
        m_resourceLock.unlock();
    }

    std::uint32_t GetDeviceID( std::uint32_t inIndex )
    {
        if( inIndex >= m_activeDevices.size() )
        {
            return 0;
        }

        return DeviceHandleToID(m_activeDevices[inIndex]);
    }

    void OpenAllDevices()
    {
        std::uint32_t deviceCount = midiInGetNumDevs();
        for ( std::uint32_t i = 0; i < deviceCount; ++i)
        {
            OpenDevice( i );
        }
    }

    void RefreshDevices()
    {
        m_resourceLock.lock();

        while( !m_pendingDevicesToClose.empty() )
        {
            CloseDevice( m_pendingDevicesToClose.top() );
            m_pendingDevicesToClose.pop();
        }

        OpenAllDevices();

        m_resourceLock.unlock();
    }

    void CloseAllDevices()
    {
        m_resourceLock.lock();
        while( !m_activeDevices.empty() )
        {
            CloseDevice( m_activeDevices.front() );
        }
        m_resourceLock.unlock();
    }

    std::size_t GetNumActiveDevices()
    {
        return m_activeDevices.size();
    }

    MidiMessage GetNextMessage()
    {
        RefreshDevices();
        if (m_messageQueue.empty())
        {
            return {};
        }

        m_resourceLock.lock();
        MidiMessage message = m_messageQueue.front();
        m_messageQueue.pop();
        m_resourceLock.unlock();

        return message;
    }

    void ClearAllPendingMessages()
    {
        m_resourceLock.lock();
        while( !m_messageQueue.empty() )
        {
            m_messageQueue.pop();
        }
        m_resourceLock.unlock();
    }

    bool HasMessages() const
    {
        return !m_messageQueue.empty();
    }

private:
    inline DeviceID DeviceHandleToID( DeviceHandle inHandle )
    {
        return static_cast<DeviceID>( reinterpret_cast<uint64_t>( inHandle ) );
    }
    inline DeviceHandle DeviceIDToHandle( DeviceID inID )
    {
        return reinterpret_cast<DeviceHandle>( static_cast<uint64_t>( inID ) );
    }

    static void CALLBACK MidiMessageProc( HMIDIIN inMidiHandle, UINT inMessage, DWORD_PTR inInstance, DWORD_PTR inParam1, DWORD_PTR inParam2 )
    {
        auto instance = reinterpret_cast<MidiDeviceManager*>( inInstance );
        switch( inMessage )
        {
        case MIM_DATA:
        {
            DeviceID deviceID = instance->DeviceHandleToID( inMidiHandle );
            std::uint32_t data = static_cast<std::uint32_t>( inParam1 );
            instance->m_resourceLock.lock();
            instance->m_messageQueue.push( { deviceID, data } );
            instance->m_resourceLock.unlock();
            break;
        }
        case MIM_CLOSE:
        {
            instance->m_resourceLock.lock();
            instance->m_pendingDevicesToClose.push( inMidiHandle );
            instance->m_resourceLock.unlock();
            break;
        }
        default:
            return;
        }
    }

private:
    std::queue<MidiMessage> m_messageQueue;
    std::vector<DeviceHandle> m_activeDevices;
    std::stack<DeviceHandle> m_pendingDevicesToClose;

    std::recursive_mutex m_resourceLock;
};