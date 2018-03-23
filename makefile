.SUFFIXES:
.PRECIOUS: build/%.c

LIBS :=

LIBS += lib/libc8asm/libc8asm.a
CPPFLAGS += -Ilib/libc8asm
LDFLAGS += -Llib/libc8asm
LDLIBS += -lc8asm

CFLAGS += -g -std=c99 -pedantic -Wall -Wextra -Werror -MMD
CPPFLAGS += -Isrc -Ibuild

OBJS :=
OBJS += build/utils.o
OBJS += build/lexer.o
OBJS += build/parser.o
OBJS += build/code_generation.o
OBJS += build/null_terminated_array.o

.PHONY: all
all: build/c8c

.PHONY: clean
clean:
	find ./build ! -name '.gitignore' -and ! -path './build' -exec $(RM) -rf '{}' \;
	$(MAKE) -C lib/libc8asm clean

.PHONY: test
test: test-null_terminated_array
	@echo "======= TESTS ======= " ;                                    \
	for test in ./build/*_test ; do                                     \
	    test_name=`echo $$test | sed "s/\.\/build\/\(.*\)_test/\1/"` ;  \
	    echo -n "$$test_name: " ;                                       \
	    if $$test ; then echo "PASS" ; else echo "FAIL" ; fi            \
	done ;                                                              \
	echo "======= TESTS ======="  ;

.PHONY: test-null_terminated_array
test-null_terminated_array: build/null_terminated_array_test

build/%.o: build/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%_test: build/%_test.o
	$(CC) $^ -o $@ 

build/%_test.o: src/%.c
	$(CC) -g -Wall -Wextra -DTESTING -c $< -o $@

build/%.c: src/%.y
	ln -fT $< $(patsubst %.c, %.y, $@)
	lemon -q $(patsubst %.c, %.y, $@)

build/%.c: src/%.re
	re2c $< -o $@

lib/libc8asm/libc8asm.a:
	$(MAKE) -C lib/libc8asm

build/lexer.o: build/parser.c

build/c8c: $(OBJS) $(LIBS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

-include build/*.d

