// Copyright 2018 David Brotz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
#include "common.h"
#include "machine.h"
#include "SDL.h"

const std::array<Uint32, 4> rgb_table = { 0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000 };

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: gb_emu ROM");
        return 1;
    }

    char* rom_file_name = argv[1];

    FILE* rom_file = fopen(rom_file_name, "rb");
    fseek(rom_file, 0, SEEK_END);
    long file_size = ftell(rom_file);
    size_t buffer_size = std::max(file_size, 0x8000L);
    rewind(rom_file);
    std::vector<u8> rom(buffer_size, 0);

    if (fread(rom.data(), file_size, 1, rom_file) != 1)
    {
        perror(nullptr);
        return 1;
    }

    fclose(rom_file);

    Machine machine;

    machine.LoadROM(std::move(rom));
    machine.Reset();

    const int screen_scale = 2;
    const int screen_width = lcd_width * screen_scale;
    const int screen_height = lcd_height * screen_scale;

    if (SDL_Init(SDL_INIT_VIDEO) == -1)
    {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);

    SDL_Window *window = SDL_CreateWindow("GB Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, 0);

    if (window == nullptr)
    {
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr)
    {
        fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_RenderSetLogicalSize(renderer, lcd_width, lcd_height) < 0)
    {
        fprintf(stderr, "Unable to set logical size for rendering: %s\n", SDL_GetError());
    }

    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, lcd_width, lcd_height);

    if (texture == nullptr)
    {
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        return 1;
    }

    for (;;)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                return 0;
            }
        }

        machine.Run(17556);

        const FramebufferArray& fb = machine.GetFramebuffer();

        std::array<Uint32, lcd_width * lcd_height> pixels;

        for (int y = 0; y < lcd_height; y++)
        {
            for (int x = 0; x < lcd_width; x++)
            {
                u8 color_index = fb[(y * lcd_width) + x];
                pixels[(y * lcd_width) + x] = rgb_table[color_index];
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels.data(), lcd_width * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    return 0;
}
