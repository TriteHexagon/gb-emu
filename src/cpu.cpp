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
#include "cpu.h"
#include "machine.h"

#ifdef GBEMU_BIG_ENDIAN
#define LO_REG 1
#define HI_REG 0
#else
#define LO_REG 0
#define HI_REG 1
#endif

#define REG_AF (m_reg_af.word)
#define REG_BC (m_reg_bc.word)
#define REG_DE (m_reg_de.word)
#define REG_HL (m_reg_hl.word)
#define REG_SP (m_reg_sp)
#define REG_PC (m_reg_pc)

#define REG_A (m_reg_af.byte[HI_REG])
#define REG_F (m_reg_af.byte[LO_REG])
#define REG_B (m_reg_bc.byte[HI_REG])
#define REG_C (m_reg_bc.byte[LO_REG])
#define REG_D (m_reg_de.byte[HI_REG])
#define REG_E (m_reg_de.byte[LO_REG])
#define REG_H (m_reg_hl.byte[HI_REG])
#define REG_L (m_reg_hl.byte[LO_REG])

#define REG_IF (m_reg_if)
#define REG_IE (m_reg_ie)

const int flag_c_shift = 4;
const int flag_h_shift = 5;
const int flag_n_shift = 6;
const int flag_z_shift = 7;

const unsigned int flag_c = Bit(flag_c_shift);
const unsigned int flag_h = Bit(flag_h_shift);
const unsigned int flag_n = Bit(flag_n_shift);
const unsigned int flag_z = Bit(flag_z_shift);

void CPU::Reset()
{
    REG_AF = 0x01B0;
    REG_BC = 0x0013;
    REG_DE = 0x00D8;
    REG_HL = 0x014D;
    REG_SP = 0xFFFE;
    REG_PC = 0x0100;

    m_halt_state = HaltState::Off;

    m_ime = false;
    m_ime_state = IMEState::Stable;

    REG_IF = 0;
    REG_IE = 0;

    m_cycles_left = 0;
}

void CPU::Run(unsigned int cycles)
{
    m_cycles_left += cycles;

    while (m_cycles_left > 0)
    {
        m_instruction_cycles = 0;

        HandleInterrupts();

        if (m_halt_state == HaltState::On)
        {
            AddCycles(1);
        }
        else
        {
            if (m_ime_state == IMEState::EnableNow)
            {
                m_ime = true;
                m_ime_state = IMEState::Stable;
            }

            ExecInstruction();

            if (m_ime_state == IMEState::EnableRequested)
            {
                m_ime_state = IMEState::EnableNow;
            }
        }

        m_cycles_left -= m_instruction_cycles;
    }
}

u8 CPU::ReadIF()
{
    return REG_IF | ~intr_all;
}

void CPU::WriteIF(u8 val)
{
    REG_IF = val & intr_all;
}

u8 CPU::ReadIE()
{
    return REG_IE;
}

void CPU::WriteIE(u8 val)
{
    REG_IE = val;
}

void CPU::SetInterruptFlag(u16 mask)
{
    REG_IF |= mask;
}

void CPU::ClearInterruptFlag(u16 mask)
{
    REG_IF &= ~mask;
}

void CPU::AddCycles(unsigned int cycles)
{
    m_instruction_cycles += cycles;

    m_machine.GetTimer().Update(cycles);
    m_machine.GetGraphics().Update(cycles);
}

u8 CPU::ReadMem8(u16 addr)
{
    u8 val = m_machine.GetMemory().Read(addr);
    AddCycles(1);
    return val;
}

u16 CPU::ReadMem16(u16 addr)
{
    u16 val = m_machine.GetMemory().Read(addr);
    AddCycles(1);
    val |= (u16)m_machine.GetMemory().Read(addr + 1) << 8;
    AddCycles(1);
    return val;
}

void CPU::WriteMem8(u16 addr, u8 val)
{
    m_machine.GetMemory().Write(addr, val);
    AddCycles(1);
}

void CPU::WriteMem16(u16 addr, u16 val)
{
    m_machine.GetMemory().Write(addr, (u8)val);
    AddCycles(1);
    m_machine.GetMemory().Write(addr + 1, val >> 8);
    AddCycles(1);
}

u8 CPU::ReadNextByte()
{
    u8 val = ReadMem8(REG_PC);

    if (m_halt_state == HaltState::Bug)
    {
        m_halt_state = HaltState::On;
    }
    else
    {
        REG_PC++;
    }

    return val;
}

u16 CPU::ReadNextWord()
{
    u16 val = ReadMem8(REG_PC++);
    val |= (u16)ReadMem8(REG_PC++) << 8;
    return val;
}

u8 CPU::ZFlag(u8 val)
{
    return (val == 0) ? flag_z : 0;
}

u8 CPU::SwapNybbles(u8 val)
{
    return ((val & 0xF) << 4) | (val >> 4);
}

void CPU::Push(u16 val)
{
    WriteMem8(--REG_SP, val >> 8);
    WriteMem8(--REG_SP, (u8)val);
}

u16 CPU::Pop()
{
    u16 val = ReadMem8(REG_SP++);
    val |= (u16)ReadMem8(REG_SP++) << 8;
    return val;
}

void CPU::Call(u16 addr)
{
    AddCycles(1);
    Push(REG_PC);
    REG_PC = addr;
}

void CPU::Jump(u16 addr)
{
    AddCycles(1);
    REG_PC = addr;
}

void CPU::Return()
{
    REG_PC = Pop();
    AddCycles(1);
}

u16 CPU::CalcRelativeJumpTarget()
{
    s8 offset = ReadNextByte();
    return REG_PC + offset;
}

void CPU::DisableInterrupts()
{
    m_ime = false;
    m_ime_state = IMEState::Stable;
}

void CPU::Op_ADC_A(u8 val)
{
    int carry = (REG_F & flag_c) >> flag_c_shift;
    u16 temp = REG_A + val + carry;
    REG_F = ZFlag((u8)temp);
    if (temp > 0xFF)
    {
        REG_F |= flag_c;
    }
    if ((REG_A & 0xF) + (val & 0xF) + carry > 0xF)
    {
        REG_F |= flag_h;
    }
    REG_A = (u8)temp;
}

void CPU::Op_ADD_A(u8 val)
{
    u16 temp = REG_A + val;
    REG_F = ZFlag((u8)temp);
    if (temp > 0xFF)
    {
        REG_F |= flag_c;
    }
    if ((REG_A & 0xF) + (val & 0xF) > 0xF)
    {
        REG_F |= flag_h;
    }
    REG_A = (u8)temp;
}

void CPU::Op_ADD_HL(u16 val)
{
    u32 temp = REG_HL + val;
    REG_F &= flag_z;
    if (temp > 0xFFFF)
    {
        REG_F |= flag_c;
    }
    if (((REG_HL ^ val) ^ temp) & 0x1000)
    {
        REG_F |= flag_h;
    }
    REG_HL = temp;
    AddCycles(1);
}

void CPU::Op_ADD_SP_Imm()
{
    u8 val = ReadNextByte();
    unsigned int temp = (u8)REG_SP + val;
    REG_F = (temp > 0xFF) ? flag_c : 0;
    if (((REG_SP ^ val) ^ temp) & 0x10)
    {
        REG_F |= flag_h;
    }
    REG_SP += (s8)val;
    AddCycles(2);
}

void CPU::Op_AND_A(u8 val)
{
    REG_A &= val;
    REG_F = ZFlag(REG_A) | flag_h;
}

void CPU::Op_BIT_PtrHL(int bit)
{
    REG_F &= flag_c;
    REG_F |= ZFlag(ReadMem8(REG_HL) & Bit(bit)) | flag_h;
}

void CPU::Op_BIT_Reg8(int bit, u8 reg)
{
    REG_F &= flag_c;
    REG_F |= ZFlag(reg & Bit(bit)) | flag_h;
}

void CPU::Op_CALL()
{
    Call(ReadNextWord());
}

void CPU::Op_CALL_NZ()
{
    u16 target = ReadNextWord();
    if (!(REG_F & flag_z))
    {
        Call(target);
    }
}

void CPU::Op_CALL_Z()
{
    u16 target = ReadNextWord();
    if (REG_F & flag_z)
    {
        Call(target);
    }
}

void CPU::Op_CALL_NC()
{
    u16 target = ReadNextWord();
    if (!(REG_F & flag_c))
    {
        Call(target);
    }
}

void CPU::Op_CALL_C()
{
    u16 target = ReadNextWord();
    if (REG_F & flag_c)
    {
        Call(target);
    }
}

void CPU::Op_CCF()
{
    REG_F ^= flag_c;
    REG_F &= flag_z | flag_c;
}

void CPU::Op_CP_A(u8 val)
{
    REG_F = flag_n;
    if (REG_A == val)
    {
        REG_F |= flag_z;
    }
    if (val > REG_A)
    {
        REG_F |= flag_c;
    }
    if ((val & 0xF) > (REG_A & 0xF))
    {
        REG_F |= flag_h;
    }
}

void CPU::Op_CPL_A()
{
    REG_A = ~REG_A;
    REG_F |= flag_n | flag_h;
}

void CPU::Op_DAA()
{
    unsigned int a = REG_A;

    if (REG_F & flag_n)
    {
        if (REG_F & flag_h)
        {
            a = (a - 0x6) & 0xFF;
        }
        if (REG_F & flag_c)
        {
            a -= 0x60;
        }
    }
    else
    {
        if ((REG_F & flag_h) || (a & 0xF) > 0x9)
        {
            a += 0x6;
        }
        if ((REG_F & flag_c) || a > 0x9F)
        {
            a += 0x60;
        }
    }

    REG_F &= ~(flag_h | flag_z);

    if (a & 0x100)
    {
        REG_F |= flag_c;
    }

    REG_A = a;
    REG_F |= ZFlag(REG_A);
}

void CPU::Op_DEC_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    val--;
    WriteMem8(REG_HL, val);
    REG_F &= flag_c;
    REG_F |= ZFlag(val) | flag_n;
    if ((val & 0x0F) == 0x0F)
    {
        REG_F |= flag_h;
    }
}

void CPU::Op_DEC_Reg8(u8& reg)
{
    reg--;
    REG_F &= flag_c;
    REG_F |= ZFlag(reg) | flag_n;
    if ((reg & 0x0F) == 0x0F)
    {
        REG_F |= flag_h;
    }
}

void CPU::Op_DEC_Reg16(u16& reg)
{
    reg--;
    AddCycles(1);
}

void CPU::Op_DI()
{
    DisableInterrupts();
}

void CPU::Op_EI()
{
    m_ime_state = IMEState::EnableRequested;
}

void CPU::Op_HALT()
{
    if (!m_ime && ((REG_IE & REG_IF) & intr_all))
    {
        m_halt_state = HaltState::Bug;
    }
    else
    {
        m_halt_state = HaltState::On;
    }
}

void CPU::Op_INC_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    val++;
    WriteMem8(REG_HL, val);
    REG_F &= flag_c;
    REG_F |= ZFlag(val);
    if ((val & 0x0F) == 0)
    {
        REG_F |= flag_h;
    }
}

void CPU::Op_INC_Reg8(u8& reg)
{
    reg++;
    REG_F &= flag_c;
    REG_F |= ZFlag(reg);
    if ((reg & 0x0F) == 0)
    {
        REG_F |= flag_h;
    }
}

void CPU::Op_INC_Reg16(u16& reg)
{
    reg++;
    AddCycles(1);
}

void CPU::Op_JP()
{
    Jump(ReadNextWord());
}

void CPU::Op_JP_NZ()
{
    u16 target = ReadNextWord();
    if (!(REG_F & flag_z))
    {
        Jump(target);
    }
}

void CPU::Op_JP_Z()
{
    u16 target = ReadNextWord();
    if (REG_F & flag_z)
    {
        Jump(target);
    }
}

void CPU::Op_JP_NC()
{
    u16 target = ReadNextWord();
    if (!(REG_F & flag_c))
    {
        Jump(target);
    }
}

void CPU::Op_JP_C()
{
    u16 target = ReadNextWord();
    if (REG_F & flag_c)
    {
        Jump(target);
    }
}

void CPU::Op_JR()
{
    Jump(CalcRelativeJumpTarget());
}

void CPU::Op_JR_NZ()
{
    u16 target = CalcRelativeJumpTarget();
    if (!(REG_F & flag_z))
    {
        Jump(target);
    }
}

void CPU::Op_JR_Z()
{
    u16 target = CalcRelativeJumpTarget();
    if (REG_F & flag_z)
    {
        Jump(target);
    }
}

void CPU::Op_JR_NC()
{
    u16 target = CalcRelativeJumpTarget();
    if (!(REG_F & flag_c))
    {
        Jump(target);
    }
}

void CPU::Op_JR_C()
{
    u16 target = CalcRelativeJumpTarget();
    if (REG_F & flag_c)
    {
        Jump(target);
    }
}

void CPU::Op_LD_HL_SPOffset()
{
    u8 val = ReadNextByte();
    unsigned int temp = (u8)REG_SP + val;
    REG_F = (temp > 0xFF) ? flag_c : 0;
    if (((REG_SP ^ val) ^ temp) & 0x10)
    {
        REG_F |= flag_h;
    }
    REG_HL = REG_SP + (s8)val;
    AddCycles(1);
}

void CPU::Op_OR_A(u8 val)
{
    REG_A |= val;
    REG_F = ZFlag(REG_A);
}

void CPU::Op_POP(u16& reg)
{
    reg = Pop();
}

void CPU::Op_POP_AF()
{
    REG_AF = Pop() & 0xFFF0; // the lower nibble of F is always 0
}

void CPU::Op_PUSH(u16 reg)
{
    Push(reg);
    AddCycles(1);
}

void CPU::Op_RES_PtrHL(int bit)
{
    u8 val = ReadMem8(REG_HL);
    WriteMem8(REG_HL, val & ~Bit(bit));
}

void CPU::Op_RES_Reg8(int bit, u8& reg)
{
    reg &= ~Bit(bit);
}

void CPU::Op_RET()
{
    Return();
}

void CPU::Op_RET_NZ()
{
    AddCycles(1);
    if (!(REG_F & flag_z))
    {
        Return();
    }
}

void CPU::Op_RET_Z()
{
    AddCycles(1);
    if (REG_F & flag_z)
    {
        Return();
    }
}

void CPU::Op_RET_NC()
{
    AddCycles(1);
    if (!(REG_F & flag_c))
    {
        Return();
    }
}

void CPU::Op_RET_C()
{
    AddCycles(1);
    if (REG_F & flag_c)
    {
        Return();
    }
}

void CPU::Op_RETI()
{
    m_ime = true;
    m_ime_state = IMEState::Stable;
    Return();
}

void CPU::Op_RL_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    u8 carry = (val & 0x80) >> (7 - flag_c_shift);
    val = (val << 1) | ((REG_F & flag_c) >> flag_c_shift);
    WriteMem8(REG_HL, val);
    REG_F = ZFlag(val) | carry;
}

void CPU::Op_RL_Reg8(u8& reg)
{
    u8 carry = (reg & 0x80) >> (7 - flag_c_shift);
    reg = (reg << 1) | ((REG_F & flag_c) >> flag_c_shift);
    REG_F = ZFlag(reg) | carry;
}

void CPU::Op_RLA()
{
    u8 carry = (REG_A & 0x80) >> (7 - flag_c_shift);
    REG_A = (REG_A << 1) | ((REG_F & flag_c) >> flag_c_shift);
    REG_F = carry;
}

void CPU::Op_RLC_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = (val & 0x80) >> (7 - flag_c_shift);
    val = (val << 1) | (val >> 7);
    WriteMem8(REG_HL, val);
    REG_F |= ZFlag(val);
}

void CPU::Op_RLC_Reg8(u8& reg)
{
    REG_F = (reg & 0x80) >> (7 - flag_c_shift);
    reg = (reg << 1) | (reg >> 7);
    REG_F |= ZFlag(reg);
}

void CPU::Op_RLCA()
{
    REG_F = (REG_A & 0x80) >> (7 - flag_c_shift);
    REG_A = (REG_A << 1) | (REG_A >> 7);
}

void CPU::Op_RR_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    u8 carry = (val & 0x01) << flag_c_shift;
    val = (val >> 1) | ((REG_F & flag_c) << (7 - flag_c_shift));
    WriteMem8(REG_HL, val);
    REG_F = ZFlag(val) | carry;
}

void CPU::Op_RR_Reg8(u8& reg)
{
    u8 carry = (reg & 0x01) << flag_c_shift;
    reg = (reg >> 1) | ((REG_F & flag_c) << (7 - flag_c_shift));
    REG_F = ZFlag(reg) | carry;
}

void CPU::Op_RRA()
{
    u8 carry = (REG_A & 0x01) << flag_c_shift;
    REG_A = (REG_A >> 1) | ((REG_F & flag_c) << (7 - flag_c_shift));
    REG_F = carry;
}

void CPU::Op_RRC_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = (val & 0x01) << flag_c_shift;
    val = (val >> 1) | (val << 7);
    WriteMem8(REG_HL, val);
    REG_F |= ZFlag(val);
}

void CPU::Op_RRC_Reg8(u8& reg)
{
    REG_F = (reg & 0x01) << flag_c_shift;
    reg = (reg >> 1) | (reg << 7);
    REG_F |= ZFlag(reg);
}

void CPU::Op_RRCA()
{
    REG_F = (REG_A & 0x01) << flag_c_shift;
    REG_A = (REG_A >> 1) | (REG_A << 7);
}

void CPU::Op_RST(u16 addr)
{
    Call(addr);
}

void CPU::Op_SBC_A(u8 val)
{
    int carry = (REG_F & flag_c) >> flag_c_shift;
    u8 temp = REG_A - val - carry;
    REG_F = ZFlag(temp) | flag_n;
    if (val + carry > REG_A)
    {
        REG_F |= flag_c;
    }
    if ((val & 0xF) + carry > (REG_A & 0xF))
    {
        REG_F |= flag_h;
    }
    REG_A = temp;
}

void CPU::Op_SCF()
{
    REG_F &= flag_z;
    REG_F |= flag_c;
}

void CPU::Op_SET_PtrHL(int bit)
{
    u8 val = ReadMem8(REG_HL);
    WriteMem8(REG_HL, val | Bit(bit));
}

void CPU::Op_SET_Reg8(int bit, u8& reg)
{
    reg |= Bit(bit);
}

void CPU::Op_SLA_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = (val & 0x80) >> (7 - flag_c_shift);
    val <<= 1;
    WriteMem8(REG_HL, val);
    REG_F |= ZFlag(val);
}

void CPU::Op_SLA_Reg8(u8& reg)
{
    REG_F = (reg & 0x80) >> (7 - flag_c_shift);
    reg <<= 1;
    REG_F |= ZFlag(reg);
}

void CPU::Op_SRA_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = (val & 0x01) << flag_c_shift;
    val = (val & 0x80) | (val >> 1);
    WriteMem8(REG_HL, val);
    REG_F |= ZFlag(val);
}

void CPU::Op_SRA_Reg8(u8& reg)
{
    REG_F = (reg & 0x01) << flag_c_shift;
    reg = (reg & 0x80) | (reg >> 1);
    REG_F |= ZFlag(reg);
}

void CPU::Op_SRL_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = (val & 0x01) << flag_c_shift;
    val >>= 1;
    WriteMem8(REG_HL, val);
    REG_F |= ZFlag(val);
}

void CPU::Op_SRL_Reg8(u8& reg)
{
    REG_F = (reg & 0x01) << flag_c_shift;
    reg >>= 1;
    REG_F |= ZFlag(reg);
}

void CPU::Op_STOP()
{
    // TODO: implement STOP
}

void CPU::Op_SUB_A(u8 val)
{
    u8 temp = REG_A - val;
    REG_F = ZFlag(temp) | flag_n;
    if (val > REG_A)
    {
        REG_F |= flag_c;
    }
    if ((val & 0xF) > (REG_A & 0xF))
    {
        REG_F |= flag_h;
    }
    REG_A = temp;
}

void CPU::Op_SWAP_PtrHL()
{
    u8 val = ReadMem8(REG_HL);
    REG_F = ZFlag(val);
    WriteMem8(REG_HL, SwapNybbles(val));
}

void CPU::Op_SWAP_Reg8(u8& reg)
{
    REG_F = ZFlag(reg);
    reg = SwapNybbles(reg);
}

void CPU::Op_XOR_A(u8 val)
{
    REG_A ^= val;
    REG_F = ZFlag(REG_A);
}

void CPU::CallInterruptHandler(u16 interrupt_vector)
{
    DisableInterrupts();
    AddCycles(3);
    Push(REG_PC);
    REG_PC = interrupt_vector;
}

void CPU::HandleInterrupts()
{
    u16 ready_interrupts = REG_IF & REG_IE;

    if (ready_interrupts != 0)
    {
        m_halt_state = HaltState::Off;

        if (m_ime)
        {
            if (ready_interrupts & intr_vblank)
            {
                ClearInterruptFlag(intr_vblank);
                CallInterruptHandler(0x40);
            }
            else if (ready_interrupts & intr_lcdc_status)
            {
                ClearInterruptFlag(intr_lcdc_status);
                CallInterruptHandler(0x48);
            }
            else if (ready_interrupts & intr_timer)
            {
                ClearInterruptFlag(intr_timer);
                CallInterruptHandler(0x50);
            }
            else if (ready_interrupts & intr_serial)
            {
                ClearInterruptFlag(intr_serial);
                CallInterruptHandler(0x58);
            }
            else if (ready_interrupts & intr_joypad)
            {
                ClearInterruptFlag(intr_joypad);
                CallInterruptHandler(0x60);
            }
        }
    }
}

void CPU::ExecInstructionPrefixCB()
{
    u8 opcode = ReadNextByte();

    switch (opcode)
    {
    case 0x00: // RLC B
        Op_RLC_Reg8(REG_B);
        break;
    case 0x01: // RLC C
        Op_RLC_Reg8(REG_C);
        break;
    case 0x02: // RLC D
        Op_RLC_Reg8(REG_D);
        break;
    case 0x03: // RLC E
        Op_RLC_Reg8(REG_E);
        break;
    case 0x04: // RLC H
        Op_RLC_Reg8(REG_H);
        break;
    case 0x05: // RLC L
        Op_RLC_Reg8(REG_L);
        break;
    case 0x06: // RLC (HL)
        Op_RLC_PtrHL();
        break;
    case 0x07: // RLC A
        Op_RLC_Reg8(REG_A);
        break;
    case 0x08: // RRC B
        Op_RRC_Reg8(REG_B);
        break;
    case 0x09: // RRC C
        Op_RRC_Reg8(REG_C);
        break;
    case 0x0A: // RRC D
        Op_RRC_Reg8(REG_D);
        break;
    case 0x0B: // RRC E
        Op_RRC_Reg8(REG_E);
        break;
    case 0x0C: // RRC H
        Op_RRC_Reg8(REG_H);
        break;
    case 0x0D: // RRC L
        Op_RRC_Reg8(REG_L);
        break;
    case 0x0E: // RRC (HL)
        Op_RRC_PtrHL();
        break;
    case 0x0F: // RRC A
        Op_RRC_Reg8(REG_A);
        break;
    case 0x10: // RL B
        Op_RL_Reg8(REG_B);
        break;
    case 0x11: // RL C
        Op_RL_Reg8(REG_C);
        break;
    case 0x12: // RL D
        Op_RL_Reg8(REG_D);
        break;
    case 0x13: // RL E
        Op_RL_Reg8(REG_E);
        break;
    case 0x14: // RL H
        Op_RL_Reg8(REG_H);
        break;
    case 0x15: // RL L
        Op_RL_Reg8(REG_L);
        break;
    case 0x16: // RL (HL)
        Op_RL_PtrHL();
        break;
    case 0x17: // RL A
        Op_RL_Reg8(REG_A);
        break;
    case 0x18: // RR B
        Op_RR_Reg8(REG_B);
        break;
    case 0x19: // RR C
        Op_RR_Reg8(REG_C);
        break;
    case 0x1A: // RR D
        Op_RR_Reg8(REG_D);
        break;
    case 0x1B: // RR E
        Op_RR_Reg8(REG_E);
        break;
    case 0x1C: // RR H
        Op_RR_Reg8(REG_H);
        break;
    case 0x1D: // RR L
        Op_RR_Reg8(REG_L);
        break;
    case 0x1E: // RR (HL)
        Op_RR_PtrHL();
        break;
    case 0x1F: // RR A
        Op_RR_Reg8(REG_A);
        break;
    case 0x20: // SLA B
        Op_SLA_Reg8(REG_B);
        break;
    case 0x21: // SLA C
        Op_SLA_Reg8(REG_C);
        break;
    case 0x22: // SLA D
        Op_SLA_Reg8(REG_D);
        break;
    case 0x23: // SLA E
        Op_SLA_Reg8(REG_E);
        break;
    case 0x24: // SLA H
        Op_SLA_Reg8(REG_H);
        break;
    case 0x25: // SLA L
        Op_SLA_Reg8(REG_L);
        break;
    case 0x26: // SLA (HL)
        Op_SLA_PtrHL();
        break;
    case 0x27: // SLA A
        Op_SLA_Reg8(REG_A);
        break;
    case 0x28: // SRA B
        Op_SRA_Reg8(REG_B);
        break;
    case 0x29: // SRA C
        Op_SRA_Reg8(REG_C);
        break;
    case 0x2A: // SRA D
        Op_SRA_Reg8(REG_D);
        break;
    case 0x2B: // SRA E
        Op_SRA_Reg8(REG_E);
        break;
    case 0x2C: // SRA H
        Op_SRA_Reg8(REG_H);
        break;
    case 0x2D: // SRA L
        Op_SRA_Reg8(REG_L);
        break;
    case 0x2E: // SRA (HL)
        Op_SRA_PtrHL();
        break;
    case 0x2F: // SRA A
        Op_SRA_Reg8(REG_A);
        break;
    case 0x30: // SWAP B
        Op_SWAP_Reg8(REG_B);
        break;
    case 0x31: // SWAP C
        Op_SWAP_Reg8(REG_C);
        break;
    case 0x32: // SWAP D
        Op_SWAP_Reg8(REG_D);
        break;
    case 0x33: // SWAP E
        Op_SWAP_Reg8(REG_E);
        break;
    case 0x34: // SWAP H
        Op_SWAP_Reg8(REG_H);
        break;
    case 0x35: // SWAP L
        Op_SWAP_Reg8(REG_L);
        break;
    case 0x36: // SWAP (HL)
        Op_SWAP_PtrHL();
        break;
    case 0x37: // SWAP A
        Op_SWAP_Reg8(REG_A);
        break;
    case 0x38: // SRL B
        Op_SRL_Reg8(REG_B);
        break;
    case 0x39: // SRL C
        Op_SRL_Reg8(REG_C);
        break;
    case 0x3A: // SRL D
        Op_SRL_Reg8(REG_D);
        break;
    case 0x3B: // SRL E
        Op_SRL_Reg8(REG_E);
        break;
    case 0x3C: // SRL H
        Op_SRL_Reg8(REG_H);
        break;
    case 0x3D: // SRL L
        Op_SRL_Reg8(REG_L);
        break;
    case 0x3E: // SRL (HL)
        Op_SRL_PtrHL();
        break;
    case 0x3F: // SRL A
        Op_SRL_Reg8(REG_A);
        break;
    case 0x40: // BIT 0,B
        Op_BIT_Reg8(0, REG_B);
        break;
    case 0x41: // BIT 0,C
        Op_BIT_Reg8(0, REG_C);
        break;
    case 0x42: // BIT 0,D
        Op_BIT_Reg8(0, REG_D);
        break;
    case 0x43: // BIT 0,E
        Op_BIT_Reg8(0, REG_E);
        break;
    case 0x44: // BIT 0,H
        Op_BIT_Reg8(0, REG_H);
        break;
    case 0x45: // BIT 0,L
        Op_BIT_Reg8(0, REG_L);
        break;
    case 0x46: // BIT 0,(HL)
        Op_BIT_PtrHL(0);
        break;
    case 0x47: // BIT 0,A
        Op_BIT_Reg8(0, REG_A);
        break;
    case 0x48: // BIT 1,B
        Op_BIT_Reg8(1, REG_B);
        break;
    case 0x49: // BIT 1,C
        Op_BIT_Reg8(1, REG_C);
        break;
    case 0x4A: // BIT 1,D
        Op_BIT_Reg8(1, REG_D);
        break;
    case 0x4B: // BIT 1,E
        Op_BIT_Reg8(1, REG_E);
        break;
    case 0x4C: // BIT 1,H
        Op_BIT_Reg8(1, REG_H);
        break;
    case 0x4D: // BIT 1,L
        Op_BIT_Reg8(1, REG_L);
        break;
    case 0x4E: // BIT 1,(HL)
        Op_BIT_PtrHL(1);
        break;
    case 0x4F: // BIT 1,A
        Op_BIT_Reg8(1, REG_A);
        break;
    case 0x50: // BIT 2,B
        Op_BIT_Reg8(2, REG_B);
        break;
    case 0x51: // BIT 2,C
        Op_BIT_Reg8(2, REG_C);
        break;
    case 0x52: // BIT 2,D
        Op_BIT_Reg8(2, REG_D);
        break;
    case 0x53: // BIT 2,E
        Op_BIT_Reg8(2, REG_E);
        break;
    case 0x54: // BIT 2,H
        Op_BIT_Reg8(2, REG_H);
        break;
    case 0x55: // BIT 2,L
        Op_BIT_Reg8(2, REG_L);
        break;
    case 0x56: // BIT 2,(HL)
        Op_BIT_PtrHL(2);
        break;
    case 0x57: // BIT 2,A
        Op_BIT_Reg8(2, REG_A);
        break;
    case 0x58: // BIT 3,B
        Op_BIT_Reg8(3, REG_B);
        break;
    case 0x59: // BIT 3,C
        Op_BIT_Reg8(3, REG_C);
        break;
    case 0x5A: // BIT 3,D
        Op_BIT_Reg8(3, REG_D);
        break;
    case 0x5B: // BIT 3,E
        Op_BIT_Reg8(3, REG_E);
        break;
    case 0x5C: // BIT 3,H
        Op_BIT_Reg8(3, REG_H);
        break;
    case 0x5D: // BIT 3,L
        Op_BIT_Reg8(3, REG_L);
        break;
    case 0x5E: // BIT 3,(HL)
        Op_BIT_PtrHL(3);
        break;
    case 0x5F: // BIT 3,A
        Op_BIT_Reg8(3, REG_A);
        break;
    case 0x60: // BIT 4,B
        Op_BIT_Reg8(4, REG_B);
        break;
    case 0x61: // BIT 4,C
        Op_BIT_Reg8(4, REG_C);
        break;
    case 0x62: // BIT 4,D
        Op_BIT_Reg8(4, REG_D);
        break;
    case 0x63: // BIT 4,E
        Op_BIT_Reg8(4, REG_E);
        break;
    case 0x64: // BIT 4,H
        Op_BIT_Reg8(4, REG_H);
        break;
    case 0x65: // BIT 4,L
        Op_BIT_Reg8(4, REG_L);
        break;
    case 0x66: // BIT 4,(HL)
        Op_BIT_PtrHL(4);
        break;
    case 0x67: // BIT 4,A
        Op_BIT_Reg8(4, REG_A);
        break;
    case 0x68: // BIT 5,B
        Op_BIT_Reg8(5, REG_B);
        break;
    case 0x69: // BIT 5,C
        Op_BIT_Reg8(5, REG_C);
        break;
    case 0x6A: // BIT 5,D
        Op_BIT_Reg8(5, REG_D);
        break;
    case 0x6B: // BIT 5,E
        Op_BIT_Reg8(5, REG_E);
        break;
    case 0x6C: // BIT 5,H
        Op_BIT_Reg8(5, REG_H);
        break;
    case 0x6D: // BIT 5,L
        Op_BIT_Reg8(5, REG_L);
        break;
    case 0x6E: // BIT 5,(HL)
        Op_BIT_PtrHL(5);
        break;
    case 0x6F: // BIT 5,A
        Op_BIT_Reg8(5, REG_A);
        break;
    case 0x70: // BIT 6,B
        Op_BIT_Reg8(6, REG_B);
        break;
    case 0x71: // BIT 6,C
        Op_BIT_Reg8(6, REG_C);
        break;
    case 0x72: // BIT 6,D
        Op_BIT_Reg8(6, REG_D);
        break;
    case 0x73: // BIT 6,E
        Op_BIT_Reg8(6, REG_E);
        break;
    case 0x74: // BIT 6,H
        Op_BIT_Reg8(6, REG_H);
        break;
    case 0x75: // BIT 6,L
        Op_BIT_Reg8(6, REG_L);
        break;
    case 0x76: // BIT 6,(HL)
        Op_BIT_PtrHL(6);
        break;
    case 0x77: // BIT 6,A
        Op_BIT_Reg8(6, REG_A);
        break;
    case 0x78: // BIT 7,B
        Op_BIT_Reg8(7, REG_B);
        break;
    case 0x79: // BIT 7,C
        Op_BIT_Reg8(7, REG_C);
        break;
    case 0x7A: // BIT 7,D
        Op_BIT_Reg8(7, REG_D);
        break;
    case 0x7B: // BIT 7,E
        Op_BIT_Reg8(7, REG_E);
        break;
    case 0x7C: // BIT 7,H
        Op_BIT_Reg8(7, REG_H);
        break;
    case 0x7D: // BIT 7,L
        Op_BIT_Reg8(7, REG_L);
        break;
    case 0x7E: // BIT 7,(HL)
        Op_BIT_PtrHL(7);
        break;
    case 0x7F: // BIT 7,A
        Op_BIT_Reg8(7, REG_A);
        break;
    case 0x80: // RES 0,B
        Op_RES_Reg8(0, REG_B);
        break;
    case 0x81: // RES 0,C
        Op_RES_Reg8(0, REG_C);
        break;
    case 0x82: // RES 0,D
        Op_RES_Reg8(0, REG_D);
        break;
    case 0x83: // RES 0,E
        Op_RES_Reg8(0, REG_E);
        break;
    case 0x84: // RES 0,H
        Op_RES_Reg8(0, REG_H);
        break;
    case 0x85: // RES 0,L
        Op_RES_Reg8(0, REG_L);
        break;
    case 0x86: // RES 0,(HL)
        Op_RES_PtrHL(0);
        break;
    case 0x87: // RES 0,A
        Op_RES_Reg8(0, REG_A);
        break;
    case 0x88: // RES 1,B
        Op_RES_Reg8(1, REG_B);
        break;
    case 0x89: // RES 1,C
        Op_RES_Reg8(1, REG_C);
        break;
    case 0x8A: // RES 1,D
        Op_RES_Reg8(1, REG_D);
        break;
    case 0x8B: // RES 1,E
        Op_RES_Reg8(1, REG_E);
        break;
    case 0x8C: // RES 1,H
        Op_RES_Reg8(1, REG_H);
        break;
    case 0x8D: // RES 1,L
        Op_RES_Reg8(1, REG_L);
        break;
    case 0x8E: // RES 1,(HL)
        Op_RES_PtrHL(1);
        break;
    case 0x8F: // RES 1,A
        Op_RES_Reg8(1, REG_A);
        break;
    case 0x90: // RES 2,B
        Op_RES_Reg8(2, REG_B);
        break;
    case 0x91: // RES 2,C
        Op_RES_Reg8(2, REG_C);
        break;
    case 0x92: // RES 2,D
        Op_RES_Reg8(2, REG_D);
        break;
    case 0x93: // RES 2,E
        Op_RES_Reg8(2, REG_E);
        break;
    case 0x94: // RES 2,H
        Op_RES_Reg8(2, REG_H);
        break;
    case 0x95: // RES 2,L
        Op_RES_Reg8(2, REG_L);
        break;
    case 0x96: // RES 2,(HL)
        Op_RES_PtrHL(2);
        break;
    case 0x97: // RES 2,A
        Op_RES_Reg8(2, REG_A);
        break;
    case 0x98: // RES 3,B
        Op_RES_Reg8(3, REG_B);
        break;
    case 0x99: // RES 3,C
        Op_RES_Reg8(3, REG_C);
        break;
    case 0x9A: // RES 3,D
        Op_RES_Reg8(3, REG_D);
        break;
    case 0x9B: // RES 3,E
        Op_RES_Reg8(3, REG_E);
        break;
    case 0x9C: // RES 3,H
        Op_RES_Reg8(3, REG_H);
        break;
    case 0x9D: // RES 3,L
        Op_RES_Reg8(3, REG_L);
        break;
    case 0x9E: // RES 3,(HL)
        Op_RES_PtrHL(3);
        break;
    case 0x9F: // RES 3,A
        Op_RES_Reg8(3, REG_A);
        break;
    case 0xA0: // RES 4,B
        Op_RES_Reg8(4, REG_B);
        break;
    case 0xA1: // RES 4,C
        Op_RES_Reg8(4, REG_C);
        break;
    case 0xA2: // RES 4,D
        Op_RES_Reg8(4, REG_D);
        break;
    case 0xA3: // RES 4,E
        Op_RES_Reg8(4, REG_E);
        break;
    case 0xA4: // RES 4,H
        Op_RES_Reg8(4, REG_H);
        break;
    case 0xA5: // RES 4,L
        Op_RES_Reg8(4, REG_L);
        break;
    case 0xA6: // RES 4,(HL)
        Op_RES_PtrHL(4);
        break;
    case 0xA7: // RES 4,A
        Op_RES_Reg8(4, REG_A);
        break;
    case 0xA8: // RES 5,B
        Op_RES_Reg8(5, REG_B);
        break;
    case 0xA9: // RES 5,C
        Op_RES_Reg8(5, REG_C);
        break;
    case 0xAA: // RES 5,D
        Op_RES_Reg8(5, REG_D);
        break;
    case 0xAB: // RES 5,E
        Op_RES_Reg8(5, REG_E);
        break;
    case 0xAC: // RES 5,H
        Op_RES_Reg8(5, REG_H);
        break;
    case 0xAD: // RES 5,L
        Op_RES_Reg8(5, REG_L);
        break;
    case 0xAE: // RES 5,(HL)
        Op_RES_PtrHL(5);
        break;
    case 0xAF: // RES 5,A
        Op_RES_Reg8(5, REG_A);
        break;
    case 0xB0: // RES 6,B
        Op_RES_Reg8(6, REG_B);
        break;
    case 0xB1: // RES 6,C
        Op_RES_Reg8(6, REG_C);
        break;
    case 0xB2: // RES 6,D
        Op_RES_Reg8(6, REG_D);
        break;
    case 0xB3: // RES 6,E
        Op_RES_Reg8(6, REG_E);
        break;
    case 0xB4: // RES 6,H
        Op_RES_Reg8(6, REG_H);
        break;
    case 0xB5: // RES 6,L
        Op_RES_Reg8(6, REG_L);
        break;
    case 0xB6: // RES 6,(HL)
        Op_RES_PtrHL(6);
        break;
    case 0xB7: // RES 6,A
        Op_RES_Reg8(6, REG_A);
        break;
    case 0xB8: // RES 7,B
        Op_RES_Reg8(7, REG_B);
        break;
    case 0xB9: // RES 7,C
        Op_RES_Reg8(7, REG_C);
        break;
    case 0xBA: // RES 7,D
        Op_RES_Reg8(7, REG_D);
        break;
    case 0xBB: // RES 7,E
        Op_RES_Reg8(7, REG_E);
        break;
    case 0xBC: // RES 7,H
        Op_RES_Reg8(7, REG_H);
        break;
    case 0xBD: // RES 7,L
        Op_RES_Reg8(7, REG_L);
        break;
    case 0xBE: // RES 7,(HL)
        Op_RES_PtrHL(7);
        break;
    case 0xBF: // RES 7,A
        Op_RES_Reg8(7, REG_A);
        break;
    case 0xC0: // SET 0,B
        Op_SET_Reg8(0, REG_B);
        break;
    case 0xC1: // SET 0,C
        Op_SET_Reg8(0, REG_C);
        break;
    case 0xC2: // SET 0,D
        Op_SET_Reg8(0, REG_D);
        break;
    case 0xC3: // SET 0,E
        Op_SET_Reg8(0, REG_E);
        break;
    case 0xC4: // SET 0,H
        Op_SET_Reg8(0, REG_H);
        break;
    case 0xC5: // SET 0,L
        Op_SET_Reg8(0, REG_L);
        break;
    case 0xC6: // SET 0,(HL)
        Op_SET_PtrHL(0);
        break;
    case 0xC7: // SET 0,A
        Op_SET_Reg8(0, REG_A);
        break;
    case 0xC8: // SET 1,B
        Op_SET_Reg8(1, REG_B);
        break;
    case 0xC9: // SET 1,C
        Op_SET_Reg8(1, REG_C);
        break;
    case 0xCA: // SET 1,D
        Op_SET_Reg8(1, REG_D);
        break;
    case 0xCB: // SET 1,E
        Op_SET_Reg8(1, REG_E);
        break;
    case 0xCC: // SET 1,H
        Op_SET_Reg8(1, REG_H);
        break;
    case 0xCD: // SET 1,L
        Op_SET_Reg8(1, REG_L);
        break;
    case 0xCE: // SET 1,(HL)
        Op_SET_PtrHL(1);
        break;
    case 0xCF: // SET 1,A
        Op_SET_Reg8(1, REG_A);
        break;
    case 0xD0: // SET 2,B
        Op_SET_Reg8(2, REG_B);
        break;
    case 0xD1: // SET 2,C
        Op_SET_Reg8(2, REG_C);
        break;
    case 0xD2: // SET 2,D
        Op_SET_Reg8(2, REG_D);
        break;
    case 0xD3: // SET 2,E
        Op_SET_Reg8(2, REG_E);
        break;
    case 0xD4: // SET 2,H
        Op_SET_Reg8(2, REG_H);
        break;
    case 0xD5: // SET 2,L
        Op_SET_Reg8(2, REG_L);
        break;
    case 0xD6: // SET 2,(HL)
        Op_SET_PtrHL(2);
        break;
    case 0xD7: // SET 2,A
        Op_SET_Reg8(2, REG_A);
        break;
    case 0xD8: // SET 3,B
        Op_SET_Reg8(3, REG_B);
        break;
    case 0xD9: // SET 3,C
        Op_SET_Reg8(3, REG_C);
        break;
    case 0xDA: // SET 3,D
        Op_SET_Reg8(3, REG_D);
        break;
    case 0xDB: // SET 3,E
        Op_SET_Reg8(3, REG_E);
        break;
    case 0xDC: // SET 3,H
        Op_SET_Reg8(3, REG_H);
        break;
    case 0xDD: // SET 3,L
        Op_SET_Reg8(3, REG_L);
        break;
    case 0xDE: // SET 3,(HL)
        Op_SET_PtrHL(3);
        break;
    case 0xDF: // SET 3,A
        Op_SET_Reg8(3, REG_A);
        break;
    case 0xE0: // SET 4,B
        Op_SET_Reg8(4, REG_B);
        break;
    case 0xE1: // SET 4,C
        Op_SET_Reg8(4, REG_C);
        break;
    case 0xE2: // SET 4,D
        Op_SET_Reg8(4, REG_D);
        break;
    case 0xE3: // SET 4,E
        Op_SET_Reg8(4, REG_E);
        break;
    case 0xE4: // SET 4,H
        Op_SET_Reg8(4, REG_H);
        break;
    case 0xE5: // SET 4,L
        Op_SET_Reg8(4, REG_L);
        break;
    case 0xE6: // SET 4,(HL)
        Op_SET_PtrHL(4);
        break;
    case 0xE7: // SET 4,A
        Op_SET_Reg8(4, REG_A);
        break;
    case 0xE8: // SET 5,B
        Op_SET_Reg8(5, REG_B);
        break;
    case 0xE9: // SET 5,C
        Op_SET_Reg8(5, REG_C);
        break;
    case 0xEA: // SET 5,D
        Op_SET_Reg8(5, REG_D);
        break;
    case 0xEB: // SET 5,E
        Op_SET_Reg8(5, REG_E);
        break;
    case 0xEC: // SET 5,H
        Op_SET_Reg8(5, REG_H);
        break;
    case 0xED: // SET 5,L
        Op_SET_Reg8(5, REG_L);
        break;
    case 0xEE: // SET 5,(HL)
        Op_SET_PtrHL(5);
        break;
    case 0xEF: // SET 5,A
        Op_SET_Reg8(5, REG_A);
        break;
    case 0xF0: // SET 6,B
        Op_SET_Reg8(6, REG_B);
        break;
    case 0xF1: // SET 6,C
        Op_SET_Reg8(6, REG_C);
        break;
    case 0xF2: // SET 6,D
        Op_SET_Reg8(6, REG_D);
        break;
    case 0xF3: // SET 6,E
        Op_SET_Reg8(6, REG_E);
        break;
    case 0xF4: // SET 6,H
        Op_SET_Reg8(6, REG_H);
        break;
    case 0xF5: // SET 6,L
        Op_SET_Reg8(6, REG_L);
        break;
    case 0xF6: // SET 6,(HL)
        Op_SET_PtrHL(6);
        break;
    case 0xF7: // SET 6,A
        Op_SET_Reg8(6, REG_A);
        break;
    case 0xF8: // SET 7,B
        Op_SET_Reg8(7, REG_B);
        break;
    case 0xF9: // SET 7,C
        Op_SET_Reg8(7, REG_C);
        break;
    case 0xFA: // SET 7,D
        Op_SET_Reg8(7, REG_D);
        break;
    case 0xFB: // SET 7,E
        Op_SET_Reg8(7, REG_E);
        break;
    case 0xFC: // SET 7,H
        Op_SET_Reg8(7, REG_H);
        break;
    case 0xFD: // SET 7,L
        Op_SET_Reg8(7, REG_L);
        break;
    case 0xFE: // SET 7,(HL)
        Op_SET_PtrHL(7);
        break;
    case 0xFF: // SET 7,A
        Op_SET_Reg8(7, REG_A);
        break;
    }
}

bool CPU::ExecInstruction()
{
    u8 opcode = ReadNextByte();

    switch (opcode)
    {
    case 0x00: // NOP
        break;
    case 0x01: // LD BC,XX
        REG_BC = ReadNextWord();
        break;
    case 0x02: // LD (BC),A
        WriteMem8(REG_BC, REG_A);
        break;
    case 0x03: // INC BC
        Op_INC_Reg16(REG_BC);
        break;
    case 0x04: // INC B
        Op_INC_Reg8(REG_B);
        break;
    case 0x05: // DEC B
        Op_DEC_Reg8(REG_B);
        break;
    case 0x06: // LD B,X
        REG_B = ReadNextByte();
        break;
    case 0x07: // RLCA
        Op_RLCA();
        break;
    case 0x08: // LD (XX),SP
        WriteMem16(ReadNextWord(), REG_SP);
        break;
    case 0x09: // ADD HL,BC
        Op_ADD_HL(REG_BC);
        break;
    case 0x0A: // LD A,(BC)
        REG_A = ReadMem8(REG_BC);
        break;
    case 0x0B: // DEC BC
        Op_DEC_Reg16(REG_BC);
        break;
    case 0x0C: // INC C
        Op_INC_Reg8(REG_C);
        break;
    case 0x0D: // DEC C
        Op_DEC_Reg8(REG_C);
        break;
    case 0x0E: // LD C,X
        REG_C = ReadNextByte();
        break;
    case 0x0F: // RRCA
        Op_RRCA();
        break;
    case 0x10: // STOP
        Op_STOP();
        break;
    case 0x11: // LD DE,XX
        REG_DE = ReadNextWord();
        break;
    case 0x12: // LD (DE),A
        WriteMem8(REG_DE, REG_A);
        break;
    case 0x13: // INC DE
        Op_INC_Reg16(REG_DE);
        break;
    case 0x14: // INC D
        Op_INC_Reg8(REG_D);
        break;
    case 0x15: // DEC D
        Op_DEC_Reg8(REG_D);
        break;
    case 0x16: // LD D,X
        REG_D = ReadNextByte();
        break;
    case 0x17: // RLA
        Op_RLA();
        break;
    case 0x18: // JR X
        Op_JR();
        break;
    case 0x19: // ADD HL,DE
        Op_ADD_HL(REG_DE);
        break;
    case 0x1A: // LD A,(DE)
        REG_A = ReadMem8(REG_DE);
        break;
    case 0x1B: // DEC DE
        Op_DEC_Reg16(REG_DE);
        break;
    case 0x1C: // INC E
        Op_INC_Reg8(REG_E);
        break;
    case 0x1D: // DEC E
        Op_DEC_Reg8(REG_E);
        break;
    case 0x1E: // LD E,X
        REG_E = ReadNextByte();
        break;
    case 0x1F: // RRA
        Op_RRA();
        break;
    case 0x20: // JR NZ,X
        Op_JR_NZ();
        break;
    case 0x21: // LD HL,XX
        REG_HL = ReadNextWord();
        break;
    case 0x22: // LD (HL+),A
        WriteMem8(REG_HL++, REG_A);
        break;
    case 0x23: // INC HL
        Op_INC_Reg16(REG_HL);
        break;
    case 0x24: // INC H
        Op_INC_Reg8(REG_H);
        break;
    case 0x25: // DEC H
        Op_DEC_Reg8(REG_H);
        break;
    case 0x26: // LD H,X
        REG_H = ReadNextByte();
        break;
    case 0x27: // DAA
        Op_DAA();
        break;
    case 0x28: // JR Z,X
        Op_JR_Z();
        break;
    case 0x29: // ADD HL,HL
        Op_ADD_HL(REG_HL);
        break;
    case 0x2A: // LD A,(HL+)
        REG_A = ReadMem8(REG_HL++);
        break;
    case 0x2B: // DEC HL
        Op_DEC_Reg16(REG_HL);
        break;
    case 0x2C: // INC L
        Op_INC_Reg8(REG_L);
        break;
    case 0x2D: // DEC L
        Op_DEC_Reg8(REG_L);
        break;
    case 0x2E: // LD L,X
        REG_L = ReadNextByte();
        break;
    case 0x2F: // CPL
        Op_CPL_A();
        break;
    case 0x30: // JR NC,X
        Op_JR_NC();
        break;
    case 0x31: // LD SP,XX
        REG_SP = ReadNextWord();
        break;
    case 0x32: // LD (HL-),A
        WriteMem8(REG_HL--, REG_A);
        break;
    case 0x33: // INC SP
        Op_INC_Reg16(REG_SP);
        break;
    case 0x34: // INC (HL)
        Op_INC_PtrHL();
        break;
    case 0x35: // DEC (HL)
        Op_DEC_PtrHL();
        break;
    case 0x36: // LD (HL),X
        WriteMem8(REG_HL, ReadNextByte());
        break;
    case 0x37: // SCF
        Op_SCF();
        break;
    case 0x38: // JR C,X
        Op_JR_C();
        break;
    case 0x39: // ADD HL,SP
        Op_ADD_HL(REG_SP);
        break;
    case 0x3A: // LD A,(HL-)
        REG_A = ReadMem8(REG_HL--);
        break;
    case 0x3B: // DEC SP
        Op_DEC_Reg16(REG_SP);
        break;
    case 0x3C: // INC A
        Op_INC_Reg8(REG_A);
        break;
    case 0x3D: // DEC A
        Op_DEC_Reg8(REG_A);
        break;
    case 0x3E: // LD A,X
        REG_A = ReadNextByte();
        break;
    case 0x3F: // CCF
        Op_CCF();
        break;
    case 0x40: // LD B,B
        break;
    case 0x41: // LD B,C
        REG_B = REG_C;
        break;
    case 0x42: // LD B,D
        REG_B = REG_D;
        break;
    case 0x43: // LD B,E
        REG_B = REG_E;
        break;
    case 0x44: // LD B,H
        REG_B = REG_H;
        break;
    case 0x45: // LD B,L
        REG_B = REG_L;
        break;
    case 0x46: // LD B,(HL)
        REG_B = ReadMem8(REG_HL);
        break;
    case 0x47: // LD B,A
        REG_B = REG_A;
        break;
    case 0x48: // LD C,B
        REG_C = REG_B;
        break;
    case 0x49: // LD C,C
        break;
    case 0x4A: // LD C,D
        REG_C = REG_D;
        break;
    case 0x4B: // LD C,E
        REG_C = REG_E;
        break;
    case 0x4C: // LD C,H
        REG_C = REG_H;
        break;
    case 0x4D: // LD C,L
        REG_C = REG_L;
        break;
    case 0x4E: // LD C,(HL)
        REG_C = ReadMem8(REG_HL);
        break;
    case 0x4F: // LD C,A
        REG_C = REG_A;
        break;
    case 0x50: // LD D,B
        REG_D = REG_B;
        break;
    case 0x51: // LD D,C
        REG_D = REG_C;
        break;
    case 0x52: // LD D,D
        break;
    case 0x53: // LD D,E
        REG_D = REG_E;
        break;
    case 0x54: // LD D,H
        REG_D = REG_H;
        break;
    case 0x55: // LD D,L
        REG_D = REG_L;
        break;
    case 0x56: // LD D,(HL)
        REG_D = ReadMem8(REG_HL);
        break;
    case 0x57: // LD D,A
        REG_D = REG_A;
        break;
    case 0x58: // LD E,B
        REG_E = REG_B;
        break;
    case 0x59: // LD E,C
        REG_E = REG_C;
        break;
    case 0x5A: // LD E,D
        REG_E = REG_D;
        break;
    case 0x5B: // LD E,E
        break;
    case 0x5C: // LD E,H
        REG_E = REG_H;
        break;
    case 0x5D: // LD E,L
        REG_E = REG_L;
        break;
    case 0x5E: // LD E,(HL)
        REG_E = ReadMem8(REG_HL);
        break;
    case 0x5F: // LD E,A
        REG_E = REG_A;
        break;
    case 0x60: // LD H,B
        REG_H = REG_B;
        break;
    case 0x61: // LD H,C
        REG_H = REG_C;
        break;
    case 0x62: // LD H,D
        REG_H = REG_D;
        break;
    case 0x63: // LD H,E
        REG_H = REG_E;
        break;
    case 0x64: // LD H,H
        break;
    case 0x65: // LD H,L
        REG_H = REG_L;
        break;
    case 0x66: // LD H,(HL)
        REG_H = ReadMem8(REG_HL);
        break;
    case 0x67: // LD H,A
        REG_H = REG_A;
        break;
    case 0x68: // LD L,B
        REG_L = REG_B;
        break;
    case 0x69: // LD L,C
        REG_L = REG_C;
        break;
    case 0x6A: // LD L,D
        REG_L = REG_D;
        break;
    case 0x6B: // LD L,E
        REG_L = REG_E;
        break;
    case 0x6C: // LD L,H
        REG_L = REG_H;
        break;
    case 0x6D: // LD L,L
        break;
    case 0x6E: // LD L,(HL)
        REG_L = ReadMem8(REG_HL);
        break;
    case 0x6F: // LD L,A
        REG_L = REG_A;
        break;
    case 0x70: // LD (HL),B
        WriteMem8(REG_HL, REG_B);
        break;
    case 0x71: // LD (HL),C
        WriteMem8(REG_HL, REG_C);
        break;
    case 0x72: // LD (HL),D
        WriteMem8(REG_HL, REG_D);
        break;
    case 0x73: // LD (HL),E
        WriteMem8(REG_HL, REG_E);
        break;
    case 0x74: // LD (HL),H
        WriteMem8(REG_HL, REG_H);
        break;
    case 0x75: // LD (HL),L
        WriteMem8(REG_HL, REG_L);
        break;
    case 0x76: // LD (HL),(HL)
        Op_HALT();
        break;
    case 0x77: // LD (HL),A
        WriteMem8(REG_HL, REG_A);
        break;
    case 0x78: // LD A,B
        REG_A = REG_B;
        break;
    case 0x79: // LD A,C
        REG_A = REG_C;
        break;
    case 0x7A: // LD A,D
        REG_A = REG_D;
        break;
    case 0x7B: // LD A,E
        REG_A = REG_E;
        break;
    case 0x7C: // LD A,H
        REG_A = REG_H;
        break;
    case 0x7D: // LD A,L
        REG_A = REG_L;
        break;
    case 0x7E: // LD A,(HL)
        REG_A = ReadMem8(REG_HL);
        break;
    case 0x7F: // LD A,A
        break;
    case 0x80: // ADD A,B
        Op_ADD_A(REG_B);
        break;
    case 0x81: // ADD A,C
        Op_ADD_A(REG_C);
        break;
    case 0x82: // ADD A,D
        Op_ADD_A(REG_D);
        break;
    case 0x83: // ADD A,E
        Op_ADD_A(REG_E);
        break;
    case 0x84: // ADD A,H
        Op_ADD_A(REG_H);
        break;
    case 0x85: // ADD A,L
        Op_ADD_A(REG_L);
        break;
    case 0x86: // ADD A,(HL)
        Op_ADD_A(ReadMem8(REG_HL));
        break;
    case 0x87: // ADD A,A
        Op_ADD_A(REG_A);
        break;
    case 0x88: // ADC A,B
        Op_ADC_A(REG_B);
        break;
    case 0x89: // ADC A,C
        Op_ADC_A(REG_C);
        break;
    case 0x8A: // ADC A,D
        Op_ADC_A(REG_D);
        break;
    case 0x8B: // ADC A,E
        Op_ADC_A(REG_E);
        break;
    case 0x8C: // ADC A,H
        Op_ADC_A(REG_H);
        break;
    case 0x8D: // ADC A,L
        Op_ADC_A(REG_L);
        break;
    case 0x8E: // ADC A,(HL)
        Op_ADC_A(ReadMem8(REG_HL));
        break;
    case 0x8F: // ADC A,A
        Op_ADC_A(REG_A);
        break;
    case 0x90: // SUB B
        Op_SUB_A(REG_B);
        break;
    case 0x91: // SUB C
        Op_SUB_A(REG_C);
        break;
    case 0x92: // SUB D
        Op_SUB_A(REG_D);
        break;
    case 0x93: // SUB E
        Op_SUB_A(REG_E);
        break;
    case 0x94: // SUB H
        Op_SUB_A(REG_H);
        break;
    case 0x95: // SUB L
        Op_SUB_A(REG_L);
        break;
    case 0x96: // SUB (HL)
        Op_SUB_A(ReadMem8(REG_HL));
        break;
    case 0x97: // SUB A
        Op_SUB_A(REG_A);
        break;
    case 0x98: // SBC B
        Op_SBC_A(REG_B);
        break;
    case 0x99: // SBC C
        Op_SBC_A(REG_C);
        break;
    case 0x9A: // SBC D
        Op_SBC_A(REG_D);
        break;
    case 0x9B: // SBC E
        Op_SBC_A(REG_E);
        break;
    case 0x9C: // SBC H
        Op_SBC_A(REG_H);
        break;
    case 0x9D: // SBC L
        Op_SBC_A(REG_L);
        break;
    case 0x9E: // SBC (HL)
        Op_SBC_A(ReadMem8(REG_HL));
        break;
    case 0x9F: // SBC A
        Op_SBC_A(REG_A);
        break;
    case 0xA0: // AND B
        Op_AND_A(REG_B);
        break;
    case 0xA1: // AND C
        Op_AND_A(REG_C);
        break;
    case 0xA2: // AND D
        Op_AND_A(REG_D);
        break;
    case 0xA3: // AND E
        Op_AND_A(REG_E);
        break;
    case 0xA4: // AND H
        Op_AND_A(REG_H);
        break;
    case 0xA5: // AND L
        Op_AND_A(REG_L);
        break;
    case 0xA6: // AND (HL)
        Op_AND_A(ReadMem8(REG_HL));
        break;
    case 0xA7: // AND A
        Op_AND_A(REG_A);
        break;
    case 0xA8: // XOR B
        Op_XOR_A(REG_B);
        break;
    case 0xA9: // XOR C
        Op_XOR_A(REG_C);
        break;
    case 0xAA: // XOR D
        Op_XOR_A(REG_D);
        break;
    case 0xAB: // XOR E
        Op_XOR_A(REG_E);
        break;
    case 0xAC: // XOR H
        Op_XOR_A(REG_H);
        break;
    case 0xAD: // XOR L
        Op_XOR_A(REG_L);
        break;
    case 0xAE: // XOR (HL)
        Op_XOR_A(ReadMem8(REG_HL));
        break;
    case 0xAF: // XOR A
        Op_XOR_A(REG_A);
        break;
    case 0xB0: // OR B
        Op_OR_A(REG_B);
        break;
    case 0xB1: // OR C
        Op_OR_A(REG_C);
        break;
    case 0xB2: // OR D
        Op_OR_A(REG_D);
        break;
    case 0xB3: // OR E
        Op_OR_A(REG_E);
        break;
    case 0xB4: // OR H
        Op_OR_A(REG_H);
        break;
    case 0xB5: // OR L
        Op_OR_A(REG_L);
        break;
    case 0xB6: // OR (HL)
        Op_OR_A(ReadMem8(REG_HL));
        break;
    case 0xB7: // OR A
        Op_OR_A(REG_A);
        break;
    case 0xB8: // CP B
        Op_CP_A(REG_B);
        break;
    case 0xB9: // CP C
        Op_CP_A(REG_C);
        break;
    case 0xBA: // CP D
        Op_CP_A(REG_D);
        break;
    case 0xBB: // CP E
        Op_CP_A(REG_E);
        break;
    case 0xBC: // CP H
        Op_CP_A(REG_H);
        break;
    case 0xBD: // CP L
        Op_CP_A(REG_L);
        break;
    case 0xBE: // CP (HL)
        Op_CP_A(ReadMem8(REG_HL));
        break;
    case 0xBF: // CP A
        Op_CP_A(REG_A);
        break;
    case 0xC0: // RET NZ
        Op_RET_NZ();
        break;
    case 0xC1: // POP BC
        Op_POP(REG_BC);
        break;
    case 0xC2: // JP NZ,XX
        Op_JP_NZ();
        break;
    case 0xC3: // JP XX
        Op_JP();
        break;
    case 0xC4: // CALL NZ,XX
        Op_CALL_NZ();
        break;
    case 0xC5: // PUSH BC
        Op_PUSH(REG_BC);
        break;
    case 0xC6: // ADD A,X
        Op_ADD_A(ReadNextByte());
        break;
    case 0xC7: // RST 00H
        Op_RST(0x00);
        break;
    case 0xC8: // RET Z
        Op_RET_Z();
        break;
    case 0xC9: // RET
        Op_RET();
        break;
    case 0xCA: // JP Z,XX
        Op_JP_Z();
        break;
    case 0xCB: // PREFIX CB
        ExecInstructionPrefixCB();
        break;
    case 0xCC: // CALL Z,XX
        Op_CALL_Z();
        break;
    case 0xCD: // CALL XX
        Op_CALL();
        break;
    case 0xCE: // ADC A,X
        Op_ADC_A(ReadNextByte());
        break;
    case 0xCF: // RST 08H
        Op_RST(0x08);
        break;
    case 0xD0: // RET NC
        Op_RET_NC();
        break;
    case 0xD1: // POP DE
        Op_POP(REG_DE);
        break;
    case 0xD2: // JP NC,XX
        Op_JP_NC();
        break;
    case 0xD4: // CALL NC,XX
        Op_CALL_NC();
        break;
    case 0xD5: // PUSH DE
        Op_PUSH(REG_DE);
        break;
    case 0xD6: // SUB X
        Op_SUB_A(ReadNextByte());
        break;
    case 0xD7: // RST 10H
        Op_RST(0x10);
        break;
    case 0xD8: // RET C
        Op_RET_C();
        break;
    case 0xD9: // RETI
        Op_RETI();
        break;
    case 0xDA: // JP C,XX
        Op_JP_C();
        break;
    case 0xDC: // CALL C,XX
        Op_CALL_C();
        break;
    case 0xDE: // SBC X
        Op_SBC_A(ReadNextByte());
        break;
    case 0xDF: // RST 18H
        Op_RST(0x18);
        break;
    case 0xE0: // LDH (X),A
        WriteMem8(0xFF00 + ReadNextByte(), REG_A);
        break;
    case 0xE1: // POP HL
        Op_POP(REG_HL);
        break;
    case 0xE2: // LD (C),A
        WriteMem8(0xFF00 + REG_C, REG_A);
        break;
    case 0xE5: // PUSH HL
        Op_PUSH(REG_HL);
        break;
    case 0xE6: // AND X
        Op_AND_A(ReadNextByte());
        break;
    case 0xE7: // RST 20H
        Op_RST(0x20);
        break;
    case 0xE8: // ADD SP,X
        Op_ADD_SP_Imm();
        break;
    case 0xE9: // JP (HL)
        REG_PC = REG_HL;
        break;
    case 0xEA: // LD (XX),A
        WriteMem8(ReadNextWord(), REG_A);
        break;
    case 0xEE: // XOR X
        Op_XOR_A(ReadNextByte());
        break;
    case 0xEF: // RST 28H
        Op_RST(0x28);
        break;
    case 0xF0: // LDH A,(X)
        REG_A = ReadMem8(0xFF00 + ReadNextByte());
        break;
    case 0xF1: // POP AF
        Op_POP_AF();
        break;
    case 0xF2: // LD A,(C)
        REG_A = ReadMem8(0xFF00 + REG_C);
        break;
    case 0xF3: // DI
        Op_DI();
        break;
    case 0xF5: // PUSH AF
        Op_PUSH(REG_AF);
        break;
    case 0xF6: // OR X
        Op_OR_A(ReadNextByte());
        break;
    case 0xF7: // RST 30H
        Op_RST(0x30);
        break;
    case 0xF8: // LD HL,SP+X
        Op_LD_HL_SPOffset();
        break;
    case 0xF9: // LD SP,HL
        REG_SP = REG_HL;
        AddCycles(1);
        break;
    case 0xFA: // LD A,(XX)
        REG_A = ReadMem8(ReadNextWord());
        break;
    case 0xFB: // EI
        Op_EI();
        break;
    case 0xFE: // CP X
        Op_CP_A(ReadNextByte());
        break;
    case 0xFF: // RST 38H
        Op_RST(0x38);
        break;
    default:
        return false;
    }

    return true;
}
