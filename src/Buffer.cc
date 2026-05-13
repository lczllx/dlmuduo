#include "../include/Buffer.hpp"
#include "../include/log_system/lcz_log.h"

Buffer::Buffer() : _read_idx(0), _write_idx(0), _buffer(BUFFER_DEFAULT_SIZE) {}

char *Buffer::Begin() { return &*_buffer.begin(); }

void Buffer::MoveReadoffset(uint64_t len)
{
    if (len == 0)
        return;
    assert(len <= ReadableBytes());
    _read_idx += len;
}

void Buffer::MoveWriteoffset(uint64_t len)
{
    if (len == 0)
        return;
    assert(len <= TailIdleSize());
    _write_idx += len;
}

void Buffer::EnsureWritableBytes(uint64_t len)
{
    if (TailIdleSize() >= len)
        return;
    if (HeadIdleSize() + TailIdleSize() >= len)
    {
        uint64_t rez = ReadableBytes();
        std::copy(GetReadPtr(), GetReadPtr() + rez, Begin());
        _read_idx = 0;
        _write_idx = rez;
    }
    else
    {
        LCZ_DEBUG("RESIZE %lu", (_write_idx + len));
        _buffer.resize(_write_idx + len);//空间不够就扩容
    }
}

void Buffer::Write(const void *data, uint64_t len)
{
    if (len == 0)
        return;
    EnsureWritableBytes(len);
    const char *tmp = (const char *)data;
    std::copy(tmp, tmp + len, GetWritePtr());
}
//
void Buffer::WriteAndpush(const void *data, uint64_t len)
{
    Write(data, len);
    MoveWriteoffset(len);
}
void Buffer::Writestring(const std::string &data)
{
    Write(data.c_str(), data.size());
}
void Buffer::WritestringAndpush(const std::string &data)
{
    Writestring(data);
    MoveWriteoffset(data.size());
}
void Buffer::WriteBuffer(Buffer &data)
{
    Write(data.GetReadPtr(), data.ReadableBytes());
}
void Buffer::WriteBufferAndpush(Buffer &data)
{
    WriteBuffer(data);
    MoveWriteoffset(data.ReadableBytes());
}
void Buffer::Read(void *buf, uint64_t len)
{
    assert(len <= ReadableBytes());

    std::copy(GetReadPtr(), GetReadPtr() + len, (char *)buf);
}
std::string Buffer::ReadAsstring(uint64_t len)
{
    assert(len <= ReadableBytes());
    std::string str;
    str.resize(len);
    Read(&str[0], len);
    return str;
}
void Buffer::ReadAndpop(void *buf, uint64_t len)
{
    Read(buf, len);
    MoveReadoffset(len);
}
std::string Buffer::ReadAsstringandpop(uint64_t len)
{
    assert(len <= ReadableBytes());
    std::string str = ReadAsstring(len);
    MoveReadoffset(len);
    return str;
}

char *Buffer::FindcrLf() // 寻找换行字符
{
    char *res = (char *)memchr(GetReadPtr(), '\n', ReadableBytes());
    return res;
}
std::string Buffer::GetLine()
{
    char *pos = FindcrLf();
    if (pos == NULL)
        return "";
    return ReadAsstring(pos - GetReadPtr() + 1); //+1把后面的换行字符也提取出来
}
std::string Buffer::GetLineAndPop()
{
    std::string str = GetLine();
    MoveReadoffset(str.size());
    return str;
}
// 清理功能（重置缓冲区状态）
void Buffer::clear()
{
    _read_idx = _write_idx = 0;
}
