#include "pch.h"
#include <thread>
#include "MinHook.h"

#include "ConfigReader.h"
#include "HelperFunctions.h"
#include "Hooks.h"
namespace L = Logger;



void MainControlLoop(HMODULE hModule) {
    std::string LoggingFile = ConfigReader::GlobalConfig.LoggingFileName;
    Logger::Init("VDLLInject.log");
    L::LogMain("SWG_VINJ.dll v0.1 - Working");
    ConfigReader::LoadConfig("SWG_VINJ.xml");
    if (Hooks::Setup()) {
        Logger::LogMain("MAIN: All hooks applied. Engine caputured");
    }
    else {
        L::LogError("MAIN: Hooking failed! Check offsets or admin rights?");
    }
    L::LogMain("MAIN: Initialized and thread finished. Monitoring ...");

    while (true) {
        Sleep(1000);
    }

}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        std::thread(MainControlLoop, hModule).detach();
        break;
    case DLL_PROCESS_DETACH:
        Logger::LogMain("Shutting Down");
        Hooks::Shutdown();
        Logger::Shutdown();
        break;
    }
    return TRUE;
}

