#include "pch.h"         
#include "ConfigReader.h"
#include <fstream>      
#include <mutex>       
#include <chrono>      
#include <iomanip>    
#include <sstream>  
#include <string>  

namespace Logger {
	std::ofstream logFile;
	std::mutex logMutex;
    std::string GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss; // Jetzt durch <sstream> bekannt
        tm time_info;
        localtime_s(&time_info, &in_time_t);
        ss << std::put_time(&time_info, "%Y-%m-%d %X");
        return ss.str();
    }

    void Init(const std::string& filename) {
        std::lock_guard<std::mutex> lock(logMutex);
        logFile.open(filename, std::ios::out | std::ios::trunc);
        if (logFile.is_open()) {
            logFile << "[" << GetTimestamp() << "] SYSTEM: Logger initialized." << std::endl;
        }
    }

    void LogMain(const std::string& text) {
        // Level 1: Allgemeine Infos und Fehler
        if (ConfigReader::GlobalConfig.DebugLevel >= 1) {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open()) {
                logFile << "[" << GetTimestamp() << "] MAIN: " << text << std::endl;
            }
        }
    }

    void LogData(const std::string& text) {
        // Level 2: Volles Daten-Logging (jeder Treffer)
        if (ConfigReader::GlobalConfig.DebugLevel >= 2) {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open()) {
                logFile << "[" << GetTimestamp() << "] DATA: " << text << std::endl;
            }
        }
    }

    void LogError(const std::string& text) {
        // Fehler loggen wir immer ab Level 1
        if (ConfigReader::GlobalConfig.DebugLevel >= 1) {
            std::lock_guard<std::mutex> lock(logMutex);
            if (logFile.is_open()) {
                logFile << "[" << GetTimestamp() << "] ERROR: " << text << std::endl;
            }
        }
    }

    void Shutdown() {
        LogMain("Shutting down"); // FIX: LogMain statt LogGeneral
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
    }

}