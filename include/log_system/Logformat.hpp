/*
日志消息格式化类
1.定义日志格式化子项基类
2.定义各种日志格式化子项类
成员：格式化字符串，格式化子项数组
*/
#pragma once
#include<iostream>
#include<ctime>
#include "utility.hpp"
#include "message.hpp"
#include "level.hpp"
#include<memory>
#include<vector>
#include<cassert>
#include<sstream>

namespace lcz
{ 
    //抽象格式化子项基类
      class FormatItem
      {
        public:
            using ptr=std::shared_ptr<FormatItem>;//便于用指针访问
            virtual ~FormatItem(){}
            virtual void format(std::ostream &os,const Logmsg &msg)=0;
      };
      //派生类格式化子项子类-消息，等级，时间，文件名，行号，线程id，日志器名称，制表符，换行，其他
      class MsgFormatItem:public FormatItem
      {
        public:
          virtual void format(std::ostream &os,const Logmsg &msg)override
          {
                os<<msg._payload;
          }
      };
      class LevelFormatItem:public FormatItem
      {
        public:
          virtual void format(std::ostream &os,const Logmsg &msg)override
          {
              os<<LogLevel::toString(msg._level);
          }
      };
      class TimeFormatItem:public FormatItem
      {
        public:
        TimeFormatItem(const std::string &fmt="%H:%M:%S")
            :_time_format(fmt)
            {}
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
            struct tm t;//时间结构体
            localtime_r(&msg._time,&t);
            char buf[64]={0};
            strftime(buf,63,_time_format.c_str(),&t);//格式化时间
            os<<buf;//将时间放进流中
        }
       private:
            std::string _time_format;//时间格式
      };
      
      class FileFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
            os<<msg._file;
        }
      }; 
      class LineFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
            os<<msg._line;
        }
      }; 
      class ThreadFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
            os<<msg._tid;
        }
      }; 
      class LoggerFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
           os<<msg._logger;
        }
      }; 
      class TabFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
              os<<"\t";
        }
        
      }; 
      class NlineFormatItem:public FormatItem
      {
        public:
        virtual void format(std::ostream &os,const Logmsg &msg)override
        {
              os<<"\n";
        }
        
      }; 
      //解析出来的其他字符串
      class OtherFormatItem:public FormatItem
      {
        public:
        OtherFormatItem(const std::string &str)
            :_str(str)
            {}
         virtual void format(std::ostream &os,const Logmsg &msg)override
        {
             os<<_str;
        }
        private:
            std::string _str;
      };
      //日志格式化类
      class Formatter
      {
                        /*
                            %d ⽇期
                            %T 缩进
                            %t 线程id
                            %p ⽇志级别
                            %c ⽇志器名称
                            %f ⽂件名
                            %l ⾏号
                            %m ⽇志消息
                            %n 换⾏
                        */
        public:
        using ptr=std::shared_ptr<Formatter>;//用于在日志器对格式化模块进行管理
        Formatter(const std::string &pattern="[%d{%H:%M:%S}][%t][%c][%f:%l][%p]%T%m%n")
        :_pattern(pattern)
        {
            assert(parsePattern());
        }
        //对msg进行格式化，放进流中
        void format(std::ostream &os,const Logmsg &msg)
        {
          // 空items时输出原始消息
          if (_items.empty()) 
          {
            os << msg._payload;
            return;
           }
            for(auto &ite:_items)   //遍历格式化子项数组
            {
                ite->format(os,msg);
            }

        }
        std::string format(const Logmsg &msg)
        {

            std::stringstream ss;
            format(ss,msg);
            return ss.str();//返回格式化后的字符串
        }
        private:
        //解析_pattern
        bool parsePattern()
        {
          //先对_pattern进行解析，生成_format_items数组
          //解析规则：遇到%说明是格式化子项，%后面第一个字符是格式化字符，可能后面跟着{}表示格式化参数
          //不是%说明是普通字符串
          //例如[%d{%Y-%m-%d %H:%M:%S}][%p]%T%m%n
          //解析后生成的_format_items数组为：
          //OtherFormatItem("[")
          //TimeFormatItem("%Y-%m-%d %H:%M:%S")
          //OtherFormatItem("][")
          //LevelFormatItem()
          //OtherFormatItem("]")
          //TabFormatItem()
          //MsgFormatItem()
          //NlineFormatItem()
          //遍历_pattern字符串
          size_t pos=0;
          std::vector<std::pair<std::string,std::string>> fmt_order;//存放解析出来的格式化字符和参数
          std::string key,val;
          while(pos<_pattern.size())
          {
            if(_pattern[pos]!='%')
            {
              val.push_back(_pattern[pos++]);
              continue;
            }
            //走到这里说明遇到了%
            if(pos+1<_pattern.size()&&_pattern[pos+1]=='%')//如果是%%说明是普通的%，不是格式化子项
            {
              val.push_back('%');
              pos+=2;
              continue;
            }
            //走到这里说明是格式化子项,%后面第一个字符是格式化字符
            if(val.size())
            {
              fmt_order.push_back(std::make_pair("other",val));
              val.clear();
            }
            pos++;
            if(pos==_pattern.size())//说明%后面没有字符了，格式化字符串错误
            {
              std::cerr<<"pattern parse error: %unwell"<<_pattern<<" "<<pos<<std::endl;
              return false;
            }
            key.push_back(_pattern[pos++]);//格式化字符
            //看看后面是否有{}表示格式化参数
            if(pos<_pattern.size()&&_pattern[pos]=='{')//说明有格式化参数
            {
              pos++;//跳过{
              while(pos<_pattern.size()&&_pattern[pos]!='}')
              {
                val.push_back(_pattern[pos++]);
              }
              //走到末尾跳出循环，则代表没有找到}
               if(pos==_pattern.size())
               {
                  std::cerr<<"pattern parse error: {}unwell"<<_pattern<<" "<<pos<<std::endl;
                  return false;
               }
                pos++;//跳过}
              
            }
            //将key和val存入fmt_order
            fmt_order.push_back(std::make_pair(key,val));
            key.clear();
            val.clear();
          }

          //根据fmt_order生成_items数组
          for(auto &it:fmt_order)
          {
            _items.push_back(createFormatItem(it.first,it.second));
          }
          return true;
        }
        private:
        //根据不同的格式化字符创建不同的格式化子项对象
        FormatItem::ptr createFormatItem(const std::string &key,const std::string &val)
        {
           if (key == "d") return std::make_shared<TimeFormatItem>(val);
            else if (key == "T") return std::make_shared<TabFormatItem>();
            else if (key == "t") return std::make_shared<ThreadFormatItem>();
            else if (key == "p") return std::make_shared<LevelFormatItem>();
            else if (key == "c") return std::make_shared<LoggerFormatItem>();
            else if (key == "f") return std::make_shared<FileFormatItem>();
            else if (key == "l") return std::make_shared<LineFormatItem>();
            else if (key == "m") return std::make_shared<MsgFormatItem>();
            else if (key == "n") return std::make_shared<NlineFormatItem>();
            else if (key == "other") return std::make_shared<OtherFormatItem>(val);
            
            return std::make_shared<OtherFormatItem>("%" + key + val);
        }
        private: 
            std::string _pattern;//格式化规则字符串
            std::vector<FormatItem::ptr> _items;//格式化子项数组
      };
}