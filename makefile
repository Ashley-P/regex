CC     = gcc
CLFAGS = -Wall -g -O0 --always-make -Lsrc
OBJECTS_DIR = src/obj
PROJECT = libregex.a

#SRC = $(wildcard src/*.c)

_OBJECTS = regex.o
#OBJECTS = $(patsubst %.c, /obj/%.o, $(src))
OBJECTS = $(patsubst %, $(OBJECTS_DIR)/%, $(_OBJECTS))

DEPS = $(wildcard src/*.h)


.PHONY: all lib clean test

all:
	@$(MAKE) lib --no-print-directory
	@$(MAKE) test --no-print-directory

lib: $(PROJECT)

test:
	@$(MAKE) -C tests 

clean:
	del .\src\obj\*.o
	del libregex.a
	del bin\test.exe


$(OBJECTS_DIR)/%.o : src/%.c $(DEPS)
	$(CC) -c -o $@ $< ${LIBS} $(CFLAGS) 

$(PROJECT): $(OBJECTS)
	ar -cvq $@ $^
