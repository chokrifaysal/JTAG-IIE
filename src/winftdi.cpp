#include "jtag.h"
#include <windows.h>
#include "FTD2XX.H"
#include <iostream>

class WinFtdiAdapter : public JtagAdapter {
public:
    WinFtdiAdapter(uint32_t vid = 0x0403, uint32_t pid = 0x6010) 
        : handle(nullptr), vid(vid), pid(pid), state(0), bytesWritten(0) {}
    
    ~WinFtdiAdapter() {
        if (handle) close();
    }
    
    bool open() override {
        DWORD devCount = 0;
        FT_STATUS status = FT_CreateDeviceInfoList(&devCount);
        if (status != FT_OK || devCount == 0) {
            std::cerr << "No FTDI devices found\n";
            return false;
        }
        
        status = FT_Open(0, &handle);
        if (status != FT_OK) {
            std::cerr << "Failed to open FTDI device: " << status << "\n";
            return false;
        }
        
        // Configure for bitbang mode
        status = FT_SetBitMode(handle, 0x0f, 0x01);  // BITMODE_BITBANG
        if (status != FT_OK) {
            std::cerr << "Failed to set bitbang mode\n";
            FT_Close(handle);
            handle = nullptr;
            return false;
        }
        
        status = FT_SetBaudRate(handle, 9600);
        if (status != FT_OK) {
            std::cerr << "Failed to set baud rate\n";
            FT_Close(handle);
            handle = nullptr;
            return false;
        }
        
        // Initial state - all outputs low
        state = 0;
        FT_Write(handle, &state, 1, &bytesWritten);
        
        return true;
    }
    
    void close() override {
        if (handle) {
            FT_Close(handle);
            handle = nullptr;
        }
    }
    
    void set_pin(JtagPin::Type pin, bool value) override {
        uint8_t mask = 0;
        switch (pin) {
            case JtagPin::TCK: mask = 0x01; break;
            case JtagPin::TMS: mask = 0x02; break;
            case JtagPin::TDI: mask = 0x04; break;
            case JtagPin::TRST: mask = 0x08; break;
            default: return;
        }
        
        if (value) state |= mask;
        else state &= ~mask;
        
        DWORD written;
        FT_Write(handle, &state, 1, &written);
    }
    
    bool get_pin(JtagPin::Type pin) override {
        if (pin != JtagPin::TDO) return false;
        
        uint8_t val;
        FT_GetBitMode(handle, &val);
        return val & 0x10;  // TDO on bit 4
    }
    
    void delay(unsigned us) override {
        Sleep(us / 1000);
    }
    
private:
    FT_HANDLE handle;
    uint32_t vid, pid;
    uint8_t state;
    DWORD bytesWritten;
};
