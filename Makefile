ARCH = $(shell uname -m)

ifeq ($(ARCH), x86_64)
INCLUDE = -I/usr/include/X11
DEFINES = 
LIBS = -lX11
else
INCLUDE = -I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/src/hello_pi/libs/ilclient -I/opt/vc/src/hello_pi/libs/vgfont
DEFINES = -DIS_RPI -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
LIBDIR = -L/opt/vc/lib -L/opt/vc/src/hello_pi/libs/vgfont
LIBS = -lbcm_host -lvcos -lvchiq_arm
endif

INCLUDE += -I. -I/usr/include/freetype2
DEFINES += -DUSE_OPENGL -DUSE_EGL -DTARGET_POSIX -D_LINUX -DPIC -D_REENTRANT
LIBS += -lEGL -lGLESv2 -lm

FLAGS = -g -Wall -ftree-vectorize
CC = g++
CFLAGS = $(FLAGS) $(INCLUDE) $(DEFINES)

ALL = clock texture logo triangle

.PHONY: all clean

all: $(ALL)

clock: clock.o widget.o
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) -lfreetype

texture: texture.o pngloader.o widget.o
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS) -lpng

logo: logo.o mesh.o widget.o
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS)

triangle: triangle.o widget.o
	$(CC) $(CFLAGS) $(LIBDIR) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(ALL) *.o
