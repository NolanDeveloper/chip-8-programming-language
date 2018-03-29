.SUFFIXES:

CFLAGS   += -g -std=c99 -pedantic -Wall -Wextra -Werror 
CPPFLAGS += -Isrc -Ibuild

.PHONY: all
all: build/c8c

.PHONY: clean
clean:
	rm -rf build
	$(MAKE) -C lib/libc8asm clean

.PHONY: test
test: test-all

#####################################################################
### Rules for c files
#####################################################################

C_FILES   := $(shell find src -name "*.c")
O_C_FILES := $(patsubst src/%.c,build/%.o,$(C_FILES))

COMPILE_C = $(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c $< -o $@

$(O_C_FILES): build/%.o: src/%.c | dirs
	$(COMPILE_C)

#####################################################################
### Rules for Lemon parser files
#####################################################################

Y_FILES       := $(shell find src -name "*.y")
C_Y_FILES     := $(patsubst src/%.y,build/%.c,$(Y_FILES))
O_Y_FILES     := $(C_Y_FILES:.c=.o)
H_Y_FILES     := $(C_Y_FILES:.c=.h)
BUILD_Y_FILES := $(C_Y_FILES:.c=.y)

$(H_Y_FILES): %.h: %.c | dirs

$(BUILD_Y_FILES): build/%.y: src/%.y | dirs
	ln -s ../$< $@

$(C_Y_FILES): %.c: %.y | dirs
	lemon -q $<

$(O_Y_FILES): %.o: %.c | dirs
	$(COMPILE_C)

#####################################################################
### Rules for re2c files
#####################################################################

RE_FILES   := $(shell find src -name "*.re")
C_RE_FILES := $(patsubst src/%.re,build/%.c,$(RE_FILES))
O_RE_FILES := $(C_RE_FILES:.c=.o)

$(C_RE_FILES): build/%.c: src/%.re | dirs
	re2c $< -o $@ 

$(O_RE_FILES): %.o: %.c | dirs
	$(COMPILE_C)

#####################################################################
### Libraries
#####################################################################

LIBS     += lib/libc8asm/libc8asm.a
CPPFLAGS += -Ilib/libc8asm
LDFLAGS  += -Llib/libc8asm
LDLIBS   += -lc8asm

lib/libc8asm/libc8asm.a: | dirs
	$(MAKE) -C lib/libc8asm

#####################################################################
### Output executable
#####################################################################

SRCS := $(C_FILES) $(Y_FILES) $(RE_FILES)
OBJS := $(patsubst src/%,build/%.o,$(basename $(SRCS)))

LINK = $(CC) $(filter %.o,$^) -o $@ $(LDFLAGS) $(LDLIBS) 

build/c8c: $(OBJS) $(LIBS) | dirs
	$(LINK)

.PHONY: dirs
dirs:
	mkdir -p $(sort $(dir $(OBJS)))

#####################################################################
### Test executables
#####################################################################

# ToDo: how to test .re2c and .y files?

C_TEST_FILES   := $(shell find tests -name "*.c")
O_TEST_FILES   := $(patsubst tests/%.c,build/tests/%.o,$(C_TEST_FILES))
EXE_TEST_FILES := $(O_TEST_FILES:.o=.test)

$(O_TEST_FILES): CPPFLAGS+=-I$(patsubst build/tests/%,src/%,$(dir $@))
$(O_TEST_FILES): build/tests/%.o: tests/%.c | test_dirs 
	$(COMPILE_C)

$(EXE_TEST_FILES): %.test: %.o $(LIBS) | test_dirs
	$(LINK)

test-all: $(EXE_TEST_FILES)
	./runtests.sh

.PHONY: test_dirs
test_dirs:
	mkdir -p $(sort $(dir $(O_TEST_FILES)))

-include $(patsubst %.o,%.d,$(OBJS))
