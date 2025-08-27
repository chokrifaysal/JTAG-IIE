CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g
LDFLAGS = -lpthread

# Detect OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LINUX_LDLIBS = -lftdi1
    LINUX_CXXFLAGS = -I/usr/include/libftdi1
endif

# Windows cross-compilation
MINGW_PREFIX = x86_64-w64-mingw32
MINGW_CXX = $(MINGW_PREFIX)-g++
MINGW_CFLAGS = -std=c++20 -Wall -Wextra -O2 -g -static -I/usr/$(MINGW_PREFIX)/include/libftdi1
MINGW_LDFLAGS = -static -lftdi1 -lusb-1.0 -lws2_32 -lsetupapi

SRCDIR = src
BUILDDIR = build
BINDIR = bin
WINBUILDDIR = build-win
WINBINDIR = bin-win

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)
WINOBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(WINBUILDDIR)/%.o)

TARGET = $(BINDIR)/jtag
WINTARGET = $(WINBINDIR)/jtag.exe

.PHONY: all clean win-cross win-setup

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LINUX_LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(LINUX_CXXFLAGS) -c $< -o $@

$(BINDIR) $(BUILDDIR):
	mkdir -p $@

# Windows cross-compilation
win-cross: $(WINTARGET)

$(WINTARGET): $(WINOBJECTS) | $(WINBINDIR)
	$(MINGW_CXX) $(WINOBJECTS) -o $@ $(MINGW_LDFLAGS)

$(WINBUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(WINBUILDDIR)
	$(MINGW_CXX) $(MINGW_CFLAGS) -c $< -o $@

$(WINBINDIR) $(WINBUILDDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(WINBUILDDIR) $(WINBINDIR)

run: $(TARGET)
	./$(TARGET)

# Setup check
win-setup:
	@echo "Checking MinGW cross-compiler..."
	@$(MINGW_CXX) --version || (echo "MinGW not found. Install with: sudo pacman -S mingw-w64-gcc" && false)
	@echo "Checking FTDI headers..."
	@test -f /usr/$(MINGW_PREFIX)/include/libftdi1/ftdi.h || (echo "FTDI headers missing. Install with: sudo pacman -S libftdi" && false)
	@echo "Windows build setup complete"
