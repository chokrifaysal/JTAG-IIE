#include <iostream>
#include <string>
#include "jtag.h"
#include "ftdi.cpp"
#include "device.h"

int main(int argc, char* argv[])
{
    std::cout << "JTAG-IIE v0.1 - Open source JTAG debugger\n";
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <command> [args...]\n"
                  << "Commands: scan, info, reset, halt, resume\n";
        return 1;
    }

    std::string cmd = argv[1];
    
    FtdiAdapter adapter;
    Jtag jtag(&adapter);
    
    if (!jtag.init()) {
        std::cerr << "Failed to initialize JTAG\n";
        return 1;
    }
    
    uint32_t id = jtag.idcode();
    if (id == 0 || id == 0xffffffff) {
        std::cerr << "No device found\n";
        return 1;
    }
    
    Device dev(id, &jtag);
    if (!dev.init()) {
        return 1;
    }
    
    if (cmd == "scan" || cmd == "info") {
        const DeviceInfo* info = dev.info();
        if (info) {
            std::cout << "Device: " << info->vendor << " " << info->name << "\n";
            std::cout << "Flash: " << info->flash_size/1024 << "KB\n";
            std::cout << "RAM: " << info->ram_size/1024 << "KB\n";
            std::cout << "FPU: " << (info->has_fpu ? "Yes" : "No") << "\n";
        }
    } else if (cmd == "reset") {
        if (dev.reset()) std::cout << "Device reset\n";
        else std::cerr << "Reset failed\n";
    } else if (cmd == "halt") {
        if (dev.halt()) std::cout << "Device halted\n";
        else std::cerr << "Halt failed\n";
    } else if (cmd == "resume") {
        if (dev.resume()) std::cout << "Device resumed\n";
        else std::cerr << "Resume failed\n";
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
    }

    return 0;
}
