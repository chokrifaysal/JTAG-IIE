#include <iostream>
#include <string>
#include "jtag.h"
#include "ftdi.cpp"  // Yeah, including .cpp, sue me

int main(int argc, char* argv[])
{
    std::cout << "JTAG-IIE v0.1 - Open source JTAG debugger\n";
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <command> [args...]\n"
                  << "Commands: scan, idcode\n";
        return 1;
    }

    std::string cmd = argv[1];
    
    if (cmd == "scan") {
        FtdiAdapter adapter;
        Jtag jtag(&adapter);
        
        if (!jtag.init()) {
            std::cerr << "Failed to initialize JTAG\n";
            return 1;
        }
        
        std::cout << "JTAG initialized\n";
        
        uint32_t id = jtag.idcode();
        if (id != 0 && id != 0xffffffff) {
            std::cout << "Device ID: 0x" << std::hex << id << std::dec << "\n";
        } else {
            std::cout << "No device found\n";
        }
        
    } else if (cmd == "idcode") {
        std::cout << "IDCODE command (same as scan for now)\n";
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
    }

    return 0;
}
