.SUFFIXES:
.PRECIOUS: build/%.c
.PHONY: all clean

LIBS :=

LIBS += lib/libc8asm/libc8asm.a
CPPFLAGS += -Ilib/libc8asm

LDLIBS += $(LIBS)
CFLAGS += -g -std=c99 -pedantic -Wall -Wextra -Werror -MMD
CPPFLAGS += -Isrc -Ibuild
OBJS :=

all: build/c8c

clean:
	find ./build ! -name '.gitignore' -and ! -path './build' -exec $(RM) -rf {} +
	$(MAKE) -C lib/libc8asm clean

build/%.o: build/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.c: src/%.y
	ln -fT $< $(patsubst %.c, %.y, $@)
	lemon -q $(patsubst %.c, %.y, $@)

build/%.c: src/%.re
	re2c $< -o $@

lib/libc8asm/libc8asm.a:
	$(MAKE) -C lib/libc8asm

OBJS += build/utils.o
OBJS += build/lexer.o
OBJS += build/parser.o
OBJS += build/code_generation.o

-include build/*.d

build/lexer.o: build/parser.c

$(OBJS): $(LIBS)

build/c8c: $(OBJS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)
