#ifndef MUDUO_BUFFER_H
#define MUDUO_BUFFER_H

#include <cstdint>
#include <vector>
#include <cassert>
#include <cstring>
#include <string>

#define BUFFER_DEFAULT_SIZE 1024

/*用户态缓冲区，对接收发送的数据进行缓冲 */
class Buffer {
private:
    uint64_t _read_idx;//读偏移，指向可读数据的起始位置
    uint64_t _write_idx;//写偏移，指向可写空间的起始位置
    std::vector<char> _buffer;//连续空间，初始大小 BUFFER_DEFAULT_SIZE(1024)

public:
    Buffer();
    char* Begin();//空间起始位置
    
    // 简单getter在头文件实现
    char* GetWritePtr() { return Begin() + _write_idx; }//获取写位置
    char* GetReadPtr() { return Begin() + _read_idx; }//获取读位置
    uint64_t HeadIdleSize() { return _read_idx; }//缓冲区起始空间大小
    uint64_t TailIdleSize() { return _buffer.size() - _write_idx; }//缓冲区末尾空间大小
    size_t ReadableBytes() { return _write_idx - _read_idx; }//可读空间大小
    
    void MoveReadoffset(uint64_t len);//读偏移向后移动
    void MoveWriteoffset(uint64_t len);//写偏移向后移动
    void EnsureWritableBytes(uint64_t len);//确保可写空间足够
    void Write(const void* data, uint64_t len);//写入数据
    void WriteAndpush(const void* data, uint64_t len);//写入char*并移动写偏移
    void Writestring(const std::string& data);//写入string
    void WritestringAndpush(const std::string& data);////写入string并移动写偏移
    void WriteBuffer(Buffer& data);//写入buffer
    void WriteBufferAndpush(Buffer& data);//写入buffer并移动写偏移
    void Read(void* buf, uint64_t len);//读取len数据
    std::string ReadAsstring(uint64_t len);//读取len数据并以string返回
    void ReadAndpop(void* buf, uint64_t len);//读取len数据并移动读偏移
    std::string ReadAsstringandpop(uint64_t len);//读取len数据并以string返回并移动读偏移
    
    char* FindcrLf();//寻找换行字符
    std::string GetLine();//获取一行
    std::string GetLineAndPop();//获取一行并移动读偏移
    void clear();//重置读写偏移为0，不释放已分配内存
};

#endif