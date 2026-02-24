#include "pch.h"
#include "MinHook.h"
#include "HelperFunctions.h"
#include "ConfigReader.h"
#include "Hooks.h"
#include <iostream>
#include <intrin.h>
#include <cmath>

namespace Hooks {
    // 1. DEFINITIONEN
    typedef void(__fastcall* TakeDamage_Fn)(void*, unsigned int, float, void*, void*, void*, float*, char, char, int, int, void*, char, void*);
    TakeDamage_Fn fpTakeDamageOriginal = nullptr;

    // Sicherheits-Check: Prüft, ob ein Speicherbereich lesbar ist
    bool IsPointerSafe(void* ptr, size_t size) {
        if (!ptr) return false;
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
        return true;
    }

    std::string GetProjectileName(void* proj) {
        if (!proj) return "";
        uintptr_t p = (uintptr_t)proj;
        if (!IsPointerSafe((void*)(p + 0x298), 8)) return "";
        uintptr_t typePtr = *(uintptr_t*)(p + 0x298);
        if (!typePtr || !IsPointerSafe((void*)typePtr, 0x200)) return "";

        size_t len = *(size_t*)(typePtr + 0x110);
        if (len == 0) return "";
        if (len > 15) {
            char* longName = *(char**)(typePtr + 0xF8);
            return (longName && IsPointerSafe(longName, 1)) ? std::string(longName) : "";
        }
        return std::string((char*)(typePtr + 0xF8));
    }

    // 2. DER HOOK (KOMPLETT MIT DEINER LOGIK)
    void __fastcall Detour_Take_Damage(void* target, unsigned int dmgType, float damage, void* attacker,
        void* proj_string, void* p6, float* p7, char p8, char p9,
        int p10, int p11, void* p12, char p13, void* p14)
    {
        float finalDamage = damage;
        bool wasProcessed = false;

        if (attacker != nullptr && IsPointerSafe(attacker, 0x300)) {
            uintptr_t pAtt = (uintptr_t)attacker;
            uintptr_t typePtr = *(uintptr_t*)(pAtt + 0x298);

            if (typePtr != 0 && IsPointerSafe((void*)typePtr, 0x2000)) {
                std::string name = GetProjectileName(attacker);
                int rawCatValue = *(int*)(typePtr + 0x1FE8);
                auto it = ConfigReader::ProjectileMap.find(name);

                // Log: Prozess gestartet
                if (it != ConfigReader::ProjectileMap.end() || rawCatValue == 7) {
                    Logger::LogData(">>> HOOK START: " + name + " (Dmg: " + std::to_string(damage) + ")");

                    float maxD = *(float*)(typePtr + 0x4BC);

                    // Koordinaten für Distanzberechnung ziehen
                    float ax = *(float*)(pAtt + 0x78);
                    float ay = *(float*)(pAtt + 0x7C);
                    float az = *(float*)(pAtt + 0x80);
                    float bx = *(float*)(pAtt + 0x9C);
                    float by = *(float*)(pAtt + 0xA0);
                    float bz = *(float*)(pAtt + 0xA4);

                    float dx = bx - ax;
                    float dy = by - ay;
                    float dz = bz - az;
                    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

                    if (maxD > 1.0f && dist > 1.0f && std::isfinite(dist)) {
                        float r = 1.0f - (dist / maxD);
                        if (r > 1.0f) r = 1.0f; if (r < 0.0f) r = 0.0f;

                        float m1, m2, m3, minDM;
                        if (it != ConfigReader::ProjectileMap.end()) {
                            m1 = it->second.Mult1; m2 = it->second.Mult2; m3 = it->second.Mult3;
                            minDM = it->second.MinDamageMult;
                        }
                        else {
                            m1 = 0.0f; m2 = -0.15f; m3 = 1.15f; minDM = 0.5f;
                        }

                        float mult = (m1 * (r * r)) + (m2 * r) + m3;
                        if (mult < minDM) mult = minDM;
                        finalDamage *= mult;

                        // Log: Berechnungsergebnis
                        char info[256];
                        sprintf_s(info, "--- CALC: Dist: %.1f/%.1f | Mult: %.2f | FinalDmg: %.1f",
                            dist, maxD, mult, finalDamage);
                        Logger::LogData(info);
                        wasProcessed = true;
                    }

                    if (wasProcessed) Logger::LogData("<<< HOOK FINISHED");
                }
            }
        }
        
        fpTakeDamageOriginal(target, dmgType, finalDamage, attacker, proj_string, p6, p7, p8, p9, p10, p11, p12, p13, p14);
    }

    // 3. SETUP & SHUTDOWN
    bool Setup() {
        if (MH_Initialize() != MH_OK) return false;
        uintptr_t target = (uintptr_t)GetModuleHandle(NULL) + 0x3A9E30;
        if (MH_CreateHook((LPVOID)target, &Detour_Take_Damage, (LPVOID*)&fpTakeDamageOriginal) != MH_OK) return false;
        if (MH_EnableHook((LPVOID)target) != MH_OK) return false;
        return true;
    }

    void Shutdown() {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }

    // checks all hardpoints health and names of ship X
    void CheckShipHardpoints(void* target, bool print) {
        // 1. Validate the ship pointer
        if (!target || !IsPointerSafe(target, 0x300)) return;

        uintptr_t pTarget = (uintptr_t)target;

        // 2. Locate the Hardpoint Manager (Offset 0x2D0 from Ghidra)
        uintptr_t hpManager = *(uintptr_t*)(pTarget + 0x2D0);
        if (!hpManager || !IsPointerSafe((void*)hpManager, 0x20)) return;

        // 3. Get Array and Count
        int hpCount = *(int*)(hpManager + 0x10);
        uintptr_t hpArray = *(uintptr_t*)(hpManager + 0x8);

        // Sanity check on count to prevent infinite loops from garbage memory
        if (hpCount <= 0 || hpCount > 100 || !IsPointerSafe((void*)hpArray, hpCount * 8)) return;

        // 4. Iterate and evaluate
        for (int i = 0; i < hpCount; i++) {
            uintptr_t hpInstance = *(uintptr_t*)(hpArray + (i * 8));
            if (!hpInstance || !IsPointerSafe((void*)hpInstance, 0x30)) continue;

            // Health is at 0x28
            float hpHealth = *(float*)(hpInstance + 0x28);

            // Definition is at 0x20
            uintptr_t hpDef = *(uintptr_t*)(hpInstance + 0x20);
            if (!hpDef || !IsPointerSafe((void*)hpDef, 0xC0)) continue;

            // Extract Name (std::string with Short String Optimization)
            std::string hpName = "";
            size_t strLen = *(size_t*)(hpDef + 0xB8);

            if (strLen > 15) {
                char* longStr = *(char**)(hpDef + 0xA0);
                if (IsPointerSafe(longStr, 1)) hpName = longStr;
            }
            else {
                hpName = (char*)(hpDef + 0xA0);
            }

            // 5. Print out the status!
            if (print) {
                if (!hpName.empty()) {
                    if (hpHealth <= 0.0f) {
                        Logger::LogData(">>> HARDPOINT TEST: " + hpName + " is DESTROYED (0.0)");
                    }
                    else {
                        Logger::LogData(">>> HARDPOINT TEST: " + hpName + " is ALIVE (Health: " + std::to_string(hpHealth) + ")");
                    }
                }
            }
        }
    }

    std::vector<HardpointStatus> GetShipHardpointList(void* target) {
        std::vector<HardpointStatus> list;
        if (!target || !IsPointerSafe(target, 0x300)) return list;

        uintptr_t pTarget = (uintptr_t)target;
        uintptr_t hpManager = *(uintptr_t*)(pTarget + 0x2D0);
        if (!hpManager || !IsPointerSafe((void*)hpManager, 0x20)) return list;

        int hpCount = *(int*)(hpManager + 0x10);
        uintptr_t hpArray = *(uintptr_t*)(hpManager + 0x8);
        if (hpCount <= 0 || hpCount > 100 || !IsPointerSafe((void*)hpArray, hpCount * 8)) return list;

        for (int i = 0; i < hpCount; i++) {
            uintptr_t hpInstance = *(uintptr_t*)(hpArray + (i * 8));
            if (!hpInstance || !IsPointerSafe((void*)hpInstance, 0x30)) continue;

            uintptr_t hpDef = *(uintptr_t*)(hpInstance + 0x20);
            if (!hpDef) continue;

            // Name extrahieren (SSO Handling)
            std::string hpName = "";
            size_t strLen = *(size_t*)(hpDef + 0xB8);
            if (strLen > 15) {
                char* longStr = *(char**)(hpDef + 0xA0);
                if (IsPointerSafe(longStr, 1)) hpName = longStr;
            }
            else {
                hpName = (char*)(hpDef + 0xA0);
            }

            list.push_back({ hpName, *(float*)(hpInstance + 0x28), hpInstance });
        }
        return list;
    }

    // gets hardpoint string name of ship X
    float GetHardpointHealthByName(void* target, std::string targetName) {
        if (!target || !IsPointerSafe(target, 0x300)) return -1.0f;

        uintptr_t pTarget = (uintptr_t)target;
        uintptr_t hpManager = *(uintptr_t*)(pTarget + 0x2D0);

        if (!hpManager || !IsPointerSafe((void*)hpManager, 0x20)) return -1.0f;

        int hpCount = *(int*)(hpManager + 0x10);
        uintptr_t hpArray = *(uintptr_t*)(hpManager + 0x8);

        if (hpCount <= 0 || hpCount > 100 || !IsPointerSafe((void*)hpArray, hpCount * 8)) return -1.0f;

        for (int i = 0; i < hpCount; i++) {
            uintptr_t hpInstance = *(uintptr_t*)(hpArray + (i * 8));
            if (!hpInstance || !IsPointerSafe((void*)hpInstance, 0x30)) continue;

            uintptr_t hpDef = *(uintptr_t*)(hpInstance + 0x20);
            if (!hpDef || !IsPointerSafe((void*)hpDef, 0xC0)) continue;

            std::string currentName = "";
            size_t strLen = *(size_t*)(hpDef + 0xB8);
            if (strLen > 15) {
                char* longStr = *(char**)(hpDef + 0xA0);
                if (IsPointerSafe(longStr, 1)) currentName = longStr;
            }
            else {
                currentName = (char*)(hpDef + 0xA0);
            }

            if (currentName == targetName) {
                return *(float*)(hpInstance + 0x28);
            }
        }

        return -1.0f; 
    }

}