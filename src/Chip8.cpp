#include "Chip8.h"

uint8_t Chip8::getRandomByte()
{
	static std::mt19937 rng{std::random_device{}()};
	static std::uniform_int_distribution<int> dist(0, 255);
	return static_cast<uint8_t>(dist(rng));
}

Chip8::Chip8()
{
	// Initialize PC
	R_PC = ROM_START_ADDRESS;

	// Load font set into memory
	for (unsigned int i = 0; i < FONTSET_SIZE; ++i)
	{
		memory[FONTSET_START_ADDRESS + i] = FONTSET[i];
	}

	// Route opcodes to function handlers
	// using member function pointers
	routerTable[0] = &Chip8::SubHandlerFn0;
	routerTable[1] = &Chip8::OP_1nnn;
	routerTable[2] = &Chip8::OP_2nnn;
	routerTable[3] = &Chip8::OP_3xkk;
	routerTable[4] = &Chip8::OP_4xkk;
	routerTable[5] = &Chip8::OP_5xy0;
	routerTable[6] = &Chip8::OP_6xkk;
	routerTable[7] = &Chip8::OP_7xkk;
	routerTable[8] = &Chip8::SubHandlerFn8;
	routerTable[9] = &Chip8::OP_9xy0;
	routerTable[0xA] = &Chip8::OP_Annn;
	routerTable[0xB] = &Chip8::OP_Bnnn;
	routerTable[0xC] = &Chip8::OP_Cxkk;
	routerTable[0xD] = &Chip8::OP_Dxyn;
	routerTable[0xE] = &Chip8::SubHandlerFnE;
	routerTable[0xF] = &Chip8::SubHandlerFnF;

	// Initialize sub-tables with DO_NOTHING
	for (size_t i = 0; i <= 15; i++)
	{
		subTable0[i] = &Chip8::DO_NOTHING;
		subTable8[i] = &Chip8::DO_NOTHING;
		subTableE[i] = &Chip8::DO_NOTHING;
	}
	for (size_t i = 0; i <= 255; i++)
	{
		subTableF[i] = &Chip8::DO_NOTHING;
	}

	subTable0[0x0] = &Chip8::OP_00E0;
	subTable0[0xE] = &Chip8::OP_00EE;

	subTable8[0x0] = &Chip8::OP_8xy0;
	subTable8[0x1] = &Chip8::OP_8xy1;
	subTable8[0x2] = &Chip8::OP_8xy2;
	subTable8[0x3] = &Chip8::OP_8xy3;
	subTable8[0x4] = &Chip8::OP_8xy4;
	subTable8[0x5] = &Chip8::OP_8xy5;
	subTable8[0x6] = &Chip8::OP_8xy6;
	subTable8[0x7] = &Chip8::OP_8xy7;
	subTable8[0xE] = &Chip8::OP_8xyE;

	subTableE[0x1] = &Chip8::OP_ExA1;
	subTableE[0xE] = &Chip8::OP_Ex9E;

	subTableF[0x07] = &Chip8::OP_Fx07;
	subTableF[0x0A] = &Chip8::OP_Fx0A;
	subTableF[0x15] = &Chip8::OP_Fx15;
	subTableF[0x18] = &Chip8::OP_Fx18;
	subTableF[0x1E] = &Chip8::OP_Fx1E;
	subTableF[0x29] = &Chip8::OP_Fx29;
	subTableF[0x33] = &Chip8::OP_Fx33;
	subTableF[0x55] = &Chip8::OP_Fx55;
	subTableF[0x65] = &Chip8::OP_Fx65;
}

void Chip8::loadROM(char const *filepath)
{
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file)
	{
		std::cerr << "Failed to open ROM: " << filepath << "\n";
		return;
	}

	// Get file size
	std::streampos lastPos = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> buffer(lastPos);
	if (!file.read(reinterpret_cast<char *>(buffer.data()), lastPos))
	{
		throw std::runtime_error("Failed to read file");
	}
	for (long i = 0; i < lastPos; ++i)
	{
		memory[ROM_START_ADDRESS + i] = buffer[i];
		// std::cout << "Buffer[" << i << "] = " << std::hex << (static_cast<uint16_t>(static_cast<uint8_t>(buffer[i]))) << "\n";
	}

	file.close();
	std::cout << "Loaded ROM: " << filepath << "\n";
	std::cout << "ROM size = " << std::dec << lastPos << "\n";
}

void Chip8::tick()
{
	// CPU CYCLE => FETCH, DECODE, EXECUTE

	// 1) FETCH
	// Fetch 2 bytes from memory
	// Big-endian (MSB first)
	// Memory[0] = 0xAB = high byte at lower/first address
	// Memory[1] = 0xCD = low byte
	// Opcode = 0xABCD
	uint8_t highByte = memory[R_PC];
	uint8_t lowByte = memory[R_PC + 1];
	opcode = (highByte << 8) | lowByte;
	// std::cout << "Opcode: " << std::hex << opcode << "\n";

	// Increment the PC by 2 bytes
	R_PC += 2;

	// 2) DECODE
	// 0xABCD => 2 bytes => 1 byte = 0xAB, 1 byte = 0xCD
	// [ 1 byte ][ 1 byte ]
	// [ 4 bits  ][ 4 bits  ][ 4 bits  ][ 4 bits  ]
	// [ nibble3 ][ nibble2 ][ nibble1 ][ nibble0 ]
	// OP X Y N
	// 0xDXYN
	const uint8_t nibble3 = (opcode & 0xF000u) >> 12u;
	assert(nibble3 >= 0);
	assert(nibble3 <= 15);
	auto pointerToMemberFn = routerTable[nibble3];

	// 3) EXECUTE
	// ->* is pointer to member operator
	// Call a member function of this instance
	// using the address stored in pointerToFunc.
	(this->*pointerToMemberFn)();

	// INTERNALS
	// Decrement the delay timer
	if (R_DELAY_TIMER > 0)
	{
		--R_DELAY_TIMER;
	}

	// Decrement the sound timer
	if (R_BUZZER_TIMER > 0)
	{
		--R_BUZZER_TIMER;
	}
}

void Chip8::DO_NOTHING()
{
	// NOP
}

void Chip8::SubHandlerFn0()
{
	auto nibble0 = opcode & 0x000Fu;
	assert(nibble0 >= 0);
	assert(nibble0 <= 15);
	(this->*subTable0[nibble0])();
}

void Chip8::SubHandlerFn8()
{
	auto nibble0 = opcode & 0x000Fu;
	assert(nibble0 >= 0);
	assert(nibble0 <= 15);
	(this->*subTable8[nibble0])();
}

void Chip8::SubHandlerFnE()
{
	auto nibble0 = opcode & 0x000Fu;
	assert(nibble0 >= 0);
	assert(nibble0 <= 15);
	(this->*subTableE[nibble0])();
}

void Chip8::SubHandlerFnF()
{
	uint8_t lowByte = opcode & 0x00FFu;
	assert(lowByte <= 255);
	(this->*subTableF[lowByte])();
}

void Chip8::OP_00E0()
{
	// CLS
	// No arguments
	// Clear the video memory with zeros
	memset(videoMemory, 0, sizeof(videoMemory));
}

void Chip8::OP_00EE()
{
	// RET
	// No arguments
	// Pop the last address from the stack and set the PC to it
	--R_SP;
	R_PC = stackMemory[R_SP];
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ 1xxx, 2xxx, Bxxx — Jumps and Calls
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_1nnn()
{
	// JP address
	// Address: nnn (12 bits)
	// Jump to address nnn
	uint16_t address = opcode & 0x0FFFu;
	R_PC = address;
}

void Chip8::OP_2nnn()
{
	// CALL address
	// Address: nnn (12 bits)
	// Call subroutine at nnn
	// Push current PC to stack and set PC to nnn
	uint16_t address = opcode & 0x0FFFu;
	stackMemory[R_SP] = R_PC;
	++R_SP;
	R_PC = address;
}

void Chip8::OP_Bnnn()
{
	// Jump to NNN + V0
	// Address: nnn (12 bits)
	// Jump to address nnn + V0
	// Set PC to V0 + nnn
	uint16_t address = opcode & 0x0FFFu;
	R_PC = REG[0] + address;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ 3xxx, 4xxx, 5xxx and 9xxx — Conditional skips
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_3xkk()
{
	// skip if VX == NN
	// Vx: (4 bits) X = values 0-F
	// Byte: kk (8 bits)
	// If Vx == kk, increment PC by 2
	// 	Opcode = 0xABCD
	// 0xABCD & 0x0F00 = 0x0B00
	// 0x0B00 >> 8u = 0x0B
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	if (REG[Vx] == byte)
	{
		R_PC += 2;
	}
}

void Chip8::OP_4xkk()
{
	// skip if VX != NN
	// Vx: (4 bits) X = values 0-F
	// Byte: kk (8 bits)
	// If Vx != kk, increment PC by 2
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	if (REG[Vx] != byte)
	{
		R_PC += 2;
	}
}

void Chip8::OP_5xy0()
{
	// skip if VX == VY
	// Vx: (4 bits) X = values 0-F
	// Vy: (4 bits) Y = values 0-F
	// If Vx == Vy, increment PC by 2
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	if (REG[Vx] == REG[Vy])
	{
		R_PC += 2;
	}
}

void Chip8::OP_9xy0()
{
	// skip if VX != VY
	// Vx: (4 bits) X = values 0-F
	// Vy: (4 bits) Y = values 0-F
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	if (REG[Vx] != REG[Vy])
	{
		R_PC += 2;
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ 6xxx–7xxx — Loads / Adds
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_6xkk()
{
	// LD Vx, byte
	// LOAD byte kk into register Vx
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	REG[Vx] = byte;
}

void Chip8::OP_7xkk()
{
	// ADD Vx, byte
	// Set Vx = Vx + kk
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	REG[Vx] += byte;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ 8xxx — Arithmetic / Bitwise
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_8xy0()
{
	// LD Vx, Vy, aka, Vx <= Vy
	// This copies the value in register Vy into register Vx.
	// The flag register VF is not affected.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	REG[Vx] = REG[Vy];
}

void Chip8::OP_8xy1()
{
	// OR Vx, Vy
	// 8XY1 performs a bitwise OR between two REG.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	REG[Vx] |= REG[Vy];
}

void Chip8::OP_8xy2()
{
	// AND Vx, Vy
	// 8XY2 performs a bitwise AND between two REG.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	REG[Vx] &= REG[Vy];
}

void Chip8::OP_8xy3()
{
	// XOR Vx, Vy
	// 8XY3 performs a bitwise XOR between two REG.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	REG[Vx] ^= REG[Vy];
}

void Chip8::OP_8xy4()
{
	// ADD Vx, Vy
	// Set Vx = Vx + Vy, set VF = carry
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint16_t sum = REG[Vx] + REG[Vy];
	// Set carry flag
	if (sum > 255u)
	{
		REG[0xF] = 1;
	}
	else
	{
		REG[0xF] = 0;
	}
	REG[Vx] = sum & 0xFFu;
}

void Chip8::OP_8xy5()
{
	// SUB Vx, Vy
	// Set Vx = Vx - Vy, set VF = NOT borrow
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	if (REG[Vx] > REG[Vy])
	{
		REG[0xF] = 1;
	}
	else
	{
		REG[0xF] = 0;
	}
	REG[Vx] -= REG[Vy];
}

void Chip8::OP_8xy6()
{
	// the value in Vy is shifted right by 1.
	// Set Vx = Vx SHR 1
	// VF is set to the least significant bit of Vx before the shift.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	// Save LSB in VF
	REG[0xF] = (REG[Vx] & 0x0001u);
	REG[Vx] = REG[Vx] >> 1;
}

void Chip8::OP_8xy7()
{
	// SUBN Vx, Vy
	// This is a reverse subtract instruction
	// Vy minus Vx → stored in Vx.
	// Set Vx = Vy - Vx, set VF = NOT borrow
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	// If no borrow, set VF to 1
	// Has borrow if Vx >= Vy
	// 20 − 20 = 0 => No borrow → VF = 1
	if (REG[Vy] >= REG[Vx])
	{
		REG[0xF] = 1;
	}
	else
	{
		REG[0xF] = 0;
	}
	REG[Vx] = REG[Vy] - REG[Vx];
}

void Chip8::OP_8xyE()
{
	// SHL Vx
	// the value in Vx is shifted left by 1.
	// VF = MSB (bit 7) of Vy before the shift
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t bit7 = (REG[Vx] & 0x80u) >> 7u;
	REG[0xF] = bit7;
	REG[Vx] <<= 1;
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ Annn, Bnnn, Cnnn, Dnnn — Memory and Random
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// Ax — Index register
void Chip8::OP_Annn()
{
	// LD I, address
	// Address: nnn (12 bits)
	// Set index register = nnn
	uint16_t address = opcode & 0x0FFFu;
	R_I = address;
}

void Chip8::OP_Cxkk()
{
	// RND Vx, byte
	// VX = rand() & NN
	// Set Vx = random byte AND kk
	// Vx: (4 bits) X = values 0-F
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;
	REG[Vx] = getRandomByte() & byte;
}

void Chip8::OP_Dxyn()
{
	// DRW Vx, Vy, height
	// Drawing sprites to the screen
	// Reads N bytes from memory starting at I.
	// Draw N-byte sprite at (VX, VY), set VF on collision

	// Vx: (4 bits) X = values 0-F
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	// Vy: (4 bits) Y = values 0-F
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	// Height: n (4 bits)
	uint8_t numRows = opcode & 0x000Fu;

	// Reset VF to check for collisions
	REG[0xF] = 0;

	// Wrap around screen coordinates
	uint8_t startX = REG[Vx] % VIDEO_WIDTH;
	uint8_t startY = REG[Vy] % VIDEO_HEIGHT;
	for (uint8_t row = 0; row < numRows; ++row)
	{
		uint8_t spriteByte = memory[R_I + row];
		for (uint8_t col = 0; col < 8; ++col)
		{
			uint8_t spritePixel = spriteByte & (0b1000'0000 >> col);
			uint8_t x = (startX + col) % VIDEO_WIDTH;
			uint8_t y = (startY + row) % VIDEO_HEIGHT;
			uint32_t *screenPixel = &videoMemory[y * VIDEO_WIDTH + x];
			if (spritePixel)
			{
				if (*screenPixel == 0xFFFFFFFF)
				{
					// Collision detected
					REG[0xF] = 1;
				}
				// Always XOR the pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ Ex — Keypad skip
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_Ex9E()
{
	// SKP Vx
	// skip if key VX pressed
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = REG[Vx];
	if (keypadMemory[key])
	{
		R_PC += 2;
	}
}

void Chip8::OP_ExA1()
{
	// SKNP Vx
	// skip if key VX not pressed
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t key = REG[Vx];
	if (!keypadMemory[key])
	{
		R_PC += 2;
	}
}

void Chip8::OP_Fx0A()
{
	// LD Vx, K
	// Wait for a key press, store the value of the key in Vx
	// Block until a key is pressed, store the value of the key in Vx
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	bool keyFound = false;
	for (uint8_t k = 0; k < 16; k++)
	{
		if (keypadMemory[k])
		{
			REG[Vx] = k;
			keyFound = true;
			break;
		}
	}
	if (!keyFound)
	{
		// Repeat this instruction by preventing PC from advancing
		R_PC -= 2;
	}
}

// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// @@@ Ex — TIMER
// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void Chip8::OP_Fx07()
{
	// LD Vx, DT
	// Set Vx = delay timer value
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	REG[Vx] = R_DELAY_TIMER;
}

void Chip8::OP_Fx15()
{
	// LD DT, Vx
	// delay timer = VX
	// Set delay timer = Vx
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	R_DELAY_TIMER = REG[Vx];
}

void Chip8::OP_Fx18()
{
	// LD ST, Vx
	// sound timer = VX
	// Set sound timer = Vx
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	R_BUZZER_TIMER = REG[Vx];
}

void Chip8::OP_Fx1E()
{
	// ADD I, Vx
	// Set I = I + Vx
	// Add the value of register Vx to register I
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	R_I += REG[Vx];
}

void Chip8::OP_Fx29()
{
	// LD F, Vx
	// Set I = location of FONT_SPRITE for digit Vx
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = REG[Vx];
	R_I = FONTSET_START_ADDRESS + (BYTES_PER_CHAR * digit);
}

void Chip8::OP_Fx33()
{
	// LD BCD, Vx
	// Store BCD representation of Vx in memory locations I, I+1, and I+2
	// This converts the value in Vx (0–255) into three decimal digits
	// So the value is stored as decimal digits, not ASCII, not binary.
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = REG[Vx];

	// If Vx = 254 and I = 300
	// memory[300] = 2 (2 x 100)
	// memory[301] = 5 (5 x 10)
	// memory[302] = 4 (4 x 1)

	memory[R_I + 0] = value / 100;
	memory[R_I + 1] = (value / 10) % 10;
	memory[R_I + 2] = value % 10;
}

void Chip8::OP_Fx55()
{
	// Store REG V0 through Vx into memory starting at I
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	for (uint8_t w = 0; w <= Vx; ++w)
	{
		memory[R_I + w] = REG[w];
	}
}

void Chip8::OP_Fx65()
{
	// 0xF265, 0xF365, ...
	// Load REG V0 through Vx from memory starting at I
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	for (uint8_t w = 0; w <= Vx; ++w)
	{
		REG[w] = memory[R_I + w];
	}
}
