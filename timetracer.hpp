
#ifndef TRACERLIB_TIMETRACER_H
#define TRACERLIB_TIMETRACER_H

#ifdef TRACE
#define GET_MACRO(_0, _1, NAME, ...) NAME
#define TIME_TRACE(...) GET_MACRO(_0, ##__VA_ARGS__, TIME_TRACE_1, TIME_TRACE_0)(__VA_ARGS__)
#define TIME_TRACE_0() TracerLib::TimeTracer _TimeTracer__LINE__(__FILE__,__func__,__LINE__)
#define TIME_TRACE_1(TAG_NAME) TracerLib::TimeTracer _TimeTracer__LINE__(__FILE__,TAG_NAME,__LINE__)
#define TIME_TRACER_ENABLE(FILENAME) TracerLib::TimeTracer::enable(FILENAME)
#define TIME_TRACER_DISABLE TracerLib::TimeTracer::disable()
#define TIME_TRACE_VALUE(NAME, VALUE) TracerLib::TimeTracer::traceQuantity(NAME, VALUE)
#else
#define TIME_TRACE()
#define TIME_TRACE(TAG_NAME) 
#define TIME_TRACER_ENABLE(FILENAME)
#define TIME_TRACER_DISABLE
#define TIME_TRACE_VALUE(NAME, VALUE)
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include "omp.h"

namespace TracerLib
{

class TimeTracer {
public:
    TimeTracer(const std::string& file, const std::string& function, int lineNumber);
    ~TimeTracer();
    static void enable(const std::string& jsonFileName);
    static void enable();
    static void disable();
    static void start(const std::string& jsonFileName);
    static void start();
    static void stop();
    static void traceQuantity(const std::string& name, long val);
    bool printToTerminalIsSet();
    void setPrintToTerminal(bool s);
private:
    static void writeHeader();
    static void writeFooter();
    static void writeToFile(const std::string& str);
    static void setZeroTime();
    static long long getTimeNow();
    void initialize();
    void formatFileName();
    void formatFunctionName();
    void registerConstructionTime();
    void registerFinalTime();
    void registerThreadId();
    void calculateDuration();
    void printDurationMiliSec();
    void writeBeginEvent();
    void writeEndEvent();

    static bool ms_bIsEnabled;                  /**< Bool variable to store if the "class" (ie. the TimeTracer) has been enabled. */
    static std::ofstream ms_OutputFileStream;   /**< Output file stream to write results. */
    static bool ms_bIsFirstEvent;               /**< Bool variable to check if this is the first event to be written to the file. */
    static std::mutex ms_outFileMutex;          /**< Mutex to guard the writing to file between threads. */
    static long long ms_iZeroTime;              /**< Integer value to store the origin-time (ie the Zero time) from which all other time measurements will depend. */

    bool m_bIsInitialized;          /**< Store if this object has been initialized properly. */
    bool m_bPrintToTerminal;        /**< Store if it is needed from this TimeTracer object. to print to terminal too. */
    std::string m_sFileName;        /**< String to store the code file name where the TimeTracer obj is instantiated. */
    std::string m_sFunctionName;    /**< String to store the function name. */
    int m_iLineNumber;              /**< The line number within the code file where the TimeTracer obj is instantiated. */
    std::string m_iThreadId;        /**< A string identifier for the thread id in which the TimeTracer obj is instantiated. */
    long long m_iBeginTime;         /**< The time when the tracer TimeTracer obj is created. */
    long long m_iEndTime;           /**< The time when the tracer TimeTracer obj is destructed. */
    double m_dDurationMillis;        /**< The time difference between TimeTracer object creation and destruction in in milli seconds. */

}; // TimeTracer

static const char* defaultTracerFileName("default_TimeTracer_file.json");
bool TimeTracer::ms_bIsEnabled(false);
std::ofstream TimeTracer::ms_OutputFileStream;
bool TimeTracer::ms_bIsFirstEvent(true);
std::mutex TimeTracer::ms_outFileMutex;
long long TimeTracer::ms_iZeroTime(0);

TimeTracer::TimeTracer(const std::string &file, const std::string &function, int lineNumber)
: m_bIsInitialized(false)
, m_bPrintToTerminal(false)
, m_sFileName(file)
, m_sFunctionName(function)
, m_iLineNumber(lineNumber)
, m_iThreadId("0")
, m_iBeginTime(0)
, m_iEndTime(0)
, m_dDurationMillis(0.)
{
    if (ms_bIsEnabled)
    {
        initialize();
        writeBeginEvent();
     }
}

//=============================================================================================================

TimeTracer::~TimeTracer()
{
    if (ms_bIsEnabled && m_bIsInitialized)
    {
        registerFinalTime();
        writeEndEvent();
        if (m_bPrintToTerminal)
        {
            calculateDuration();
            printDurationMiliSec();
        }
    }
}

//=============================================================================================================

void TimeTracer::enable(const std::string &jsonFileName)
{
    ms_OutputFileStream.open(jsonFileName);
    writeHeader();
    setZeroTime();
    if (ms_OutputFileStream.is_open())
    {
        ms_bIsEnabled = true;
    }
}

//=============================================================================================================

void TimeTracer::enable()
{
    enable(defaultTracerFileName);
}

//=============================================================================================================

void TimeTracer::disable()
{
    if (ms_bIsEnabled)
    {
        writeFooter();
        ms_OutputFileStream.flush();
        ms_OutputFileStream.close();
        ms_bIsEnabled = false;
    }
}

//=============================================================================================================

void TimeTracer::start(const std::string &jsonFileName)
{
    enable(jsonFileName);
}

//=============================================================================================================

void TimeTracer::start()
{
    enable();
}

//=============================================================================================================

void TimeTracer::stop()
{
    disable();
}

//=============================================================================================================

void TimeTracer::traceQuantity(const std::string &name, long val)
{
    long long timeNow = getTimeNow() - ms_iZeroTime;
    std::string s;
    s.append("{\"name\":\"").append(name).append("\",\"ph\":\"C\",\"ts\":");
    s.append(std::to_string(timeNow)).append(",\"pid\":1,\"tid\":1");
    s.append(",\"args\":{\"").append(name).append("\":").append(std::to_string(val)).append("}}\n");
    writeToFile(s);
}

//=============================================================================================================

void TimeTracer::initialize()
{
    registerConstructionTime();
    registerThreadId();
    formatFileName();
    m_bIsInitialized = true;
}

//=============================================================================================================

void TimeTracer::setZeroTime()
{
    ms_iZeroTime = getTimeNow();
}

//=============================================================================================================

void TimeTracer::registerConstructionTime()
{
    m_iBeginTime = getTimeNow() - ms_iZeroTime;
}

//=============================================================================================================

void TimeTracer::registerFinalTime()
{
    m_iEndTime = getTimeNow() - ms_iZeroTime;
}

//=============================================================================================================

long long TimeTracer::getTimeNow()
{
    auto timeNow = std::chrono::high_resolution_clock::now();
    return std::chrono::time_point_cast<std::chrono::microseconds>(timeNow).time_since_epoch().count();
}

//=============================================================================================================

void TimeTracer::registerThreadId()
{
    auto longId = std::hash<std::thread::id>()(std::this_thread::get_id());
    // int longId = omp_get_thread_num();
    m_iThreadId = std::to_string(longId).substr(0, 5);
}

//=============================================================================================================

void TimeTracer::formatFunctionName()
{
    const char* pattern(" __cdecl");
    const int patternLength(8);
    size_t pos = m_sFunctionName.find(pattern);
    if (pos != std::string::npos) {
        m_sFunctionName.replace(pos, patternLength, "");
    }
}

//=============================================================================================================

void TimeTracer::formatFileName()
{
    const char* patternIn("\\");
    const char* patternOut("\\\\");
    const int patternOutLength(4);
    size_t start_pos = 0;
    while ((start_pos = m_sFileName.find(patternIn, start_pos)) != std::string::npos)
    {
        m_sFileName.replace(start_pos, 1, patternOut);
        start_pos += patternOutLength;
    }
}

//=============================================================================================================

void TimeTracer::calculateDuration()
{
    m_dDurationMillis = (m_iEndTime - m_iBeginTime) * 0.001;
}

//=============================================================================================================

void TimeTracer::printDurationMiliSec()
{
    std::cout << "Scope: " << m_sFileName << " - " << m_sFunctionName << " DurationMs: " << m_dDurationMillis << "ms.\n";
}

//=============================================================================================================

void TimeTracer::writeHeader()
{
    writeToFile("{\"displayTimeUnit\": \"ms\",\"traceEvents\":[\n");
}

//=============================================================================================================

void TimeTracer::writeFooter()
{
    writeToFile("]}");
}

//=============================================================================================================

void TimeTracer::writeToFile(const std::string& str)
{
    ms_outFileMutex.lock();
    if(ms_OutputFileStream.is_open()) {
        ms_OutputFileStream << str;
    }
    ms_outFileMutex.unlock();
}

//=============================================================================================================

void TimeTracer::writeBeginEvent()
{
    std::string s;
    if (!ms_bIsFirstEvent)
        s.append(",");

    s.append("{\"name\":\"").append(m_sFunctionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"B\",\"ts\":").append(std::to_string(m_iBeginTime)).append(",\"pid\":1,\"tid\":");
    s.append(m_iThreadId).append(",\"args\":{\"file path\":\"").append(m_sFileName).append("\",\"line number\":");
    s.append(std::to_string(m_iLineNumber)).append("}}\n");
    writeToFile(s);
    ms_bIsFirstEvent = false;
}

//=============================================================================================================

void TimeTracer::writeEndEvent()
{
    std::string s;
    s.append(",{\"name\":\"").append(m_sFunctionName).append("\",\"cat\":\"bst\",");
    s.append("\"ph\":\"E\",\"ts\":").append(std::to_string(m_iEndTime)).append(",\"pid\":1,\"tid\":");
    s.append(m_iThreadId).append(",\"args\":{\"file path\":\"").append(m_sFileName).append("\",\"line number\":");
    s.append(std::to_string(m_iLineNumber)).append("}}\n");
    writeToFile(s);
    
}

//=============================================================================================================

bool TimeTracer::printToTerminalIsSet()
{
    return m_bPrintToTerminal;
}

//=============================================================================================================

void TimeTracer::setPrintToTerminal(bool s)
{
    m_bPrintToTerminal = s;
}

} // namespace TRACERLIB

#endif //if TRACERLIB_TRACER_H defined
