#include "logger.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace IkarusLogger;

void expectedUsage()
{
}

void threadFunc()
{
    logObj().log("Test Log");

    logObj(LogLevel::Error) << "Error Log" << 11 << IkarusLogger::endl;
}

void threadFunc2()
{
    static int thread_num = 0;

    for(int i = 0;i < 10; i++)
    {
        logObj() << "Test Thread #" << ++thread_num;
        logObj() << "Iteration #" << i+1 << IkarusLogger::endl;
    }
}

int main()
{
    std::cout << "Started logging" << std::endl;

    Logger& logger = Logger::get();
    logger.init("Logs.txt", LogOutput::Everywhere);
    logger.open();

    logger.log("test log", LogLevel::Debug);

    std::vector<std::thread> threads;
    for(int i = 0; i < 10; i++)
    {
        threads.emplace_back(std::thread(i % 2 ? threadFunc : threadFunc2));
    }

    threadFunc();

    logger << 12345;

    for(auto& th : threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }

    logger.close();

    return 0;
}
