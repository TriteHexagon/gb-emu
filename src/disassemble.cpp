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

#include <assert.h>
#include <vector>
#include "common.h"
#include "disassemble.h"

static const char s_asm_table[0x100][15] =
{
    "nop",            // 0x00
    "ld bc,W",        // 0x01
    "ld [bc],a",      // 0x02
    "inc bc",         // 0x03
    "inc b",          // 0x04
    "dec b",          // 0x05
    "ld b,B",         // 0x06
    "rlca",           // 0x07
    "ld [W],sp",      // 0x08
    "add hl,bc",      // 0x09
    "ld a,[bc]",      // 0x0A
    "dec bc",         // 0x0B
    "inc c",          // 0x0C
    "dec c",          // 0x0D
    "ld c,B",         // 0x0E
    "rrca",           // 0x0F
    "stop",           // 0x10
    "ld de,W",        // 0x11
    "ld [de],a",      // 0x12
    "inc de",         // 0x13
    "inc d",          // 0x14
    "dec d",          // 0x15
    "ld d,B",         // 0x16
    "rla",            // 0x17
    "jr !",           // 0x18
    "add hl,de",      // 0x19
    "ld a,[de]",      // 0x1A
    "dec de",         // 0x1B
    "inc e",          // 0x1C
    "dec e",          // 0x1D
    "ld e,B",         // 0x1E
    "rra",            // 0x1F
    "jr nz,!",        // 0x20
    "ld hl,W",        // 0x21
    "ldi [hl],a",     // 0x22
    "inc hl",         // 0x23
    "inc h",          // 0x24
    "dec h",          // 0x25
    "ld h,B",         // 0x26
    "daa",            // 0x27
    "jr z,!",         // 0x28
    "add hl,hl",      // 0x29
    "ldi a,[hl]",     // 0x2A
    "dec hl",         // 0x2B
    "inc l",          // 0x2C
    "dec l",          // 0x2D
    "ld l,B",         // 0x2E
    "cpl a",          // 0x2F
    "jr nc,!",        // 0x30
    "ld sp,W",        // 0x31
    "ldd [hl],a",     // 0x32
    "inc sp",         // 0x33
    "inc [hl]",       // 0x34
    "dec [hl]",       // 0x35
    "ld [hl],B",      // 0x36
    "scf",            // 0x37
    "jr c,!",         // 0x38
    "add hl,sp",      // 0x39
    "ldd a,[hl]",     // 0x3A
    "dec sp",         // 0x3B
    "inc a",          // 0x3C
    "dec a",          // 0x3D
    "ld a,B",         // 0x3E
    "ccf",            // 0x3F
    "ld b,b",         // 0x40
    "ld b,c",         // 0x41
    "ld b,d",         // 0x42
    "ld b,e",         // 0x43
    "ld b,h",         // 0x44
    "ld b,l",         // 0x45
    "ld b,[hl]",      // 0x46
    "ld b,a",         // 0x47
    "ld c,b",         // 0x48
    "ld c,c",         // 0x49
    "ld c,d",         // 0x4A
    "ld c,e",         // 0x4B
    "ld c,h",         // 0x4C
    "ld c,l",         // 0x4D
    "ld c,[hl]",      // 0x4E
    "ld c,a",         // 0x4F
    "ld d,b",         // 0x50
    "ld d,c",         // 0x51
    "ld d,d",         // 0x52
    "ld d,e",         // 0x53
    "ld d,h",         // 0x54
    "ld d,l",         // 0x55
    "ld d,[hl]",      // 0x56
    "ld d,a",         // 0x57
    "ld e,b",         // 0x58
    "ld e,c",         // 0x59
    "ld e,d",         // 0x5A
    "ld e,e",         // 0x5B
    "ld e,h",         // 0x5C
    "ld e,l",         // 0x5D
    "ld e,[hl]",      // 0x5E
    "ld e,a",         // 0x5F
    "ld h,b",         // 0x60
    "ld h,c",         // 0x61
    "ld h,d",         // 0x62
    "ld h,e",         // 0x63
    "ld h,h",         // 0x64
    "ld h,l",         // 0x65
    "ld h,[hl]",      // 0x66
    "ld h,a",         // 0x67
    "ld l,b",         // 0x68
    "ld l,c",         // 0x69
    "ld l,d",         // 0x6A
    "ld l,e",         // 0x6B
    "ld l,h",         // 0x6C
    "ld l,l",         // 0x6D
    "ld l,[hl]",      // 0x6E
    "ld l,a",         // 0x6F
    "ld [hl],b",      // 0x70
    "ld [hl],c",      // 0x71
    "ld [hl],d",      // 0x72
    "ld [hl],e",      // 0x73
    "ld [hl],h",      // 0x74
    "ld [hl],l",      // 0x75
    "halt",           // 0x76
    "ld [hl],a",      // 0x77
    "ld a,b",         // 0x78
    "ld a,c",         // 0x79
    "ld a,d",         // 0x7A
    "ld a,e",         // 0x7B
    "ld a,h",         // 0x7C
    "ld a,l",         // 0x7D
    "ld a,[hl]",      // 0x7E
    "ld a,a",         // 0x7F
    "add a,b",        // 0x80
    "add a,c",        // 0x81
    "add a,d",        // 0x82
    "add a,e",        // 0x83
    "add a,h",        // 0x84
    "add a,l",        // 0x85
    "add a,[hl]",     // 0x86
    "add a,a",        // 0x87
    "adc a,b",        // 0x88
    "adc a,c",        // 0x89
    "adc a,d",        // 0x8A
    "adc a,e",        // 0x8B
    "adc a,h",        // 0x8C
    "adc a,l",        // 0x8D
    "adc a,[hl]",     // 0x8E
    "adc a,a",        // 0x8F
    "sub a,b",        // 0x90
    "sub a,c",        // 0x91
    "sub a,d",        // 0x92
    "sub a,e",        // 0x93
    "sub a,h",        // 0x94
    "sub a,l",        // 0x95
    "sub a,[hl]",     // 0x96
    "sub a,a",        // 0x97
    "sbc a,b",        // 0x98
    "sbc a,c",        // 0x99
    "sbc a,d",        // 0x9A
    "sbc a,e",        // 0x9B
    "sbc a,h",        // 0x9C
    "sbc a,l",        // 0x9D
    "sbc a,[hl]",     // 0x9E
    "sbc a,a",        // 0x9F
    "and a,b",        // 0xA0
    "and a,c",        // 0xA1
    "and a,d",        // 0xA2
    "and a,e",        // 0xA3
    "and a,h",        // 0xA4
    "and a,l",        // 0xA5
    "and a,[hl]",     // 0xA6
    "and a,a",        // 0xA7
    "xor a,b",        // 0xA8
    "xor a,c",        // 0xA9
    "xor a,d",        // 0xAA
    "xor a,e",        // 0xAB
    "xor a,h",        // 0xAC
    "xor a,l",        // 0xAD
    "xor a,[hl]",     // 0xAE
    "xor a,a",        // 0xAF
    "or a,b",         // 0xB0
    "or a,c",         // 0xB1
    "or a,d",         // 0xB2
    "or a,e",         // 0xB3
    "or a,h",         // 0xB4
    "or a,l",         // 0xB5
    "or a,[hl]",      // 0xB6
    "or a,a",         // 0xB7
    "cp a,b",         // 0xB8
    "cp a,c",         // 0xB9
    "cp a,d",         // 0xBA
    "cp a,e",         // 0xBB
    "cp a,h",         // 0xBC
    "cp a,l",         // 0xBD
    "cp a,[hl]",      // 0xBE
    "cp a,a",         // 0xBF
    "ret nz",         // 0xC0
    "pop bc",         // 0xC1
    "jp nz,W",        // 0xC2
    "jp W",           // 0xC3
    "call nz,W",      // 0xC4
    "push bc",        // 0xC5
    "add a,B",        // 0xC6
    "rst $00",        // 0xC7
    "ret z",          // 0xC8
    "ret",            // 0xC9
    "jp z,W",         // 0xCA
    "",               // 0xCB
    "call z,W",       // 0xCC
    "call W",         // 0xCD
    "adc a,B",        // 0xCE
    "rst $08",        // 0xCF
    "ret nc",         // 0xD0
    "pop de",         // 0xD1
    "jp nc,W",        // 0xD2
    "invalid",        // 0xD3
    "call nc,W",      // 0xD4
    "push de",        // 0xD5
    "sub a,B",        // 0xD6
    "rst $10",        // 0xD7
    "ret c",          // 0xD8
    "reti",           // 0xD9
    "jp c,W",         // 0xDA
    "invalid",        // 0xDB
    "call c,W",       // 0xDC
    "invalid",        // 0xDD
    "sbc a,B",        // 0xDE
    "rst $18",        // 0xDF
    "ld [$FF00+B],a", // 0xE0
    "pop hl",         // 0xE1
    "ld [$FF00+c],a", // 0xE2
    "invalid",        // 0xE3
    "invalid",        // 0xE4
    "push hl",        // 0xE5
    "and a,B",        // 0xE6
    "rst $20",        // 0xE7
    "add sp,#",       // 0xE8
    "jp hl",          // 0xE9
    "ld [W],a",       // 0xEA
    "invalid",        // 0xEB
    "invalid",        // 0xEC
    "invalid",        // 0xED
    "xor a,B",        // 0xEE
    "rst $28",        // 0xEF
    "ld a,[$FF00+B]", // 0xF0
    "pop af",         // 0xF1
    "ld a,[$FF00+c]", // 0xF2
    "di",             // 0xF3
    "invalid",        // 0xF4
    "push af",        // 0xF5
    "or a,B",         // 0xF6
    "rst $30",        // 0xF7
    "ld hl,sp*",      // 0xF8
    "ld sp,hl",       // 0xF9
    "ld a,[W]",       // 0xFA
    "ei",             // 0xFB
    "invalid",        // 0xFC
    "invalid",        // 0xFD
    "cp a,B",         // 0xFE
    "rst $38"         // 0xFF
};

static const char s_asm_table_cb[0x100][11] =
{
    "rlc b",      // 0x00
    "rlc c",      // 0x01
    "rlc d",      // 0x02
    "rlc e",      // 0x03
    "rlc h",      // 0x04
    "rlc l",      // 0x05
    "rlc [hl]",   // 0x06
    "rlc a",      // 0x07
    "rrc b",      // 0x08
    "rrc c",      // 0x09
    "rrc d",      // 0x0A
    "rrc e",      // 0x0B
    "rrc h",      // 0x0C
    "rrc l",      // 0x0D
    "rrc [hl]",   // 0x0E
    "rrc a",      // 0x0F
    "rl b",       // 0x10
    "rl c",       // 0x11
    "rl d",       // 0x12
    "rl e",       // 0x13
    "rl h",       // 0x14
    "rl l",       // 0x15
    "rl [hl]",    // 0x16
    "rl a",       // 0x17
    "rr b",       // 0x18
    "rr c",       // 0x19
    "rr d",       // 0x1A
    "rr e",       // 0x1B
    "rr h",       // 0x1C
    "rr l",       // 0x1D
    "rr [hl]",    // 0x1E
    "rr a",       // 0x1F
    "sla b",      // 0x20
    "sla c",      // 0x21
    "sla d",      // 0x22
    "sla e",      // 0x23
    "sla h",      // 0x24
    "sla l",      // 0x25
    "sla [hl]",   // 0x26
    "sla a",      // 0x27
    "sra b",      // 0x28
    "sra c",      // 0x29
    "sra d",      // 0x2A
    "sra e",      // 0x2B
    "sra h",      // 0x2C
    "sra l",      // 0x2D
    "sra [hl]",   // 0x2E
    "sra a",      // 0x2F
    "swap b",     // 0x30
    "swap c",     // 0x31
    "swap d",     // 0x32
    "swap e",     // 0x33
    "swap h",     // 0x34
    "swap l",     // 0x35
    "swap [hl]",  // 0x36
    "swap a",     // 0x37
    "srl b",      // 0x38
    "srl c",      // 0x39
    "srl d",      // 0x3A
    "srl e",      // 0x3B
    "srl h",      // 0x3C
    "srl l",      // 0x3D
    "srl [hl]",   // 0x3E
    "srl a",      // 0x3F
    "bit 0,b",    // 0x40
    "bit 0,c",    // 0x41
    "bit 0,d",    // 0x42
    "bit 0,e",    // 0x43
    "bit 0,h",    // 0x44
    "bit 0,l",    // 0x45
    "bit 0,[hl]", // 0x46
    "bit 0,a",    // 0x47
    "bit 1,b",    // 0x48
    "bit 1,c",    // 0x49
    "bit 1,d",    // 0x4A
    "bit 1,e",    // 0x4B
    "bit 1,h",    // 0x4C
    "bit 1,l",    // 0x4D
    "bit 1,[hl]", // 0x4E
    "bit 1,a",    // 0x4F
    "bit 2,b",    // 0x50
    "bit 2,c",    // 0x51
    "bit 2,d",    // 0x52
    "bit 2,e",    // 0x53
    "bit 2,h",    // 0x54
    "bit 2,l",    // 0x55
    "bit 2,[hl]", // 0x56
    "bit 2,a",    // 0x57
    "bit 3,b",    // 0x58
    "bit 3,c",    // 0x59
    "bit 3,d",    // 0x5A
    "bit 3,e",    // 0x5B
    "bit 3,h",    // 0x5C
    "bit 3,l",    // 0x5D
    "bit 3,[hl]", // 0x5E
    "bit 3,a",    // 0x5F
    "bit 4,b",    // 0x60
    "bit 4,c",    // 0x61
    "bit 4,d",    // 0x62
    "bit 4,e",    // 0x63
    "bit 4,h",    // 0x64
    "bit 4,l",    // 0x65
    "bit 4,[hl]", // 0x66
    "bit 4,a",    // 0x67
    "bit 5,b",    // 0x68
    "bit 5,c",    // 0x69
    "bit 5,d",    // 0x6A
    "bit 5,e",    // 0x6B
    "bit 5,h",    // 0x6C
    "bit 5,l",    // 0x6D
    "bit 5,[hl]", // 0x6E
    "bit 5,a",    // 0x6F
    "bit 6,b",    // 0x70
    "bit 6,c",    // 0x71
    "bit 6,d",    // 0x72
    "bit 6,e",    // 0x73
    "bit 6,h",    // 0x74
    "bit 6,l",    // 0x75
    "bit 6,[hl]", // 0x76
    "bit 6,a",    // 0x77
    "bit 7,b",    // 0x78
    "bit 7,c",    // 0x79
    "bit 7,d",    // 0x7A
    "bit 7,e",    // 0x7B
    "bit 7,h",    // 0x7C
    "bit 7,l",    // 0x7D
    "bit 7,[hl]", // 0x7E
    "bit 7,a",    // 0x7F
    "res 0,b",    // 0x80
    "res 0,c",    // 0x81
    "res 0,d",    // 0x82
    "res 0,e",    // 0x83
    "res 0,h",    // 0x84
    "res 0,l",    // 0x85
    "res 0,[hl]", // 0x86
    "res 0,a",    // 0x87
    "res 1,b",    // 0x88
    "res 1,c",    // 0x89
    "res 1,d",    // 0x8A
    "res 1,e",    // 0x8B
    "res 1,h",    // 0x8C
    "res 1,l",    // 0x8D
    "res 1,[hl]", // 0x8E
    "res 1,a",    // 0x8F
    "res 2,b",    // 0x90
    "res 2,c",    // 0x91
    "res 2,d",    // 0x92
    "res 2,e",    // 0x93
    "res 2,h",    // 0x94
    "res 2,l",    // 0x95
    "res 2,[hl]", // 0x96
    "res 2,a",    // 0x97
    "res 3,b",    // 0x98
    "res 3,c",    // 0x99
    "res 3,d",    // 0x9A
    "res 3,e",    // 0x9B
    "res 3,h",    // 0x9C
    "res 3,l",    // 0x9D
    "res 3,[hl]", // 0x9E
    "res 3,a",    // 0x9F
    "res 4,b",    // 0xA0
    "res 4,c",    // 0xA1
    "res 4,d",    // 0xA2
    "res 4,e",    // 0xA3
    "res 4,h",    // 0xA4
    "res 4,l",    // 0xA5
    "res 4,[hl]", // 0xA6
    "res 4,a",    // 0xA7
    "res 5,b",    // 0xA8
    "res 5,c",    // 0xA9
    "res 5,d",    // 0xAA
    "res 5,e",    // 0xAB
    "res 5,h",    // 0xAC
    "res 5,l",    // 0xAD
    "res 5,[hl]", // 0xAE
    "res 5,a",    // 0xAF
    "res 6,b",    // 0xB0
    "res 6,c",    // 0xB1
    "res 6,d",    // 0xB2
    "res 6,e",    // 0xB3
    "res 6,h",    // 0xB4
    "res 6,l",    // 0xB5
    "res 6,[hl]", // 0xB6
    "res 6,a",    // 0xB7
    "res 7,b",    // 0xB8
    "res 7,c",    // 0xB9
    "res 7,d",    // 0xBA
    "res 7,e",    // 0xBB
    "res 7,h",    // 0xBC
    "res 7,l",    // 0xBD
    "res 7,[hl]", // 0xBE
    "res 7,a",    // 0xBF
    "set 0,b",    // 0xC0
    "set 0,c",    // 0xC1
    "set 0,d",    // 0xC2
    "set 0,e",    // 0xC3
    "set 0,h",    // 0xC4
    "set 0,l",    // 0xC5
    "set 0,[hl]", // 0xC6
    "set 0,a",    // 0xC7
    "set 1,b",    // 0xC8
    "set 1,c",    // 0xC9
    "set 1,d",    // 0xCA
    "set 1,e",    // 0xCB
    "set 1,h",    // 0xCC
    "set 1,l",    // 0xCD
    "set 1,[hl]", // 0xCE
    "set 1,a",    // 0xCF
    "set 2,b",    // 0xD0
    "set 2,c",    // 0xD1
    "set 2,d",    // 0xD2
    "set 2,e",    // 0xD3
    "set 2,h",    // 0xD4
    "set 2,l",    // 0xD5
    "set 2,[hl]", // 0xD6
    "set 2,a",    // 0xD7
    "set 3,b",    // 0xD8
    "set 3,c",    // 0xD9
    "set 3,d",    // 0xDA
    "set 3,e",    // 0xDB
    "set 3,h",    // 0xDC
    "set 3,l",    // 0xDD
    "set 3,[hl]", // 0xDE
    "set 3,a",    // 0xDF
    "set 4,b",    // 0xE0
    "set 4,c",    // 0xE1
    "set 4,d",    // 0xE2
    "set 4,e",    // 0xE3
    "set 4,h",    // 0xE4
    "set 4,l",    // 0xE5
    "set 4,[hl]", // 0xE6
    "set 4,a",    // 0xE7
    "set 5,b",    // 0xE8
    "set 5,c",    // 0xE9
    "set 5,d",    // 0xEA
    "set 5,e",    // 0xEB
    "set 5,h",    // 0xEC
    "set 5,l",    // 0xED
    "set 5,[hl]", // 0xEE
    "set 5,a",    // 0xEF
    "set 6,b",    // 0xF0
    "set 6,c",    // 0xF1
    "set 6,d",    // 0xF2
    "set 6,e",    // 0xF3
    "set 6,h",    // 0xF4
    "set 6,l",    // 0xF5
    "set 6,[hl]", // 0xF6
    "set 6,a",    // 0xF7
    "set 7,b",    // 0xF8
    "set 7,c",    // 0xF9
    "set 7,d",    // 0xFA
    "set 7,e",    // 0xFB
    "set 7,h",    // 0xFC
    "set 7,l",    // 0xFD
    "set 7,[hl]", // 0xFE
    "set 7,a"     // 0xFF
};

static const int s_opcode_to_instruction_length[0x100] =
{
    1, // 0x00
    3, // 0x01
    1, // 0x02
    1, // 0x03
    1, // 0x04
    1, // 0x05
    2, // 0x06
    1, // 0x07
    3, // 0x08
    1, // 0x09
    1, // 0x0A
    1, // 0x0B
    1, // 0x0C
    1, // 0x0D
    2, // 0x0E
    1, // 0x0F
    1, // 0x10
    3, // 0x11
    1, // 0x12
    1, // 0x13
    1, // 0x14
    1, // 0x15
    2, // 0x16
    1, // 0x17
    2, // 0x18
    1, // 0x19
    1, // 0x1A
    1, // 0x1B
    1, // 0x1C
    1, // 0x1D
    2, // 0x1E
    1, // 0x1F
    2, // 0x20
    3, // 0x21
    1, // 0x22
    1, // 0x23
    1, // 0x24
    1, // 0x25
    2, // 0x26
    1, // 0x27
    2, // 0x28
    1, // 0x29
    1, // 0x2A
    1, // 0x2B
    1, // 0x2C
    1, // 0x2D
    2, // 0x2E
    1, // 0x2F
    2, // 0x30
    3, // 0x31
    1, // 0x32
    1, // 0x33
    1, // 0x34
    1, // 0x35
    2, // 0x36
    1, // 0x37
    2, // 0x38
    1, // 0x39
    1, // 0x3A
    1, // 0x3B
    1, // 0x3C
    1, // 0x3D
    2, // 0x3E
    1, // 0x3F
    1, // 0x40
    1, // 0x41
    1, // 0x42
    1, // 0x43
    1, // 0x44
    1, // 0x45
    1, // 0x46
    1, // 0x47
    1, // 0x48
    1, // 0x49
    1, // 0x4A
    1, // 0x4B
    1, // 0x4C
    1, // 0x4D
    1, // 0x4E
    1, // 0x4F
    1, // 0x50
    1, // 0x51
    1, // 0x52
    1, // 0x53
    1, // 0x54
    1, // 0x55
    1, // 0x56
    1, // 0x57
    1, // 0x58
    1, // 0x59
    1, // 0x5A
    1, // 0x5B
    1, // 0x5C
    1, // 0x5D
    1, // 0x5E
    1, // 0x5F
    1, // 0x60
    1, // 0x61
    1, // 0x62
    1, // 0x63
    1, // 0x64
    1, // 0x65
    1, // 0x66
    1, // 0x67
    1, // 0x68
    1, // 0x69
    1, // 0x6A
    1, // 0x6B
    1, // 0x6C
    1, // 0x6D
    1, // 0x6E
    1, // 0x6F
    1, // 0x70
    1, // 0x71
    1, // 0x72
    1, // 0x73
    1, // 0x74
    1, // 0x75
    1, // 0x76
    1, // 0x77
    1, // 0x78
    1, // 0x79
    1, // 0x7A
    1, // 0x7B
    1, // 0x7C
    1, // 0x7D
    1, // 0x7E
    1, // 0x7F
    1, // 0x80
    1, // 0x81
    1, // 0x82
    1, // 0x83
    1, // 0x84
    1, // 0x85
    1, // 0x86
    1, // 0x87
    1, // 0x88
    1, // 0x89
    1, // 0x8A
    1, // 0x8B
    1, // 0x8C
    1, // 0x8D
    1, // 0x8E
    1, // 0x8F
    1, // 0x90
    1, // 0x91
    1, // 0x92
    1, // 0x93
    1, // 0x94
    1, // 0x95
    1, // 0x96
    1, // 0x97
    1, // 0x98
    1, // 0x99
    1, // 0x9A
    1, // 0x9B
    1, // 0x9C
    1, // 0x9D
    1, // 0x9E
    1, // 0x9F
    1, // 0xA0
    1, // 0xA1
    1, // 0xA2
    1, // 0xA3
    1, // 0xA4
    1, // 0xA5
    1, // 0xA6
    1, // 0xA7
    1, // 0xA8
    1, // 0xA9
    1, // 0xAA
    1, // 0xAB
    1, // 0xAC
    1, // 0xAD
    1, // 0xAE
    1, // 0xAF
    1, // 0xB0
    1, // 0xB1
    1, // 0xB2
    1, // 0xB3
    1, // 0xB4
    1, // 0xB5
    1, // 0xB6
    1, // 0xB7
    1, // 0xB8
    1, // 0xB9
    1, // 0xBA
    1, // 0xBB
    1, // 0xBC
    1, // 0xBD
    1, // 0xBE
    1, // 0xBF
    1, // 0xC0
    1, // 0xC1
    3, // 0xC2
    3, // 0xC3
    3, // 0xC4
    1, // 0xC5
    2, // 0xC6
    1, // 0xC7
    1, // 0xC8
    1, // 0xC9
    3, // 0xCA
    2, // 0xCB
    3, // 0xCC
    3, // 0xCD
    2, // 0xCE
    1, // 0xCF
    1, // 0xD0
    1, // 0xD1
    3, // 0xD2
    0, // 0xD3
    3, // 0xD4
    1, // 0xD5
    2, // 0xD6
    1, // 0xD7
    1, // 0xD8
    1, // 0xD9
    3, // 0xDA
    0, // 0xDB
    3, // 0xDC
    0, // 0xDD
    2, // 0xDE
    1, // 0xDF
    2, // 0xE0
    1, // 0xE1
    1, // 0xE2
    0, // 0xE3
    0, // 0xE4
    1, // 0xE5
    2, // 0xE6
    1, // 0xE7
    2, // 0xE8
    1, // 0xE9
    3, // 0xEA
    0, // 0xEB
    0, // 0xEC
    0, // 0xED
    2, // 0xEE
    1, // 0xEF
    2, // 0xF0
    1, // 0xF1
    1, // 0xF2
    1, // 0xF3
    0, // 0xF4
    1, // 0xF5
    2, // 0xF6
    1, // 0xF7
    2, // 0xF8
    1, // 0xF9
    3, // 0xFA
    1, // 0xFB
    0, // 0xFC
    0, // 0xFD
    2, // 0xFE
    1, // 0xFF
};

int GetInstructionLengthByOpcode(u8 opcode)
{
    return s_opcode_to_instruction_length[opcode];
}

void AppendNum(std::vector<char>& buffer, s32 val, int num_digits, bool include_plus)
{
    static const char hex_chars[] = "0123456789ABCDEF";

    assert((u32)val != 0x80000000);

    u32 abs_val = (u32)std::abs(val);

    if (val >= 0)
    {
        if (include_plus)
        {
            buffer.push_back('+');
        }
    }
    else
    {
        buffer.push_back('-');
    }

    buffer.push_back('$');

    for (int i = num_digits - 1; i >= 0; i--)
    {
        int digit = (abs_val >> (i * 4)) & 0xF;
        buffer.push_back(hex_chars[digit]);
    }
}

std::string Disassemble(u16 instruction_addr, const std::array<u8, 3>& instruction)
{
    std::vector<char> buffer;
    u8 opcode = instruction[0];

    if (opcode == 0xCB)
    {
        return s_asm_table_cb[instruction[1]];
    }

    const char* format = s_asm_table[opcode];

    for (int i = 0; format[i]; i++)
    {
        switch (format[i])
        {
        case 'B': // byte (unsigned 8-bit immediate)
        {
            u8 n = instruction[1];
            AppendNum(buffer, n, 2, false);
            break;
        }
        case 'W': // word (unsigned 16-bit immediate)
        {
            u16 n = ((u16)instruction[2] << 8) | instruction[1];
            AppendNum(buffer, n, 4, false);
            break;
        }
        case  '!': // relative address
        {
            u16 n = instruction_addr + 2 + (s8)instruction[1];
            AppendNum(buffer, n, 4, false);
            break;
        }
        case '#': // signed 8-bit immediate
        {
            s8 n = (s8)instruction[1];
            AppendNum(buffer, n, 2, false);
            break;
        }
        case '*': // signed 8-bit immediate (prefixed with '+' if positive)
        {
            s8 n = (s8)instruction[1];
            AppendNum(buffer, n, 2, true);
            break;
        }
        default:
            buffer.push_back(format[i]);
        }
    }

    return std::string(buffer.data(), buffer.size());
}
