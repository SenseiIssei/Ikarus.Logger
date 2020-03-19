#pragma once

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <chrono>
#include <ctime>
#include <thread>
#include <mutex>

namespace IkarusLogger
{

enum class LogLevel
{
    Info,
    Debug,
    Error,
    Warning,
    Fatal
};

enum class LogOutput
{
    Console,
    File,
    Everywhere
};

std::string logToString(const LogLevel level)
{
    std::map<LogLevel, std::string> M_LEVEL_MAP =
    {
        { LogLevel::Info, "INFO" },
        { LogLevel::Debug, "DEBUG" },
        { LogLevel::Warning, "WARNING" },
        { LogLevel::Error, "ERROR" },
        { LogLevel::Fatal, "FATAL" }
    };

    std::string result = "NONE";
    auto it = M_LEVEL_MAP.find(level);
    if(it != M_LEVEL_MAP.end())
    {
        result = it->second;
    }
    return result;
}

const std::string endl = "\n";

class Logger
{
private:
    std::mutex m_logMutex;
    std::mutex m_threadChunkMutex;

private:
    static std::string timestamp()
    {
        using namespace std::chrono;

        auto now = system_clock::now();
        time_t tt = system_clock::to_time_t(now);

        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&tt), "%F %T");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();

        return ss.str();
    }

    static std::string threadId()
    {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        return ss.str();
    }

    static std::string wrapBlock(const std::string& value, const std::string& prefix = "[", const std::string& sufix = "]")
    {
        return prefix + value + sufix;
    }

private:
    Logger()
    {

    }

    ~Logger()
    {

    }

public:
    static Logger& get()
    {
        static Logger self;
        return self;
    }

public:

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void init(const std::string& fileName,
              const LogOutput output = LogOutput::Everywhere)
    {
        m_fileName = fileName;
        m_output = output;
    }

    void open()
    {
        if(m_isOpened)
        {
            return;
        }

        if(m_output == LogOutput::Everywhere || m_output == LogOutput::File)
        {
            m_file.open(m_fileName, std::ios::out);
            m_isOpened = m_file.is_open();

            if(!m_isOpened)
            {
                throw std::runtime_error("Couldn't open a log file!");
            }
        }

        m_isOpened = true;
    }

    void close()
    {
        if(!m_isOpened)
        {
            return;
        }

        if(m_output == LogOutput::Everywhere || m_output == LogOutput::File)
        {
            m_file.close();
        }
    }

    void flush()
    {
        if(!m_isOpened)
        {
            return;
        }

        {
            std::unique_lock<std::mutex> locker(m_threadChunkMutex);
            for(const auto& elem : m_threadLinesMap)
            {
                log(elem.second.rdbuf(), m_threadsLevelMap[elem.first]);
            }
            m_threadLinesMap.clear();
            m_threadsLevelMap.clear();
        }

        if(m_output == LogOutput::Everywhere || m_output == LogOutput::File)
        {
            m_file.flush();
        }
    }

    void log(const std::string& msg, const LogLevel level = LogLevel::Debug)
    {
        std::string result = wrapBlock(timestamp()) +
                " " + wrapBlock(threadId()) +
                " " + wrapBlock(logToString(level)) +
                " " + msg;

        std::unique_lock<std::mutex> locker(m_logMutex);
        if(m_output == LogOutput::Console)
        {
            std::cout << result << std::endl;
        }
        else if(m_output == LogOutput::File)
        {
            m_file << result << std::endl;
        }
        else
        {
            std::cout << result << std::endl;
        }
    }

    template<class T>
    void log(const T& t, const LogLevel level= LogLevel::Debug)
    {
        std::stringstream ss;
        ss << t;
        log(ss.str(), level);
    }

    void addChunk(const std::string& chunk)
    {
        std::unique_lock<std::mutex> locker(m_threadChunkMutex);
        m_threadLinesMap[threadId()] << chunk;
    }

    void flushChunk()
    {
        const std::string threadID = threadId();
        std::unique_lock<std::mutex> locker(m_threadChunkMutex);
        log(m_threadLinesMap[threadID].rdbuf(), m_threadsLevelMap[threadID]);

        m_threadLinesMap.erase(threadID);
        m_threadsLevelMap.erase(threadID);
    }

    Logger& operator()(const LogLevel level)
    {
        std::unique_lock<std::mutex> locker(m_threadChunkMutex);
        m_threadsLevelMap[threadId()] = level;
        return get();
    }

private:
    std::string m_fileName;
    std::fstream m_file;
    LogOutput m_output = LogOutput::Everywhere;

    std::map<std::string, LogLevel> m_threadsLevelMap;
    std::map<std::string, std::stringstream> m_threadLinesMap;


    bool m_isOpened = false;
};

Logger& operator<<(Logger& logger, const std::string& chunk)
{
    if(chunk == IkarusLogger::endl)
    {
        logger.flushChunk();
    }
    else
    {
        logger.addChunk(chunk);
    }
    return logger;
}

template<class T>
Logger& operator<<(Logger& logger, const T& t)
{
    std::stringstream ss;
    ss << t;
    logger.addChunk(ss.str());
    return logger;
}

Logger& logObj(const LogLevel level = LogLevel::Debug)
{
    return Logger::get()(level);
}

}
