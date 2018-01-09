.SUFFIXES:
.PRECIOUS: build/%.c
.PHONY: all clean

CFLAGS += -g -std=c99 -pedantic -Wall -Wextra -Werror -MMD
CPPFLAGS += -Isrc -Ibuild
OBJS :=

all: build/c8c

clean:
	find ./build ! -name '.gitignore' -and ! -path './build' -exec $(RM) -rf {} +

build/%.o: build/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.c: src/%.y
	ln -fT $< $(patsubst %.c, %.y, $@)
	lemon -q $(patsubst %.c, %.y, $@)

build/%.c: src/%.re
	re2c $< -o $@

OBJS += build/code_generation.o
OBJS += build/utils.o
OBJS += build/lexer.o
OBJS += build/parser.o

-include build/*.d

build/lexer.o: build/parser.c

build/c8c: $(OBJS)
	$(CC) $(LDFLAGS) $(LDLIBS) -o $@ $^
