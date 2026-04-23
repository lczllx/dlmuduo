/*
异步工作器
1.双缓冲区
2.互斥锁
3.条件变量
4.回调函数 针对缓冲区中数据的处理接口-外界传入一个函数，告诉异步工作器如何处理

*/
#pragma once
#include<functional>
#include <condition_variable>
#include <memory>
#include<mutex>
#include<atomic>
#include<thread>
#include "buffer.hpp"
#include "Logger.hpp"

namespace lcz
{
     enum class AsyncType//异步工作类型
    {
      ASYNC_SAFE,//安全状态，缓冲区满了就阻塞，避免资源耗尽
      ASYNC_UNSAFE//不考虑资源耗尽问题，无限扩容（常用于测试）
    };
    using Functor = std::function<void (Buffer &)>;//无返回值，传入buffer引用对象
   
    class AsyncLooper
    {
        public:
        using ptr = std::shared_ptr<AsyncLooper>;
        AsyncLooper(const Functor &func,AsyncType type=lcz::AsyncType::ASYNC_SAFE):_callback(func),_looper_type(type),_stop(false),_thread(std::thread(&AsyncLooper::threadentry,this)){};
        ~AsyncLooper(){stop();}
        void push(const char*data,size_t len)
        {
            std::unique_lock<std::mutex> lock(_mutex);
           //看是否可以向缓冲区加入数据集
           if(_looper_type==AsyncType::ASYNC_SAFE)
            _cond_pro.wait(lock,[&](){return _stop || len<=_pro_buf.abletowritelen();});
           //将数据放进生产缓冲区
            _pro_buf.push(data,len);
           //唤醒消费线程对缓冲区数据进行处理
            _cond_con.notify_one();

        }
        void stop()
        {         
             _stop = true;
            _cond_con.notify_all();//唤醒所有工作线程
            _cond_pro.notify_all();
            if (_thread.joinable()) {
                _thread.join();
            }
        //     // 再次检查并处理可能剩余的数据（双重保障）
        //   std::lock_guard<std::mutex> lock(_mutex);
        //   if (!_pro_buf.empty()) {
        //       std::cout << "处理停止后剩余的 " << _pro_buf.abletoreadlen() 
        //                 << " 字节数据" << std::endl;
        //       _callback(_pro_buf);
        //       _pro_buf.reset();
        //   }
        }
       private:
       //线程入口函数 对消费缓冲区中的数据进行处理，处理后，初始化缓冲区，交换缓冲区
       void threadentry()
       {
        // time_t last_sync_time = time(nullptr);
        // const time_t sync_interval = 1; // 每秒同步一次
           while (true) 
           {
                //  Buffer local_buf; 
                //为互斥锁添加生命周期      
                {
                    //判断生产缓冲区有没有数据，有就交换，没有则阻塞
                    std::unique_lock<std::mutex> lock(_mutex);
                    //退出标志被设置且生产缓冲区中没有数据再退出
                    if (_stop && _pro_buf.empty()) break;

                    _cond_con.wait(lock, [&](){ return _stop || !_pro_buf.empty(); });
                                        
                    _pro_buf.swap(_con_buf);
                    
                    //唤醒生产者，是安全状态时才唤醒
                    if (_looper_type == AsyncType::ASYNC_SAFE) 
                    {
                        _cond_pro.notify_all();
                    }
                    //被唤醒后对数据进行处理
                    _callback(_con_buf);

                    // // 检查是否需要同步
                    // time_t now = time(nullptr);
                    // if (now - last_sync_time >= sync_interval) {
                    //     // 调用所有sink的同步方法
                    //     sync_all_sinks();
                    //     last_sync_time = now;
                    // }

                    //初始化缓冲区
                    _con_buf.reset();
                }        
                // if (!local_buf.empty()) 
                // {
                //     _callback(_con_buf);
                // }

                
            }
            
            // // 确保处理所有剩余数据
            // std::lock_guard<std::mutex> lock(_mutex);
            // if (!_pro_buf.empty()) {
            //     _callback(_pro_buf);
            //     _pro_buf.reset();
            // }
    
       }
       private:
       Functor _callback;//具体对缓冲区数据进行处理的回调函数，由异步工作器使用者传入


        private:
        AsyncType _looper_type;
        std::atomic<bool> _stop;//停止标志
        Buffer _pro_buf;//生产缓冲区
        Buffer _con_buf;//消费缓冲区
        std::mutex _mutex;//互斥锁
        std::condition_variable _cond_pro;//生产者等待队列条件变量
        std::condition_variable _cond_con;//消费者等待队列条件变量
        std::thread _thread;//异步工作器对应的工作线程
    };
}
