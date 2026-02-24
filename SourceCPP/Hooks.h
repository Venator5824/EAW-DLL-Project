#pragma once

namespace Hooks {
	void CheckShipHardpoints(void* target);
	bool IsPointerSafe(void* ptr, size_t size);
	void __fastcall Detour_Take_Damage(void* target, unsigned int dmgType, float damage, void* attacker,
		void* proj_string, void* p6, float* p7, char p8, char p9,
		int p10, int p11, void* p12, char p13, void* p14);
	std::string GetProjectileName(void* proj);
	bool Setup();
	void Shutdown();
}

struct HardpointStatus {
	std::string name;
	float health;
	uintptr_t instancePtr; 
};