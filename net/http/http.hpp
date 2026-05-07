#ifndef __M_HTTP__
#define __M_HTTP__
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sys/stat.h>
#include<unordered_map>
#include "../../include/m_ser.hpp"

#define DEFAULT_TIMEOUT 10
static std::unordered_map<int,std::string> statu_msg={
            {100, "Continue"},
            {101, "Switching Protocol"},
            {102, "Processing"},
            {103, "Early Hints"},
            {200, "OK"},
            {201, "Created"},
            {202, "Accepted"},
            {203, "Non-Authoritative Information"},
            {204, "No Content"},
            {205, "Reset Content"},
            {206, "Partial Content"},
            {207, "Multi-Status"},
            {208, "Already Reported"},
            {226, "IM Used"},
            {300, "Multiple Choice"},
            {301, "Moved Permanently"},
            {302, "Found"},
            {303, "See Other"},
            {304, "Not Modified"},
            {305, "Use Proxy"},
            {306, "unused"},
            {307, "Temporary Redirect"},
            {308, "Permanent Redirect"},
            {400, "Bad Request"},
            {401, "Unauthorized"},
            {402, "Payment Required"},
            {403, "Forbidden"},
            {404, "Not Found"},
            {405, "Method Not Allowed"},
            {406, "Not Acceptable"},
            {407, "Proxy Authentication Required"},
            {408, "Request Timeout"},
            {409, "Conflict"},
            {410, "Gone"},
            {411, "Length Required"},
            {412, "Precondition Failed"},
            {413, "Payload Too Large"},
            {414, "URI Too Long"},
            {415, "Unsupported Media Type"},
            {416, "Range Not Satisfiable"},
            {417, "Expectation Failed"},
            {418, "I'm a teapot"},
            {421, "Misdirected Request"},
            {422, "Unprocessable Entity"},
            {423, "Locked"},
            {424, "Failed Dependency"},
            {425, "Too Early"},
            {426, "Upgrade Required"},
            {428, "Precondition Required"},
            {429, "Too Many Requests"},
            {431, "Request Header Fields Too Large"},
            {451, "Unavailable For Legal Reasons"},
            {501, "Not Implemented"},
            {502, "Bad Gateway"},
            {503, "Service Unavailable"},
            {504, "Gateway Timeout"},
            {505, "HTTP Version Not Supported"},
            {506, "Variant Also Negotiates"},
            {507, "Insufficient Storage"},
            {508, "Loop Detected"},
            {510, "Not Extended"},
            {511, "Network Authentication Required"}
        };
         static std::unordered_map<std::string, std::string> mime_msg = {
            {".aac", "audio/aac"},
            {".abw", "application/x-abiword"},
            {".arc", "application/x-freearc"},
            {".avi", "video/x-msvideo"},
            {".azw", "application/vnd.amazon.ebook"},
            {".bin", "application/octet-stream"},
            {".bmp", "image/bmp"},
            {".bz", "application/x-bzip"},
            {".bz2", "application/x-bzip2"},
            {".csh", "application/x-csh"},
            {".css", "text/css"},
            {".csv", "text/csv"},
            {".doc", "application/msword"},
            {".docx", "application/vnd.openxmlformatsofficedocument.wordprocessingml.document"},
            {".eot", "application/vnd.ms-fontobject"},
            {".epub", "application/epub+zip"},
            {".gif", "image/gif"},
            {".htm", "text/html"},
            {".html", "text/html"},
            {".ico", "image/vnd.microsoft.icon"},
            {".ics", "text/calendar"},
            {".jar", "application/java-archive"},
            {".jpeg", "image/jpeg"},
            {".jpg", "image/jpeg"},
            {".js", "text/javascript"},
            {".json", "application/json"},
            {".jsonld", "application/ld+json"},
            {".mid", "audio/midi"},
            {".midi", "audio/x-midi"},
            {".mjs", "text/javascript"},
            {".mp3", "audio/mpeg"},
            {".mpeg", "video/mpeg"},
            {".mpkg", "application/vnd.apple.installer+xml"},
            {".odp", "application/vnd.oasis.opendocument.presentation"},
            {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
            {".odt", "application/vnd.oasis.opendocument.text"},
            {".oga", "audio/ogg"},
            {".ogv", "video/ogg"},
            {".ogx", "application/ogg"},
            {".otf", "font/otf"},
            {".png", "image/png"},
            {".pdf", "application/pdf"},
            {".ppt", "application/vnd.ms-powerpoint"},
            {".pptx", "application/vnd.openxmlformatsofficedocument.presentationml.presentation"},
            {".rar", "application/x-rar-compressed"},
            {".rtf", "application/rtf"},
            {".sh", "application/x-sh"},
            {".svg", "image/svg+xml"},
            {".swf", "application/x-shockwave-flash"},
            {".tar", "application/x-tar"},
            {".tif", "image/tiff"},
            {".tiff", "image/tiff"},
            {".ttf", "font/ttf"},
            {".txt", "text/plain"},
            {".vsd", "application/vnd.visio"},
            {".wav", "audio/wav"},
            {".weba", "audio/webm"},
            {".webm", "video/webm"},
            {".webp", "image/webp"},
            {".woff", "font/woff"},
            {".woff2", "font/woff2"},
            {".xhtml", "application/xhtml+xml"},
            {".xls", "application/vnd.ms-excel"},
            {".xlsx", "application/vnd.openxmlformatsofficedocument.spreadsheetml.sheet"},
            {".xml", "application/xml"},
            {".xul", "application/vnd.mozilla.xul+xml"},
            {".zip", "application/zip"},
            {".3gp", "video/3gpp"},
            {".3g2", "video/3gpp2"},
            {".7z", "application/x-7z-compressed"}
            };
//支持一些零碎的功能
class Util
{
    public:
    //字符串分割函数
    static size_t Split(const std::string &src,const std::string &sep,std::vector<std::string> *arry)
    {
        size_t offset=0;
        while(offset<src.size())
        {
            size_t pos = src.find(sep, offset);
            if (pos == std::string::npos)
            {
                if (offset < src.size())
                {
                    arry->push_back(src.substr(offset));
                }
                break;
            }
            if (pos > offset)
            {
                arry->push_back(src.substr(offset, pos - offset));
            }
            //即使 pos == offset（连续分隔符），也要移动offset，避免死循环
            offset = pos + sep.size();
            
        }
        return arry->size();
    }
    //读取文件内容 -慢
    static bool ReadFile(const std::string &filename,std::string *buf)//使用Buffer会占用很多空间
    {
        std::ifstream ifs(filename,std::ios::binary);
        if(!ifs.is_open())
        {
            L_ERROR("open %s file failed", filename.c_str());
            return false;
        }
        size_t fsize=0;
        ifs.seekg(0,ifs.end);
        fsize=ifs.tellg();
        if (fsize <= 0)
        {
            L_ERROR("file %s is empty or size error", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.seekg(0,ifs.beg);
        buf->resize(static_cast<size_t>(fsize));//将一个文件大小或其他整数值安全地转换为size_t类型，并用它来调整缓冲区的大小
       ifs.read(&(*buf)[0], fsize);
        if (ifs.good()==false)
        {
            L_ERROR("read %s file failed", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }
    //向文件写入数据
    static bool WriteFile(const std::string &filename,const std::string &buf)
    {
        std::ofstream ofs(filename,std::ios::binary | std::ios::trunc);// 以二进制模式打开文件，trunc表示清空原有内容
        if(!ofs.is_open())
        {
            L_ERROR("open %s file failed", filename.c_str());
            return false;
        }
        ofs.write(buf.c_str(),buf.size());
        if(!ofs.good())
        {
            L_ERROR("write %s file failed", filename.c_str());
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }
    //URL编码 -格式：将特殊ascii值，转为2个16进制字符 前缀：%
    //RFC3986文档规定，绝对不编码字符：.-_~  编码格式：%HH
    //W3C标准规定，查询字符串中的空格，需要编码为+，解码则相反
    static std::string UrlEncode(const std::string &url,bool is_space_to_plus)
    {
        std::string output;
        // 可以用unsigned char防止负值导致格式化异常
        for (auto &c : url)
        {
            // 判断是否是字母或数字，或指定不编码字符
            if ((c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '.' || c == '_' || c == '~')
            {
                output.push_back(c); // 不编码，直接复制
            }
            else if (c == ' ')
            {
                if (is_space_to_plus)output.push_back('+'); // 空格转成'+'
                else
                {
                    char buf[4] = {0};
                    std::snprintf(buf, sizeof(buf), "%%20"); // 空格转 %20，空格的 ASCII 码十六进制是 0x20，所以空格被编码为 %20
                    output.append(buf);
                }
            }
            else
            {
                char buf[4] = {0};
                std::snprintf(buf, sizeof(buf), "%%%02X", c); // %%打印%符号
                output.append(buf);
            }
        }

        return output;
    }
    //单个十六进制字符转成对应的十进制整数值
    static char Hextoi(char ch)
    {
        if(ch>='0' && ch<='9')return ch-'0';
        else if(ch>='A' && ch<='Z')return ch-'A'+10;
        else if(ch>='a' && ch<='z')return ch-'a'+10;
        return -1;
    }
    //URL解码
    static std::string UrlDecode(const std::string &url,bool is_plus_to_space)
    {
       
        std::string res;
        const size_t len = url.size();
        for(size_t i=0;i<len;i++)
        {
            if(url[i]=='+'&& is_plus_to_space)res.push_back(' ');
            else if(url[i]=='%')
            {
                if (i + 2 < len)
                {
                    char ret1=Hextoi(url[i+1]);
                    char ret2=Hextoi(url[i+2]);
                    // 检查十六进制字符是否有效
                    if (ret1 != -1 && ret2 != -1)
                    {
                        //将两个16进制数字合成一个字节（8位）的字符
                        char decoded_char = (ret1 << 4) | ret2;
                        res.push_back(decoded_char);
                        i += 2; // 跳过已解码的两个字符
                        continue;
                    }
                   
                }
            }
           res.push_back(url[i]);
        }
        return res;

    }
    //响应状态码的描述信息获取
    static std::string StatuDesc(int statu)
    {
        
        auto it=statu_msg.find(statu);
        if(it!=statu_msg.end()){return it->second;}
        return "Unknown";
    }
    //根据文件后缀名获取文件mime
    static std::string ExtMime(std::string &filename)
    {       
        //先获取文件扩展名
        size_t pos= filename.find_last_of('.');
        if (pos == std::string::npos || pos + 1 >= filename.size())
            return "application/octet-stream";  // 无扩展名或扩展名为空
        //再找扩展名对应的mime
        std::string ext=filename.substr(pos);
        auto it = mime_msg.find(ext);
        if(it==mime_msg.end()){return "application/octet-stream";}
        return it->second;
    }
      //判断一个文件是否是一个目录
    static bool IsDirectory(const std::string &filename)
    {
        struct stat st;
        // 使用stat系统调用获取文件信息，参数一需要const char*，传入filename.c_str()
        int ret=stat(filename.c_str(),&st);
        if(ret<0){return false;}
        // 使用宏判断文件模式是否为目录类型
        return S_ISDIR(st.st_mode);
    }
    //判断一个文件是不是应该普通文件
    static bool IsRegular(const std::string &filename)
    {
        struct stat st;
         // 使用stat系统调用获取文件信息，参数一需要const char*，传入filename.c_str()
        int ret=stat(filename.c_str(),&st);
        if(ret<0)return false;
        // 使用宏判断文件模式是否为普通文件
        return S_ISREG(st.st_mode);
    }
    //http请求的资源路径的有效性判断
    static bool ValidPath(const std::string &path)
    {
       std::vector<std::string>v1;
        Split(path,"/",&v1);
        int level=0;
        for(auto &e:v1)
        {
            if(e=="..")
            {
                level--;
                if(level<0)return false;
               
            }
            else if (!e.empty() && e != ".")
            {
                // 非空且不是当前目录"."，增层级
                level++;
            }            
        }
        return true;
    }
};

//请求信息模块
class HttpRequest
{
    public:
    std::string _method;//请求方法
    std::string _path;//资源路径
    std::string _version;//协议版本
    std::string _body;//请求正文
    std::smatch _matches;//路径资源的正则提取数据
    std::unordered_map<std::string,std::string> _headers;//头部字段
    std::unordered_map<std::string,std::string> _params;//查询字符串
    public:
    HttpRequest():_version("HTTP/1.1"){}
    //重置接口
    void ReSet()
    {
        _headers.clear();_params.clear();
        _method.clear();_path.clear();
        _body.clear();_version="HTTP/1.1";
        std::smatch match;_matches.swap(match);
    }
    //插入头部字段
    void SetHeader(const std::string &key,const std::string &val){_headers.insert(std::make_pair(key, val));}
    //判断是否存在头部字段
    bool HasHeader(const std::string &key)const
    {
        auto it = _headers.find(key);
        if(it==_headers.end()){return false;}
        return true;
    }
    //获取头部字段
    std::string GetHeader(const std::string &key)const
    {
        auto it = _headers.find(key);
        if(it==_headers.end()){return "";}
        return it->second;
    }
    //插入查询字符串
    void SetParam(const std::string &key,const std::string &val){_params.insert(std::make_pair(key, val));}
   
    //判断是否存在询字符串
    bool HasParam(const std::string &key)const
    {
        auto it = _params.find(key);
        if(it==_params.end()){return false;}
        return true;
    }
    //获取查询字符串
    std::string GetParam(const std::string &key)const
    {
        auto it = _params.find(key);
        if(it==_params.end()){return "";}
        return it->second;
    }
    //获取正文长度
    size_t ContentLength()const
    {
        bool ret=HasHeader("Content-Length");
        if(!ret){return 0;}
        std::string res=GetHeader("Content-Length");
        return std::stol(res);
    }
    // 判断是否需要关闭连接
    // HTTP/1.1 默认 keep-alive，只有显式 Connection: close 才关闭
    // HTTP/1.0 默认 close，只有显式 Connection: keep-alive 才保持
    bool IsClose() const {
        if (_version == "HTTP/1.1" || _version == "HTTP/1.0") {
            if (HasHeader("Connection")) {
                std::string v = GetHeader("Connection");
                std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                if (v == "close") return true;
                if (v == "keep-alive") return false;
            }
            return _version != "HTTP/1.1"; // 1.1 default keep-alive, 1.0 default close
        }
        // HTTP/0.9 fallback: 默认 close
        if (HasHeader("Connection")) {
            std::string v = GetHeader("Connection");
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            if (v == "keep-alive") return false;
        }
        return true;
    }
};

//存储http响应信息要素，提供简单接口
class HttpResponse
{
    public:
    int _statu;///状态码
    bool _redirect_flag;//重定向设置符
    std::string _body;//响应正文
    std::string _redirect_url;//重定向的url
    std::unordered_map<std::string,std::string>_headers;//头部字段

    public:
    HttpResponse():_statu(200),_redirect_flag(false){}
    HttpResponse(int statu):_statu(statu),_redirect_flag(false){}
    //重置接口
    void ReSet()
    {
        _statu=200;_headers.clear();
        _redirect_url.clear();_body.clear();
        _redirect_flag=false;
    }
    //插入头部字段
    void SetHeader(const std::string &key,const std::string &val){ _headers.insert(std::make_pair(key, val));}
    //判断是否存在头部字段
    bool HasHeader(const std::string &key)const
    {
        auto it = _headers.find(key);
        if(it==_headers.end()){return false;}
        return true;
    }
    //获取头部字段
    std::string GetHeader(const std::string &key)const
    {
        auto it = _headers.find(key);
        if(it==_headers.end()){return "";}
        return it->second;
    }
    //设置正文---
    void SetContent(const std::string &body,const std::string &type = "text/html"/*---*/)
    {
        _body=body;
        SetHeader("Content-Type",type);
    }
    //设置状态码
    void SetStatu(int statu=302)
    {
        _statu=statu;
    }
    //重定向响应
    void SetRedirect(const std::string &url,int statu=302)
    {
        _statu=statu;
        _redirect_flag=true;
        _redirect_url=url;
    }
    //是否是短链接
    bool IsClose()const
    {
        if(HasHeader("Connection")&&(GetHeader("Connection")=="keep-alive")){return false;}
        return true;
    }
};
enum class HttpRecvStatus:int{
RECV_HTTP_ERROR,// 解析错误状态
RECV_HTTP_LINE,  // 正在接收请求行
RECV_HTTP_HEAD, // 正在接收请求头
RECV_HTTP_BODY,  // 正在接收请求体
RECV_HTTP_OVER   // 请求接收完成
};
// typedef enum {
//     RECV_HTTP_ERROR,
//     RECV_HTTP_LINE,
//     RECV_HTTP_HEAD,
//     RECV_HTTP_BODY,
//     RECV_HTTP_OVER
// }HttpRecvStatu;
#define MAX_LINE 8192// HTTP协议中请求行和头部最大长度限制
class HttpContext
{
    private:
    int _resp_statu;//响应状态码
    HttpRecvStatus _recv_statu;//当前接收及解析的阶段状态
    //HttpRecvStatu _recv_statu;//当前接收及解析的阶段状态
    HttpRequest _request;//已经解析得到的请求信息
    std::unordered_map<std::string,std::string> _dispatch_cache;//dispatcher匹配时的缓存
    private:
     // 解析请求行，格式如："GET /path?query HTTP/1.1"
    bool ParseHttpLine(const std::string &line)
    {    
         //L_DEBUG("开始解析请求行: [%s]", line.c_str());
        std::smatch matchs;
        //正则表达式匹配请求方法、路径、查询字符串及协议版本（静态仅编译一次）
        static const std::regex e(
            "^(GET|HEAD|POST|PUT|DELETE)\\s+([^?\n\r]*)(?:\\?([^\\n\\r ]*))?\\s+(HTTP/1\\.[01])(?:\\r?\\n)?$",
            std::regex::icase);
        //std::regex e("(GET|HEAD|POST|PUT|DELETE) ([^?]*)(?:\\?(.*))? (HTTP/1\\.[01])(?:\n|\r\n)?", std::regex::icase);
        //std::regex e("^(GET|HEAD|POST|PUT|DELETE|OPTIONS|PATCH)\\s+([^\\s?]+)(?:\\?(.*))?\\s+(HTTP/1\\.[01])", std::regex::icase);
        bool ret = std::regex_match(line, matchs, e);
        if(!ret){// 解析失败，设置错误状态和响应码
             // L_DEBUG("请求行解析失败: %s", line.c_str()); 
            _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
            _resp_statu=400;//Bad request
            return false;
        }
        //url本身 请求方法 资源路径 查询字符串 协议版本
        _request._path=Util::UrlDecode(matchs[2],false);// URL解码路径部分
        //将小写的请求方法转为大写，分别后续进行匹配
        _request._method=matchs[1];
         std::transform(_request._method.begin(),_request._method.end(),_request._method.begin(),::toupper/*加::表示调用的时c全局接口*/);
        _request._version=matchs[4];
        std::vector<std::string> query_arry;
        std::string query_str=matchs[3];
        //查询字符串的格式是k=v&k=v&...,需要将其分割
        Util::Split(query_str,"&",&query_arry);
        for(auto &q_str:query_arry)
        {
            size_t pos = q_str.find("=");
            if(pos==std::string::npos)
            {// 查询参数格式错误
                _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
                _resp_statu=400;//Bad request
                return false;
            }
            std::string key=Util::UrlDecode(q_str.substr(0,pos),true);
            std::string val=Util::UrlDecode(q_str.substr(pos+1),true);
            _request.SetParam(key,val);// 设置请求参数
        }
        return true;
    }
    // 接收并解析请求行
    bool RecvHttpLine(Buffer *buf)
    {
        if(!(_recv_statu==HttpRecvStatus::RECV_HTTP_LINE))return false;
        //获取第一行数据
        std::string line=buf->GetLineAndPop();
         // 还未收到完整请求行
        if(line.size()==0)
        {
            if(buf->ReadableBytes()>MAX_LINE)
            {
                _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
                _resp_statu=414;// URI Too Long
                return false;
            }
            return true;// 等待更多数据
        }
        if(line.size()>MAX_LINE) // 请求行过长
        {
            _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
            _resp_statu=414;//URI too long
            return false;
        }
        bool ret=ParseHttpLine(line);
        if(!ret){return false;}
        _recv_statu=HttpRecvStatus::RECV_HTTP_HEAD;// 解析完请求行，切换到请求头阶段
        return true;
    }
     // 解析单行请求头，格式如："Host: www.example.com"
    bool ParseHttpHead(std::string &line)
    {
        //key: val\r\nkey: val\r\n
        if (line.back() == '\n') line.pop_back();//末尾是换⾏则去掉换⾏字符
        if (line.back() == '\r') line.pop_back();//末尾是回⻋则去掉回⻋字符
        size_t pos =line.find(": ");
         if(pos==std::string::npos){ // 头部格式错误
            _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
            _resp_statu=400;//Bad request
            return false;
        }
        std::string key=line.substr(0,pos);
        std::string val=line.substr(pos+2);
        _request.SetHeader(key,val); // 设置请求头字段
        return true;
    }    
     // 接收并解析请求头
    bool RecvHttpHead(Buffer *buf)
    {
         if(!(_recv_statu==HttpRecvStatus::RECV_HTTP_HEAD))return false;
        //头部不止一行
        while(true)
        {
            //获取一行数据
            std::string line=buf->GetLineAndPop();
            if(line.size()==0)
            {// 头部还未完整收到
                if(buf->ReadableBytes()>MAX_LINE)
                {
                    _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
                    _resp_statu=414;//URI too long
                    return false;
                }
                return true; // 等待更多数据
            }
            if(line.size()>MAX_LINE)
            {
                _recv_statu=HttpRecvStatus::RECV_HTTP_ERROR;
                _resp_statu=414;//URI too long
                return false;
            }
            if(line=="\n"||line=="\r\n"/*||line=="\r"---*/)break;// 空行标志头部结束
            bool ret=ParseHttpHead(line);
            if(!ret){return false;}
        }
        _recv_statu=HttpRecvStatus::RECV_HTTP_BODY;// 请求头接收完毕，进入请求体阶段
        return true;

    }
     // 接收请求体
    bool RecvHttpBody(Buffer *buf)
    {
        if(!(_recv_statu==HttpRecvStatus::RECV_HTTP_BODY))return false;
        //先获取正文长度
        size_t contentlength=_request.ContentLength();
        if (contentlength == 0) {
            //没有正⽂，则请求接收解析完毕
            _recv_statu = HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        //再看看还有多少没有获取
        size_t last_length=contentlength-_request._body.size();
        // if(last_length==0)// 已经接收完整请求体---
        // {
        //     _recv_statu=HttpRecvStatus::RECV_HTTP_OVER;
        //     return true;
        // }
        if(buf->ReadableBytes()>=last_length)// 缓冲区中数据足够补全请求体
        {
            _request._body.append(buf->GetReadPtr(),last_length);
            buf->MoveReadoffset(last_length);
            _recv_statu=HttpRecvStatus::RECV_HTTP_OVER;
            return true;
        }
        //数据不够，先取出来，然后等待新数据的到来
        _request._body.append(buf->GetReadPtr(),buf->ReadableBytes());
        buf->MoveReadoffset(buf->ReadableBytes());
        return true;
    }
    public:
    HttpContext():_resp_statu(200),_recv_statu(HttpRecvStatus::RECV_HTTP_LINE){}
    //重置接口
    void ReSet(){_resp_statu=200;_recv_statu=HttpRecvStatus::RECV_HTTP_LINE;_request.ReSet();}
    int RespStatu(){return _resp_statu;}
    HttpRecvStatus RecvStatu(){return _recv_statu;}
    HttpRequest& Request(){return _request;}
    //接收并解析http请求
    void RecvHttpRequest(Buffer *buf)
    {
        //L_DEBUG("开始解析HTTP请求,当前状态: %d", (int)_recv_statu);
        switch(_recv_statu)
        {
            case HttpRecvStatus::RECV_HTTP_LINE:RecvHttpLine(buf);
                [[fallthrough]];
            case HttpRecvStatus::RECV_HTTP_HEAD:RecvHttpHead(buf);
                [[fallthrough]];
            case HttpRecvStatus::RECV_HTTP_BODY:RecvHttpBody(buf);
                break;
            case HttpRecvStatus::RECV_HTTP_ERROR:
            case HttpRecvStatus::RECV_HTTP_OVER:
                break;
        }
         //L_DEBUG("解析完成，新状态: %d", (int)_recv_statu);
        return;
    }
};

class HttpServer
{
    private:
    using Handler=std::function<void(const HttpRequest &,HttpResponse *)>;//？为什么是一个引用一个指针---const+&表示传的是输入参数，指针表示输出参数 对其进行修改的
    //路由映射表里面的哈希表里面的string是一个正则表达式，防止需要准备过多的映射函数处理
    //由于每次进行请求处理时都需要对正则表达式进行编译，为了更好的性能，可以设置成std::vector<std::pair<std::regex/*编译完毕的正则表达式*/,Handler>>
    using Route_Table=std::vector<std::pair<std::regex,Handler>>;//本来不想这样设置，但是不设置要改的太多了
    Route_Table _get_route;
    // std::unordered_map<std::string, Handler> _get_exact_route;  // 压测用
    Route_Table _put_route;
    Route_Table _delete_route;
    Route_Table _post_route;
    std::string _basedir;//静态资源根目录
    TcpServer _server;
    private:
    void ErrorHandler(const HttpRequest &req,HttpResponse *resp)
    {
        (void)req;
        //组织错误页面
        std::string body;
        body+="<html>";
        body+="<head>";
        body+="<meta http-equiv='Content-Type' content='text/html;charset=utf-8'>";//添加这个防止乱码，"改为'防止和字符串里面的"冲突
        body+="</head>";
        body+="<body>";
        body+="<h1>";
        body+=std::to_string(resp->_statu);
        body+=" ";
        body+=Util::StatuDesc(resp->_statu);
        body+="</h1>";
        body+="</body>";
        body+="</html>";
        //将页面数据，当作响应正文放入resp中
        resp->SetContent(body,"text/html");
    }
    //将HttpResponse里面的内容按照http协议格式组织发送
    void WriteResponse(const PtrConnection &conne,const HttpRequest &req,HttpResponse *resp)
    {
        //完善头部字段
        if(req.IsClose())
        {//如果是短链接
            resp->SetHeader("Connection","Close");
        }else{
            resp->SetHeader("Connection","keep-alive");
        }
        // 始终设置 Content-Length（keep-alive 下空 body 也需要 Content-Length: 0）
        if(resp->HasHeader("Content-Length")==false)
        {
            resp->SetHeader("Content-Length",std::to_string(resp->_body.size()));
        }
        // 如果响应体不为空且没有设置Content-Type头部，设置默认内容类型
        if((!resp->_body.empty()) && resp->HasHeader("Content-Type")==false)
        {
            resp->SetHeader("Content-Type","application/octet-stream");
        }
        if(resp->_redirect_flag==true)// 如果是重定向响应，设置Location头部
        {
            resp->SetHeader("Location",resp->_redirect_url);
        }
        //将resp中的要素按照http协议格式进行组织（减少拷贝与分配）
        std::string resp_str;
        resp_str.reserve(128 + resp->_body.size());
        // 写入状态行：版本 状态码 状态描述
        resp_str.append(req._version)
                .append(" ")
                .append(std::to_string(resp->_statu))
                .append(" ")
                .append(Util::StatuDesc(resp->_statu))
                .append("\r\n");
        // 写入所有头部字段
        for(auto &head:resp->_headers){
            resp_str.append(head.first).append(": ").append(head.second).append("\r\n");
        }
        // 头部与主体之间的空行
        resp_str.append("\r\n");
        // 写入响应主体（如果有）
        if(req._method!="HEAD"){
            resp_str.append(resp->_body);
        }
        //发送数据
        conne->Send(resp_str.c_str(),resp_str.size());
    }
    //判断是不是静态资源请求
    bool IsFileHandler(const HttpRequest &req)
    {
        // 检查基础目录是否设置，未设置则无法处理静态资源
        if(_basedir.empty()){return false;}
        // 静态资源请求只支持GET和HEAD方法
        if(req._method!="GET" && req._method!="HEAD"){return false;}
        // 验证请求路径是否合法（防止路径遍历攻击等）
        if(Util::ValidPath(req._path)==false){return false;}
         // 构建完整的文件路径：基础目录 + 请求路径
        std::string req_path=_basedir+req._path;
        if(req._path.back()=='/')// 如果请求路径以'/'结尾，默认寻找index.html文件
        {
            req_path+="index.html";
        }
        // 检查目标路径是否为普通文件（非目录、符号链接等）
        if(Util::IsRegular(req_path)==false){return false;}
       
        return true;

    }
    //静态资源的请求处理 将静态资源的数据取出来放到resp的body中，并设置mime
    void FileHandler(const HttpRequest &req,HttpResponse *resp)
    {
        // 构建完整的文件路径：基础目录 + 请求路径
        std::string req_path=_basedir+req._path;
        if(req._path.back()=='/')// 如果请求路径以'/'结尾，默认寻找index.html文件
        {
            req_path+="index.html";
        }
        // req._path=req_path; // 验证通过，将完整文件路径设置回请求对象中
        bool ret=Util::ReadFile(req_path,&resp->_body);
        if(!ret)return ;
        std::string mime=Util::ExtMime(req_path);/*资源扩展名*/
        resp->SetHeader("Content-Type",mime);
        return ;
    }
    //功能性请求的分类处理
    void Dispathcher(HttpRequest &req,HttpResponse *resp,Route_Table &route_table)
    {
        //在对应请求方法的路由表中查找，有就调用对应的处理函数，没有就设置404
        //路由映射表里面的哈希表里面的string是一个正则表达式 /number/(\d+)  ->  /number/2123
        for(auto &each:route_table)
        {
            const std::regex &re = each.first;//-
            const Handler &func=each.second;
            bool ret= std::regex_match(req._path,req._matches,re);
            if(!ret){continue;}
            return func(req,resp);//处理一个就勾八return了？
        }
        resp->_statu=404;
    }
    //根据不同的请求方式选择不同的路由表
    void Route(HttpRequest &req,HttpResponse *resp)
    {
        //先看看是不是静态资源请求
        if(IsFileHandler(req)==true)
        {
            return FileHandler(req,resp);
        }
        if(req._method=="GET" || req._method=="HEAD"){return Dispathcher(req,resp,_get_route);}
        else if(req._method=="POST"){return Dispathcher(req,resp,_post_route);}
        else if(req._method=="PUT"){return Dispathcher(req,resp,_put_route);}
        else if(req._method=="DELETE"){return Dispathcher(req,resp,_delete_route);}
        //以上都不符合，那就是不支持
        resp->_statu=405;//Method Not Allowed
        return;
    }
    //设置上下文
    void OnConnected(const PtrConnection &conne){conne->SetContext(HttpContext());L_DEBUG("newconnection %p", conne.get());}
    //缓冲区数据解析处理
    void OnMessage(const PtrConnection &conne,Buffer *buf)
    {
        while(buf->ReadableBytes()>0)
        {
            //获取上下文
            HttpContext *context=conne->GetContext()->get<HttpContext>();
            //通过上下文对接收缓冲区数据进行解析，得到httprequest对象
            context->RecvHttpRequest(buf);
             //L_DEBUG("解析后状态: %d, 响应码: %d", (int)context->RecvStatu(), context->RespStatu());
            HttpResponse resp(context->RespStatu());//根据响应状态码构建resp
            HttpRequest &req=context->Request();
            if(context->RespStatu()>=400)//---这里出错了，感觉是这里的问题
            {
                L_DEBUG("检测到错误，关闭连接。状态: %d", (int)context->RecvStatu());
                ErrorHandler(req,&resp);//填充一个错误页面给resp
                WriteResponse(conne,req,&resp);//组织发送给客户端
                context->ReSet();//-
                buf->MoveReadoffset(buf->ReadableBytes());//出错了就把缓冲区数据清空//-
                conne->Shutdown();//关闭连接
                return;
            }
            if(context->RecvStatu()!=HttpRecvStatus::RECV_HTTP_OVER)
            {//如果没有接收完毕，那么现在缓冲区的数据不是完整的，等下一次处理
                return;
            }
            //请求路由和处理业务
            Route(req,&resp);
            //对httpresponse进行组织发送
            WriteResponse(conne,req,&resp);
            //重置上下文
            context->ReSet();
            //根据长短链接判断是否需要继续处理
            if(req.IsClose()){conne->Shutdown();}
        }
        return;
    }
    public:
    HttpServer(int port,int timeout=DEFAULT_TIMEOUT):_server(port){
        _server.EnableInactiveRelease(timeout);
        _server.SetConnectedCallBack(std::bind(&HttpServer::OnConnected,this,std::placeholders::_1));
        _server.SetMessageCallBack(std::bind(&HttpServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2));
    }
    void Get(const std::string &pattern,const Handler &handler){ _get_route.push_back(std::make_pair(std::regex(pattern),handler));}
    // void GetExact(const std::string &path,const Handler &handler){ _get_exact_route[path]=handler; }
    void Post(const std::string &pattern,const Handler &handler){ _post_route.push_back(std::make_pair(std::regex(pattern),handler));}
    void Put(const std::string &pattern,const Handler &handler){ _put_route.push_back(std::make_pair(std::regex(pattern),handler));}
    void Delete(const std::string &pattern,const Handler &handler){ _delete_route.push_back(std::make_pair(std::regex(pattern),handler));}
    void SetBasedir(const std::string &path){
        if (!Util::IsDirectory(path)) {
            L_ERROR("SetBasedir failed: %s is not a directory", path.c_str());
            abort();
        }
        _basedir=path;
    }
    void SetThreadCnt(int cnt){_server.SetThreadCnt(cnt);}
    void Listen(){_server.Start();}
};

#endif