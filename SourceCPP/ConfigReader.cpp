#include "pch.h"
#include "ConfigReader.h"
#include "HelperFunctions.h"
#include "pugixml.hpp" // Die leichtgewichtige XML-Bibliothek

namespace ConfigReader {
    // 1. Definition der globalen Variablen (Hier wird der Speicher reserviert)
    Config GlobalConfig;
    std::unordered_map<std::string, ProjectileData> ProjectileMap;

    // 2. Die XML-Parsing-Logik
    void LoadConfig(const std::string& filePath) {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filePath.c_str());

        // Wenn die Datei nicht existiert oder kaputt ist, brechen wir sicher ab
        // Die DLL nutzt dann einfach die hardcodierten Standardwerte (Defaults)
        if (!result) {
            Logger::LogError("CONFIG: Failed to load XML! (" + std::string(result.description()) + ") -> Using hardcoded defaults.");
            return;
        }

        Logger::LogMain("CONFIG: XML loaded successfully. Parsing data...");

        // Zum Hauptknoten navigieren
        pugi::xml_node dllData = doc.child("xml").child("DLLData");
        if (!dllData) dllData = doc.child("DLLData"); // Fallback, falls der <xml> Root fehlt

        if (dllData) {
            // --- A. LOGGING DATA LADEN ---
            pugi::xml_node loggingNode = dllData.child("LoggingData");
            if (loggingNode) {
                // .as_uint(1) bedeutet: Nimm den Wert, oder '1' wenn er nicht gelesen werden kann
                GlobalConfig.DebugLevel = (uint8_t)loggingNode.child("uDebugLevel").attribute("value").as_uint(1);
                GlobalConfig.LoggingFileName = loggingNode.child("sLoggingFile").attribute("value").as_string("LogDLLInject.log");
            }

            // --- B. PROJECTILE DATA LADEN ---
            pugi::xml_node projNode = dllData.child("ProjectilesData");
            if (projNode) {
                GlobalConfig.Enabled = projNode.child("Enable").attribute("value").as_bool(true);

                pugi::xml_node itemsNode = projNode.child("Items");
                if (itemsNode) {
                    // Durch alle <Item> Tags iterieren
                    for (pugi::xml_node item = itemsNode.child("Item"); item; item = item.next_sibling("Item")) {
                        std::string key = item.attribute("key").as_string();

                        // Leere Keys ignorieren
                        if (key.empty()) continue;

                        ProjectileData data;
                        // Werte auslesen (mit Fallbacks auf unsere Defaults, falls ein Tag in der XML fehlt)
                        data.MinDamageMult = item.child("fMinDamageMultAbsolute").attribute("value").as_float(0.0f);
                        data.Mult1 = item.child("fMult1").attribute("value").as_float(0.0f);
                        data.Mult2 = item.child("fMult2").attribute("value").as_float(-0.20f);
                        data.Mult3 = item.child("fMult3").attribute("value").as_float(1.0f);

                        // In die Map eintragen
                        ProjectileMap[key] = data;

                        // Wenn DebugLevel hoch genug ist, loggen wir den erfolgreichen Import
                        if (GlobalConfig.DebugLevel >= 2) {
                            Logger::LogData("Loaded Projectile: " + key + " [m1: " + std::to_string(data.Mult1) + ", m2: " + std::to_string(data.Mult2) + ", m3: " + std::to_string(data.Mult3) + "]");
                        }
                    }
                }
            }
        }

        Logger::LogMain("CONFIG: Parsing complete. Mapped " + std::to_string(ProjectileMap.size()) + " custom projectiles.");
    }
}