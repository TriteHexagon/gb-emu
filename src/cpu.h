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

#include "common.h"

struct Hardware;

const unsigned int intr_vblank = Bit(0);
const unsigned int intr_lcdc_status = Bit(1);
const unsigned int intr_timer = Bit(2);
const unsigned int intr_serial = Bit(3);
const unsigned int intr_joypad = Bit(4);
const unsigned int intr_all = 0x1F;

class CPU
{
public:
    explicit CPU(Hardware& hw) : m_hw(hw)
    {
    }

    void Reset();
    void Run(unsigned int cycles);

    u8 ReadIF();
    void WriteIF(u8 val);

    u8 ReadIE();
    void WriteIE(u8 val);

    void SetInterruptFlag(u16 mask);
    void ClearInterruptFlag(u16 mask);

    bool IsDoubleSpeed();

    u8 ReadKEY1();
    void WriteKEY1(u8 val);

    void SetTraceLogEnabled(bool enabled);

private:
    union RegisterPair
    {
        u16 word;
        u8 byte[2];
    };

    enum class IMEState
    {
        Stable,
        EnableRequested,
        EnableNow,
    };

    enum class HaltState
    {
        Off,
        On,
        Bug,
    };

    void AddCycles(unsigned int cycles);

    u8 ReadMem8(u16 addr);
    u16 ReadMem16(u16 addr);
    void WriteMem8(u16 addr, u8 val);
    void WriteMem16(u16 addr, u16 val);
    u8 ReadNextByte();
    u16 ReadNextWord();

    u8 ZFlag(u8 val);
    u8 SwapNybbles(u8 val);
    void Push(u16 val);
    u16 Pop();
    void Call(u16 addr);
    void Jump(u16 addr);
    void Return();
    u16 CalcRelativeJumpTarget();
    void DisableInterrupts();

    void Op_ADC_A(u8 val);
    void Op_ADD_A(u8 val);
    void Op_ADD_HL(u16 val);
    void Op_ADD_SP_Imm();
    void Op_AND_A(u8 val);
    void Op_BIT_PtrHL(int bit);
    void Op_BIT_Reg8(int bit, u8 reg);
    void Op_CALL();
    void Op_CALL_NZ();
    void Op_CALL_Z();
    void Op_CALL_NC();
    void Op_CALL_C();
    void Op_CCF();
    void Op_CP_A(u8 val);
    void Op_CPL_A();
    void Op_DAA();
    void Op_DEC_PtrHL();
    void Op_DEC_Reg8(u8& reg);
    void Op_DEC_Reg16(u16& reg);
    void Op_DI();
    void Op_EI();
    void Op_HALT();
    void Op_INC_PtrHL();
    void Op_INC_Reg8(u8& reg);
    void Op_INC_Reg16(u16& reg);
    void Op_JP();
    void Op_JP_NZ();
    void Op_JP_Z();
    void Op_JP_NC();
    void Op_JP_C();
    void Op_JR();
    void Op_JR_NZ();
    void Op_JR_Z();
    void Op_JR_NC();
    void Op_JR_C();
    void Op_LD_HL_SPOffset();
    void Op_OR_A(u8 val);
    void Op_POP(u16& reg);
    void Op_POP_AF();
    void Op_PUSH(u16 reg);
    void Op_RES_PtrHL(int bit);
    void Op_RES_Reg8(int bit, u8& reg);
    void Op_RET();
    void Op_RET_NZ();
    void Op_RET_Z();
    void Op_RET_NC();
    void Op_RET_C();
    void Op_RETI();
    void Op_RL_PtrHL();
    void Op_RL_Reg8(u8& reg);
    void Op_RLA();
    void Op_RLC_PtrHL();
    void Op_RLC_Reg8(u8& reg);
    void Op_RLCA();
    void Op_RR_PtrHL();
    void Op_RR_Reg8(u8& reg);
    void Op_RRA();
    void Op_RRC_PtrHL();
    void Op_RRC_Reg8(u8& reg);
    void Op_RRCA();
    void Op_RST(u16 addr);
    void Op_SBC_A(u8 val);
    void Op_SCF();
    void Op_SET_PtrHL(int bit);
    void Op_SET_Reg8(int bit, u8& reg);
    void Op_SLA_PtrHL();
    void Op_SLA_Reg8(u8& reg);
    void Op_SRA_PtrHL();
    void Op_SRA_Reg8(u8& reg);
    void Op_SRL_PtrHL();
    void Op_SRL_Reg8(u8& reg);
    void Op_STOP();
    void Op_SUB_A(u8 val);
    void Op_SWAP_PtrHL();
    void Op_SWAP_Reg8(u8& reg);
    void Op_XOR_A(u8 val);

    void CallInterruptHandler(u16 interrupt_vector);
    void HandleInterrupts();

    void ExecInstructionPrefixCB();
    bool ExecInstruction();

    Hardware& m_hw;

    RegisterPair m_reg_af, m_reg_bc, m_reg_de, m_reg_hl; // general registers
    u16 m_reg_sp; // stack pointer
    u16 m_reg_pc; // program counter

    HaltState m_halt_state;

    bool m_ime; // interrupt master enable
    IMEState m_ime_state;
    u8 m_reg_if; // interrupt request flags
    u8 m_reg_ie; // interrupt enable flags

    bool m_double_speed;
    bool m_should_switch_speed;

    unsigned int m_instruction_cycles;

    int m_cycles_left;

    bool m_trace_log_enabled;
};
