#
# File listings
#

PRODUCT = KlakHap

SRCS_C = Hap/hap.c MP4/mp4demux.c
vpath %.c Hap MP4

SRCS_CC = Snappy/snappy-c.cc \
	      Snappy/snappy-sinksource.cc \
	      Snappy/snappy-stubs-internal.cc \
	      Snappy/snappy.cc
vpath %.cc Snappy

SRCS_CPP = Source/KlakHap.cpp
vpath %.cpp Source

SRCS = $(SRCS_C) $(SRCS_CC) $(SRCS_CPP)

OBJ_DIR = build-$(PLATFORM)-$(ARCH)

#
# Intermediate/output files
#

OBJS_C   = $(addprefix $(OBJ_DIR)/, $(notdir $(patsubst %.c,  %.o, $(SRCS_C)  )))
OBJS_CC  = $(addprefix $(OBJ_DIR)/, $(notdir $(patsubst %.cc, %.o, $(SRCS_CC) )))
OBJS_CPP = $(addprefix $(OBJ_DIR)/, $(notdir $(patsubst %.cpp,%.o, $(SRCS_CPP))))

OBJS = $(OBJS_C) $(OBJS_CC) $(OBJS_CPP)

ifeq ($(TARGET_TYPE), dll)
  TARGET = $(OBJ_DIR)/$(PRODUCT).$(TARGET_TYPE)
else
  TARGET = $(OBJ_DIR)/lib$(PRODUCT).$(TARGET_TYPE)
endif

#
# Toolchain
#

ifndef AR
  AR = ar
endif

ifeq ($(origin CC),default)
  CC = clang
endif

ifeq ($(origin CXX),default)
  CXX = clang++
endif

ifndef STRIP
  STRIP = strip
endif

#
# Compiler/linker options
#

CPPFLAGS += -ISnappy -IHap -IMP4 -IUnity
CFLAGS += -O2 -Wall -Wextra -Wno-sign-compare -Wno-implicit-fallthrough
CXXFLAGS += -O2 -Wall -Wextra -Wno-unused-parameter -Wno-switch -Wno-unknown-pragmas -std=c++17

#
# Building rules
#

all: $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

copy: $(TARGET)
ifeq ($(OS),Windows_NT)
	powershell -NoProfile -Command "Copy-Item -LiteralPath '$(TARGET)' -Destination '../Packages/jp.keijiro.klak.hap/Plugin/$(PLATFORM)/' -Force"
else
	cp $(TARGET) ../Packages/jp.keijiro.klak.hap/Plugin/$(PLATFORM)
endif

$(OBJ_DIR)/$(PRODUCT).dll: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $@

$(OBJ_DIR)/lib$(PRODUCT).dylib: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/lib$(PRODUCT).so: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: %.cc | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
