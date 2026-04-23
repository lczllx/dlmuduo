/*实现异步日志缓冲区*/
#pragma once
#include<iostream>
#include<vector>
#include "utility.hpp"

namespace lcz
{
    #define DEFAULT_BUFFER_SIZE (1*1024*1024)// 默认缓冲区大小 1MB
    #define THRESHOLD_BUFFER_SIZE (3*1024*1024)// 缓冲区扩容阈值 3MB
    #define INCREMENT_BUF_SIZE (1*1024*1024)// 超过阈值后每次扩容 1MB
    class Buffer//线性缓冲区
    {
        public:
        Buffer():_buf(DEFAULT_BUFFER_SIZE),_write_pos(0),_read_pos(0){}
        //向缓冲区写入数据
        void push(const char* data,size_t len)
        {
            //考虑空间不够就扩容，将空间控制交给异步工作器
            getenoughsize(len);
            //将数据拷贝进缓冲区
            std::copy(data,data+len,_buf.begin()+_write_pos);

            //将当前写入位置向后偏移
            movewriter(len);
        }
 

        //返回可读数据起始地址
        const char* begin()
        {
            return &_buf[_read_pos];
        }
        
        //返回可写数据的长度
        size_t abletowritelen()const
        {
            //仅仅对固定大小缓冲区提供
           return (_buf.size()-_write_pos);
        }
        //返回可读数据的长度
        size_t abletoreadlen()
        {
            return (_write_pos-_read_pos);
        }
        
        void movereader(size_t len)
        {
            assert(len<abletoreadlen());
            _read_pos+=len;
        }

        //返回移动读写位置
        const char* readpos(){return (&_buf[_read_pos]);}
        const char* writepos(){return (&_buf[_write_pos]);}

        //初始化缓冲区
        void reset()
        {
            _write_pos=0;
            _read_pos=0;
        }
        //交换缓冲区
        void swap(Buffer& buf)
        {
            _buf.swap(buf._buf);//交换的是地址
            std::swap(_read_pos,buf._read_pos);
            std::swap(_write_pos,buf._write_pos);
        }
        bool empty()
        {
            return _write_pos==_read_pos;
        }
        /*检查缓冲区是否已满 */
        bool full() 
        {
            return (abletowritelen() == 0);
        }
        
        private:
        //对读写指针进行向后偏移操作
        void movewriter(size_t len)
        {
            assert((len+_write_pos)<=_buf.size());
            _write_pos+=len;
        }
        //对空间进行动态扩容
        void getenoughsize(size_t len)
        {
            if(len<abletowritelen())return;
            size_t new_size=0;
            if(_buf.size()<THRESHOLD_BUFFER_SIZE)
            {
                new_size=_buf.size()*2+len;
            }
            else{
                new_size=_buf.size()+INCREMENT_BUF_SIZE+len;
            }

            _buf.resize(new_size);
        }

        private:
            std::vector<char> _buf;
            size_t _write_pos;// 当前可读写位置
            size_t _read_pos;// 当前可读位置
    };
}
