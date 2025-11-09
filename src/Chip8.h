#pragma once

#include <iostream>
#include <fstream>
#include <random>
#include <cassert>

// Video
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;
// Keypad
const unsigned int KEY_COUNT = 16;
// Memory
const unsigned int MEMORY_SIZE = 4096;
// CPU registers
const unsigned int REGISTER_COUNT = 16;
// Stack levels
const unsigned int STACK_LEVELS = 16;
// ROM
const unsigned int ROM_START_ADDRESS = 0x200;
// FONT
const unsigned int BYTES_PER_CHAR = 5;
const unsigned int FONTSET_SIZE = 16 * BYTES_PER_CHAR;
const unsigned int FONTSET_START_ADDRESS = 0x50;

// clang-format off
const uint8_t FONTSET[FONTSET_SIZE] =
{
	// 0
	0b1111'0000, 
	0b1001'0000,
	0b1001'0000,
	0b1001'0000,
	0b1111'0000,
	// 1
	0b0010'0000,
	0b0110'0000, 
	0b0010'0000, 
	0b0010'0000, 
	0b0111'0000,
	// 2
	0b1111'0000,
	0b0001'0000,
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	// 3
	0b1111'0000,
	0b0001'0000,
	0b1111'0000,
	0b0001'0000,
	0b1111'0000,
	// 4
	0b1001'0000,
	0b1001'0000,
	0b1111'0000,
	0b0001'0000,
	0b0001'0000,
	// 5
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	0b0001'0000,
	0b1111'0000,
	// 6
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	0b1001'0000,
	0b1111'0000,
	// 7
	0b1111'0000,
	0b0001'0000,
	0b0010'0000,
	0b0100'0000,
	0b0100'0000,
	// 8
	0b1111'0000,
	0b1001'0000,
	0b1111'0000,
	0b1001'0000,
	0b1111'0000,
	// 9
	0b1111'0000,
	0b1001'0000,
	0b1111'0000,
	0b0001'0000,
	0b1111'0000,
	// A
	0b1111'0000,
	0b1001'0000,
	0b1111'0000,
	0b1001'0000,
	0b1001'0000,
	// B
	0b1110'0000,
	0b1001'0000,
	0b1110'0000,
	0b1001'0000,
	0b1110'0000,
	// C
	0b1111'0000,
	0b1000'0000,
	0b1000'0000,
	0b1000'0000,
	0b1111'0000,
	// D
	0b1110'0000,
	0b1001'0000,
	0b1001'0000,
	0b1001'0000,
	0b1110'0000,
	// E
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	// F
	0b1111'0000,
	0b1000'0000,
	0b1111'0000,
	0b1000'0000,
	0b1000'0000
};
// clang-format on

class Chip8
{
public:
	uint8_t keypadMemory[KEY_COUNT]{};
	uint32_t videoMemory[VIDEO_WIDTH * VIDEO_HEIGHT]{};
	uint8_t R_BUZZER_TIMER{};
	Chip8();
	void loadROM(char const *filename);
	void tick();

private:
	uint8_t getRandomByte();
	void DO_NOTHING();

	// Opcode implementations, 34 total
	void OP_00E0();
	void OP_00EE();
	void OP_1nnn();
	void OP_2nnn();
	void OP_3xkk();
	void OP_4xkk();
	void OP_5xy0();
	void OP_6xkk();
	void OP_7xkk();
	void OP_8xy0();
	void OP_8xy1();
	void OP_8xy2();
	void OP_8xy3();
	void OP_8xy4();
	void OP_8xy5();
	void OP_8xy6();
	void OP_8xy7();
	void OP_8xyE();
	void OP_9xy0();
	void OP_Annn();
	void OP_Bnnn();
	void OP_Cxkk();
	void OP_Dxyn();
	void OP_Ex9E();
	void OP_ExA1();
	void OP_Fx07();
	void OP_Fx0A();
	void OP_Fx15();
	void OP_Fx18();
	void OP_Fx1E();
	void OP_Fx29();
	void OP_Fx33();
	void OP_Fx55();
	void OP_Fx65();

	// Functions to handle nested routing
	void SubHandlerFn0();
	void SubHandlerFn8();
	void SubHandlerFnE();
	void SubHandlerFnF();

	// Table to hold member function pointers
	using OpFunc = void (Chip8::*)();
	OpFunc routerTable[16]; // 0-15 or 0-xF
	OpFunc subTable0[16];	// 0-15 or 0-xF
	OpFunc subTable8[16];	// 0-15 or 0-xF
	OpFunc subTableE[16];	// 0-15 or 0-xF
	OpFunc subTableF[256];	// 0-255 or 1 byte

	// Current opcode
	uint16_t opcode{};

	uint8_t memory[MEMORY_SIZE]{};
	uint16_t stackMemory[STACK_LEVELS]{};
	// V0-VF, V0 = register[0]
	uint8_t REG[REGISTER_COUNT]{};
	// Index register
	uint16_t R_I{};
	// Stack pointer register
	uint8_t R_SP{};
	// Program counter register
	uint16_t R_PC{};
	// Timer/sound registers
	uint8_t R_DELAY_TIMER{};
};
