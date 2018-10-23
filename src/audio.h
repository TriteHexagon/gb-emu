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
#include <vector>
#include "common.h"

const int sample_rate = 48000;

struct Hardware;

class Audio
{
public:
    explicit Audio(Hardware& hw) : m_hw(hw)
    {
    }

    void Reset();

    const std::vector<float>& GetSampleBuffer();
    void ClearSampleBuffer();

    u8 ReadNR10();
    void WriteNR10(u8 val);

    u8 ReadNR11();
    void WriteNR11(u8 val);

    u8 ReadNR12();
    void WriteNR12(u8 val);

    u8 ReadNR13();
    void WriteNR13(u8 val);

    u8 ReadNR14();
    void WriteNR14(u8 val);

    u8 ReadNR21();
    void WriteNR21(u8 val);

    u8 ReadNR22();
    void WriteNR22(u8 val);

    u8 ReadNR23();
    void WriteNR23(u8 val);

    u8 ReadNR24();
    void WriteNR24(u8 val);

    u8 ReadNR30();
    void WriteNR30(u8 val);

    u8 ReadNR31();
    void WriteNR31(u8 val);

    u8 ReadNR32();
    void WriteNR32(u8 val);

    u8 ReadNR33();
    void WriteNR33(u8 val);

    u8 ReadNR34();
    void WriteNR34(u8 val);

    u8 ReadNR41();
    void WriteNR41(u8 val);

    u8 ReadNR42();
    void WriteNR42(u8 val);

    u8 ReadNR43();
    void WriteNR43(u8 val);

    u8 ReadNR44();
    void WriteNR44(u8 val);

    u8 ReadNR50();
    void WriteNR50(u8 val);

    u8 ReadNR51();
    void WriteNR51(u8 val);

    u8 ReadNR52();
    void WriteNR52(u8 val);

    u8 ReadWaveRAM(u8 addr);
    void WriteWaveRAM(u8 addr, u8 val);

    void TimerTick();

    void Update(unsigned int cycles);

private:
    struct Length
    {
        bool enabled;
        u16 counter;
    };

    struct Envelope
    {
        u8 initial_volume;
        u8 volume;
        u8 counter;
        u8 period;
        bool increasing;
        bool enabled;
    };

    void UpdateLength(Length& length, bool& chan_enabled);
    void UpdateEnvelope(Envelope& envelope);

    void UpdatePulse1();
    void UpdatePulse2();
    void UpdateWave();
    void UpdateNoise();

    u8 GetPulse1Output();
    u8 GetPulse2Output();
    u8 GetWaveOutput();
    u8 GetNoiseOutput();

    void OutputSample();

    Hardware& m_hw;

    std::vector<float> m_sample_buffer;

    u64 m_total_cycles;

    bool m_audio_enable;

    u8 m_timer_ticks;

    // Pulse 1 sweep
    bool m_sweep_enabled;
    u16 m_sweep_frequency;
    u8 m_sweep_shift;
    bool m_sweep_decreasing;
    u8 m_sweep_counter;
    u8 m_sweep_period;

    // Pulse 1 (channel 1)
    bool m_pulse1_enabled;
    Length m_pulse1_length;
    u8 m_pulse1_duty;
    Envelope m_pulse1_envelope;
    u16 m_pulse1_frequency;
    u16 m_pulse1_counter;
    u8 m_pulse1_duty_counter;

    // Pulse 2 (channel 2)
    bool m_pulse2_enabled;
    Length m_pulse2_length;
    u8 m_pulse2_duty;
    Envelope m_pulse2_envelope;
    u16 m_pulse2_frequency;
    u16 m_pulse2_counter;
    u8 m_pulse2_duty_counter;

    // Wave (channel 3)
    bool m_wave_enabled;
    bool m_wave_dac_enabled;
    Length m_wave_length;
    u8 m_wave_output_level;
    u16 m_wave_frequency;
    u16 m_wave_counter;
    u16 m_wave_pos_counter;

    // Noise (channel 4)
    bool m_noise_enabled;
    Length m_noise_length;
    Envelope m_noise_envelope;
    u8 m_noise_shift_clock_frequency;
    bool m_noise_narrow;
    u8 m_noise_div_ratio;
    u16 m_noise_counter;
    u16 m_noise_lfsr;

    u8 m_nr10_val;
    u8 m_nr11_val;
    u8 m_nr12_val;
    u8 m_nr14_val;

    u8 m_nr21_val;
    u8 m_nr22_val;
    u8 m_nr24_val;

    u8 m_nr30_val;
    u8 m_nr32_val;
    u8 m_nr34_val;

    u8 m_nr42_val;
    u8 m_nr43_val;
    u8 m_nr44_val;

    u8 m_nr50_val;
    u8 m_nr51_val;

    u8 m_so1_volume;
    u8 m_so2_volume;

    bool m_so1_ch1_enable;
    bool m_so1_ch2_enable;
    bool m_so1_ch3_enable;
    bool m_so1_ch4_enable;
    bool m_so2_ch1_enable;
    bool m_so2_ch2_enable;
    bool m_so2_ch3_enable;
    bool m_so2_ch4_enable;

    std::array<u8, 16> m_wave_ram;
};
