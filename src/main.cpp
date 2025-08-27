#include <iostream>
#include <string>
#include <fstream>
#include "jtag.h"
#include "device.h"
#include "flash.h"

#ifdef _WIN32
#include "winftdi.cpp"
using JtagAdapterType = WinFtdiAdapter;
#else
#include "ftdi.cpp"
using JtagAdapterType = FtdiAdapter;
#endif

void usage(const char* name) {
    std::cout << "Usage: " << name << " <command> [args...]\n"
              << "Commands:\n"
              << "  scan                 - Scan for devices\n"
              << "  info                 - Show device info\n"
              << "  reset                - Reset device\n"
              << "  halt                 - Halt device\n"
              << "  resume               - Resume device\n"
              << "  flash <file.bin>     - Program binary file\n"
              << "  erase                - Erase entire flash\n"
              << "  dump <addr> <len>    - Dump memory to stdout\n";
}

int main(int argc, char* argv[])
{
    std::cout << "JTAG-IIE v0.1 - Open source JTAG debugger\n";
    
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];
    
    JtagAdapterType adapter;
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
    
    Flash flash(&dev, &jtag);
    
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
    } else if (cmd == "flash") {
        if (argc < 3) {
            std::cerr << "Need filename\n";
            return 1;
        }
        
        std::ifstream file(argv[2], std::ios::binary);
        if (!file) {
            std::cerr << "Can't open " << argv[2] << "\n";
            return 1;
        }
        
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
        
        if (!flash.detect()) {
            std::cerr << "Flash not supported\n";
            return 1;
        }
        
        if (!flash.load_driver()) {
            std::cerr << "Failed to load flash driver\n";
            return 1;
        }
        
        std::cout << "Erasing flash...\n";
        if (!flash.erase(0x08000000, data.size())) {
            std::cerr << "Erase failed\n";
            return 1;
        }
        
        std::cout << "Programming...\n";
        if (!flash.program(0x08000000, data.data(), data.size())) {
            std::cerr << "Program failed\n";
            return 1;
        }
        
        std::cout << "Programming complete\n";
    } else if (cmd == "erase") {
        if (!flash.detect()) {
            std::cerr << "Flash not supported\n";
            return 1;
        }
        
        if (!flash.load_driver()) {
            std::cerr << "Failed to load flash driver\n";
            return 1;
        }
        
        std::cout << "Erasing flash...\n";
        if (flash.erase(0x08000000, 64*1024)) {
            std::cout << "Erase complete\n";
        } else {
            std::cerr << "Erase failed\n";
        }
    } else if (cmd == "dump") {
        if (argc < 4) {
            std::cerr << "Need address and length\n";
            return 1;
        }
        
        uint32_t addr = strtoul(argv[2], nullptr, 0);
        uint32_t len = strtoul(argv[3], nullptr, 0);
        
        std::vector<uint8_t> buf(len);
        if (flash.read(addr, buf.data(), len)) {
            for (uint32_t i = 0; i < len; i++) {
                if (i % 16 == 0) printf("%08x: ", addr + i);
                printf("%02x ", buf[i]);
                if (i % 16 == 15) printf("\n");
            }
            if (len % 16) printf("\n");
        } else {
            std::cerr << "Read failed\n";
        }
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
        usage(argv[0]);
    }

    return 0;
}
