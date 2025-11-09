all: clean build run

build:
	mkdir -p build
	g++ \
	 `sdl2-config --libs --cflags` \
	 -std=c++17  \
	 -Wall -lm \
	 -o ./build/chip8 \
	 ./src/*.cpp

run:
# 	./build/chip8 10 30 10 ./roms/IBM_Logo.ch8
# 	./build/chip8 5 16 10 ./roms/Pong1player.ch8
# 	./build/chip8 10 16 10 ./roms/Rocket_Launcher.ch8
# 	./build/chip8 10 16 10 ./roms/Space_Invaders_David_Winter_alt.ch8
	./build/chip8 10 16 10 ./roms/Space_Invaders_David_Winter.ch8
# 	./build/chip8 10 16 10 ./roms/Tetris_Fran_Dachille_1991.ch8

clean:
	rm -rf build