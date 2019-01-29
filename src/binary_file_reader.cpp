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

#include "binary_file_reader.h"

BinaryFileReader::BinaryFileReader(const std::string& file_name)
{
    m_file = fopen(file_name.c_str(), "rb");
    m_ok = false;
    m_size = -1;

    if (m_file == nullptr)
    {
        return;
    }

    if (fseek(m_file, 0, SEEK_END) != 0)
    {
        Close();
        return;
    }

    m_size = ftell(m_file);

    if (m_size == -1)
    {
        Close();
        return;
    }

    rewind(m_file);

    m_ok = true;
}

BinaryFileReader::~BinaryFileReader()
{
    Close();
}

bool BinaryFileReader::ReadBytes(void* dest, size_t count)
{
    if (m_file == nullptr)
    {
        return false;
    }

    if (fread(dest, 1, count, m_file) != count)
    {
        m_ok = false;
    }

    return m_ok;
}

void BinaryFileReader::Close()
{
    if (m_file != nullptr)
    {
        fclose(m_file);
        m_file = nullptr;
    }
}
