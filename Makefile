
# Default target
all:
	mkdir -p dist
	g++ -Iinclude -g -fPIE -O1 --std=c++23 -march=native source/*.cpp -o dist/out
	objdump -d dist/out -l > dist/asm.s
