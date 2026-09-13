# =====================================================================
#  Compila no Linux e no Windows (MinGW / MSYS2).
#
#  Linux:    make
#  Windows:  mingw32-make        (com freeglut instalado)
#
#  A variavel OS so existe no Windows; e' o que distingue os dois casos.
# =====================================================================

CXX      = g++
CXXFLAGS = -Wall -g -std=c++11 -Isrc
SRC      = $(wildcard src/*.cpp)
OBJ      = $(SRC:.cpp=.o)

ifeq ($(OS),Windows_NT)
    BIN    = pdi.exe
    LDLIBS = -lfreeglut -lopengl32 -lglu32
    LIMPAR = del /Q
else
    BIN    = pdi
    LDLIBS = -lglut -lGLU -lGL
    LIMPAR = rm -f
endif

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(LIMPAR) $(OBJ) $(BIN)

run: $(BIN)
	./$(BIN)

.PHONY: all clean run
