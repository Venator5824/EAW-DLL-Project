// ConfigReader.h
#pragma once
#include "pch.h"
#include <string>
#include <unordered_map>

struct ProjectileData {
	float MinDamageMult = 0.35f;
	float Mult1 = 0.07f;
	float Mult2 = 1.11f;
	float Mult3 = 0.40f;
};

struct Config {
	bool Enabled = true;
	uint8_t DebugLevel = 1;
	std::string LoggingFileName;
};

namespace ConfigReader {
	extern Config GlobalConfig;
	extern std::unordered_map<std::string, ProjectileData> ProjectileMap;
	void LoadConfig(const std::string& filePath);
} 

////EOF