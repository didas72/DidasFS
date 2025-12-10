AR=ar
AR_FLAGS=rcs
CC=gcc
C_FLAGS=-Wall -Wextra -pedantic -ggdb -Wno-unknown-pragmas
VAL_FLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes -s
MOCK_FLAGS=-DMOCK_DEVICE
#MOCK_FLAGS=

DIR_SRC=src
DIR_TEST=tests
DIR_OBJ=build/obj
DIR_BIN=build/bin
DIR_DOC=doc

OUTLIB=$(DIR_BIN)/libdfs.a
TESTLIB=$(DIR_BIN)/libdfs_test.a
TESTBIN=$(DIR_BIN)/tester

SRCS=$(shell find $(DIR_SRC) -type f -name '*.c')
OBJS=$(patsubst $(DIR_SRC)/%.c,$(DIR_OBJ)/%.o,$(SRCS))

TSOBJS=$(patsubst $(DIR_SRC)/%.c,$(DIR_OBJ)/%_test.o,$(SRCS))

TSRCS=$(shell find $(DIR_TEST) -type f -name '*.c')
TOBJS=$(patsubst $(DIR_TEST)/%.c,$(DIR_OBJ)/tests/%.o,$(TSRCS))


.PHONY: all build build-tests test debug memleak release loc doc clean-all clean clean-tests clean-doc


all: $(OUTLIB)
build: $(OUTLIB)
build-tests: $(TESTBIN)
rebuild: clean $(OUTLIB)
rebuild-tests: clean $(TESTBIN)

release: C_FLAGS=-Wall -Wextra -O2 -Wno-unknown-pragmas
release: clean $(OUTLIB)



$(DIR_OBJ)/%_test.o: $(DIR_SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -include tests/mocks_interface.h $(MOCK_FLAGS) -o $@ -c $<

$(DIR_OBJ)/%.o: $(DIR_SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -o $@ -c $<

$(OUTLIB): $(OBJS)
	@mkdir -p $(@D)
	$(AR) $(AR_FLAGS) $@ $^

$(TESTLIB): $(TSOBJS)
	@mkdir -p $(@D)
	$(AR) $(AR_FLAGS) $@ $^

$(TESTBIN): $(TESTLIB) $(TOBJS)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) $(TOBJS) -o $@ -lrt -lm -ldfs_test -L$(DIR_BIN)

$(DIR_OBJ)/tests/%.o: $(DIR_TEST)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -Wno-unused-function $(MOCK_FLAGS) -o $@ -c $<



test: $(TESTBIN) clean-tests
	./$(TESTBIN) 2> stderr_redirect.log

debug: $(TESTBIN)
	gdb ./$(TESTBIN)

val: $(TESTBIN)
	valgrind $(VAL_FLAGS) $(TESTBIN)

loc:
	@cloc --hide-rate $(DIR_SRC) 
	
doc:
	doxygen Doxyfile


clean-all: clean clean-tests clean-doc

clean:
	@$(RM) -r $(DIR_OBJ) $(DIR_BIN)

clean-tests:
	@$(RM) *.hex *.log

clean-doc:
	@$(RM) -r $(DIR_DOC)/doxygen
