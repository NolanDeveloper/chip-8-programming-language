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
OBJS += build/tiny_set.o

# Find all source files containing macro TESTING. The files will be compiled
# separately with this macro defined in order to perform testing. With this
# technique you can unit test any source file. Just add '#ifdef TESTING'
# section with main function in the end. Test is considered passed if and only
# if main returns 0 Otherwise it's considered failed. 
TESTS := $(shell \
    find src -name "*.c" \
        -exec sh -c "grep 'TESTING' '{}' > /dev/null 2>&1" \; \
        -print)

TEST_TARGETS := $(patsubst src/%.c, test-%, $(TESTS))

.PHONY: all
all: build/c8c

build/c8c: $(OBJS) $(LIBS)
	$(CC) $^ -o $@ $(LDFLAGS) $(LDLIBS)

.PHONY: clean
clean:
	find ./build ! -name '.gitignore' -and ! -path './build' -exec $(RM) -rf '{}' \;
	$(MAKE) -C lib/libc8asm clean

.PHONY: test
test: $(TEST_TARGETS)
	@echo "======= TESTS =======" ;                                     \
	total=0 ; passed=0 ; failed=0 ;                                     \
	for test in ./build/*_test ;                                        \
	do                                                                  \
	    test_name=`echo $$test | sed "s/\.\/build\/\(.*\)_test/\1/"` ;  \
	    echo -n "$$test_name: " ;                                       \
	    if $$test ;                                                     \
	    then                                                            \
	        echo "PASS" ;                                               \
	        passed=$$((passed + 1)) ;                                   \
	    else                                                            \
	        echo "FAIL" ;                                               \
	        failed=$$((failed + 1)) ;                                   \
	    fi                                                              \
	done ;                                                              \
	echo "****** Summary ******" ;                                      \
	echo "passed:" $$passed ;                                           \
	echo "failed:" $$failed ;                                           \
	echo "total:" $$((passed + failed)) ;                               \
	echo "======= TESTS =======" ;

.PHONY: $(TEST_TARGETS)
$(TEST_TARGETS): test-%: build/%_test

build/%.o: build/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%_test: build/%_test.o
	$(CC) $^ -o $@ 

build/%_test.o: CPPFLAGS+=-DTESTING
build/%_test.o: src/%.c 
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/%.c: src/%.y
	ln -fT $< $(patsubst %.c, %.y, $@)
	lemon -q $(patsubst %.c, %.y, $@)

build/%.c: src/%.re
	re2c $< -o $@

lib/libc8asm/libc8asm.a:
	$(MAKE) -C lib/libc8asm

build/lexer.o: build/parser.c

-include build/*.d

