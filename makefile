# C compiler options
CC := c99
FLAG = -g -O0 -o 
MAIN := server
SRC := src
INCLUDE := include
HEADER := header

# OS env
MK := touch
RM := rm

build:
	$(CC) $(FLAG) $(MAIN) $(SRC)/* -I$(INCLUDE) -lpthread

mkhdr:
	$(MK) $(INCLUDE)/$(HEADER).h

clean:
	$(RM) $(MAIN)
	
