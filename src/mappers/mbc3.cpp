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

#include <time.h>
#include <array>
#include "mbc3.h"

const unsigned int dh_halt = Bit(6);
const unsigned int dh_overflow = Bit(7);

class RTCDataReader
{
public:
    RTCDataReader(const std::vector<u8>& rtc_data) : m_rtc_data(rtc_data)
    {
        m_pos = 0;
    }

    u32 ReadU32()
    {
        if (m_pos + 3 >= m_rtc_data.size())
        {
            return 0;
        }

        u32 val = 0;

        for (int i = 0; i < 4; i++)
        {
            val |= (m_rtc_data[m_pos++] << (i * 8));
        }

        return val;
    }

    s64 ReadS64()
    {
        if (m_pos + 7 >= m_rtc_data.size())
        {
            return 0;
        }

        u64 val = 0;

        for (int i = 0; i < 8; i++)
        {
            val |= (m_rtc_data[m_pos++] << (i * 8));
        }

        return (s64)val;
    }

private:
    const std::vector<u8>& m_rtc_data;
    size_t m_pos;
};

class RTCDataWriter
{
public:
    void WriteU32(u32 val)
    {
        for (int i = 0; i < 4; i++)
        {
            m_rtc_data.push_back(val & 0xFF);
            val >>= 8;
        }
    }

    void WriteS64(s64 val)
    {
        u64 uval = (u64)val;

        for (int i = 0; i < 8; i++)
        {
            m_rtc_data.push_back(uval & 0xFF);
            uval >>= 8;
        }
    }

    std::vector<u8> GetData()
    {
        return m_rtc_data;
    }

private:
    std::vector<u8> m_rtc_data;
};

MBC3::MBC3(ROMInfo& rom_info) :
    m_rom(std::move(*rom_info.rom)),
    m_ram(std::move(*rom_info.ram)),
    m_has_rtc(rom_info.has_rtc)
{
    if (rom_info.has_rtc)
    {
        if (rom_info.loaded_rtc_data)
        {
            RTCDataReader reader(*rom_info.rtc_data);

            u32 s = reader.ReadU32();
            u32 m = reader.ReadU32();
            u32 h = reader.ReadU32();
            u32 dl = reader.ReadU32();
            u32 dh = reader.ReadU32();
            u32 latched_s = reader.ReadU32();
            u32 latched_m = reader.ReadU32();
            u32 latched_h = reader.ReadU32();
            u32 latched_dl = reader.ReadU32();
            u32 latched_dh = reader.ReadU32();
            m_rtc_last_update = reader.ReadS64();

            if (s >= 60 || latched_s >= 60
                || m >= 60 || latched_m >= 60
                || h >= 24 || latched_h >= 24
                || (dl & ~0xFF) || (latched_dl & ~0xFF)
                || (dh & ~0xFF) || (latched_dh & ~0xFF))
            {
                ResetRTCData();
            }
            else
            {
                m_rtc_s = s;
                m_rtc_m = m;
                m_rtc_h = h;
                m_rtc_dl = dl;
                m_rtc_dh = dh;
                m_rtc_latched_s = latched_s;
                m_rtc_latched_m = latched_m;
                m_rtc_latched_h = latched_h;
                m_rtc_latched_dl = latched_dl;
                m_rtc_latched_dh = latched_dh;
                UpdateRTC();
            }
        }
        else
        {
            ResetRTCData();
        }
    }
}

void MBC3::Reset()
{
    m_rom_map = nullptr;
    m_ram_map = nullptr;
    m_rom_bank = 1;
    m_ram_bank = 0;
    m_ram_enable = false;
    UpdateMapping();

    m_rtc_current_reg = RTC_REG_NONE;
    m_rtc_latch_state = 0;
}

u8 MBC3::Read(u16 addr)
{
    if (addr < 0x4000)
    {
        return m_rom[addr];
    }
    else if (addr < 0x8000)
    {
        return m_rom_map[addr - 0x4000];
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram_enable)
    {
        switch (m_rtc_current_reg)
        {
        case RTC_REG_NONE:
            if (m_ram.size() != 0)
            {
                return m_ram_map[(addr - 0xA000) & (m_ram.size() - 1)];
            }
            return 0;
        case RTC_REG_S:
            return m_rtc_latched_s;
        case RTC_REG_M:
            return m_rtc_latched_m;
        case RTC_REG_H:
            return m_rtc_latched_h;
        case RTC_REG_DL:
            return m_rtc_latched_dl;
        case RTC_REG_DH:
            return m_rtc_latched_dh;
        }
    }

    return 0;
}

void MBC3::Write(u16 addr, u8 val)
{
    if (addr < 0x2000)
    {
        m_ram_enable = (val == 0x0A);
    }
    else if (addr < 0x4000)
    {
        val &= 0x7F;
        if (val == 0)
        {
            val = 1;
        }
        m_rom_bank = val;
        UpdateMapping();
    }
    else if (addr < 0x6000)
    {
        if (m_has_rtc)
        {
            if (val >= RTC_REG_S && val <= RTC_REG_DH)
            {
                m_rtc_current_reg = (RTCReg)val;
            }
            else
            {
                m_rtc_current_reg = RTC_REG_NONE;
                m_ram_bank = val & 0x3;
                UpdateMapping();
            }
        }
        else
        {
            m_ram_bank = val & 0x3;
            UpdateMapping();
        }
    }
    else if (addr < 0x8000)
    {
        if (m_has_rtc)
        {
            switch (m_rtc_latch_state)
            {
            case 0:
                if (val == 0)
                {
                    m_rtc_latch_state = 1;
                }
                break;
            case 1:
                if (val == 1)
                {
                    m_rtc_latch_state = 0;
                    UpdateRTC();
                    m_rtc_latched_s = m_rtc_s;
                    m_rtc_latched_m = m_rtc_m;
                    m_rtc_latched_h = m_rtc_h;
                    m_rtc_latched_dl = m_rtc_dl;
                    m_rtc_latched_dh = m_rtc_dh;
                }
                else if (val != 0)
                {
                    m_rtc_latch_state = 0;
                }
                break;
            }
        }
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram_enable)
    {
        if (m_rtc_current_reg == RTC_REG_NONE)
        {
            if (m_ram.size() != 0)
            {
                m_ram_map[(addr - 0xA000) & (m_ram.size() - 1)] = val;
            }
        }
        else
        {
            UpdateRTC();
            switch (m_rtc_current_reg)
            {
            case RTC_REG_S:
                m_rtc_s = val;
                break;
            case RTC_REG_M:
                m_rtc_m = val;
                break;
            case RTC_REG_H:
                m_rtc_h = val;
                break;
            case RTC_REG_DL:
                m_rtc_dl = val;
                break;
            case RTC_REG_DH:
            {
                bool was_halted = ((m_rtc_dh & dh_halt) != 0);
                bool is_halted = ((val & dh_halt) != 0);
                m_rtc_dh = val;
                if (!is_halted && !was_halted)
                {
                    m_rtc_last_update = time(nullptr);
                }
                break;
            }
            }
        }
    }
}

std::vector<u8> MBC3::GetRTCData()
{
    if (!m_has_rtc)
    {
        return std::vector<u8>();
    }

    UpdateRTC();

    RTCDataWriter writer;

    writer.WriteU32(m_rtc_s);
    writer.WriteU32(m_rtc_m);
    writer.WriteU32(m_rtc_h);
    writer.WriteU32(m_rtc_dl);
    writer.WriteU32(m_rtc_dh);

    writer.WriteU32(m_rtc_latched_s);
    writer.WriteU32(m_rtc_latched_m);
    writer.WriteU32(m_rtc_latched_h);
    writer.WriteU32(m_rtc_latched_dl);
    writer.WriteU32(m_rtc_latched_dh);

    writer.WriteS64(m_rtc_last_update);

    return writer.GetData();
}

void MBC3::UpdateMapping()
{
    u32 rom_addr = (m_rom_bank * 0x4000) & (m_rom.size() - 1);
    m_rom_map = &m_rom[rom_addr];

    if (m_ram.size() != 0)
    {
        u32 ram_addr = (m_ram_bank * 0x2000) & (m_ram.size() - 1);
        m_ram_map = &m_ram[ram_addr];
    }
}

void MBC3::UpdateRTC()
{
    const s64 seconds_per_day = 24 * 60 * 60;
    const s64 seconds_per_hour = 60 * 60;
    const s64 seconds_per_minute = 60;

    if (!(m_rtc_dh & dh_halt))
    {
        s64 now = time(nullptr);
        s64 delta = now - m_rtc_last_update;
        m_rtc_last_update = now;

        if (delta < 0)
        {
            return;
        }

        s64 delta_days = delta / seconds_per_day;
        delta %= seconds_per_day;
        s64 delta_hours = delta / seconds_per_hour;
        delta %= seconds_per_hour;
        s64 delta_minutes = delta / seconds_per_minute;
        delta %= seconds_per_minute;
        s64 delta_seconds = delta;

        u64 days = ((m_rtc_dh & 0x1) << 8) | m_rtc_dl;

        m_rtc_s += (u8)delta_seconds;
        if (m_rtc_s >= 60)
        {
            m_rtc_s -= 60;
            m_rtc_m++;
        }

        m_rtc_m += (u8)delta_minutes;
        if (m_rtc_m >= 60)
        {
            m_rtc_m -= 60;
            m_rtc_h++;
        }

        m_rtc_h += (u8)delta_hours;
        if (m_rtc_h >= 24)
        {
            m_rtc_h -= 24;
            days++;
        }

        days += delta_days;
        if (days < 512)
        {
            m_rtc_dl = days & 0xFF;
            m_rtc_dh = (days >> 8) & 0x1;
        }
        else
        {
            m_rtc_s = 59;
            m_rtc_m = 59;
            m_rtc_h = 23;
            m_rtc_dl = 0xFF;
            m_rtc_dh = dh_overflow | 0x1;
        }
    }
}

void MBC3::ResetRTCData()
{
    m_rtc_s = 0;
    m_rtc_m = 0;
    m_rtc_h = 0;
    m_rtc_dl = 0;
    m_rtc_dh = dh_halt;
    m_rtc_latched_s = 0;
    m_rtc_latched_m = 0;
    m_rtc_latched_h = 0;
    m_rtc_latched_dl = 0;
    m_rtc_latched_dh = dh_halt;
    m_rtc_last_update = time(nullptr);
}
