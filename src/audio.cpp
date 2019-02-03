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

#include "common.h"
#include "audio.h"
#include "machine.h"

const std::array<u8, 4> duty_cycles =
{
    0b00000001,
    0b10000001,
    0b10000111,
    0b01111110,
};

const double sample_ratio = 2097152.0 / sample_rate;

u16 CalcPulsePeriod(u16 frequency)
{
    return 2 * (2048 - frequency);
}

u16 CalcWavePeriod(u16 frequency)
{
    return 2048 - frequency;
}

void Audio::Reset()
{
    m_total_cycles = 0;

    m_audio_enable = false;

    m_timer_ticks = 0;

    // Pulse 1 sweep
    m_sweep_enabled = false;
    m_sweep_frequency = 0;
    m_sweep_shift = 0;
    m_sweep_decreasing = false;
    m_sweep_counter = 0;
    m_sweep_period = 0;

    // Pulse 1 (channel 1)
    m_pulse1_enabled = false;
    m_pulse1_length = {};
    m_pulse1_duty = 0;
    m_pulse1_envelope = {};
    m_pulse1_frequency = 0;
    m_pulse1_counter = 0;
    m_pulse1_duty_counter = 0;

    // Pulse 2 (channel 2)
    m_pulse2_enabled = false;
    m_pulse2_length = {};
    m_pulse2_duty = 0;
    m_pulse2_envelope = {};
    m_pulse2_frequency = 0;
    m_pulse2_counter = 0;
    m_pulse2_duty_counter = 0;

    // Wave (channel 3)
    m_wave_enabled = false;
    m_wave_dac_enabled = false;
    m_wave_length = {};
    m_wave_output_level = 0;
    m_wave_frequency = 0;
    m_wave_counter = 0;
    m_wave_pos_counter = 0;

    // Noise (channel 4)
    m_noise_enabled = false;
    m_noise_length = {};
    m_noise_envelope = {};
    m_noise_shift_clock_frequency = 0;
    m_noise_narrow = false;
    m_noise_div_ratio = 0;
    m_noise_counter = 0;
    m_noise_lfsr = 0;

    m_nr10_val = 0;
    m_nr11_val = 0;
    m_nr12_val = 0;
    m_nr14_val = 0;

    m_nr21_val = 0;
    m_nr22_val = 0;
    m_nr24_val = 0;

    m_nr30_val = 0;
    m_nr32_val = 0;
    m_nr34_val = 0;

    m_nr42_val = 0;
    m_nr43_val = 0;
    m_nr44_val = 0;

    m_nr50_val = 0;
    m_nr51_val = 0;

    m_so1_volume = 0;
    m_so2_volume = 0;

    m_so1_ch1_enable = false;
    m_so1_ch2_enable = false;
    m_so1_ch3_enable = false;
    m_so1_ch4_enable = false;
    m_so2_ch1_enable = false;
    m_so2_ch2_enable = false;
    m_so2_ch3_enable = false;
    m_so2_ch4_enable = false;

    m_wave_ram = {};
}

const std::vector<float>& Audio::GetSampleBuffer()
{
    return m_sample_buffer;
}

void Audio::ClearSampleBuffer()
{
    m_sample_buffer.clear();
}

void Audio::ConsumeSampleBuffer(size_t num_samples)
{
    m_sample_buffer.erase(m_sample_buffer.begin(), m_sample_buffer.begin() + num_samples);
}

u8 Audio::ReadNR10()
{
    return m_nr10_val | 0x80;
}

void Audio::WriteNR10(u8 val)
{
    m_nr10_val = val;
    m_sweep_shift = val & 0x7;
    m_sweep_decreasing = (val >> 3) & 1;
    m_sweep_period = (val >> 4) & 0x7;
}

u8 Audio::ReadNR11()
{
    return m_nr11_val | 0x3F;
}

void Audio::WriteNR11(u8 val)
{
    m_nr11_val = val;
    m_pulse1_length.counter = 64 - (val & 0x3F);
    m_pulse1_duty = val >> 6;
}

u8 Audio::ReadNR12()
{
    return m_nr12_val;
}

void Audio::WriteNR12(u8 val)
{
    m_nr12_val = val;
    m_pulse1_envelope.period = val & 0x7;
    m_pulse1_envelope.increasing = (val >> 3) & 1;
    m_pulse1_envelope.initial_volume = val >> 4;
}

u8 Audio::ReadNR13()
{
    return 0xFF;
}

void Audio::WriteNR13(u8 val)
{
    m_pulse1_frequency = (m_pulse1_frequency & 0x700) | val;
}

u8 Audio::ReadNR14()
{
    return m_nr14_val | 0xBF;
}

void Audio::WriteNR14(u8 val)
{
    m_nr14_val = val;
    m_pulse1_frequency = ((val & 0x7) << 8) | (m_pulse1_frequency & 0xFF);
    m_pulse1_length.enabled = (val >> 6) & 1;
    bool restart = (val >> 7) & 1;

    if (restart)
    {
        m_pulse1_enabled = true;
        if (m_pulse1_length.counter == 0)
        {
            m_pulse1_length.counter = 64;
        }
        m_pulse1_counter = CalcPulsePeriod(m_pulse1_frequency);
        m_pulse1_envelope.enabled = true;
        m_pulse1_envelope.counter = m_pulse1_envelope.period;
        m_pulse1_envelope.volume = m_pulse1_envelope.initial_volume;
        m_sweep_frequency = m_pulse1_frequency;
        m_sweep_counter = m_sweep_period;
        m_sweep_enabled = (m_sweep_period != 0) || (m_sweep_shift != 0);
    }
}

u8 Audio::ReadNR21()
{
    return m_nr21_val | 0x3F;
}

void Audio::WriteNR21(u8 val)
{
    m_nr21_val = val;
    m_pulse2_length.counter = 64 - (val & 0x3F);
    m_pulse2_duty = val >> 6;
}

u8 Audio::ReadNR22()
{
    return m_nr22_val;
}

void Audio::WriteNR22(u8 val)
{
    m_nr22_val = val;
    m_pulse2_envelope.period = val & 0x7;
    m_pulse2_envelope.increasing = (val >> 3) & 1;
    m_pulse2_envelope.initial_volume = val >> 4;
}

u8 Audio::ReadNR23()
{
    return 0xFF;
}

void Audio::WriteNR23(u8 val)
{
    m_pulse2_frequency = (m_pulse2_frequency & 0x700) | val;
}

u8 Audio::ReadNR24()
{
    return m_nr24_val | 0xBF;
}

void Audio::WriteNR24(u8 val)
{
    m_nr24_val = val;
    m_pulse2_frequency = ((val & 0x7) << 8) | (m_pulse2_frequency & 0xFF);
    m_pulse2_length.enabled = (val >> 6) & 1;
    bool restart = (val >> 7) & 1;

    if (restart)
    {
        m_pulse2_enabled = true;
        if (m_pulse2_length.counter == 0)
        {
            m_pulse2_length.counter = 64;
        }
        m_pulse2_counter = CalcPulsePeriod(m_pulse2_frequency);
        m_pulse2_envelope.enabled = true;
        m_pulse2_envelope.counter = m_pulse2_envelope.period;
        m_pulse2_envelope.volume = m_pulse2_envelope.initial_volume;
    }
}

u8 Audio::ReadNR30()
{
    return m_nr30_val | 0x7F;
}

void Audio::WriteNR30(u8 val)
{
    m_nr30_val = val;
    m_wave_dac_enabled = (val >> 7) & 1;
}

u8 Audio::ReadNR31()
{
    return 0xFF;
}

void Audio::WriteNR31(u8 val)
{
    m_wave_length.counter = 256 - val;
}

u8 Audio::ReadNR32()
{
    return m_nr32_val | 0x9F;
}

void Audio::WriteNR32(u8 val)
{
    m_nr32_val = val;
    m_wave_output_level = (val >> 5) & 0x3;
}

u8 Audio::ReadNR33()
{
    return 0xFF;
}

void Audio::WriteNR33(u8 val)
{
    m_wave_frequency = (m_wave_frequency & 0x700) | val;
}

u8 Audio::ReadNR34()
{
    return m_nr34_val | 0xBF;
}

void Audio::WriteNR34(u8 val)
{
    m_nr34_val = val;
    m_wave_frequency = ((val & 0x7) << 8) | (m_wave_frequency & 0xFF);
    m_wave_length.enabled = (val >> 6) & 1;
    bool restart = (val >> 7) & 1;

    if (restart)
    {
        m_wave_enabled = true;
        if (m_wave_length.counter == 0)
        {
            m_wave_length.counter = 256;
        }
        m_wave_counter = CalcWavePeriod(m_wave_frequency);
        m_wave_pos_counter = 0;
    }
}

u8 Audio::ReadNR41()
{
    return 0xFF;
}

void Audio::WriteNR41(u8 val)
{
    m_noise_length.counter = 64 - (val & 0x3F);
}

u8 Audio::ReadNR42()
{
    return m_nr42_val;
}

void Audio::WriteNR42(u8 val)
{
    m_nr42_val = val;
    m_noise_envelope.period = val & 0x7;
    m_noise_envelope.increasing = (val >> 3) & 1;
    m_noise_envelope.initial_volume = val >> 4;
}

u8 Audio::ReadNR43()
{
    return m_nr43_val;
}

void Audio::WriteNR43(u8 val)
{
    m_nr43_val = val;
    m_noise_div_ratio = val & 0x7;
    m_noise_narrow = (val >> 3) & 1;
    m_noise_shift_clock_frequency = (val >> 4);
}

u8 Audio::ReadNR44()
{
    return m_nr44_val | 0xBF;
}

void Audio::WriteNR44(u8 val)
{
    m_nr44_val = val;
    m_noise_length.enabled = (val >> 6) & 1;
    bool restart = (val >> 7) & 1;

    if (restart)
    {
        m_noise_enabled = true;
        if (m_noise_length.counter == 0)
        {
            m_noise_length.counter = 64;
        }
        m_noise_envelope.enabled = true;
        m_noise_envelope.counter = m_noise_envelope.period;
        m_noise_envelope.volume = m_noise_envelope.initial_volume;
        m_noise_lfsr = 0x7FFF;
    }
}

u8 Audio::ReadNR50()
{
    return m_nr50_val;
}

void Audio::WriteNR50(u8 val)
{
    m_nr50_val = val;
    m_so1_volume = val & 0x7;
    m_so2_volume = (val >> 4) & 0x7;
}

u8 Audio::ReadNR51()
{
    return m_nr51_val;
}

void Audio::WriteNR51(u8 val)
{
    m_nr51_val = val;
    m_so1_ch1_enable = (val >> 0) & 1;
    m_so1_ch2_enable = (val >> 1) & 1;
    m_so1_ch3_enable = (val >> 2) & 1;
    m_so1_ch4_enable = (val >> 3) & 1;
    m_so2_ch1_enable = (val >> 4) & 1;
    m_so2_ch2_enable = (val >> 5) & 1;
    m_so2_ch3_enable = (val >> 6) & 1;
    m_so2_ch4_enable = (val >> 7) & 1;
}

u8 Audio::ReadNR52()
{
    return (m_audio_enable << 7)
        | 0x70
        | (m_noise_enabled << 3)
        | (m_wave_enabled << 2)
        | (m_pulse2_enabled << 1)
        | (m_pulse1_enabled << 0);
}

void Audio::WriteNR52(u8 val)
{
    m_audio_enable = (val >> 7) & 1;
}

u8 Audio::ReadWaveRAM(u8 addr)
{
    return m_wave_ram[addr];
}

void Audio::WriteWaveRAM(u8 addr, u8 val)
{
    m_wave_ram[addr] = val;
}

void Audio::UpdateLength(Length& length, bool& chan_enabled)
{
    if (length.enabled)
    {
        if (length.counter != 0)
        {
            length.counter--;

            if (length.counter == 0)
            {
                chan_enabled = false;
            }
        }
    }
}

void Audio::UpdateEnvelope(Envelope& envelope)
{
    if (envelope.enabled && envelope.period != 0)
    {
        envelope.counter--;

        if (envelope.counter == 0)
        {
            envelope.counter = envelope.period;

            if (envelope.increasing)
            {
                if (envelope.volume == 15)
                {
                    envelope.enabled = false;
                }
                else
                {
                    envelope.volume++;
                }
            }
            else
            {
                if (envelope.volume == 0)
                {
                    envelope.enabled = false;
                }
                else
                {
                    envelope.volume--;
                }
            }
        }
    }
}

void Audio::TimerTick()
{
    if (!m_audio_enable)
    {
        return;
    }

    m_timer_ticks++;

    // length
    UpdateLength(m_pulse1_length, m_pulse1_enabled);
    UpdateLength(m_pulse2_length, m_pulse2_enabled);
    UpdateLength(m_wave_length, m_wave_enabled);
    UpdateLength(m_noise_length, m_noise_enabled);

    // envelope
    if ((m_timer_ticks & 3) == 3)
    {
        UpdateEnvelope(m_pulse1_envelope);
        UpdateEnvelope(m_pulse2_envelope);
        UpdateEnvelope(m_noise_envelope);
    }

    // sweep
    if (m_timer_ticks & 1)
    {
        if (m_sweep_enabled && m_sweep_period != 0)
        {
            m_sweep_counter--;

            if (m_sweep_counter == 0)
            {
                m_sweep_counter = m_sweep_period;

                if (m_sweep_shift != 0)
                {
                    s16 delta = m_sweep_frequency >> m_sweep_shift;

                    if (m_sweep_decreasing)
                    {
                        delta = -delta;
                    }

                    u16 new_frequency = m_sweep_frequency + delta;

                    if (new_frequency < 2048)
                    {
                        m_sweep_frequency = new_frequency;
                        m_pulse1_frequency = new_frequency;
                    }
                    else
                    {
                        m_pulse1_enabled = false;
                    }
                }
            }
        }
    }
}

void Audio::UpdatePulse1()
{
    if (m_pulse1_enabled)
    {
        m_pulse1_counter--;

        if (m_pulse1_counter == 0)
        {
            m_pulse1_counter = CalcPulsePeriod(m_pulse1_frequency);

            m_pulse1_duty_counter++;
            m_pulse1_duty_counter &= 0x7;
        }
    }
}

void Audio::UpdatePulse2()
{
    if (m_pulse2_enabled)
    {
        m_pulse2_counter--;

        if (m_pulse2_counter == 0)
        {
            m_pulse2_counter = CalcPulsePeriod(m_pulse2_frequency);

            m_pulse2_duty_counter++;
            m_pulse2_duty_counter &= 0x7;
        }
    }
}

void Audio::UpdateWave()
{
    if (m_wave_enabled)
    {
        m_wave_counter--;

        if (m_wave_counter == 0)
        {
            m_wave_counter = CalcWavePeriod(m_wave_frequency);

            m_wave_pos_counter++;
            m_wave_pos_counter &= 0x1F;
        }
    }
}

void Audio::UpdateNoise()
{
    if (m_noise_enabled && m_noise_shift_clock_frequency < 14)
    {
        m_noise_counter--;

        if (m_noise_counter == 0)
        {
            u16 div_ratio = m_noise_div_ratio ? m_noise_div_ratio * 8 : 4;
            m_noise_counter = div_ratio << m_noise_shift_clock_frequency;
            u16 right_xor = ((m_noise_lfsr >> 1) & 1) ^ (m_noise_lfsr & 1);
            m_noise_lfsr >>= 1;
            m_noise_lfsr |= (right_xor << 14);
            if (m_noise_narrow)
            {
                m_noise_lfsr &= ~Bit(6);
                m_noise_lfsr |= (right_xor << 6);
            }
        }
    }
}

u8 Audio::GetPulse1Output()
{
    if (m_pulse1_enabled && ((duty_cycles[m_pulse1_duty] >> m_pulse1_duty_counter) & 1))
    {
        return m_pulse1_envelope.volume;
    }

    return 0;
}

u8 Audio::GetPulse2Output()
{
    if (m_pulse2_enabled && ((duty_cycles[m_pulse2_duty] >> m_pulse2_duty_counter) & 1))
    {
        return m_pulse2_envelope.volume;
    }

    return 0;
}

u8 Audio::GetWaveOutput()
{
    if (m_wave_enabled && m_wave_output_level != 0)
    {
        u8 sample = m_wave_ram[m_wave_pos_counter >> 1];
        if (!(m_wave_pos_counter & 1))
        {
            sample >>= 4;
        }
        else
        {
            sample &= 0xF;
        }
        sample >>= (m_wave_output_level - 1);
        return sample;
    }

    return 0;
}

u8 Audio::GetNoiseOutput()
{
    if (m_noise_enabled && ((m_noise_lfsr & 1) ^ 1))
    {
        return m_noise_envelope.volume;
    }

    return 0;
}

void Audio::OutputSample()
{
    std::lock_guard<std::mutex> lock(m_sample_buffer_mutex);

    if (!m_audio_enable)
    {
        m_sample_buffer.push_back(0);
        m_sample_buffer.push_back(0);
        return;
    }

    float pulse1_output = (float)GetPulse1Output() / 15;
    float pulse2_output = (float)GetPulse2Output() / 15;
    float wave_output = (float)GetWaveOutput() / 15;
    float noise_output = (float)GetNoiseOutput() / 15;

    float so1_output = 0;

    if (m_so1_ch1_enable) so1_output += pulse1_output;
    if (m_so1_ch2_enable) so1_output += pulse2_output;
    if (m_so1_ch3_enable) so1_output += wave_output;
    if (m_so1_ch4_enable) so1_output += noise_output;

    so1_output /= 4;

    so1_output *= (m_so1_volume + 1) / 8;

    m_sample_buffer.push_back(so1_output);

    float so2_output = 0;

    if (m_so2_ch1_enable) so2_output += pulse1_output;
    if (m_so2_ch2_enable) so2_output += pulse2_output;
    if (m_so2_ch3_enable) so2_output += wave_output;
    if (m_so2_ch4_enable) so2_output += noise_output;

    so2_output /= 4;

    so2_output *= (m_so2_volume + 1) / 8;

    m_sample_buffer.push_back(so2_output);
}

void Audio::Update(unsigned int cycles)
{
    while (cycles > 0)
    {
        m_total_cycles++;

        if (!m_wave_dac_enabled)
        {
            m_wave_enabled = false;
        }

        UpdatePulse1();
        UpdatePulse2();
        UpdateWave();
        UpdateNoise();

        s64 prev_sample_num = (s64)((double)(m_total_cycles - 1) / sample_ratio);
        s64 sample_num = (s64)((double)m_total_cycles / sample_ratio);

        if (sample_num != prev_sample_num)
        {
            OutputSample();
        }

        cycles--;
    }
}
