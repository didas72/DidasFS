AR=ar
AR_FLAGS=rcs
CC=gcc
C_FLAGS=-Wall -Wextra -ggdb -Wno-unknown-pragmas
VAL_FLAGS=--leak-check=full --show-leak-kinds=all --track-origins=yes -s

SRC=src
TEST=tests
OBJ=build/obj
BIN=build/bin
DOC=doc

OUTBIN=$(BIN)/libdfs.a
TESTBIN=$(BIN)/tester

SRCS=$(shell find $(SRC) -type f -name '*.c')
OBJS=$(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(SRCS))

TSRCS=$(wildcard $(TEST)/*.c)
TOBJS=$(patsubst $(TEST)/%.c,$(OBJ)/tests/%.o,$(TSRCS))


.PHONY: all build build-tests test debug memleak release loc doc clean-all clean clean-tests clean-doc


all: $(OUTBIN)
build: $(OUTBIN)
build-tests: $(TESTBIN)
rebuild: clean $(OUTBIN)
rebuild-tests: clean $(TESTBIN)

release: C_FLAGS=-Wall -Wextra -O2 -Wno-unknown-pragmas
release: clean
release: $(OUTBIN)



$(OUTBIN): $(OBJS)
	@mkdir -p $(@D)
	$(AR) $(AR_FLAGS) $@ $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@



test: $(TESTBIN) clean-tests
	./$(TESTBIN) 2> stderr_redirect.log

$(TESTBIN): $(TOBJS) $(OUTBIN)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) $(TOBJS) -o $@ -ldfs -Lbuild/bin

$(OBJ)/tests/%.o: $(TEST)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@



debug: $(TESTBIN)
	gdb ./$(TESTBIN)

memleak: $(TESTBIN)
	valgrind $(VAL_FLAGS) $(TESTBIN)

loc:
	@scc -s lines --no-cocomo --no-gitignore -w --size-unit binary --exclude-dir tests/framework src

doc:
	doxygen Doxyfile


clean-all: clean clean-tests clean-doc

clean:
	@$(RM) -r $(OBJ) $(BIN)

clean-tests:
	@$(RM) *.hex *.log

clean-doc:
	@$(RM) -r $(DOC)/doxygen
