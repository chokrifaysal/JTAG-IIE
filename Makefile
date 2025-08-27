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

win-cross: $(WINTARGET)

$(WINTARGET): $(WIN_OBJECTS) $(SRCDIR)/libftd2xx.a | $(WINBINDIR)
	$(MINGW_CXX) $(WIN_OBJECTS) $(SRCDIR)/libftd2xx.a -o $@ $(MINGW_LDFLAGS)

$(WINBUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(WINBUILDDIR)
	$(MINGW_CXX) $(MINGW_CFLAGS) -c $< -o $@

$(WINBINDIR) $(WINBUILDDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILDDIR) $(BINDIR) $(WINBUILDDIR) $(WINBINDIR)

run: $(TARGET)
	./$(TARGET)

win-setup:
	@echo "Local FTD2XX files detected:"
	@ls $(SRCDIR)/FTD2XX.* 2>/dev/null || echo "FTD2XX files missing"
