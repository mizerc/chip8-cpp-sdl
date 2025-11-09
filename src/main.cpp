#include <stdlib.h>
#include <iostream>
#include <array>
#include <SDL.h>
#include "Chip8.h"

struct AudioState
{
	double sampleIndex = 0.0;
	double freq = 180.0;
	int sampleRate = 44100;
	bool play = false;
};

// SDL calls this function when it needs more audio samples
// It tells how much audio data it needs by the samplesToFill parameter
void audioCallback(void *userdata, Uint8 *streamToFill, int amountSamplesToFill)
{
	AudioState *state = (AudioState *)userdata;
	// format = AUDIO_F32   → float (4 bytes)
	float *buffer = (float *)streamToFill;
	int amountFloatSamples = amountSamplesToFill / sizeof(float);
	for (int i = 0; i < amountFloatSamples; i++)
	{
		float sample = 0.0f;
		if (state->play)
		{
			double time = state->sampleIndex / state->sampleRate;
			sample = (fmod(time * state->freq, 1.0) < 0.5) ? 0.5f : -0.5f;
		}
		buffer[i] = sample;
		state->sampleIndex += 1.0;
	}
}

int main(int argc, char **argv)
{
	if (argc != 5)
	{
		std::cerr << "Usage: " << argv[0] << "<CyclesPerFrame> <frameDurationTargetMs> <Scale> <ROM>\n";
		return EXIT_FAILURE;
	}
	int cyclesPerFrame = std::stoi(argv[1]);
	int frameDurationTargetMs = std::stoi(argv[2]);
	int videoScale = std::stoi(argv[3]);
	char const *romPath = argv[4];

	// uint32_t videoMemory[VIDEO_WIDTH * VIDEO_HEIGHT]{};
	SDL_Window *sdlWindow{};
	SDL_Renderer *sdlRenderer{};
	SDL_Texture *sdlTexture{};

	const int WINDOW_W = VIDEO_WIDTH * videoScale;
	const int WINDOW_H = VIDEO_HEIGHT * videoScale;

	std::cout << "hi\n";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0)
	{
		std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
		return EXIT_FAILURE;
	}

	// Create SDL Window
	sdlWindow = SDL_CreateWindow(
		"CHIP8 EMULATOR",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WINDOW_W,
		WINDOW_H,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (!sdlWindow)
	{
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
		SDL_Log("Error creating SDL Window: %s", SDL_GetError());
		SDL_Quit();
		return EXIT_FAILURE;
	}

	// Initialize SDL Renderer
	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);

	// Initialize SDL Texture
	sdlTexture = SDL_CreateTexture(
		sdlRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, VIDEO_WIDTH, VIDEO_HEIGHT);

	// Audio
	AudioState audio;
	SDL_AudioSpec want{}, have{};
	want.freq = audio.sampleRate;
	want.format = AUDIO_F32; // float 32 format
	want.channels = 1;		 // mono
	want.samples = 1024;
	want.callback = audioCallback;
	want.userdata = &audio;
	SDL_AudioDeviceID audioDev =
		SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
	if (!audioDev)
	{
		printf("SDL_OpenAudioDevice error: %s\n", SDL_GetError());
	}
	else
	{
		SDL_PauseAudioDevice(audioDev, 0);
	}

	// Create input state (scancode to use as index)
	std::array<bool, SDL_NUM_SCANCODES> keyDown{};

	// Application state
	bool quit = false;

	// Timing
	float lastFrameTimeMs = 0;

	// Initialize Chip-8 system
	Chip8 chip8;
	chip8.loadROM(romPath);

	// Main loop
	while (!quit)
	{
		// Time control
		Uint32 nowMs = SDL_GetTicks();
		float deltaTimeMs = (nowMs - lastFrameTimeMs);
		// Wait to meet cycle delay
		if (deltaTimeMs < frameDurationTargetMs)
		{
			continue;
		}
		lastFrameTimeMs = nowMs;

		// Poll events from queue
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			if (e.type == SDL_KEYDOWN)
			{
				keyDown[e.key.keysym.scancode] = true;
			}
			if (e.type == SDL_KEYUP)
			{
				keyDown[e.key.keysym.scancode] = false;
			}
		}

		// Process input
		if (keyDown[SDL_SCANCODE_ESCAPE])
		{
			quit = true;
		}
		// Map keyboard state to Chip-8 memory
		chip8.keypadMemory[0x0] = keyDown[SDL_SCANCODE_1];
		chip8.keypadMemory[0x1] = keyDown[SDL_SCANCODE_2];
		chip8.keypadMemory[0x2] = keyDown[SDL_SCANCODE_3];
		chip8.keypadMemory[0x3] = keyDown[SDL_SCANCODE_4];
		chip8.keypadMemory[0x4] = keyDown[SDL_SCANCODE_Q];
		chip8.keypadMemory[0x5] = keyDown[SDL_SCANCODE_W];
		chip8.keypadMemory[0x6] = keyDown[SDL_SCANCODE_E];
		chip8.keypadMemory[0x7] = keyDown[SDL_SCANCODE_R];
		chip8.keypadMemory[0x8] = keyDown[SDL_SCANCODE_A];
		chip8.keypadMemory[0x9] = keyDown[SDL_SCANCODE_S];
		chip8.keypadMemory[0xA] = keyDown[SDL_SCANCODE_D];
		chip8.keypadMemory[0xB] = keyDown[SDL_SCANCODE_F];
		chip8.keypadMemory[0xC] = keyDown[SDL_SCANCODE_Z];
		chip8.keypadMemory[0xD] = keyDown[SDL_SCANCODE_X];
		chip8.keypadMemory[0xE] = keyDown[SDL_SCANCODE_C];
		chip8.keypadMemory[0xF] = keyDown[SDL_SCANCODE_V];

		// Update object color based on frame time
		// for(unsigned int y = 0; y < VIDEO_HEIGHT; ++y) {
		// 	for(unsigned int x = 0; x < VIDEO_WIDTH; ++x) {
		// 		uint32_t *pixelAddr = &videoMemory[y * VIDEO_WIDTH + x];
		// 		*pixelAddr = ((x ^ y) & 0x1) ? 0xFFFFFFFF : 0xFF000000;
		// 	}
		// }

		for (unsigned int cycle = 0; cycle < cyclesPerFrame; ++cycle)
		{
			chip8.tick();
		}

		// Update audio state
		audio.play = (chip8.R_BUZZER_TIMER > 0);

		// Copy video memory to texture
		int rowSizeBytes = sizeof(uint32_t) * VIDEO_WIDTH;
		// void const* framebuffer = videoMemory;
		void const *framebuffer = chip8.videoMemory;
		SDL_UpdateTexture(sdlTexture, nullptr, framebuffer, rowSizeBytes);
		// Clear renderer
		SDL_RenderClear(sdlRenderer);
		// Copy texture to renderer
		// by default will scalethe texture to fit the destination window size
		SDL_RenderCopy(sdlRenderer, sdlTexture, nullptr, nullptr);
		// Present renderer
		SDL_RenderPresent(sdlRenderer);
	}

	// Cleanup audio
	SDL_CloseAudioDevice(audioDev);

	// Cleanup renderer and texture
	SDL_DestroyTexture(sdlTexture);
	SDL_DestroyRenderer(sdlRenderer);

	// Cleanup SDL
	SDL_DestroyWindow(sdlWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
