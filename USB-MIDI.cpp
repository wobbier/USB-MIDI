// USB-MIDI.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "include/MidiDeviceManager.h"

int main()
{
    MidiDeviceManager manager;
    manager.OpenAllDevices();
    for (int i = 0; i < manager.GetNumActiveDevices(); ++i )
    {
        std::string deviceName = manager.GetDeviceName( i );
        std::cout << deviceName.c_str() << std::endl;
    }

    while( true )
    {
        MidiMessage res = manager.GetNextMessage();
        if( res.Encode() > 0 )
        {
            std::cout << res.ToString().c_str() << std::endl;
        }
    }

    return 0;
}