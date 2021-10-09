# C compiler options
CC := c99
CFLAG = -g -O0
MAIN := server
SRC_DIR := src
TARGET_DIR := obj
INCLUDE_DIR := include

# OS env
RM := rm
MKDIR := mkdir

DEPS = $(patsubst $(SRC_DIR)/%.c, $(TARGET_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))

build: $(TARGET_DIR) $(MAIN)

$(TARGET_DIR):
	$(MKDIR) $(TARGET_DIR)

$(TARGET_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAG) -o $@ $^ -c -I$(INCLUDE_DIR)

$(MAIN): $(DEPS)
	$(CC) -o $@ $^ -lpthread


clean:
	$(RM) -rf $(TARGET_DIR)
	$(RM) $(MAIN)	
