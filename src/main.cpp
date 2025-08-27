#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    std::cout << "JTAG-IIE v0.1 - Open source JTAG debugger\n";
    
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <command> [args...]\n";
        return 1;
    }

    std::string cmd = argv[1];
    
    if (cmd == "scan") {
        std::cout << "Scanning for devices...\n";
    } else if (cmd == "flash") {
        std::cout << "Flash programming not implemented yet\n";
    } else {
        std::cout << "Unknown command: " << cmd << "\n";
    }

    return 0;
}
