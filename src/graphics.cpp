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

#include "common.h"
#include "graphics.h"
#include "machine.h"

const int lcdc_bg_enable_shift = 0;
const int lcdc_sprite_enable_shift = 1;
const int lcdc_sprite_size_shift = 2;
const int lcdc_bg_tilemap_select_shift = 3;
const int lcdc_pattern_table_select_shift = 4;
const int lcdc_window_enable_shift = 5;
const int lcdc_window_tilemap_select_shift = 6;
const int lcdc_display_enable_shift = 7;

const int stat_display_mode_shift = 0;
const int stat_coincidence_flag_shift = 2;
const int stat_mode0_intr_enable_shift = 3;
const int stat_mode1_intr_enable_shift = 4;
const int stat_mode2_intr_enable_shift = 5;
const int stat_coincidence_intr_enable_shift = 6;

const int stat_display_mode_mask = 0x3;

const int vblank_start = 144;
const int vblank_end = 153;

const int hblank_cycles = 51;
const int oam_search_cycles = 20;
const int pixel_transfer_cycles = 43;
const int scanline_cycles = 114;

void Graphics::Reset()
{
    m_vram = {};
    m_oam = {};

    m_bg_enable = false;
    m_sprite_enable = false;
    m_sprite_size = SpriteSize::SPRITE_SIZE_8X8;
    m_bg_tilemap_select = BGTilemapSelect::BG_TILEMAP_9800;
    m_pattern_table_select = PatternTableSelect::PATTERN_TABLE_8800;
    m_window_enable = false;
    m_window_tilemap_select = WindowTilemapSelect::WINDOW_TILEMAP_9800;
    m_display_enable = false;

    m_display_mode = DisplayMode::HBlank;
    m_coincidence_flag = false;
    m_mode0_intr_enable = false;
    m_mode1_intr_enable = false;
    m_mode2_intr_enable = false;
    m_coincidence_intr_enable = false;

    m_scy = 0;
    m_scx = 0;

    m_ly = 0;
    m_lyc = 0;

    m_bgp = {};
    m_obp0 = {};
    m_obp1 = {};

    m_wy = 0;
    m_wx = 0;

    m_bg_tilemap = &m_vram[0x1800];
    m_window_tilemap = &m_vram[0x1800];

    m_cycles_left = 0;

    WhiteOutFramebuffers();

    m_current_framebuffer = 0;
}

u8 Graphics::ReadVRAM(u16 addr)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        return m_vram[addr];
    }

    return 0xFF;
}

void Graphics::WriteVRAM(u16 addr, u8 val)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        m_vram[addr] = val;
    }
}

u8 Graphics::ReadOAM(u16 addr)
{
    if (m_display_mode == DisplayMode::VBlank || m_display_mode == DisplayMode::HBlank)
    {
        return m_oam[addr];
    }

    return 0xFF;
}

void Graphics::WriteOAM(u16 addr, u8 val)
{
    if (m_display_mode == DisplayMode::VBlank || m_display_mode == DisplayMode::HBlank)
    {
        m_oam[addr] = val;
    }
}

u8 Graphics::ReadLCDC()
{
    return (m_bg_enable << lcdc_bg_enable_shift)
        | (m_sprite_enable << lcdc_sprite_enable_shift)
        | (m_sprite_size << lcdc_sprite_size_shift)
        | (m_bg_tilemap_select << lcdc_bg_tilemap_select_shift)
        | (m_pattern_table_select << lcdc_pattern_table_select_shift)
        | (m_window_enable << lcdc_window_enable_shift)
        | (m_window_tilemap_select << lcdc_window_tilemap_select_shift)
        | (m_display_enable << lcdc_display_enable_shift);
}

void Graphics::WriteLCDC(u8 val)
{
    bool old_display_enable = m_display_enable;

    m_bg_enable = ((val >> lcdc_bg_enable_shift) & 1);
    m_sprite_enable = ((val >> lcdc_sprite_enable_shift) & 1);
    m_sprite_size = (SpriteSize)((val >> lcdc_sprite_size_shift) & 1);
    m_bg_tilemap_select = (BGTilemapSelect)((val >> lcdc_bg_tilemap_select_shift) & 1);
    m_pattern_table_select = (PatternTableSelect)((val >> lcdc_pattern_table_select_shift) & 1);
    m_window_enable = ((val >> lcdc_window_enable_shift) & 1);
    m_window_tilemap_select = (WindowTilemapSelect)((val >> lcdc_window_tilemap_select_shift) & 1);
    m_display_enable = ((val >> lcdc_display_enable_shift) & 1);

    if (m_display_enable != old_display_enable)
    {
        if (m_display_enable)
        {
            m_ly = 0;
            EnterModeOAMSearch();
        }
        else
        {
            m_display_mode = DisplayMode::HBlank;
            WhiteOutFramebuffers();
        }
    }

    if (m_bg_tilemap_select == BG_TILEMAP_9800)
    {
        m_bg_tilemap = &m_vram[0x1800];
    }
    else
    {
        m_bg_tilemap = &m_vram[0x1C00];
    }

    if (m_window_tilemap_select == WINDOW_TILEMAP_9800)
    {
        m_window_tilemap = &m_vram[0x1800];
    }
    else
    {
        m_window_tilemap = &m_vram[0x1C00];
    }
}

u8 Graphics::ReadSTAT()
{
    return ((int)m_display_mode << stat_display_mode_shift)
        | (m_coincidence_flag << stat_coincidence_flag_shift)
        | (m_mode0_intr_enable << stat_mode0_intr_enable_shift)
        | (m_mode1_intr_enable << stat_mode1_intr_enable_shift)
        | (m_mode2_intr_enable << stat_mode2_intr_enable_shift)
        | (m_coincidence_intr_enable << stat_coincidence_intr_enable_shift);
}

void Graphics::WriteSTAT(u8 val)
{
    m_display_mode = (DisplayMode)((val >> stat_display_mode_shift) & stat_display_mode_mask);
    m_coincidence_flag = ((val >> stat_coincidence_flag_shift) & 1);
    m_mode0_intr_enable = ((val >> stat_mode0_intr_enable_shift) & 1);
    m_mode1_intr_enable = ((val >> stat_mode1_intr_enable_shift) & 1);
    m_mode2_intr_enable = ((val >> stat_mode2_intr_enable_shift) & 1);
    m_coincidence_intr_enable = ((val >> stat_coincidence_intr_enable_shift) & 1);
}

u8 Graphics::ReadSCY()
{
    return m_scy;
}

void Graphics::WriteSCY(u8 val)
{
    m_scy = val;
}

u8 Graphics::ReadSCX()
{
    return m_scx;
}

void Graphics::WriteSCX(u8 val)
{
    m_scx = val;
}

u8 Graphics::ReadLY()
{
    return m_ly;
}

u8 Graphics::ReadLYC()
{
    return m_lyc;
}

void Graphics::WriteLYC(u8 val)
{
    m_lyc = val;
    CompareLYWithLYC();
}

void Graphics::WriteDMA(u8 val)
{
    // TODO: Implement accurately.
    for (size_t i = 0; i < m_oam.size(); i++)
    {
        m_oam[i] = m_machine.GetMemory().Read(((u16)val << 8) + i);
    }
}

u8 ReadPalette(std::array<u8, 4>& pal)
{
    return (pal[3] << 6) | (pal[2] << 4) | (pal[1] << 2) | (pal[0] << 0);
}

void WritePalette(std::array<u8, 4>& pal, u8 val)
{
    pal[0] = (val >> 0) & 0x3;
    pal[1] = (val >> 2) & 0x3;
    pal[2] = (val >> 4) & 0x3;
    pal[3] = (val >> 6) & 0x3;
}

u8 Graphics::ReadBGP()
{
    return ReadPalette(m_bgp);
}

void Graphics::WriteBGP(u8 val)
{
    WritePalette(m_bgp, val);
}

u8 Graphics::ReadOBP0()
{
    return ReadPalette(m_obp0);
}

void Graphics::WriteOBP0(u8 val)
{
    WritePalette(m_obp0, val);
}

u8 Graphics::ReadOBP1()
{
    return ReadPalette(m_obp1);
}

void Graphics::WriteOBP1(u8 val)
{
    WritePalette(m_obp1, val);
}

u8 Graphics::ReadWY()
{
    return m_wy;
}

void Graphics::WriteWY(u8 val)
{
    m_wy = val;
}

u8 Graphics::ReadWX()
{
    return m_wx;
}

void Graphics::WriteWX(u8 val)
{
    m_wx = val;
}

void Graphics::Update(unsigned int cycles)
{
    if (m_display_enable)
    {
        m_cycles_left -= cycles;

        if (m_cycles_left <= 0)
        {
            switch (m_display_mode)
            {
            case DisplayMode::HBlank:
                m_ly++;
                CompareLYWithLYC();

                if (m_ly == vblank_start)
                {
                    EnterModeVBlank();
                }
                else
                {
                    EnterModeOAMSearch();
                }
                break;
            case DisplayMode::VBlank:
                if (m_ly == vblank_end)
                {
                    m_ly = 0;
                    CompareLYWithLYC();
                    EnterModeOAMSearch();
                }
                else
                {
                    m_ly++;
                    CompareLYWithLYC();
                    m_cycles_left += scanline_cycles;
                }
                break;
            case DisplayMode::OAMSearch:
                EnterModePixelTransfer();
                break;
            case DisplayMode::PixelTransfer:
                EnterModeHBlank();
                break;
            }
        }
    }
}

void Graphics::EnterModeHBlank()
{
    m_display_mode = DisplayMode::HBlank;
    m_cycles_left += hblank_cycles;
    if (m_mode0_intr_enable)
    {
        m_machine.GetCPU().SetInterruptFlag(intr_lcdc_status);
    }
}

void Graphics::EnterModeVBlank()
{
    m_display_mode = DisplayMode::VBlank;
    m_cycles_left += scanline_cycles;
    m_machine.GetCPU().SetInterruptFlag(intr_vblank);
    if (m_mode1_intr_enable)
    {
        m_machine.GetCPU().SetInterruptFlag(intr_lcdc_status);
    }
    RefreshScreen();
}

void Graphics::EnterModeOAMSearch()
{
    m_display_mode = DisplayMode::OAMSearch;
    m_cycles_left += oam_search_cycles;
    if (m_mode2_intr_enable)
    {
        m_machine.GetCPU().SetInterruptFlag(intr_lcdc_status);
    }
}

void Graphics::EnterModePixelTransfer()
{
    m_display_mode = DisplayMode::PixelTransfer;
    m_cycles_left += pixel_transfer_cycles;
    DrawScanline();
}

void Graphics::CompareLYWithLYC()
{
    if (m_ly == m_lyc)
    {
        m_coincidence_flag = true;
        if (m_coincidence_intr_enable)
        {
            m_machine.GetCPU().SetInterruptFlag(intr_lcdc_status);
        }
    }
    else
    {
        m_coincidence_flag = false;
    }
}

void Graphics::WhiteOutFramebuffers()
{
    for (auto fb_it = std::begin(m_framebuffers); fb_it != std::end(m_framebuffers); ++fb_it)
    {
        for (auto pixel_it = std::begin(*fb_it); pixel_it != std::end(*fb_it); ++pixel_it)
        {
            *pixel_it = 0;
        }
    }
}

void Graphics::RefreshScreen()
{
    m_current_framebuffer ^= 1;
}

void Graphics::GetBackgroundPixelPlanes(
    u8* tilemap,
    unsigned int tile_x,
    unsigned int tile_y,
    unsigned int fine_y,
    u8& plane1,
    u8& plane2)
{
    u8 tile_num = tilemap[(tile_y * virtual_screen_width) + tile_x];
    unsigned int offset;

    if (m_pattern_table_select)
    {
        // 8000-8FFF
        offset = (tile_num * 16) + (fine_y * 2);
    }
    else
    {
        // 8800-97FF
        offset = 0x1000 + ((s8)tile_num * 16) + (fine_y * 2);
    }

    plane1 = m_vram[offset];
    plane2 = m_vram[offset + 1];
}

unsigned int Graphics::GetPixelFromPlanes(u8 plane1, u8 plane2, unsigned int fine_x, bool flip_x)
{
    unsigned int shift = flip_x ? fine_x : (tile_width - 1) - fine_x;
    return ((plane1 >> shift) & 1) | (((plane2 >> shift) << 1) & 2);
}

void Graphics::DrawBackground(FramebufferArray& fb)
{
    unsigned int x = 0;
    unsigned int line = (m_ly + m_scy) & 0xFF;

    unsigned int tile_x = m_scx >> 3;
    unsigned int tile_fine_x = m_scx & 7;

    unsigned int tile_y = line >> 3;
    unsigned int tile_fine_y = line & 7;

    for (;;)
    {
        u8 plane1, plane2;
        GetBackgroundPixelPlanes(m_bg_tilemap, tile_x, tile_y, tile_fine_y, plane1, plane2);

        for (;;)
        {
            unsigned int pixel = GetPixelFromPlanes(plane1, plane2, tile_fine_x, false);

            fb[(m_ly * lcd_width) + x] = m_bgp[pixel];

            if (x == lcd_width - 1)
            {
                return;
            }

            x++;

            if (tile_fine_x == tile_width - 1)
            {
                if (tile_x == virtual_screen_width - 1)
                {
                    tile_x = 0;
                }
                else
                {
                    tile_x++;
                }

                tile_fine_x = 0;
                break;
            }

            tile_fine_x++;
        }
    }
}

void Graphics::WhiteOutScanline(FramebufferArray& fb)
{
    for (int x = 0; x < lcd_width; x++)
    {
        fb[(m_ly * lcd_width) + x] = 0;
    }
}

void Graphics::DrawScanline()
{
    FramebufferArray& fb = m_framebuffers[m_current_framebuffer];

    if (m_bg_enable)
    {
        DrawBackground(fb);
    }
    else
    {
        WhiteOutScanline(fb);
    }
}
