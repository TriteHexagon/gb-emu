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

using FramebufferArray = std::array<u32, lcd_width * lcd_height>;

struct Hardware;

class Graphics
{
public:
    explicit Graphics(Hardware& hw) : m_hw(hw)
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

    u8 ReadVBK();
    void WriteVBK(u8 val);

    void WriteHDMA1(u8 val);

    void WriteHDMA2(u8 val);

    void WriteHDMA3(u8 val);

    void WriteHDMA4(u8 val);

    u8 ReadHDMA5();
    void WriteHDMA5(u8 val);

    u8 ReadBCPS();
    void WriteBCPS(u8 val);

    u8 ReadBCPD();
    void WriteBCPD(u8 val);

    u8 ReadOCPS();
    void WriteOCPS(u8 val);

    u8 ReadOCPD();
    void WriteOCPD(u8 val);

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

    void UpdateTilemapSelection();
    void EnterModeHBlank();
    void EnterModeVBlank();
    void EnterModeOAMSearch();
    void EnterModePixelTransfer();
    void CompareLYWithLYC();
    void DoHBlankDMA();
    void WhiteOutFramebuffers();
    void RefreshScreen();
    u32 GetRGBColor_DMG(unsigned int pixel, std::array<u8, 4> pal);
    u32 GetRGBColor_CGB(unsigned int pixel, std::array<u8, 64> pal, unsigned int pal_slot);
    void GetBackgroundPixelPlanes(
        u8* tilemap,
        unsigned int tile_x,
        unsigned int tile_y,
        unsigned int tile_fine_y,
        unsigned int vram_bank,
        bool flip_y,
        u8& plane1,
        u8& plane2);
    unsigned int GetPixelFromPlanes(u8 plane1, u8 plane2, unsigned int fine_x, bool flip_x);
    void DrawBackground_Helper(
        FramebufferArray& fb,
        u8* tilemap,
        u8* attr_table,
        unsigned int x,
        unsigned int tile_x,
        unsigned int tile_fine_x,
        unsigned int tile_y,
        unsigned int tile_fine_y);
    void DrawBackground(FramebufferArray& fb);
    void DrawWindow(FramebufferArray& fb);
    void DrawSprites(FramebufferArray& fb);
    void WhiteOutScanline(FramebufferArray& fb);
    void DrawScanline();

    Hardware& m_hw;

    std::array<u8, 0x4000> m_vram; // video RAM
    std::array<u8, 0xA0> m_oam;    // object attribute memory

    u8* m_vram_map;
    int m_vram_bank;

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
    std::array<std::array<u8, 4>, 2> m_obp;

    u8 m_wy;
    u8 m_wx;

    u16 m_hdma_src;
    u16 m_hdma_dest;
    bool m_hdma_active;
    int m_hdma_length;

    bool m_bcp_auto_increment;
    int m_bcp_index;
    std::array<u8, 64> m_bcp;
    bool m_ocp_auto_increment;
    int m_ocp_index;
    std::array<u8, 64> m_ocp;

    u8* m_bg_tilemap;
    u8* m_bg_attr_table;
    u8* m_window_tilemap;
    u8* m_window_attr_table;

    u8 m_latched_wy;
    u8 m_window_line;

    int m_cycles_left;

    std::array<FramebufferArray, 2> m_framebuffers;
    int m_current_framebuffer;

    std::array<int, lcd_width> m_bg_color_indices;
    std::array<bool, lcd_width> m_bg_high_priority;
};
