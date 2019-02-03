// Copyright 2018-2019 David Brotz
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
#include <string.h>
#include <vector>
#include <string>
#include <mutex>
#include <algorithm>
#include "common.h"
#include "machine.h"
#include "SDL.h"

const unsigned int num_audio_channels = 2;
const unsigned int sdl_audio_buffer_size = 4096;

void UpdateJoypad(Machine& machine)
{
    const Uint8* state = SDL_GetKeyboardState(nullptr);

    u8 dpad_keys = 0;
    u8 button_keys = 0;

    if (state[SDL_SCANCODE_RIGHT])
    {
        dpad_keys |= joyp_dpad_right;
    }

    if (state[SDL_SCANCODE_LEFT])
    {
        dpad_keys |= joyp_dpad_left;
    }

    if (state[SDL_SCANCODE_UP])
    {
        dpad_keys |= joyp_dpad_up;
    }

    if (state[SDL_SCANCODE_DOWN])
    {
        dpad_keys |= joyp_dpad_down;
    }

    if (state[SDL_SCANCODE_X])
    {
        button_keys |= joyp_button_a;
    }

    if (state[SDL_SCANCODE_Z])
    {
        button_keys |= joyp_button_b;
    }

    if (state[SDL_SCANCODE_BACKSPACE])
    {
        button_keys |= joyp_button_select;
    }

    if (state[SDL_SCANCODE_RETURN])
    {
        button_keys |= joyp_button_start;
    }

    machine.SetKeyState(dpad_keys, button_keys);
}

// Check if the emulator's audio buffer is more than 1.5 times as large as the SDL audio buffer.
bool IsAudioBufferOverfilled(Machine& machine)
{
    std::lock_guard<std::mutex> lock(machine.GetAudioSampleBufferMutex());

    const std::vector<float>& audio_sample_buffer = machine.GetAudioSampleBuffer();

    return audio_sample_buffer.size() > (150 * sdl_audio_buffer_size * num_audio_channels) / 100;
}

void MainLoop(SDL_Renderer* renderer, SDL_Texture* texture, Machine& machine)
{
    for (;;)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                return;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_t:
                    machine.SetTraceLogEnabled(true);
                    break;
                case SDLK_y:
                    machine.SetTraceLogEnabled(false);
                    break;
                }
            }
        }

        UpdateJoypad(machine);

        machine.Run(17556 * 2);

        SDL_UpdateTexture(texture, NULL, machine.GetFramebuffer().data(), lcd_width * sizeof(Uint32));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        while (IsAudioBufferOverfilled(machine))
        {
            SDL_Delay(1);
        }
    }
}

void AudioCallback(void* userdata, Uint8* stream, int len)
{
    Machine* machine = static_cast<Machine*>(userdata);

    std::lock_guard<std::mutex> lock(machine->GetAudioSampleBufferMutex());

    const std::vector<float>& audio_sample_buffer = machine->GetAudioSampleBuffer();

    size_t available_len = std::min(static_cast<size_t>(len), audio_sample_buffer.size() * sizeof(float));

    memcpy(stream, audio_sample_buffer.data(), available_len);
    memset(stream + available_len, 0, len - available_len);

    machine->ConsumeAudioSampleBuffer(available_len / sizeof(float));
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Usage: gb_emu ROM");
        return 1;
    }

    std::string rom_file_name = argv[1];

    ROMInfo rom_info;
    LoadROMStatus load_rom_status = LoadROM(rom_file_name, rom_info);

    switch (load_rom_status)
    {
    case LoadROMStatus::OK:
        // no error
        break;
    case LoadROMStatus::ROMFileOpenFailed:
        fprintf(stderr, "Unable to open ROM file\n");
        return 1;
    case LoadROMStatus::ROMFileReadFailed:
        fprintf(stderr, "Unable to read ROM file\n");
        return 1;
    case LoadROMStatus::ROMTooSmall:
        fprintf(stderr, "ROM file is too small (min: 32KB)\n");
        return 1;
    case LoadROMStatus::ROMTooLarge:
        fprintf(stderr, "ROM file is too large (max: 8MB)\n");
        return 1;
    case LoadROMStatus::ROMSizeNotPowerOfTwo:
        fprintf(stderr, "ROM file size is a not power of 2\n");
        return 1;
    case LoadROMStatus::ROMSizeMismatch:
        fprintf(stderr, "ROM file size doesn't match the header\n");
        return 1;
    case LoadROMStatus::RAMInfoInconsistent:
        fprintf(stderr, "catridge type and RAM size are inconsistent\n");
        return 1;
    case LoadROMStatus::BatteryFileReadFailed:
        fprintf(stderr, "Unable to read battery file\n");
        return 1;
    case LoadROMStatus::BatteryFileWrongSize:
        fprintf(stderr, "Battery file is the wrong size\n");
        return 1;
    case LoadROMStatus::UnknownCartridgeType:
        fprintf(stderr, "unknown cartridge type: 0x%02X\n", rom_info.cart_type);
        return 1;
    case LoadROMStatus::UnknownRAMSize:
        fprintf(stderr, "unknown RAM size: 0x%02X\n", rom_info.ram_size_index);
        return 1;
    }

    Machine machine(rom_info);

    const int screen_scale = 2;
    const int screen_width = lcd_width * screen_scale;
    const int screen_height = lcd_height * screen_scale;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
    {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);

    SDL_Window* window = SDL_CreateWindow("GB Emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, 0);

    if (window == nullptr)
    {
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    if (renderer == nullptr)
    {
        fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_RenderSetLogicalSize(renderer, lcd_width, lcd_height) < 0)
    {
        fprintf(stderr, "Unable to set logical size for rendering: %s\n", SDL_GetError());
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, lcd_width, lcd_height);

    if (texture == nullptr)
    {
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        return 1;
    }

    SDL_AudioSpec desired_spec, obtained_spec;
    SDL_memset(&desired_spec, 0, sizeof(desired_spec));
    desired_spec.freq = sample_rate;
    desired_spec.format = AUDIO_F32;
    desired_spec.channels = num_audio_channels;
    desired_spec.samples = sdl_audio_buffer_size;
    desired_spec.callback = AudioCallback;
    desired_spec.userdata = &machine;

    SDL_AudioDeviceID audio_dev = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &obtained_spec, 0);

    if (audio_dev == 0)
    {
        fprintf(stderr, "Unable to open audio device: %s\n", SDL_GetError());
        return 1;
    }

    SDL_PauseAudioDevice(audio_dev, 0);

    MainLoop(renderer, texture, machine);

    if (rom_info.has_battery)
    {
        SaveBatteryStatus save_battery_status = SaveBattery(rom_info.battery_file_name, machine.GetRAM(), machine.GetRTCData());

        switch (save_battery_status)
        {
        case SaveBatteryStatus::OK:
            // no error
            break;
        case SaveBatteryStatus::FileOpenFailed:
            fprintf(stderr, "Unable to open battery file for writing\n");
            return 1;
        case SaveBatteryStatus::FileWriteFailed:
            fprintf(stderr, "Unable to write battery file\n");
            return 1;
        }
    }

    return 0;
}
