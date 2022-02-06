# C compiler options
CC := c99
CFLAG = -g -O0 -c -Wall
MAIN := cserver
SRC_DIR := src
INCLUDE_DIR := include
THREADPOOL_DIR := threadpool/src
OBJ := obj

# OS env
RM := rm
MKDIR := mkdir

DEPS := $(patsubst $(SRC_DIR)/%.c, $(OBJ)/%.o, $(wildcard *.c, $(SRC_DIR)/*.c)) $(patsubst $(THREADPOOL_DIR)/%.c, $(OBJ)/%.o, $(wildcard *.c, $(THREADPOOL_DIR)/*.c))

build: $(OBJ) $(MAIN)
	$(RM) -rf $(OBJ)

$(MAIN): $(DEPS)
	$(CC) -o $(MAIN) $^ -lpthread 

$(OBJ):
	$(MKDIR) $@

$(OBJ)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAG) -o $@ $^ -I$(INCLUDE_DIR) -I$(THREADPOOL_DIR)

$(OBJ)/%.o: $(THREADPOOL_DIR)/%.c
	$(CC) $(CFLAG) -o $@ $^ -I$(INCLUDE_DIR) -I$(THREADPOOL_DIR)

clean:
	$(RM) $(MAIN)
