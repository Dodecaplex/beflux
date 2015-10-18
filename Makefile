CC = gcc
CCFLAGS = -Wall -Wextra -O2

EXE = beflux.exe

RM = rm -f

all: $(EXE)

$(EXE): obj/beflux.o
	$(CC) $< -o $@

obj/beflux.o: src\beflux.c src\beflux.h
	$(CC) $(CCFLAGS) -c $< -o $@

lib: src\beflux.c src\beflux.h
	$(CC) $(CCFLAGS) -DLIBBEFLUX -c src\beflux.c -o obj\libbeflux.o
	ar ruv libbeflux.a obj\libbeflux.o
	ranlib libbeflux.a

lib_test: src\libbeflux_test.c libbeflux.a
	$(CC) $(CCFLAGS) src\libbeflux_test.c libbeflux.a -o libbeflux_test.exe

clean:
	$(RM) obj/*.o *.exe libbeflux.a
