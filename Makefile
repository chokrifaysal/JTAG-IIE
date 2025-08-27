CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -g
LDFLAGS = -lpthread

# Linux build settings
LINUX_CXXFLAGS = -I/usr/include/libftdi1
LINUX_LDLIBS = -lftdi1

# Windows cross-compilation
MINGW_PREFIX = x86_64-w64-mingw32
MINGW_CXX = $(MINGW_PREFIX)-g++
MINGW_CFLAGS = -std=c++20 -Wall -Wextra -O2 -g -static -I$(SRCDIR)
MINGW_LDFLAGS = -static -L$(SRCDIR) -lftd2xx -lsetupapi -lws2_32

SRCDIR = src
BUILDDIR = build
BINDIR = bin
WINBUILDDIR = build-win
WINBINDIR = bin-win

# Linux sources
LINUX_SOURCES = $(filter-out $(SRCDIR)/winftdi.cpp, $(wildcard $(SRCDIR)/*.cpp))
LINUX_OBJECTS = $(LINUX_SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Windows sources
WIN_SOURCES = $(filter-out $(SRCDIR)/ftdi.cpp, $(wildcard $(SRCDIR)/*.cpp))
WIN_OBJECTS = $(WIN_SOURCES:$(SRCDIR)/%.cpp=$(WINBUILDDIR)/%.o)

TARGET = $(BINDIR)/jtag
WINTARGET = $(WINBINDIR)/jtag.exe

.PHONY: all clean linux win-cross win-setup

all: linux

linux: $(TARGET)

$(TARGET): $(LINUX_OBJECTS) | $(BINDIR)
	$(CXX) $(LINUX_OBJECTS) -o $@ $(LDFLAGS) $(LINUX_LDLIBS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $(LINUX_CXXFLAGS) -c $< -o $@

$(BINDIR) $(BUILDDIR):
	mkdir -p $@

# Windows cross-compilation
win-cross: $(WINTARGET)

$(WINTARGET): $(WIN_OBJECTS) $(SRCDIR)/libftd2xx.a | $(WINBINDIR)
	$(MINGW_CXX) $(WIN_OBJECTS) $(SRCDIR)/libftd2xx.a -o $@ $(MINGW_LDFLAGS)

# Auto-create static library from DLL
$(SRCDIR)/libftd2xx.a: $(SRCDIR)/FTD2XX.dll $(SRCDIR)/FTD2XX.def
	@echo "Creating Windows static library..."
	$(MINGW_PREFIX)-dlltool -d $(SRCDIR)/FTD2XX.def -l $@ 2>/dev/null || \
	$(MINGW_PREFIX)-dlltool --input-def $(SRCDIR)/FTD2XX.def --output-lib $@ 2>/dev/null || \
	(echo "Failed to create library - check MinGW tools" && exit 1)

$(WINBUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(WINBUILDDIR)
	$(MINGW_CXX) $(MINGW_CFLAGS) -c $< -o $@

$(WINBINDIR) $(WINBUILDDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(WINBUILDDIR) $(WINBINDIR)
	rm -f $(SRCDIR)/libftd2xx.a

run: $(TARGET)
	./$(TARGET)

# Setup verification
win-setup:
	@echo "Checking Windows build dependencies..."
	@test -f $(SRCDIR)/FTD2XX.dll || echo "Warning: FTD2XX.dll missing"
	@test -f $(SRCDIR)/FTD2XX.H || echo "Warning: FTD2XX.H missing"
	@$(MINGW_PREFIX)-dlltool --version 2>/dev/null || echo "Install: sudo pacman -S mingw-w64-binutils"
