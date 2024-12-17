#include "Logger.h"
#include "Timestamp.h"

#include <iostream>




// 获取日志类唯一实例
Logger &Logger::GetInstance()
{
    static Logger log;
    return log;
}
// 设置日志的级别
void Logger::SetLevel(int level)
{
    loglevel_ = level;
}
// 写日志
void Logger::log(std::string msg)
{
    switch (loglevel_)
    {
    case FIFO:
        std::cout << "[FIFO]" ;
        break;
    case ERROR:
        std::cout << "[ERROR]";
        break;
    case FATAL:
        std::cout << "[FATAL]" ;
        break;
    case DEBUG:
        std::cout << "[DEBUG]" ;
        break;
    default:
        break;
    }

    std::cout<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
}