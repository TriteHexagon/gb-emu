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

#include <string>
#include <vector>
#include <memory>
#include "common.h"

enum class LoadROMStatus
{
    OK,
    ROMFileOpenFailed,
    ROMFileReadFailed,
    ROMTooSmall,
    ROMTooLarge,
    ROMSizeNotPowerOfTwo,
    ROMSizeMismatch,
    RAMInfoInconsistent,
    RAMFileReadFailed,
    RAMFileWrongSize,
    UnknownCartridgeType,
    UnknownRAMSize,
};

enum class SaveRAMStatus
{
    OK,
    FileOpenFailed,
    FileWriteFailed,
};

enum class MapperType
{
    PlainROM,
    MBC1,
    MBC3,
};

struct ROMInfo
{
    MapperType mapper_type;
    std::unique_ptr<std::vector<u8>> rom;
    std::unique_ptr<std::vector<u8>> ram;
    bool is_gbc_aware;
    bool is_sgb_aware;
    bool has_battery;
    bool has_rtc;
    u8 cart_type;
    u8 ram_size_index;
    std::string save_file_name;
};

LoadROMStatus LoadROM(const std::string& file_name, ROMInfo& info);
SaveRAMStatus SaveRAM(const std::string& file_name, const std::vector<u8>& ram);
