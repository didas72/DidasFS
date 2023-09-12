CC=gcc
C_FLAGS=-Wall -Wextra -g

SRC=src
OBJ=build/obj
BIN=build/bin

OUTBIN=$(BIN)/main

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))


all:$(OUTBIN)

release: C_FLAGS=-Wall -Wextra -O2
release: clean
release: $(OUTBIN)

run: $(OUTBIN)
	./$(OUTBIN)

$(OUTBIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) $(OBJS) -o $@

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)/* $(BIN)/*
