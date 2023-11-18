AR=ar
AR_FLAGS=rcs
CC=gcc
C_FLAGS=-Wall -Wextra -ggdb -Wno-unknown-pragmas
VAL_FLAGS=--leak-check=full

SRC=src
TEST=tests
OBJ=build/obj
BIN=build/bin
DOC=doc

OUTBIN=$(BIN)/main.a
TESTBIN=$(BIN)/tester

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TSRCS=$(wildcard $(TEST)/*.c)
TOBJS=$(patsubst $(TEST)/%.c, $(OBJ)/tests/%.o, $(TSRCS))


.PHONY: all
.PHONY: build
.PHONY: build-tests
.PHONY: test
.PHONY: debug
.PHONY: memleak
.PHONY: release
.PHONY: clean
.PHONY: loc
.PHONY: doc
.PHONY: doc-clean


all: $(OUTBIN)
build: $(OUTBIN)
build-tests: $(TESTBIN)

release: C_FLAGS=-Wall -Wextra -O2 -Wno-unknown-pragmas
release: clean
release: $(OUTBIN)



$(OUTBIN): $(OBJS)
	@mkdir -p $(@D)
	$(AR) $(AR_FLAGS) $@ $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@



test: $(TESTBIN)
	./$(TESTBIN)

$(TESTBIN): $(OBJS) $(TOBJS)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) $^ -o $@

$(OBJ)/tests/%.o: $(TEST)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@



debug: $(TESTBIN)
	gdb ./$(TESTBIN)

memleak: $(TESTBIN)
	valgrind $(VAL_FLAGS) $(TESTBIN)


clean:
	$(RM) -r $(OBJ) $(BIN)

loc:
	scc -s lines --no-cocomo --no-gitignore -w --size-unit binary --exclude-ext md,makefile --exclude-dir tests/framework

doc:
	doxygen Doxyfile

clean-doc:
	$(RM) -r $(DOC)/doxygen
