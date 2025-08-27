#include <iostream>
#include <string>
#include <fstream>
#include "jtag.h"
#include "device.h"
#include "flash.h"
#include "config.h"

#ifdef _WIN32
#include "winftdi.cpp"
using JtagAdapterType = WinFtdiAdapter;
#else
#include "ftdi.cpp"
using JtagAdapterType = FtdiAdapter;
#endif

void usage(const char* name, const Config& cfg) {
    std::cout << "JTAG-IIE v0.1 - Open source JTAG debugger\n\n";
    std::cout << "Usage: " << name << " [options] <command> [args...]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  scan                 - Scan for devices\n";
    std::cout << "  info                 - Show device info\n";
    std::cout << "  reset                - Reset device\n";
    std::cout << "  halt                 - Halt device\n";
    std::cout << "  resume               - Resume device\n";
    std::cout << "  flash <file.bin>     - Program binary file\n";
    std::cout << "  erase                - Erase entire flash\n";
    std::cout << "  dump <addr> <len>    - Dump memory to stdout\n\n";
    std::cout << "Options:\n";
    std::cout << "  -v, --verbose        - Verbose output\n";
    std::cout << "  -f, --force          - Force operations\n";
    std::cout << "  --vid VID            - USB vendor ID (default 0x" << std::hex << cfg.vid << ")\n";
    std::cout << "  --pid PID            - USB product ID (default 0x" << cfg.pid << ")\n";
    std::cout << "  --config file.cfg    - Load config file\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << name << " --vid 0x1234 flash firmware.bin\n";
}

int main(int argc, char* argv[])
{
    Config cfg = Config::from_args(argc, argv);
    
    if (argc < 2) {
        usage(argv[0], cfg);
        return 1;
    }

    // Find command position
    int cmd_pos = 1;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (!arg.empty() && arg[0] != '-') {
            cmd_pos = i;
            break;
        }
    }
    
    if (cmd_pos >= argc) {
        usage(argv[0], cfg);
        return 1;
    }

    std::string cmd = argv[cmd_pos];
    
    if (cfg.verbose) {
        std::cout << "JTAG-IIE - verbose mode\n";
        std::cout << "VID: 0x" << std::hex << cfg.vid << " PID: 0x" << cfg.pid << "\n";
    }
    
    JtagAdapterType adapter(cfg.vid, cfg.pid);
    Jtag jtag(&adapter);
    
    if (!jtag.init()) {
        std::cerr << "Failed to initialize JTAG adapter\n";
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
        if (dev.reset()) {
            std::cout << "Device reset\n";
        } else {
            std::cerr << "Reset failed\n";
        }
    } else if (cmd == "halt") {
        if (dev.halt()) {
            std::cout << "Device halted\n";
        } else {
            std::cerr << "Halt failed\n";
        }
    } else if (cmd == "resume") {
        if (dev.resume()) {
            std::cout << "Device resumed\n";
        } else {
            std::cerr << "Resume failed\n";
        }
    } else if (cmd == "flash") {
        if (cmd_pos + 1 >= argc) {
            std::cerr << "Need filename\n";
            return 1;
        }
        
        std::string filename = argv[cmd_pos + 1];
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Can't open " << filename << "\n";
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
        
        if (cfg.verbose) {
            std::cout << "File size: " << data.size() << " bytes\n";
        }
        
        if (!cfg.force) {
            std::cout << "About to erase and program " << data.size() << " bytes. Continue? [y/N] ";
            std::string response;
            std::getline(std::cin, response);
            if (response != "y" && response != "Y") {
                std::cout << "Aborted\n";
                return 0;
            }
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
        if (cmd_pos + 2 >= argc) {
            std::cerr << "Need address and length\n";
            return 1;
        }
        
        uint32_t addr = strtoul(argv[cmd_pos + 1], nullptr, 0);
        uint32_t len = strtoul(argv[cmd_pos + 2], nullptr, 0);
        
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
    } else if (cmd == "config") {
        if (cmd_pos + 1 >= argc) {
            cfg.save("jtag.cfg");
            std::cout << "Saved default config to jtag.cfg\n";
        } else {
            std::string filename = argv[cmd_pos + 1];
            cfg.save(filename);
            std::cout << "Saved config to " << filename << "\n";
        }
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
        usage(argv[0], cfg);
    }

    return 0;
}
