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

#pragma once

#include <array>
#include "common.h"

const int lcd_width = 160;
const int lcd_height = 144;

using FramebufferArray = std::array<u8, lcd_width * lcd_height>;

class Machine;

class Graphics
{
public:
    explicit Graphics(Machine& machine) : m_machine(machine)
    {
    }

    void Reset();

    const FramebufferArray& GetFramebuffer() const
    {
        return m_framebuffers[m_current_framebuffer ^ 1];
    }

    u8 ReadVRAM(u16 addr);
    void WriteVRAM(u16 addr, u8 val);

    u8 ReadOAM(u16 addr);
    void WriteOAM(u16 addr, u8 val);

    u8 ReadLCDC();
    void WriteLCDC(u8 val);

    u8 ReadSTAT();
    void WriteSTAT(u8 val);

    u8 ReadSCY();
    void WriteSCY(u8 val);

    u8 ReadSCX();
    void WriteSCX(u8 val);

    u8 ReadLY();

    u8 ReadLYC();
    void WriteLYC(u8 val);

    void WriteDMA(u8 val);

    u8 ReadBGP();
    void WriteBGP(u8 val);

    u8 ReadOBP0();
    void WriteOBP0(u8 val);

    u8 ReadOBP1();
    void WriteOBP1(u8 val);

    u8 ReadWY();
    void WriteWY(u8 val);

    u8 ReadWX();
    void WriteWX(u8 val);

    void Update(unsigned int cycles);

private:
    enum SpriteSize
    {
        SPRITE_SIZE_8X8 = 0,
        SPRITE_SIZE_8X16 = 1
    };

    enum BGTilemapSelect
    {
        BG_TILEMAP_9800 = 0,
        BG_TILEMAP_9C00 = 1
    };

    enum PatternTableSelect
    {
        PATTERN_TABLE_8800 = 0,
        PATTERN_TABLE_8000 = 1
    };

    enum WindowTilemapSelect
    {
        WINDOW_TILEMAP_9800 = 0,
        WINDOW_TILEMAP_9C00 = 1
    };

    enum class DisplayMode
    {
        HBlank = 0,
        VBlank = 1,
        OAMSearch = 2,
        PixelTransfer = 3
    };

    static const int virtual_screen_width = 32;
    static const int virtual_screen_height = 32;
    static const int tile_width = 8;
    static const int tile_height = 8;

    void EnterModeHBlank();
    void EnterModeVBlank();
    void EnterModeOAMSearch();
    void EnterModePixelTransfer();
    void CompareLYWithLYC();
    void WhiteOutFramebuffers();
    void RefreshScreen();
    void GetBackgroundPixelPlanes(
        u8* tilemap,
        unsigned int tile_x,
        unsigned int tile_y,
        unsigned int fine_y,
        u8& plane1,
        u8& plane2);
    unsigned int GetPixelFromPlanes(u8 plane1, u8 plane2, unsigned int fine_x, bool flip_x);
    void DrawBackground(FramebufferArray& fb);
    void WhiteOutScanline(FramebufferArray& fb);
    void DrawScanline();

    Machine& m_machine;

    std::array<u8, 0x2000> m_vram; // video RAM
    std::array<u8, 0xA0> m_oam;    // object attribute memory

    // LCDC
    bool m_bg_enable;
    bool m_sprite_enable;
    SpriteSize m_sprite_size;
    BGTilemapSelect m_bg_tilemap_select;
    PatternTableSelect m_pattern_table_select;
    bool m_window_enable;
    WindowTilemapSelect m_window_tilemap_select;
    bool m_display_enable;

    // STAT
    DisplayMode m_display_mode;
    bool m_coincidence_flag;
    bool m_mode0_intr_enable;
    bool m_mode1_intr_enable;
    bool m_mode2_intr_enable;
    bool m_coincidence_intr_enable;

    u8 m_scy;
    u8 m_scx;

    u8 m_ly;
    u8 m_lyc;

    std::array<u8, 4> m_bgp;
    std::array<u8, 4> m_obp0;
    std::array<u8, 4> m_obp1;

    u8 m_wy;
    u8 m_wx;

    u8 *m_bg_tilemap;
    u8 *m_window_tilemap;

    int m_cycles_left;

    std::array<FramebufferArray, 2> m_framebuffers;
    int m_current_framebuffer;
};
