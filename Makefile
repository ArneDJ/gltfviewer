CC=g++
CFLAGS=-lm -lSDL2 -lGL -lGLEW
OUTPUT=gltfviewer.out

SRC = $(wildcard src/*.cpp)

main : $(src)
	$(CC) -o $(OUTPUT) $(SRC) $(CFLAGS)
