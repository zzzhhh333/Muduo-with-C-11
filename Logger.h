#pragma once

#include <string>

#include "noncopyable.h"
#include <muduo/net/TcpServer.h>

//LOG_FIFO("%s %d",arg1,arg2);

#define LOG_FIFO(logmsgFormat,...)\
    do\
    {\
       Logger &logg=Logger::GetInstance(); \
       logg.SetLevel(FIFO);\
       char buf[1024]={0};\
       snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
       logg.log(buf);\
    }while(0);

#define LOG_ERROR(logmsgFormat,...)\
    do\
    {\
       Logger &logg=Logger::GetInstance(); \
       logg.SetLevel(ERROR);\
       char buf[1024]={0};\
       snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
       logg.log(buf);\
    }while(0);

#define LOG_FATAL(logmsgFormat,...)\
    do\
    {\
       Logger &logg=Logger::GetInstance(); \
       logg.SetLevel(FATAL);\
       char buf[1024]={0};\
       snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
       logg.log(buf);\
       exit(-1);\
    }while(0);

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat,...)\
    do\
    {\
       Logger &logg=Logger::GetInstance(); \
       logg.SetLevel(DEBUG);\
       char buf[1024]={0};\
       snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
       logg.log(buf);\
    }while(0);
#else
    #define LOG_DEBUG(logmsgFormat,...)
#endif



//日志类的级别：FIFO ERROR FATAL DEBUG
enum Level
{
    FIFO,   //正常信息
    ERROR,  //错误信息
    FATAL,  //core信息
    DEBUG,  //调试信息
};




class Logger : noncopyable
{
public:
    //获取日志类唯一实例
    static Logger& GetInstance();
    //设置日志的级别
    void SetLevel(int level);
    //写日志
    void log(std::string msg);
private:
    Logger()=default;
    int loglevel_;

};