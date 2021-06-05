#
# File listings
#

TARGET = KlakHap

C_SRCS = ../Hap/hap.c \
         ../MP4/mp4demux.c

CPP_SRCS = ../Snappy/snappy-c.cc \
           ../Snappy/snappy-sinksource.cc \
           ../Snappy/snappy-stubs-internal.cc \
           ../Snappy/snappy.cc \
           ../Source/KlakHap.cpp

#
# Intermediate/output files
#

C_OBJS = $(addprefix $(ARCH)/, $(notdir $(C_SRCS:.c=.o)))
CC_OBJS = $(addprefix $(ARCH)/, $(notdir $(CPP_SRCS:.cc=.o)))
CPP_OBJS = $(CC_OBJS:.cpp=.o)
OUTPUT = $(ARCH)/$(TARGET).so

#
# Compiler/linker options
#

TARGET_OPT = -target $(ARCH)-apple-macos10.12

C_FLAGS = $(TARGET_OPT) -O2 -fPIC
C_FLAGS += -Wall -Wno-switch -Wno-unknown-pragmas -Wno-unused-result
C_FLAGS += -I../Hap -I../MP4 -I../Snappy -I../Source -I../Unity

CPP_FLAGS = $(C_FLAGS) -std=c++11

LD_FLAGS = $(TARGET_OPT) -shared -rdynamic -fPIC

LIBS = -lstdc++

#
# Building rules
#

all: $(OUTPUT)

clean:
	rm -f $(OUTPUT) $(C_OBJS) $(CPP_OBJS)

$(OUTPUT): $(C_OBJS) $(CPP_OBJS)
	gcc $(LD_FLAGS) -o $@ $^ $(LIBS)

$(ARCH)/%.o: ../Hap/%.c
	gcc $(C_FLAGS) -c -o $@ $<

$(ARCH)/%.o: ../MP4/%.c
	gcc $(C_FLAGS) -c -o $@ $<

$(ARCH)/%.o: ../Snappy/%.cc
	g++ $(CPP_FLAGS) -c -o $@ $<

$(ARCH)/%.o: ../Source/%.cpp
	g++ $(CPP_FLAGS) -c -o $@ $<
