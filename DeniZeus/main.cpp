#include <stdio.h>
#include "stdafx.h"
#include "memory.h"
#include <iostream>
#include <string>
#include <thread>
#include <future>
#include <d3d9.h>
#include <io.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include "JSON.h"
#include <urlmon.h>
#include <cstdlib>
#include "FindPattern.h"
#pragma comment(lib, "urlmon.lib")

//Netvar Downloader Start
int dwGlowObjectManager;
int dwlocalPlayer;
int dwForceJump;
int clientState;
int forceAttack;
int entityList;
int clientAngle;
int aimPunch;
int glowIndex;
int iTeamNum;
int vecOrigin;
int vecViewOffset;
int SpottedByMask;
int iHealth;
int fFlags;
int boneMatrix;
int m_flFallbackWear;
int m_nFallbackPaintKit;
int m_iItemIDHigh;
int m_iEntityQuality;
int m_iItemDefinitionIndex;
int m_hActiveWeapon;
int m_hMyWeapons;
int CrosshairId;
//End

byte redGlowFixer[6];
byte greenGlowFixer[6];
byte blueGlowFixer[6];
byte alphaGlowFixer[6];
byte occGlowFixer[4];

void* colorGlowFixerOffset;
void* alphaGlowFixerOffset;
void* occGlowFixerOffset;

#define bDormant 0xED // It's a son of a bitch
#define PLAYER_ON_FLOOR (1 << 0)
#define PLAYER_CROUCHED 0x6
#define M_RADPI 57.295779513082

typedef struct Vector {
	float x, y, z;
}Vector;

struct GlowBase
{
	float r;
	float g;
	float b;
	float a;
	uint8_t unk1[16];
	bool m_bRenderWhenOccluded;
	bool m_bRenderWhenUnoccluded;
	bool m_bFullBloom;
};

struct BoneBase {
	byte padding[12];
	float x;
	byte padding2[12];
	float y;
	byte padding3[12];
	float z;

};

struct PlayerStruct {
	int Team;
	bool Ignore;
	bool Dormant;
	bool Spotted;
	int	Health;
	int	GlowIndex;
	int	Base;
	Vector Pos;
	float Angle[3];
	float FOV;
};
//Stucts End

PlayerStruct Players[33];
NBQMemory mem;
const char *title = "===== DeniZeus Simple External =====";
int key = 0;
int key1 = 0;
float aimsmooth;
int aimfov;
DWORD enemyteam;
bool showwnd = true;
bool isopened;
bool isopenedskin;
bool isopenedaim;
bool isopenedtrigger;
bool glowtype;
DWORD localPlayer;
int localHealth;
int lineint = 24;
int lasttarget = -2;
int Ind = 0;

DWORD dwPID = 0;
DWORD dwClient = 0;
DWORD dwEngine = 0;
HANDLE hProcess;
DWORD clientbase;
#define PI 3.14159265359

float degreesToRadians(float angle_in_degrees) {
	return angle_in_degrees * (PI / 180.0);
}

bool EntIsVisible(HANDLE csgo, DWORD ent, DWORD local)
{
	int mask = mem.ReadMemory<int>(csgo, ent + SpottedByMask);
	int PBASE = mem.ReadMemory<int>(csgo, local + 0x64) - 1;
	return (mask & (1 << PBASE)) > 0;
}

float getFOV(float *src, Vector flLocalAngles)
{
	float back[2];
	float smoothdiff[2];
	float srcangle[2];
	srcangle[0] = src[0];
	srcangle[1] = src[1];
	srcangle[2] = src[2];

	srcangle[0] -= flLocalAngles.x;
	srcangle[1] -= flLocalAngles.y;
	if (srcangle[0] > 180)  srcangle[0] -= 360;
	if (srcangle[1] > 180)  srcangle[1] -= 360;
	if (srcangle[0] < -180) srcangle[0] += 360;
	if (srcangle[1] < -180) srcangle[1] += 360;
	smoothdiff[0] = (srcangle[0]);
	smoothdiff[1] = (srcangle[1]);
	back[0] = smoothdiff[0];
	back[1] = smoothdiff[1];
	if (back[0] > 180)  back[0] -= 360;
	if (back[1] > 180)  back[1] -= 360;
	if (back[0] < -180) back[0] += 360;
	if (back[1] < -180) back[1] += 360;
	if (back[0] > 89.0f) back[0] = 89.0f;
	else if (back[0] < -89.0f) back[0] = -89.0f;
	if (back[1] > 180.0f) back[1] = 180.0f;
	else if (back[1] < -180.0f) back[1] = -180.0f;
	back[2] = 0.f;

	return back[1];
}

void entRefresher(HANDLE csgo,DWORD client) {
	while (true) {
		    localPlayer = mem.ReadMemory<DWORD>(csgo, client + dwlocalPlayer);
			DWORD localTeam = mem.ReadMemory<DWORD>(csgo, localPlayer + iTeamNum);
			localHealth = mem.ReadMemory<int>(csgo, localPlayer + iHealth);
			enemyteam = localTeam == 0x3 ? 0x2 : 0x3;
			if (isopenedaim) {
				Players[0].Pos = mem.ReadMemory<Vector>(csgo, localPlayer + vecOrigin);
				Vector VecView = mem.ReadMemory<Vector>(csgo, localPlayer + vecViewOffset);
				Players[0].Pos.x += VecView.x;
				Players[0].Pos.y += VecView.y;
				Players[0].Pos.z += VecView.z;
			}
			if (!localPlayer && !localHealth)
				continue;

			for (int i = 1; i < 33; i++) {
				DWORD player = mem.ReadMemory<int>(csgo, client + entityList + ((i-1) * 0x10));
				if (player == 0 && player == localPlayer) {
					Players[i].Ignore = true;
					continue;
				}
				else {
					Players[i].Ignore = false;
				}
				
				Players[i].Dormant = mem.ReadMemory<bool>(csgo, player + bDormant);

				if (Players[i].Dormant) {
					Players[i].Ignore = true;
					continue;
				}
				else {
					Players[i].Ignore = false;
				}

				DWORD playerteam = mem.ReadMemory<DWORD>(csgo, player + iTeamNum);
			
				Players[i].Team = playerteam;

				if (playerteam == localTeam) {
					Players[i].Ignore = true;
					continue;
				}
				else {
					Players[i].Ignore = false;
				}

				Players[i].Ignore = false;
				Players[i].Base = player;
				if (isopenedaim) {
					DWORD playerbonemtrix = mem.ReadMemory<DWORD>(csgo, player + boneMatrix);
					BoneBase temp = mem.ReadMemory<BoneBase>(csgo, (playerbonemtrix + (0x30 * 8)));
					Players[i].Pos.x = temp.x;
					Players[i].Pos.y = temp.y;
					Players[i].Pos.z = temp.z;
					Players[i].Spotted = EntIsVisible(csgo, player, localPlayer);
				}
				Players[i].Health = mem.ReadMemory<int>(csgo, player + iHealth);
				Players[i].GlowIndex = mem.ReadMemory<int>(csgo, player + glowIndex);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}
float scrToWorld(float X, float Y, float Z, float eX, float eY, float eZ)
{
	return(sqrtf((eX - X) * (eX - X) + (eY - Y) * (eY - Y) + (eZ - Z) * (eZ - Z)));
}

void CalcAngle(Vector src, Vector dst, float *angles)
{
	float Delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	angles[0] = atan(Delta[2] / sqrt(Delta[0] * Delta[0] + Delta[1] * Delta[1])) * M_RADPI;
	angles[1] = atan(Delta[1] / Delta[0]) * M_RADPI;
	angles[2] = 0.0f;
	if (Delta[0] >= 0.0) angles[1] += 180.0f;
}
void Smooth(float x, float y, float *src, float *back, Vector flLocalAngles, float smooth)
{
	float smoothdiff[2];
	src[0] -= flLocalAngles.x;
	src[1] -= flLocalAngles.y;
	if (src[0] > 180)  src[0] -= 360;
	if (src[1] > 180)  src[1] -= 360;
	if (src[0] < -180) src[0] += 360;
	if (src[1] < -180) src[1] += 360;
	 smoothdiff[0] = (src[0] - x) * smooth;
	 smoothdiff[1] = (src[1] - y) * smooth;
	back[0] = flLocalAngles.x + smoothdiff[0];
	back[1] = flLocalAngles.y + smoothdiff[1];
	back[2] = flLocalAngles.z;
	if (back[0] > 180)  back[0] -= 360;
	if (back[1] > 180)  back[1] -= 360;
	if (back[0] < -180) back[0] += 360;
	if (back[1] < -180) back[1] += 360;
	if (back[0] > 89.0f) back[0] = 89.0f;
	else if (back[0] < -89.0f) back[0] = -89.0f;
	if (back[1] > 180.0f) back[1] = 180.0f;
	else if (back[1]< -180.0f) back[1] = -180.0f;
	back[2] = 0.f;

}

float CloseEnt()
{
	//Variables
	float fLowest = 1000000, TMP;
	int iIndex = -1;
	Vector localAngles = mem.ReadMemory<Vector>(hProcess, clientbase + clientAngle);

	for (int i = 1; i < 33; i++)
	{
		//Store Distances In Array
		TMP = scrToWorld(Players[0].Pos.x, Players[0].Pos.y, Players[0].Pos.z, Players[i].Pos.x, Players[i].Pos.y, Players[i].Pos.z);
		CalcAngle(Players[0].Pos, Players[i].Pos, Players[i].Angle);
		Players[i].FOV = abs(getFOV(Players[i].Angle, localAngles));
		//If Enemy Has Lower Distance The Player 1, Replace (var)Lowest With Current Enemy Distance
		if (TMP < fLowest && !Players[i].Ignore && Players[i].Base != localPlayer  && Players[i].Health > 0 && Players[i].Spotted && Players[i].FOV < aimfov)
		{
			fLowest = TMP;
			iIndex = i;
		}
	}
	return iIndex;
}

float AngleDifference(float* ViewAngles, float* TargetAngles, float Distance)
{
	float pitch = sin(degreesToRadians(ViewAngles[0] - TargetAngles[0]));
	float yaw = sin(degreesToRadians(ViewAngles[1] - TargetAngles[1]));
	return sqrt(powf(pitch, 2.0) + powf(yaw, 2.0));
}

void retryAim(HANDLE csgo, DWORD client, DWORD engine) {
isopenedaim = true;
while (true) {
	if (isopenedaim && localPlayer && localHealth) {
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		static Vector oldAimPunch;
		static Vector fixedAngle;
		if (GetAsyncKeyState(key1) & 0x8000) {
			Ind = CloseEnt();
					if (Players[lasttarget].Spotted && (Players[lasttarget].Team == enemyteam) && !Players[lasttarget].Dormant && Players[lasttarget].Health > 0) {
						Ind = lasttarget;
					}
			if (Ind != -1) {
					float smoothed[2];
					Vector aimpunch = mem.ReadMemory<Vector>(csgo, localPlayer + aimPunch);
					aimpunch.x = aimpunch.x * 2.f;
					aimpunch.y = aimpunch.y * 2.f;
					Vector localAngles = mem.ReadMemory<Vector>(csgo, clientbase + clientAngle);
					CalcAngle(Players[0].Pos, Players[Ind].Pos, Players[Ind].Angle);
					Smooth(aimpunch.x, aimpunch.y, Players[Ind].Angle, smoothed, localAngles, aimsmooth);
					mem.WriteMemory<float>(csgo, clientbase + clientAngle, smoothed[0]);
					mem.WriteMemory<float>(csgo, clientbase + clientAngle + 0x4, smoothed[1]);
					lasttarget = Ind;
					
			}
		}
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}
//Skins List
DWORD fallbackPaint1 = 37;
DWORD fallbackPaint2 = 658;
DWORD fallbackPaint3 = 660;
DWORD fallbackPaint4 = 38;
DWORD fallbackPaint7 = 675;
DWORD fallbackPaint8 = 455;
DWORD fallbackPaint9 = 344;
DWORD fallbackPaint10 = 723;
DWORD fallbackPaint11 = 493;
DWORD fallbackPaint13 = 398;
DWORD fallbackPaint14 = 401;
DWORD fallbackPaint16 = 309;
DWORD fallbackPaint17 = 433;
DWORD fallbackPaint19 = 636;
DWORD fallbackPaint24 = 704;
DWORD fallbackPaint25 = 393;
DWORD fallbackPaint26 = 542;
DWORD fallbackPaint27 = 696;
DWORD fallbackPaint28 = 514;
DWORD fallbackPaint29 = 720;
DWORD fallbackPaint30 = 614;
DWORD fallbackPaint32 = 591;
DWORD fallbackPaint33 = 696;
DWORD fallbackPaint34 = 262;
DWORD fallbackPaint35 = 537;
DWORD fallbackPaint36 = 678;
DWORD fallbackPaint38 = 312;
DWORD fallbackPaint39 = 750;
DWORD fallbackPaint40 = 624;
DWORD fallbackPaint60 = 445;
DWORD fallbackPaint61 = 653;
// End
void skinsX(HANDLE csgo, DWORD client)
{
	const int itemIDHigh = -1;
	const int entityQuality = 3;
	const float fallbackWear = 0.0001f;
	isopenedskin = true;
	while (true)
	{
		if (isopenedskin && localPlayer) {

			if (localPlayer == 0)
			{
				continue;
			}
			for (int i = 0; i < 8; i++)
			{
				DWORD myWeapons = mem.ReadMemory<DWORD>(csgo, localPlayer + m_hMyWeapons + i * 0x4) & 0xfff;
				DWORD weaponEntity = mem.ReadMemory<DWORD>(csgo, client + entityList + (myWeapons - 1) * 0x10);
				if (weaponEntity == 0) { continue; }
				short weaponID = mem.ReadMemory<short>(csgo, weaponEntity + m_iItemDefinitionIndex);
				DWORD fallbackPaint = 0;
				if (weaponID == 1) { fallbackPaint = fallbackPaint1; } /* Desert Eagle | Blaze */
				else if (weaponID == 2) { fallbackPaint = fallbackPaint2; } /* Dual Berettas | Cobra Strike */
				else if (weaponID == 3) { fallbackPaint = fallbackPaint3; } /* Five-SeveN | Hyper Beast */
				else if (weaponID == 4) { fallbackPaint = fallbackPaint4; } /* Glock-18 | Fade */
				else if (weaponID == 7) { fallbackPaint = fallbackPaint7; } /* AK-47 | Empress */
				else if (weaponID == 8) { fallbackPaint = fallbackPaint8; } /* AUG | Akihabara Accept */
				else if (weaponID == 9) { fallbackPaint = fallbackPaint9; } /* AWP | Dragon Lore */
				else if (weaponID == 10) { fallbackPaint = fallbackPaint10; } /* FAMAS | Eye of Athena */
				else if (weaponID == 11) { fallbackPaint = fallbackPaint11; } /* G3SG1 | Flux */
				else if (weaponID == 13) { fallbackPaint = fallbackPaint13; } /* Galil AR | Chatterbox */
				else if (weaponID == 14) { fallbackPaint = fallbackPaint14; } /* M249 | System Lock */
				else if (weaponID == 16) { fallbackPaint = fallbackPaint16; } /* M4A4 - Howl */
				else if (weaponID == 17) { fallbackPaint = fallbackPaint17; } /* MAC-10 | Neon Rider */
				else if (weaponID == 19) { fallbackPaint = fallbackPaint19; } /* P90 | Shallow Grave */
				else if (weaponID == 24) { fallbackPaint = fallbackPaint24; } /* UMP-45 | Arctic Wolf */
				else if (weaponID == 25) { fallbackPaint = fallbackPaint25; } /* XM1014 | Tranquility */
				else if (weaponID == 26) { fallbackPaint = fallbackPaint26; } /* PP-Bizon | Judgement of Anubis */
				else if (weaponID == 27) { fallbackPaint = fallbackPaint27; } /* MP7 | Bloodsport */
				else if (weaponID == 28) { fallbackPaint = fallbackPaint28; } /* Negev | Power Loader */
				else if (weaponID == 29) { fallbackPaint = fallbackPaint29; } /* Sawed-Off | Devourer */
				else if (weaponID == 30) { fallbackPaint = fallbackPaint30; } /* Tec-9 | Fuel Injector */
				else if (weaponID == 32) { fallbackPaint = fallbackPaint32; } /* P2000 | Imperial Dragon */
				else if (weaponID == 33) { fallbackPaint = fallbackPaint33; } /* MP7 | Bloodsport */
				else if (weaponID == 34) { fallbackPaint = fallbackPaint34; } /* MP9 | Rose Iron */
				else if (weaponID == 35) { fallbackPaint = fallbackPaint35; } /* Nova | Hyper Beast */
				else if (weaponID == 36) { fallbackPaint = fallbackPaint36; } /* P250 | See Ya Later */
				else if (weaponID == 38) { fallbackPaint = fallbackPaint38; } /* SCAR-20 | Cyrex */
				else if (weaponID == 39) { fallbackPaint = fallbackPaint39; } /* SG 553 | Integrale */
				else if (weaponID == 40) { fallbackPaint = fallbackPaint40; } /* SSG 08 | Dragonfire */
				else if (weaponID == 60) { fallbackPaint = fallbackPaint60; } /* M4A1-S | Hot Rod */
				else if (weaponID == 61) { fallbackPaint = fallbackPaint61; } /* USP-S | Neo-Noir */
				else
				{
					mem.WriteMemory<int>(csgo, weaponEntity + m_iEntityQuality, entityQuality);
				}
				mem.WriteMemory<int>(csgo, weaponEntity + m_iItemIDHigh, itemIDHigh);
				mem.WriteMemory<DWORD>(csgo, weaponEntity + m_nFallbackPaintKit, fallbackPaint);
				mem.WriteMemory<float>(csgo, weaponEntity + m_flFallbackWear, fallbackWear);
			}
		}

		if (isopenedskin) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		else {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
}

void glowPlayer(HANDLE csgo, DWORD client, GlowBase entity, DWORD entityadr, int Health) {
	entity.r = 1.f - (float)(Health / 100.f);
	entity.g = (float)(Health / 100.f);
	entity.b = 0.f;
	entity.a = 1.f;
	entity.m_bRenderWhenOccluded = true;
	mem.WriteMemory<GlowBase>(csgo, entityadr + 0x4, entity);
}

void FixedGlowPlayer(HANDLE csgo, DWORD client, GlowBase entity, DWORD entityadr) {
	entity.r = 1.f;
	entity.g = 0.f;
	entity.b = 0.f;
	entity.a = 1.f;
	entity.m_bRenderWhenOccluded = true;
	mem.WriteMemory<GlowBase>(csgo, entityadr + 0x4, entity);
}

void FixedGlowPlayerClose(HANDLE csgo, DWORD client, GlowBase entity, DWORD entityadr) {
	entity.r = 0.f;
	entity.g = 0.f;
	entity.b = 0.f;
	entity.a = 0.f;
	entity.m_bRenderWhenOccluded = false;
	mem.WriteMemory<GlowBase>(csgo, entityadr + 0x4, entity);
}

void recover() {
	WriteProcessMemory(hProcess, (LPVOID)colorGlowFixerOffset, &redGlowFixer, sizeof(redGlowFixer), NULL);
	WriteProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 11), &greenGlowFixer, sizeof(greenGlowFixer), NULL);
	WriteProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 22), &blueGlowFixer, sizeof(blueGlowFixer), NULL);
	WriteProcessMemory(hProcess, (LPVOID)((DWORD)alphaGlowFixerOffset), &alphaGlowFixer, sizeof(alphaGlowFixer), NULL);
	WriteProcessMemory(hProcess, (LPVOID)((DWORD)occGlowFixerOffset), &occGlowFixer, sizeof(occGlowFixer), NULL);
}

HWND hWnd;
void retryHotkeys(HANDLE csgo, DWORD client) {
	isopened = true;
	while (true) {
		if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) {
			isopened = !isopened;
			if (isopened) {
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Glow hack opened!                                                     ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
				byte cleaner[6];
				std::fill(cleaner, cleaner + sizeof(cleaner), 144);
				byte cleaner4[4];
				std::fill(cleaner4, cleaner4 + sizeof(cleaner4), 144);
				WriteProcessMemory(csgo, (LPVOID)colorGlowFixerOffset, &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)colorGlowFixerOffset + 11), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)colorGlowFixerOffset + 22), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)alphaGlowFixerOffset), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)occGlowFixerOffset), &cleaner4, sizeof(cleaner4), NULL);
			}
			else
			{
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Glow hack closed!                                                     ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
				WriteProcessMemory(csgo, (LPVOID)colorGlowFixerOffset, &redGlowFixer, sizeof(redGlowFixer), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)colorGlowFixerOffset + 11), &greenGlowFixer, sizeof(greenGlowFixer), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)colorGlowFixerOffset + 22), &blueGlowFixer, sizeof(blueGlowFixer), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)alphaGlowFixerOffset), &alphaGlowFixer, sizeof(alphaGlowFixer), NULL);
				WriteProcessMemory(csgo, (LPVOID)((DWORD)occGlowFixerOffset), &occGlowFixer, sizeof(occGlowFixer), NULL);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if (GetAsyncKeyState(VK_NUMPAD9) & 0x8000) {
			isopenedaim = !isopenedaim;
			if (isopenedaim) {
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Aimbot hack opened!                                                   ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			}
			else
			{
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Aimbot hack closed!                                                   ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if (GetAsyncKeyState(VK_NUMPAD7) & 0x8000) {
			isopenedtrigger = !isopenedtrigger;
			if (isopenedtrigger) {
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Trigger hack opened!                                                  ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			}
			else {
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Trigger hack closed!                                                  ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if (GetAsyncKeyState(VK_NUMPAD6)) {

			isopenedskin = !isopenedskin;
			if (isopenedskin) {
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Skin hack opened!                                                     ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			}
			else
			{
				COORD coord;
				coord.X = 0;
				coord.Y = lineint;
				lineint++;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
				std::wcout << L" ║ Skin hack closed!                                                     ║" << std::endl;
				std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;

			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		if ((GetAsyncKeyState(VK_NUMPAD4) & 0x8000) || (GetAsyncKeyState(VK_END) & 0x8000)) {
			exit(0);
		}
		if (GetAsyncKeyState(VK_NUMPAD5) & 0x8000) {
			if (showwnd) {
				hWnd = GetConsoleWindow();
				ShowWindow(hWnd, SW_HIDE);
				showwnd = false;
			}
			else {
				hWnd = GetConsoleWindow();
				ShowWindow(hWnd, SW_SHOW);
				showwnd = true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void retryGlowFixed(HANDLE csgo, DWORD client) {

	while (true) {
		if (isopened && localPlayer) {
			DWORD GlowObject = mem.ReadMemory<DWORD>(csgo, client + dwGlowObjectManager);
			for (int i = 1; i < 33; i++) {
				if (Players[i].Ignore) {
					GlowBase entity = mem.ReadMemory<GlowBase>(csgo, GlowObject + ((Players[i].GlowIndex) * 0x38) + 0x4);
					DWORD entityadr = GlowObject + ((Players[i].GlowIndex) * 0x38);
					FixedGlowPlayerClose(csgo, client, entity, entityadr);
				}
			}
			for (int i = 1; i < 33; i++) {
				if (!Players[i].Ignore) {
					GlowBase entity = mem.ReadMemory<GlowBase>(csgo, GlowObject + ((Players[i].GlowIndex) * 0x38) + 0x4);
					DWORD entityadr = GlowObject + ((Players[i].GlowIndex) * 0x38);
					FixedGlowPlayer(csgo, client, entity, entityadr);
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void retryGlow(HANDLE csgo, DWORD client) {

	while (true) {
		if (isopened) {
			DWORD GlowObject = mem.ReadMemory<DWORD>(csgo, client + dwGlowObjectManager);
			for (int i = 1; i < 33; i++) {
				if (!Players[i].Ignore) {
					GlowBase entity = mem.ReadMemory<GlowBase>(csgo, GlowObject + ((Players[i].GlowIndex) * 0x38) + 0x4);
					DWORD entityadr = GlowObject + ((Players[i].GlowIndex) * 0x38);
					glowPlayer(csgo, client, entity, entityadr, Players[i].Health);
				}
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void retryTrigger(HANDLE csgo, DWORD client) {
	isopenedtrigger = true;
	while (true) {
		if (isopenedtrigger && (GetAsyncKeyState(key) & 0x8000)) {
			int crs = mem.ReadMemory<int>(csgo, localPlayer + CrosshairId);
			if (crs > 0 && crs < 64) {
				DWORD crossPlayer = mem.ReadMemory<DWORD>(csgo, client + entityList + (crs - 1) * 0x10);
				DWORD crossTeam = mem.ReadMemory<DWORD>(csgo, crossPlayer + iTeamNum);
				if (enemyteam = crossTeam && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
					mem.WriteMemory<int>(csgo, client + forceAttack, 1);
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
					mem.WriteMemory<int>(csgo, client + forceAttack, 0);
				}
			}
			
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void retryBunny(HANDLE csgo, DWORD client) {
	while (true) {
		while (localPlayer && localHealth) {
			int flags = mem.ReadMemory<int>(csgo, localPlayer + fFlags);
			if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && flags & PLAYER_ON_FLOOR) {
				mem.WriteMemory<DWORD>(csgo, client + dwForceJump, 6);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch (dwType) {
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		recover();
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

bool catched = false;
bool catchedfov = false;
bool catchedglow = false;
bool repeat = false;
bool repeatfov = false;
bool repeatglow = false;
std::string input1;
	void main() {
		
		std::ifstream myfile("skins.ini");
		std::string line;
			if (myfile.is_open())
			{
				int line_no = 0;
				while (line_no != 30 && getline(myfile, line)) {
					if (line_no == 0) {
						fallbackPaint1 = std::stoi(line);
					}
					else if (line_no == 1) {
						fallbackPaint2 = std::stoi(line);
					}
					else if (line_no == 2) {
						fallbackPaint3 = std::stoi(line);
					}
					else if (line_no == 3) {
						fallbackPaint4 = std::stoi(line);
					}
					else if (line_no == 4) {
						fallbackPaint7 = std::stoi(line);
					}
					else if (line_no == 5) {
						fallbackPaint8 = std::stoi(line);
					}
					else if (line_no == 6) {
						fallbackPaint9 = std::stoi(line);
					}
					else if (line_no == 7) {
						fallbackPaint10 = std::stoi(line);
					}
					else if (line_no == 8) {
						fallbackPaint11 = std::stoi(line);
					}
					else if (line_no == 9) {
						fallbackPaint13 = std::stoi(line);
					}
					else if (line_no == 10) {
						fallbackPaint14 = std::stoi(line);
					}
					else if (line_no == 11) {
						fallbackPaint16 = std::stoi(line);
					}
					else if (line_no == 12) {
						fallbackPaint17 = std::stoi(line);
					}
					else if (line_no == 13) {
						fallbackPaint19 = std::stoi(line);
					}
					else if (line_no == 14) {
						fallbackPaint24 = std::stoi(line);
					}
					else if (line_no == 15) {
						fallbackPaint25 = std::stoi(line);
					}
					else if (line_no == 16) {
						fallbackPaint26 = std::stoi(line);
					}
					else if (line_no == 17) {
						fallbackPaint27 = std::stoi(line);
					}
					else if (line_no == 18) {
						fallbackPaint28 = std::stoi(line);
					}
					else if (line_no == 19) {
						fallbackPaint29 = std::stoi(line);
					}
					else if (line_no == 20) {
						fallbackPaint30 = std::stoi(line);
					}
					else if (line_no == 21) {
						fallbackPaint32 = std::stoi(line);
					}
					else if (line_no == 21) {
						fallbackPaint33 = std::stoi(line);
					}
					else if (line_no == 22) {
						fallbackPaint34 = std::stoi(line);
					}
					else if (line_no == 23) {
						fallbackPaint35 = std::stoi(line);
					}
					else if (line_no == 24) {
						fallbackPaint36 = std::stoi(line);
					}
					else if (line_no == 25) {
						fallbackPaint38 = std::stoi(line);
					}
					else if (line_no == 26) {
						fallbackPaint39 = std::stoi(line);
					}
					else if (line_no == 27) {
						fallbackPaint40 = std::stoi(line);
					}
					else if (line_no == 28) {
						fallbackPaint60 = std::stoi(line);
					}
					else if (line_no == 29) {
						fallbackPaint61 = std::stoi(line);
					}
					line_no++;
				}
		}
		HWND console = GetConsoleWindow();
		RECT r;
		HRESULT hr = URLDownloadToFile(NULL, _T("https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.json"), _T("netvars.json"), 0, NULL);
		if (hr != S_OK) {
			MessageBoxA(NULL, "Error occoured on connection to github!", "Error", 0);
			exit(0);
		}
		FILE* infile = fopen("netvars.json", "r");
		fseek(infile, 0, SEEK_END);
		long filesize = ftell(infile);
		char* buf = new char[filesize + 1];
		fseek(infile, 0, SEEK_SET);
		fread(buf, 1, filesize, infile);
		fclose(infile);
		buf[filesize] = '\0';
		std::stringstream ss;
		ss.str(buf);
		delete[] buf;
		Json::Value netvars;
		ss >> netvars;
		dwGlowObjectManager = netvars["signatures"]["dwGlowObjectManager"].asInt();
		dwlocalPlayer = netvars["signatures"]["dwLocalPlayer"].asInt();
		dwForceJump = netvars["signatures"]["dwForceJump"].asInt();
		clientState = netvars["signatures"]["dwClientState"].asInt();
		forceAttack = netvars["signatures"]["dwForceAttack"].asInt();
		entityList = netvars["signatures"]["dwEntityList"].asInt();
		clientAngle = netvars["signatures"]["dwClientState_ViewAngles"].asInt();
		glowIndex = netvars["netvars"]["m_iGlowIndex"].asInt();
		iTeamNum = netvars["netvars"]["m_iTeamNum"].asInt();
		vecOrigin = netvars["netvars"]["m_vecOrigin"].asInt();
		vecViewOffset = netvars["netvars"]["m_vecViewOffset"].asInt();
		SpottedByMask = netvars["netvars"]["m_bSpottedByMask"].asInt();
		iHealth = netvars["netvars"]["m_iHealth"].asInt();
		fFlags = netvars["netvars"]["m_fFlags"].asInt();
		boneMatrix = netvars["netvars"]["m_dwBoneMatrix"].asInt();
		m_flFallbackWear = netvars["netvars"]["m_flFallbackWear"].asInt();
		m_nFallbackPaintKit = netvars["netvars"]["m_nFallbackPaintKit"].asInt();
		m_iItemIDHigh = netvars["netvars"]["m_iItemIDHigh"].asInt();
		m_iEntityQuality = netvars["netvars"]["m_iEntityQuality"].asInt();
		m_iItemDefinitionIndex = netvars["netvars"]["m_iItemDefinitionIndex"].asInt();
		m_hActiveWeapon = netvars["netvars"]["m_hActiveWeapon"].asInt();
		m_hMyWeapons = netvars["netvars"]["m_hMyWeapons"].asInt();
		CrosshairId = netvars["netvars"]["m_iCrosshairId"].asInt();
		aimPunch = netvars["netvars"]["m_aimPunchAngle"].asInt();
		GetWindowRect(console, &r);
		MoveWindow(console, r.left, r.top, 650, 500, TRUE);
		SetConsoleTitle(title);
		_setmode(_fileno(stdout), _O_U16TEXT);
		system("color 02");
		std::wcout << L" ╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
		std::wcout << L" ║  ______   _______  __    _  ___   _______  _______  __   __  _______  ║" << std::endl;
		std::wcout << L" ║ |      | |       ||  |  | ||   | |       ||       ||  | |  ||       | ║" << std::endl;
		std::wcout << L" ║ |  _    ||    ___||   |_| ||   | |____   ||    ___||  | |  ||  _____| ║" << std::endl;
		std::wcout << L" ║ | | |   ||   |___ |       ||   |  ____|  ||   |___ |  | |  || |_____  ║" << std::endl;
		std::wcout << L" ║ | |_|   ||    ___||  _    ||   | | ______||    ___||  | |  ||_____  | ║" << std::endl;
		std::wcout << L" ║ |       ||   |___ | | |   ||   | | |_____ |   |___ |  |_|  | _____| | ║" << std::endl;
		std::wcout << L" ║ |______| |_______||_|  |__||___| |_______||_______||_______||_______| ║" << std::endl;
		std::wcout << L" ║                                                                       ║" << std::endl;
		std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
		std::wcout << L" Waiting csgo.exe..." << std::endl;
		do {
			dwPID = mem.GetProcessIdByProcessName("csgo.exe");
			Sleep(100);
		} while (!dwPID);
		std::wcout << L" Waiting client_panorama.dll..." << std::endl;
		do {
			dwClient = mem.GetModuleBaseAddress(dwPID, "client_panorama.dll");
			Sleep(100);
		} while (!dwClient);
		std::wcout << L" Waiting engine.dll..." << std::endl;
		do {
			dwEngine = mem.GetModuleBaseAddress(dwPID, "engine.dll");
			Sleep(100);
		} while (!dwEngine);
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
		clientbase = mem.ReadMemory<DWORD>(hProcess, dwEngine + clientState);
		colorGlowFixerOffset = PatternScanExModule(hProcess, mem.GetModule(dwPID, "client_panorama.dll"), (wchar_t*)"csgo.exe", (wchar_t*)"client_panorama.dll", (char*)"\xF3\x0F\x11\x44\xC8\x00\xF3\x0F\x10\x45\x00\xF3\x0F\x11\x44\xC8\x00\xF3\x0F\x10\x45\x00\xF3\x0F\x11\x44\xC8\x00", (char*)"xxxxx?xxxx?xxxxx?xxxx?xxxxx?");
		alphaGlowFixerOffset = PatternScanExModule(hProcess, mem.GetModule(dwPID, "client_panorama.dll"), (wchar_t*)"csgo.exe", (wchar_t*)"client_panorama.dll", (char*)"\xF3\x0F\x11\x44\xC8\x00\x5F\x5E\x5B\x8B\xE5\x5D", (char*)"xxxxx?xxxxxx");
		occGlowFixerOffset = PatternScanExModule(hProcess, mem.GetModule(dwPID, "client_panorama.dll"), (wchar_t*)"csgo.exe", (wchar_t*)"client_panorama.dll", (char*)"\x88\x5C\xD1\x24\x8B\x00\x8B\x5D\xF4", (char*)"xxxxxxxxx");
		ReadProcessMemory(hProcess, colorGlowFixerOffset, redGlowFixer, sizeof(redGlowFixer), 0);
		ReadProcessMemory(hProcess, alphaGlowFixerOffset, alphaGlowFixer, sizeof(alphaGlowFixer), 0);
		ReadProcessMemory(hProcess, occGlowFixerOffset, occGlowFixer, sizeof(occGlowFixer), 0);
		ReadProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 11), greenGlowFixer, sizeof(greenGlowFixer), 0);
		ReadProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 22), blueGlowFixer, sizeof(blueGlowFixer), 0);
		system("cls");
		FILE* infile1 = fopen("settings.ini", "r");
		if (infile1 != NULL) {
			fseek(infile1, 0, SEEK_END);
			long filesize1 = ftell(infile1);
			char* buf1 = new char[filesize1 + 1];
			fseek(infile1, 0, SEEK_SET);
			fread(buf1, 1, filesize1, infile1);
			fclose(infile1);
			buf1[filesize1] = '\0';
			std::stringstream ss1;
			ss1.str(buf1);
			delete[] buf1;
			Json::Value settingsLoad;
			ss1 >> settingsLoad;
			key = settingsLoad["Trigger key"].asInt();
			key1 = settingsLoad["Aim key"].asInt();
			aimsmooth = settingsLoad["Aim smooth"].asFloat();
			aimfov = settingsLoad["Aim FOV"].asInt();
			glowtype = settingsLoad["Glow type"].asInt();
			goto LOADEDCONFIG;
		}
			std::wcout << L" ╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
			std::wcout << L" ║  ______   _______  __    _  ___   _______  _______  __   __  _______  ║" << std::endl;
			std::wcout << L" ║ |      | |       ||  |  | ||   | |       ||       ||  | |  ||       | ║" << std::endl;
			std::wcout << L" ║ |  _    ||    ___||   |_| ||   | |____   ||    ___||  | |  ||  _____| ║" << std::endl;
			std::wcout << L" ║ | | |   ||   |___ |       ||   |  ____|  ||   |___ |  | |  || |_____  ║" << std::endl;
			std::wcout << L" ║ | |_|   ||    ___||  _    ||   | | ______||    ___||  | |  ||_____  | ║" << std::endl;
			std::wcout << L" ║ |       ||   |___ | | |   ||   | | |_____ |   |___ |  |_|  | _____| | ║" << std::endl;
			std::wcout << L" ║ |______| |_______||_|  |__||___| |_______||_______||_______||_______| ║" << std::endl;
			std::wcout << L" ║                                                                       ║" << std::endl;
			std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
			std::wcout << L" Set hack keys! " << std::endl;
			std::wcout << L" Set trigger key (It's can be a mouse key) : ";
			while (key == 0) {
				for (int i = 0; i < 256; i++)
				{
					if (GetAsyncKeyState((i)& SHRT_MAX) && key == 0)
					{
						key = i;
						std::wcout << "0x" << std::to_string(key).c_str() << std::endl;
						Sleep(1000);
					}
				}
			}

			std::wcout << L" Set Aimbot key (It's can be a mouse key) : ";
			while (key1 == 0) {
				for (int i = 0; i < 256; i++)
				{

					if (GetAsyncKeyState((i)& SHRT_MAX) && key1 == 0)
					{
						key1 = i;
						std::wcout << "0x" << std::to_string(key1).c_str() << std::endl;
					}
				}
			}
			do {
				std::wcout << L" Set Aimbot smothness (0.05 - 1.0) : ";
				std::cin >> input1;
				std::replace(input1.begin(), input1.end(), ',', '.');
				try {
					std::stof(input1);
				}
				catch (...) {
					catched = true;
				}
				if (!catched) {
					aimsmooth = std::stof(input1);
					aimsmooth = floor(aimsmooth * 100000) / 100000;
				}
				if (catched) {
					std::wcout << L"Input a valid value!";
					repeat = true;
				}
				else if (aimsmooth == 0) {
					std::wcout << L"Input a value!";
					repeat = true;
				}
				else if (aimsmooth == NULL) {
					std::wcout << L"Not text! Only value.";
					repeat = true;
				}
				else if (aimsmooth < 0.05) {
					std::wcout << L"Must be higher than 0.05";
					repeat = true;
				}
				else if (aimsmooth > 1) {
					std::wcout << L"Must be lower than 1.0";
					repeat = true;
				}
				else {
					repeat = false;
				}
				catched = false;
			} while (repeat);
			Sleep(100);
			do {
				std::wcout << L" Set Aimbot FOV (1 - 90) : ";
				std::cin >> input1;
				std::replace(input1.begin(), input1.end(), ',', '.');
				try {
					std::stoi(input1);
				}
				catch (...) {
					catchedfov = true;
				}
				if (!catchedfov) {
					aimfov = std::stof(input1);
					aimfov = floor(aimfov * 100000) / 100000;
				}
				if (catchedfov) {
					std::wcout << L"Input a valid value!";
					repeatfov = true;
				}
				else if (aimfov == 0) {
					std::wcout << L"Input a value!";
					repeatfov = true;
				}
				else if (aimfov == NULL) {
					std::wcout << L"Not text! Only value.";
					repeatfov = true;
				}
				else if (aimfov < 1) {
					std::wcout << L"Must be higher than 1";
					repeatfov = true;
				}
				else if (aimfov > 90) {
					std::wcout << L"Must be lower than 180";
					repeatfov = true;
				}
				else {
					repeatfov = false;
				}
				catchedfov = false;
			} while (repeatfov);
			do {
				std::wcout << L" Set non-Flicker mode (Static mode) [ 0 / 1 ] : ";
				std::cin >> input1;

				try {
					std::stoi(input1);
				}
				catch (...) {
					catchedglow = true;
				}
				if (!catchedglow) {
					glowtype = std::stoi(input1);
				}
				if (catchedglow) {
					std::wcout << L"Input a valid value!";
					repeatglow = true;
				}
				else if (glowtype != 0 && glowtype != 1) {
					std::wcout << L"0 or 1!";
					repeatglow = true;
				}
				else {
					repeatglow = false;
				}
				catchedglow = false;
			} while (repeatglow);
			if (glowtype) {
				std::atexit(recover);
				SetConsoleCtrlHandler(ConsoleHandler, TRUE);
				byte cleaner[6];
				std::fill(cleaner, cleaner + sizeof(cleaner), 144);
				byte cleaner4[4];
				std::fill(cleaner4, cleaner4 + sizeof(cleaner4), 144);
				WriteProcessMemory(hProcess, (LPVOID)colorGlowFixerOffset, &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 11), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(hProcess, (LPVOID)((DWORD)colorGlowFixerOffset + 22), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(hProcess, (LPVOID)((DWORD)alphaGlowFixerOffset), &cleaner, sizeof(cleaner), NULL);
				WriteProcessMemory(hProcess, (LPVOID)((DWORD)occGlowFixerOffset), &cleaner4, sizeof(cleaner4), NULL);
			}
			Sleep(500);
			if (infile1 == NULL) {
				Json::Value settings;
				settings["Trigger key"] = key;
				settings["Aim key"] = key1;
				settings["Aim smooth"] = aimsmooth;
				settings["Aim FOV"] = aimfov;
				settings["Glow type"] = glowtype;
				Json::StyledWriter styledWriter;
				std::string strJson = styledWriter.write(settings);
				FILE* file = fopen("settings.ini", "w+");
				if (file)
				{
					fwrite(strJson.c_str(), 1, strJson.length(), file);
					fclose(file);
				}
			}
		LOADEDCONFIG:
		std::wcout << L" Success! Hack loading!" << std::endl;
		system("cls");
		std::wcout << L" ╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
		std::wcout << L" ║  ______   _______  __    _  ___   _______  _______  __   __  _______  ║" << std::endl;
		std::wcout << L" ║ |      | |       ||  |  | ||   | |       ||       ||  | |  ||       | ║" << std::endl;
		std::wcout << L" ║ |  _    ||    ___||   |_| ||   | |____   ||    ___||  | |  ||  _____| ║" << std::endl;
		std::wcout << L" ║ | | |   ||   |___ |       ||   |  ____|  ||   |___ |  | |  || |_____  ║" << std::endl;
		std::wcout << L" ║ | |_|   ||    ___||  _    ||   | | ______||    ___||  | |  ||_____  | ║" << std::endl;
		std::wcout << L" ║ |       ||   |___ | | |   ||   | | |_____ |   |___ |  |_|  | _____| | ║" << std::endl;
		std::wcout << L" ║ |______| |_______||_|  |__||___| |_______||_______||_______||_______| ║" << std::endl;
		std::wcout << L" ║                                                                       ║" << std::endl;
		std::wcout << L" ╠═══════════════════════════════TOGGLES═════════════════════════════════╣" << std::endl;
		std::wcout << L" ║ Aimbot Toggle     : Numpad 9                                          ║" << std::endl;
		std::wcout << L" ║ Aimbot smothness  : " << std::to_string(aimsmooth).c_str();
		for (int i = 0; i <= 49 - std::to_string(aimsmooth).length(); i++) {
			std::wcout << L" ";
		}
		std::wcout << L"║" << std::endl;
		std::wcout << L" ║ Aimbot FOV        : " << std::to_string(aimfov).c_str();
		for (int i = 0; i <= 49 - std::to_string(aimfov).length(); i++) {
			std::wcout << L" ";
		}
		std::wcout << L"║" << std::endl;
		std::wcout << L" ║ Aimbot lock key   : 0x" << std::to_string(key1).c_str();
		for (int i = 0; i <= 47 - std::to_string(key1).length(); i++) {
			std::wcout << L" ";
		}
		std::wcout << L"║" << std::endl;
		std::wcout << L" ║ Glow Hack Toggle  : Numpad 8                                          ║" << std::endl;
		std::wcout << L" ║ Trigger Toggle    : Numpad 7                                          ║" << std::endl;
		std::wcout << L" ║ Trigger key       : 0x" << std::to_string(key).c_str();
		for (int i = 0; i <= 47 - std::to_string(key).length(); i++) {
			std::wcout << L" ";
		}
		std::wcout << L"║" << std::endl;
		std::wcout << L" ║ SkinChanger Toggle: Numpad 6                                          ║" << std::endl;
		std::wcout << L" ║ Bunny key         : Press Space                                       ║" << std::endl;
		std::wcout << L" ║ Created by Denizdeni                                                  ║" << std::endl;
		std::wcout << L" ╠═════════════════════════════════OTHER═════════════════════════════════╣" << std::endl;
		std::wcout << L" ║ Toggle Console    : Numpad 5                                          ║" << std::endl;
		std::wcout << L" ║ Close Hack        : Numpad 4 and End                                  ║" << std::endl;
		std::wcout << L" ╠══════════════════════════════════LOG══════════════════════════════════╣" << std::endl;
		COORD coord;
		coord.X = 0;
		coord.Y = lineint;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
		std::wcout << L" ║                                                                       ║" << std::endl;
		std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
		if (hProcess != INVALID_HANDLE_VALUE)
		{
			auto h0 = std::async(std::launch::async, entRefresher, hProcess, dwClient);
			auto h1 = std::async(std::launch::async, skinsX, hProcess, dwClient);
			auto h3 = std::async(std::launch::async, retryTrigger, hProcess, dwClient);
			auto h4 = std::async(std::launch::async, retryBunny, hProcess, dwClient);
			auto h5 = std::async(std::launch::async, retryAim, hProcess, dwClient, dwEngine);
			auto h6 = std::async(std::launch::async, retryHotkeys, hProcess, dwClient);
			if (glowtype) {
				auto h7 = std::async(std::launch::async, retryGlowFixed, hProcess, dwClient);
				h0.get(), h1.get(), h7.get(), h3.get(), h4.get(), h5.get(), h6.get();
			}
			else {
				auto h2 = std::async(std::launch::async, retryGlow, hProcess, dwClient);
				h0.get(), h1.get(), h2.get(), h3.get(), h4.get(), h5.get(), h6.get();
			}

			

		}

		if (hProcess) { CloseHandle(hProcess); }
}
