CC = gcc
CCFLAGS = -g -Wall

RM = rm -f

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
EXE = beflux.exe

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(OBJ) -o $@

obj/beflux.o: src\beflux.c src\beflux.h
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	$(RM) obj/*.o $(EXE)
