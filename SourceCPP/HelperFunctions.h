#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <mutex>


namespace Logger {
	void Init(const std::string& filename);
	void LogMain(const std::string& message);
	void LogError(const std::string& message);
	void LogData(const std::string& message);
	void Shutdown();
}

