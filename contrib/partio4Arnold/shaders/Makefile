#Makefile for Arnold Shaders

CC = /usr/local/gcc-4.1.2/bin/g++

C++ = /usr/local/gcc-4.1.2/bin/g++

EXT	= so

SHADER = partioShaders.$(EXT)

INCLUDE = 	\
			-I$(ARNOLD_HOME)/include 		\
			-I$(REPO_PATH)/maya/plugins/src/include		\
			-I$(REPO_PATH)/cpp/include \
			-I.

CFLAGS += -c -Wall -Wsign-compare -g -fvisibility=hidden -fPIC -D_LINUX $(INCLUDE)

.SUFFIXES: .cpp .cc .o .so .c .bundle

EXT	= so

OUTPUT = ./

RELEASE = ./$(ARNOLD_REVISION_VERSION)/Linux-x86_64

LD			= $(C++)

COMPILE  = $(CC) -c $(CFLAGS) $(INCLUDES)

.cpp.o:
	$(COMPILE) $< -o $@

LIBS = -lai

LIBDIR = 	-L$(ARNOLD_HOME)/bin	\

LDFLAGS = -shared -export-dynamic --whole-archive -Bsymbolic   $(LIBDIR)

SOURCES = Loader.cpp partioPoint.cpp

OBJS = $(SOURCES:.cpp=.o)

all: $(SOURCES) $(SHADER)

$(SHADER): $(OBJS)
	-rm -f $(OUTPUT)$@
	$(LD) -o $(OUTPUT)$@ $(OBJS) $(LDFLAGS) $(LIBS)
	-mkdir -p $(RELEASE)
	 cp $(SHADER) $(RELEASE)
clean:
	-rm -f *.o *.so

install:
	-mkdir -p $(RELEASE)
	 cp $(SHADER) $(RELEASE)

