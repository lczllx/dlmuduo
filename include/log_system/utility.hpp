#ifndef __M_UTILITY_H__
#define __M_UTILITY_H__
/*实用工具类：
1.获取系统时间
2.获取文件大小
3.创建目录
4.检查文件是否存在
*/
#include <iostream>
#include <string>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>

namespace lcz
{
    namespace utility
    {
        class Date
        {
        public:
            static size_t getTime()
            {
                return size_t(time(nullptr));
            }
        };
        class File
        {
        public:
            static bool isFileexists(const std::string &pathname)
            {
                // return (access(pathname.c_str(), F_OK) == 0);//不通用，仅限类Unix系统
                struct stat st;
                if (stat(pathname.c_str(), &st) < 0) // 文件不存在
                {
                    return false;
                }

                return true;
            }
            static std::string path(const std::string &pathname)
            {
                // pathname = "/home/lcz/abc.txt" 取文件名前面的路径
                if(pathname.empty()) return ".";
                size_t pos = pathname.find_last_of("/\\");
                if (pos == std::string::npos)
                {
                    return "."; // 没有找到就在当前目录
                }
                return pathname.substr(0, pos + 1); // 包含最后的斜杠
            }
            static void createDirectory(const std::string &path)
            {
                if (path.empty()) return;

                // 如果以"./logs/"开头，去掉前缀
                std::string processed_path = path;
                const std::string prefix = "./logs/";
                if (processed_path.find(prefix) == 0) 
                {
                    processed_path = processed_path.substr(prefix.length());
                }

                std::string tmp;
                size_t len = processed_path.length();
                for (size_t i = 0; i < len; ++i) 
                {
                    tmp += processed_path[i];
                    if ((processed_path[i] == '/' || processed_path[i] == '\\') && i > 0)
                     {
                        if (tmp == "./" || tmp == "../") continue;
                        if (!isFileexists(tmp)) 
                        {
                            if (mkdir(tmp.c_str(), 0777) != 0 && errno != EEXIST) 
                            {
                                std::cerr << "mkdir error: " << strerror(errno) << " 路径: " << tmp << std::endl;
                                return;
                            }
                        }
                    }
                }
                if (!isFileexists(processed_path)) 
                {
                    if (mkdir(processed_path.c_str(), 0777) != 0 && errno != EEXIST) 
                    {
                        std::cerr << "mkdir error: " << strerror(errno) << " 路径: " << processed_path << std::endl;
                    }
                }
            }
        };
    }
}
#endif