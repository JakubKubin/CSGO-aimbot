#include "memory.h"
#include "vector.h"
#include <iostream>
#include <thread>

namespace offset
{
	constexpr ::std::ptrdiff_t dwLocalPlayer = 0xDEA98C;
	constexpr ::std::ptrdiff_t m_iTeamNum = 0xF4;
	constexpr ::std::ptrdiff_t m_vecOrigin = 0x138;
	constexpr ::std::ptrdiff_t m_vecViewOffset = 0x108;
	constexpr ::std::ptrdiff_t dwClientState = 0x59F19C;
	constexpr ::std::ptrdiff_t dwClientState_GetLocalPlayer = 0x180;
	constexpr ::std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;
	constexpr ::std::ptrdiff_t m_aimPunchAngle = 0x303C;
	constexpr ::std::ptrdiff_t dwEntityList = 0x4DFFF7C;
	constexpr ::std::ptrdiff_t m_bDormant = 0xED;
	constexpr ::std::ptrdiff_t m_lifeState = 0x25F;
	constexpr ::std::ptrdiff_t m_bSpottedByMask = 0x980;
	constexpr ::std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
	constexpr ::std::ptrdiff_t m_bSpotted = 0x93D;
	constexpr ::std::ptrdiff_t m_iHealth = 0x100;
}

constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

int main()
{
	const auto memory = Memory{ "csgo.exe" };
	const auto client = memory.GetModuleAddress("client.dll");
	const auto engine = memory.GetModuleAddress("engine.dll");

	std::cout << "Started working" << std::endl;
	printf("Memory: %x \nClient: %x \nEngine: %x \n",memory,client,engine);

	// aimbot fov
	const float bestFov = 360.f;

	for(;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		// aimbot key
		if (!GetAsyncKeyState(VK_MENU))
			continue;

		// get local player
		const auto& localPlayer = memory.Read<std::uintptr_t>(client + offset::dwLocalPlayer);

		if (localPlayer) {
			const auto& localTeam = memory.Read<int>(localPlayer + offset::m_iTeamNum);
			const auto& localHealth = memory.Read<int>(localPlayer + offset::m_iHealth);
			const auto& clientState = memory.Read<std::uintptr_t>(engine + offset::dwClientState);

			for (auto i = 1; i <= 15; ++i)
			{
				const auto& player = memory.Read<std::uintptr_t>(client + offset::dwEntityList + i * 0x10);

				if (!player)
					continue;

				memory.Write<bool>(player + offset::m_bSpotted, true);

				if (!localHealth)
					continue;

				if (memory.Read<int>(player + offset::m_iTeamNum) == localTeam)
					continue;

				if (memory.Read<bool>(player + offset::m_bDormant))
					continue;

				if (memory.Read<int>(player + offset::m_lifeState) != 0)
					continue;

				if (!memory.Read<bool>(player + offset::m_bSpottedByMask))
					continue;

				const auto& bones = memory.Read<std::uintptr_t>(player + offset::m_dwBoneMatrix);

				const auto& aimPunch = memory.Read<Vector3>(localPlayer + offset::m_aimPunchAngle) * 2;

				const auto& localEyePosition = memory.Read<Vector3>(localPlayer + offset::m_vecOrigin) +
					memory.Read<Vector3>(localPlayer + offset::m_vecViewOffset);

				const auto& viewAngles = memory.Read<Vector3>(clientState + offset::dwClientState_ViewAngles);

				const auto angle = CalculateAngle(
					localEyePosition,
					Vector3{
					memory.Read<float>(bones + 0x30 * 8 + 0x0C),
					memory.Read<float>(bones + 0x30 * 8 + 0x1C),
					memory.Read<float>(bones + 0x30 * 8 + 0x2C)
					},
					viewAngles + aimPunch
				);

				const auto fov = std::hypot(angle.x, angle.y);

				if (fov < bestFov)
				{
					if (!angle.IsZero())
						memory.Write<Vector3>(clientState + offset::dwClientState_ViewAngles, viewAngles + angle);
				}
			}
		}
	}

	return 0;
}
