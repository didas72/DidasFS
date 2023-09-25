AR=ar
AR_FLAGS=rcs
CC=gcc
C_FLAGS=-Wall -Wextra -g -Wno-unknown-pragmas

SRC=src
TEST=tests
OBJ=build/obj
BIN=build/bin

OUTBIN=$(BIN)/main.a
TESTBIN=$(BIN)/tester

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TSRCS=$(wildcard $(TEST)/*.c)
TOBJS=$(patsubst $(TEST)/%.c, $(OBJ)/tests/%.o, $(TSRCS))


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



clean:
	$(RM) -r $(OBJ) $(BIN)

loc:
	scc -s lines --no-cocomo --no-gitignore -w --size-unit binary --exclude-ext md,makefile --exclude-dir tests/framework
