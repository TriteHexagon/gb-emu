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

#include <algorithm>
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

const unsigned int stat_display_mode_mask = 0x3;

const int hdma5_length_shift = 0;
const int hdma5_hblank_shift = 7;

const unsigned int hdma5_length_mask = 0x7F;

const int cgb_pal_index_shift = 0;
const int cgb_pal_auto_increment_shift = 7;

const unsigned int cgb_pal_index_mask = 0x3F;

const unsigned int vbk_mask = 0x1;

const int vblank_start = 144;
const int vblank_end = 153;

const int hblank_cycles = 51 * 2;
const int oam_search_cycles = 20 * 2;
const int pixel_transfer_cycles = 43 * 2;
const int scanline_cycles = 114 * 2;

const int total_sprites = 40;
const int per_line_sprite_limit = 10;
const int oam_entry_size = 4;

// OAM entry offsets
const int oam_y = 0;
const int oam_x = 1;
const int oam_tile_num = 2;
const int oam_attr = 3;

// OAM attributes

const int oam_attr_cgb_pal_shift = 0;
const int oam_attr_vram_bank_shift = 3;
const int oam_attr_dmg_pal_shift = 4;
const int oam_attr_flip_x_shift = 5;
const int oam_attr_flip_y_shift = 6;
const int oam_attr_priority_shift = 7;

const unsigned int oam_attr_cgb_pal_mask = 0x7;
const unsigned int oam_attr_vram_bank_mask = 0x1;
const unsigned int oam_attr_dmg_pal_mask = 0x1;

const unsigned int oam_attr_flip_x = Bit(oam_attr_flip_x_shift);
const unsigned int oam_attr_flip_y = Bit(oam_attr_flip_y_shift);
const unsigned int oam_attr_priority = Bit(oam_attr_priority_shift);

// BG attributes

const int bg_attr_pal_shift = 0;
const int bg_attr_vram_bank_shift = 3;
const int bg_attr_flip_x_shift = 5;
const int bg_attr_flip_y_shift = 6;
const int bg_attr_priority_shift = 7;

const unsigned int bg_attr_pal_mask = 0x7;
const unsigned int bg_attr_vram_bank_mask = 0x1;

const unsigned int bg_attr_flip_x = Bit(bg_attr_flip_x_shift);
const unsigned int bg_attr_flip_y = Bit(bg_attr_flip_y_shift);
const unsigned int bg_attr_priority = Bit(bg_attr_priority_shift);

void Graphics::Reset()
{
    m_vram = {};
    m_oam = {};

    m_vram_map = &m_vram[0];
    m_vram_bank = 0;

    m_bg_enable = true;
    m_sprite_enable = false;
    m_sprite_size = SpriteSize::SPRITE_SIZE_8X8;
    m_bg_tilemap_select = BGTilemapSelect::BG_TILEMAP_9800;
    m_pattern_table_select = PatternTableSelect::PATTERN_TABLE_8000;
    m_window_enable = false;
    m_window_tilemap_select = WindowTilemapSelect::WINDOW_TILEMAP_9800;
    m_display_enable = true;

    m_display_mode = DisplayMode::HBlank;
    m_coincidence_flag = true;
    m_mode0_intr_enable = false;
    m_mode1_intr_enable = false;
    m_mode2_intr_enable = false;
    m_coincidence_intr_enable = false;

    m_scy = 0;
    m_scx = 0;

    m_ly = 0;
    m_lyc = 0;

    m_bgp = {};
    m_obp = {};

    m_wy = 0;
    m_wx = 0;

    m_hdma_src = 0;
    m_hdma_dest = 0;
    m_hdma_active = false;
    m_hdma_length = 0;

    m_bcp_auto_increment = false;
    m_bcp_index = 0;
    m_bcp.fill(0xFF);
    m_ocp_auto_increment = false;
    m_ocp_index = 0;
    m_ocp.fill(0xFF);

    m_bg_tilemap = &m_vram[0x1800];
    m_bg_attr_table = &m_vram[0x3800];
    m_window_tilemap = &m_vram[0x1800];
    m_window_attr_table = &m_vram[0x3800];

    m_cycles_left = 0;

    WhiteOutFramebuffers();

    m_current_framebuffer = 0;

    m_bg_color_indices = {};
    m_bg_high_priority = {};
}

u8 Graphics::ReadVRAM(u16 addr)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        return m_vram_map[addr];
    }

    return 0xFF;
}

void Graphics::WriteVRAM(u16 addr, u8 val)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        m_vram_map[addr] = val;
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
            EnterModeOAMSearch();
        }
        else
        {
            m_ly = 0;
            m_display_mode = DisplayMode::HBlank;
            WhiteOutFramebuffers();
        }
    }

    if (m_bg_tilemap_select == BG_TILEMAP_9800)
    {
        m_bg_tilemap = &m_vram[0x1800];
        m_bg_attr_table = &m_vram[0x3800];
    }
    else
    {
        m_bg_tilemap = &m_vram[0x1C00];
        m_bg_attr_table = &m_vram[0x3C00];
    }

    if (m_window_tilemap_select == WINDOW_TILEMAP_9800)
    {
        m_window_tilemap = &m_vram[0x1800];
        m_window_attr_table = &m_vram[0x3800];
    }
    else
    {
        m_window_tilemap = &m_vram[0x1C00];
        m_window_attr_table = &m_vram[0x3C00];
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
    for (u16 i = 0; i < m_oam.size(); i++)
    {
        m_oam[i] = m_hw.memory.Read(((u16)val << 8) + i);
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
    return ReadPalette(m_obp[0]);
}

void Graphics::WriteOBP0(u8 val)
{
    WritePalette(m_obp[0], val);
}

u8 Graphics::ReadOBP1()
{
    return ReadPalette(m_obp[1]);
}

void Graphics::WriteOBP1(u8 val)
{
    WritePalette(m_obp[1], val);
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

u8 Graphics::ReadVBK()
{
    return m_vram_bank | ~vbk_mask;
}

void Graphics::WriteVBK(u8 val)
{
    m_vram_bank = val & vbk_mask;
    m_vram_map = &m_vram[m_vram_bank * 0x2000];
}

void Graphics::WriteHDMA1(u8 val)
{
    m_hdma_src = ((u16)val << 8) | (m_hdma_src & 0xFF);
}

void Graphics::WriteHDMA2(u8 val)
{
    m_hdma_src = (m_hdma_src & 0xFF00) | (val & 0xF0);
}

void Graphics::WriteHDMA3(u8 val)
{
    m_hdma_dest = ((u16)val << 8) | (m_hdma_dest & 0xFF);
}

void Graphics::WriteHDMA4(u8 val)
{
    m_hdma_dest = (m_hdma_dest & 0xFF00) | (val & 0xF0);
}

u8 Graphics::ReadHDMA5()
{
    return (!m_hdma_active << hdma5_hblank_shift) | (m_hdma_length << hdma5_length_shift);
}

void Graphics::WriteHDMA5(u8 val)
{
    int length = (val >> hdma5_length_shift) & hdma5_length_mask;
    bool hblank = (val >> hdma5_hblank_shift) & 1;

    if (m_hdma_active)
    {
        if (!hblank)
        {
            m_hdma_active = false;
        }
        m_hdma_length = length;
    }
    else
    {
        if (hblank)
        {
            // H-Blank DMA
            m_hdma_active = true;
            m_hdma_length = length;
        }
        else
        {
            // general purpose DMA
            int transfer_length = (length + 1) * 16;
            for (int i = 0; i < transfer_length; i++)
            {
                m_hdma_dest = 0x8000 + (m_hdma_dest & 0x1FFF);
                m_hw.memory.Write(m_hdma_dest++, m_hw.memory.Read(m_hdma_src++));
            }
        }
    }
}

u8 Graphics::ReadBCPS()
{
    return (m_bcp_auto_increment << cgb_pal_auto_increment_shift) | 0x40 | (m_bcp_index << cgb_pal_index_shift);
}

void Graphics::WriteBCPS(u8 val)
{
    m_bcp_index = (val >> cgb_pal_index_shift) & cgb_pal_index_mask;
    m_bcp_auto_increment = (val >> cgb_pal_auto_increment_shift) & 1;
}

u8 Graphics::ReadBCPD()
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        return m_bcp[m_bcp_index];
    }

    return 0xFF;
}

void Graphics::WriteBCPD(u8 val)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        m_bcp[m_bcp_index] = val;
        if (m_bcp_auto_increment)
        {
            m_bcp_index++;
            m_bcp_index &= cgb_pal_index_mask;
        }
    }
}

u8 Graphics::ReadOCPS()
{
    return (m_ocp_auto_increment << cgb_pal_auto_increment_shift) | 0x40 | (m_ocp_index << cgb_pal_index_shift);
}

void Graphics::WriteOCPS(u8 val)
{
    m_ocp_index = (val >> cgb_pal_index_shift) & cgb_pal_index_mask;
    m_ocp_auto_increment = (val >> cgb_pal_auto_increment_shift) & 1;
}

u8 Graphics::ReadOCPD()
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        return m_ocp[m_ocp_index];
    }

    return 0xFF;
}

void Graphics::WriteOCPD(u8 val)
{
    if (m_display_mode != DisplayMode::PixelTransfer)
    {
        m_ocp[m_ocp_index] = val;
        if (m_ocp_auto_increment)
        {
            m_ocp_index++;
            m_ocp_index &= cgb_pal_index_mask;
        }
    }
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
    DoHBlankDMA();
    if (m_mode0_intr_enable)
    {
        m_hw.cpu.SetInterruptFlag(intr_lcdc_status);
    }
}

void Graphics::EnterModeVBlank()
{
    m_display_mode = DisplayMode::VBlank;
    m_cycles_left += scanline_cycles;
    m_hw.cpu.SetInterruptFlag(intr_vblank);
    if (m_mode1_intr_enable)
    {
        m_hw.cpu.SetInterruptFlag(intr_lcdc_status);
    }
    RefreshScreen();
}

void Graphics::EnterModeOAMSearch()
{
    m_display_mode = DisplayMode::OAMSearch;
    m_cycles_left += oam_search_cycles;
    if (m_mode2_intr_enable)
    {
        m_hw.cpu.SetInterruptFlag(intr_lcdc_status);
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
            m_hw.cpu.SetInterruptFlag(intr_lcdc_status);
        }
    }
    else
    {
        m_coincidence_flag = false;
    }
}

void Graphics::DoHBlankDMA()
{
    if (m_hdma_active)
    {
        for (int i = 0; i < 16; i++)
        {
            m_hdma_dest = 0x8000 + (m_hdma_dest & 0x1FFF);
            m_hw.memory.Write(m_hdma_dest++, m_hw.memory.Read(m_hdma_src++));
        }

        if (m_hdma_length == 0)
        {
            m_hdma_length = 0x7F;
            m_hdma_active = false;
        }
        else
        {
            m_hdma_length--;
        }
    }
}

void Graphics::WhiteOutFramebuffers()
{
    m_framebuffers = {};
}

void Graphics::RefreshScreen()
{
    m_current_framebuffer ^= 1;
}

u32 Graphics::GetRGBColor_DMG(unsigned int pixel, std::array<u8, 4> pal)
{
    static const std::array<u32, 4> rgb_table = { 0xFFFFFF, 0xAAAAAA, 0x555555, 0x000000 };

    return rgb_table[pal[pixel]];
}

unsigned int Convert5To8(unsigned int val)
{
    return (val * 255 + 15) / 31;
}

u32 Graphics::GetRGBColor_CGB(unsigned int pixel, std::array<u8, 64> pal, unsigned int pal_slot)
{
    int offset = (pal_slot * 8) + (pixel * 2);
    u16 rgb16 = ((u16)pal[offset + 1] << 8) | pal[offset];
    unsigned int r5 = (rgb16 >> 0) & 0x1F;
    unsigned int g5 = (rgb16 >> 5) & 0x1F;
    unsigned int b5 = (rgb16 >> 10) & 0x1F;
    unsigned int r8 = Convert5To8(r5);
    unsigned int g8 = Convert5To8(g5);
    unsigned int b8 = Convert5To8(b5);
    return (r8 << 16) | (g8 << 8) | b8;
}

void Graphics::GetBackgroundPixelPlanes(
    u8* tilemap,
    unsigned int tile_x,
    unsigned int tile_y,
    unsigned int tile_fine_y,
    unsigned int vram_bank,
    bool flip_y,
    u8& plane1,
    u8& plane2)
{
    if (flip_y)
    {
        tile_fine_y = (tile_height - 1) - tile_fine_y;
    }

    u8 tile_num = tilemap[(tile_y * virtual_screen_width) + tile_x];
    unsigned int offset;

    if (m_pattern_table_select)
    {
        // 8000-8FFF
        offset = (tile_num * 16) + (tile_fine_y * 2);
    }
    else
    {
        // 8800-97FF
        offset = 0x1000 + ((s8)tile_num * 16) + (tile_fine_y * 2);
    }

    offset += vram_bank * 0x2000;

    plane1 = m_vram[offset];
    plane2 = m_vram[offset + 1];
}

unsigned int Graphics::GetPixelFromPlanes(u8 plane1, u8 plane2, unsigned int fine_x, bool flip_x)
{
    unsigned int shift = flip_x ? fine_x : (tile_width - 1) - fine_x;
    return ((plane1 >> shift) & 1) | (((plane2 >> shift) << 1) & 2);
}

void Graphics::DrawBackground_Helper(
    FramebufferArray& fb,
    u8* tilemap,
    u8* attr_table,
    unsigned int x,
    unsigned int tile_x,
    unsigned int tile_fine_x,
    unsigned int tile_y,
    unsigned int tile_fine_y)
{
    u8 attr = 0;

    for (;;)
    {
        if (m_hw.is_cgb_mode)
        {
            attr = attr_table[(tile_y * virtual_screen_width) + tile_x];
        }

        unsigned int pal_slot = (attr >> bg_attr_pal_shift) & bg_attr_pal_mask;
        unsigned int vram_bank = (attr >> bg_attr_vram_bank_shift) & bg_attr_vram_bank_mask;
        bool flip_x = ((attr & bg_attr_flip_x) != 0);
        bool flip_y = ((attr & bg_attr_flip_y) != 0);
        bool high_priority = ((attr & bg_attr_priority) != 0);

        u8 plane1, plane2;
        GetBackgroundPixelPlanes(tilemap, tile_x, tile_y, tile_fine_y, vram_bank, flip_y, plane1, plane2);

        for (;;)
        {
            unsigned int pixel = GetPixelFromPlanes(plane1, plane2, tile_fine_x, flip_x);

            m_bg_color_indices[x] = pixel;
            m_bg_high_priority[x] = high_priority;

            if (m_hw.is_cgb_mode)
            {
                fb[(m_ly * lcd_width) + x] = GetRGBColor_CGB(pixel, m_bcp, pal_slot);
            }
            else
            {
                fb[(m_ly * lcd_width) + x] = GetRGBColor_DMG(pixel, m_bgp);
            }

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

void Graphics::DrawBackground(FramebufferArray& fb)
{
    unsigned int line = (m_ly + m_scy) & 0xFF;

    unsigned int tile_x = m_scx >> 3;
    unsigned int tile_fine_x = m_scx & 7;

    unsigned int tile_y = line >> 3;
    unsigned int tile_fine_y = line & 7;

    DrawBackground_Helper(fb, m_bg_tilemap, m_bg_attr_table, 0, tile_x, tile_fine_x, tile_y, tile_fine_y);
}

void Graphics::DrawWindow(FramebufferArray& fb)
{
    int window_line = m_ly - m_wy;

    if (window_line < 0)
    {
        return;
    }

    int window_x = m_wx - 7;

    if (window_x >= 160)
    {
        return;
    }

    int x = (window_x > 0) ? window_x : 0;
    unsigned int window_x_offset = x - window_x;

    unsigned int tile_x = window_x_offset >> 3;
    unsigned int tile_fine_x = window_x_offset & 7;

    unsigned int tile_y = window_line >> 3;
    unsigned int tile_fine_y = window_line & 7;

    DrawBackground_Helper(fb, m_window_tilemap, m_window_attr_table, x, tile_x, tile_fine_x, tile_y, tile_fine_y);
}

void Graphics::WhiteOutScanline(FramebufferArray& fb)
{
    for (int x = 0; x < lcd_width; x++)
    {
        fb[(m_ly * lcd_width) + x] = 0;
        m_bg_color_indices[x] = 0;
    }
}

void Graphics::DrawScanline()
{
    FramebufferArray& fb = m_framebuffers[m_current_framebuffer];

    if (m_bg_enable || m_hw.is_cgb_mode)
    {
        DrawBackground(fb);
    }
    else
    {
        WhiteOutScanline(fb);
    }

    if (m_window_enable)
    {
        DrawWindow(fb);
    }

    if (m_sprite_enable)
    {
        DrawSprites(fb);
    }
}

void Graphics::DrawSprites(FramebufferArray& fb)
{
    const int sprite_height = (m_sprite_size == SPRITE_SIZE_8X16) ? 16 : 8;

    int count = 0;
    std::array<int, total_sprites> potentially_visible_sprites;

    for (int i = 0; i < total_sprites; i++)
    {
        int sprite_y = m_oam[i * oam_entry_size + oam_y] - 16;

        if (m_ly >= sprite_y && m_ly < sprite_y + sprite_height)
        {
            potentially_visible_sprites[count++] = i;
        }
    }

    if (!m_hw.is_cgb_mode)
    {
        std::stable_sort(
            potentially_visible_sprites.begin(),
            potentially_visible_sprites.begin() + count,
            [this](int i, int j)
            {
                return m_oam[i * oam_entry_size + oam_x] < m_oam[j * oam_entry_size + oam_x];
            });
    }

    for (int i = std::min(count, per_line_sprite_limit) - 1; i >= 0; i--)
    {
        unsigned int sprite_index = potentially_visible_sprites[i];
        unsigned int oam_offset = sprite_index * oam_entry_size;

        int sprite_y = m_oam[oam_offset + oam_y] - 16;
        int sprite_x = m_oam[oam_offset + oam_x] - 8;
        u8 tile_num = m_oam[oam_offset + oam_tile_num];
        u8 attr = m_oam[oam_offset + oam_attr];

        if (m_sprite_size == SPRITE_SIZE_8X16)
        {
            tile_num &= ~1;
        }

        unsigned int cgb_pal_slot = (attr >> oam_attr_cgb_pal_shift) & oam_attr_cgb_pal_mask;
        unsigned int vram_bank = m_hw.is_cgb_mode ? ((attr >> oam_attr_vram_bank_shift) & oam_attr_vram_bank_mask) : 0;
        std::array<u8, 4>& dmg_pal = m_obp[(attr >> oam_attr_dmg_pal_shift) & oam_attr_dmg_pal_mask];
        bool flip_x = ((attr & oam_attr_flip_x) != 0);
        bool flip_y = ((attr & oam_attr_flip_y) != 0);
        bool low_priority = ((attr & oam_attr_priority) != 0);

        unsigned int sprite_line = m_ly - sprite_y;

        if (flip_y)
        {
            sprite_line = (sprite_height - 1) - sprite_line;
        }

        unsigned int vram_offset = (vram_bank * 0x2000) + (tile_num * 16) + (sprite_line * 2);
        u8 plane1 = m_vram[vram_offset];
        u8 plane2 = m_vram[vram_offset + 1];

        int x;
        int fine_x;

        if (sprite_x < 0)
        {
            x = 0;
            fine_x = -sprite_x;
        }
        else
        {
            x = sprite_x;
            fine_x = 0;
        }


        while (x < lcd_width && fine_x < tile_width)
        {
            unsigned int pixel = GetPixelFromPlanes(plane1, plane2, fine_x, flip_x);

            if (pixel != 0 && (!m_bg_enable || (!m_bg_high_priority[x] && !low_priority) || m_bg_color_indices[x] == 0))
            {
                if (m_hw.is_cgb_mode)
                {
                    fb[(m_ly * lcd_width) + x] = GetRGBColor_CGB(pixel, m_ocp, cgb_pal_slot);
                }
                else
                {
                    fb[(m_ly * lcd_width) + x] = GetRGBColor_DMG(pixel, dmg_pal);
                }
            }

            x++;
            fine_x++;
        }
    }
}
