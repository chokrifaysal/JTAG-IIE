# JTAG-IIE  
**Open-source JTAG debugger / programmer for ARM Cortex-M and more**  
*Stop paying $500 for a flashing LED.*

---

## What it is

JTAG-IIE is a **full-featured, cross-platform** JTAG tool that replaces overpriced commercial dongles.  
Out of the box it can:

- Detect and identify 50+ MCUs (STM32, GD32, LPC, …)  
- Flash, erase, verify firmware on STM32F1xx (more drivers coming)  
- Halt / resume / reset targets  
- Dump memory or individual registers  
- Work on **Linux** (libftdi) and **Windows** (FTD2XX driver)

All code is MIT-licensed, no blobs, no phone-home.

---

## Quick start

### 1. Grab a binary  
[Releases →](https://github.com/chokrifaysal/JTAG-IIE/releases)

| Platform | Asset | Size |
|----------|-------|------|
| Linux x86-64 | `arch-linux-build.tar.xz` | 154 KB |
| Windows x86-64 | `windows_x86-64.tar.xz` | 2.63 MB |

### 2. Plug in your FTDI-based adapter  
Any FT232H, FT2232, C232HM, etc. works.  
**Pinout (bitbang):**
```plaintext
FTDI pin → JTAG pin
D0  → TCK
D1  → TMS
D2  → TDI
D3  → TRST (optional)
D4  → TDO
```

### 3. Run

```bash
# Linux
tar -xf arch-linux-build.tar.xz
./jtag scan

# Windows
tar -xf windows_x86-64.tar.xz
jtag.exe scan

# Example session:
$ ./jtag flash firmware.bin
Found STMicroelectronics STM32F103C8
Erasing flash...
Programming...
Programming complete
```

### 4. Building from source
#### Linux (native)
```bash
sudo pacman -S libftdi  # or apt install libftdi1-dev
make linux
./bin/jtag
```
#### Windows (cross-compile on Linux)
```bash
sudo pacman -S mingw-w64-gcc
make win-cross       # produces bin-win/jtag.exe
```

### 5. Usage
```bash
jtag <command> [args]

scan                 # list connected devices
info                 # show chip details
reset                # hardware reset
halt / resume        # core control
flash <file.bin>     # program binary
erase                # mass-erase
dump <addr> <len>    # hex-dump memory
```

### 6. Supported devices

| MCU family | Flash driver | Notes |
|------------|--------------|-------|
| STM32F1xx  | ✅ STM32F1Flash | 1 kB pages |
| STM32F4xx  | 🔜 Planned | |
| GD32F1xx   | ✅ (uses STM32F1) | |
| LPC17xx    | 🔜 Planned | |

Add more in `src/flash.cpp` – PRs welcome.

### 7. Hacking
- **Adapters**: inherit from JtagAdapter (see ftdi.cpp, winftdi.cpp)
- **Devices**: add entries in DeviceDB and implement a FlashDriver
- **CLI**: extend main.cpp – keep it lean

### 8. License
MIT © 2025 chokrifaysal

### 9. Release notes
- **v0.1.0** – Initial release
  – Linux & Windows binaries
  – STM32F1 flash driver
  – FTDI bitbang support

[Full changelog](https://github.com/chokrifaysal/JTAG-IIE/releases)
