gcc -Wall -Wno-switch -Wno-unknown-pragmas -Wno-unused-result \
    -O2 -fPIC -Wl,--gc-sections \
    -I../Hap \
    -I../MP4 \
    -I../Snappy \
    -I../Source \
    -I../Unity \
    ../Hap/hap.c \
    ../MP4/mp4demux.c \
    ../Snappy/snappy-c.cc \
    ../Snappy/snappy-sinksource.cc \
    ../Snappy/snappy-stubs-internal.cc \
    ../Snappy/snappy.cc \
    ../Source/KlakHap.cpp \
    -lstdc++ \
    -shared -o libKlakHap.so
