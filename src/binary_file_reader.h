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

#include <stdio.h>
#include <string>
#include "common.h"

class BinaryFileReader
{
public:
    BinaryFileReader(const std::string& file_name);
    ~BinaryFileReader();

    BinaryFileReader(const BinaryFileReader&) = delete;
    BinaryFileReader& operator=(const BinaryFileReader&) = delete;

    bool IsOpen() const
    {
        return m_file != nullptr;
    }

    bool IsOK() const
    {
        return m_ok;
    }

    long GetSize() const
    {
        return m_size;
    }

    bool ReadBytes(void* dest, size_t count);

    void Close();

private:
    FILE* m_file;
    bool m_ok;
    long m_size;
};
