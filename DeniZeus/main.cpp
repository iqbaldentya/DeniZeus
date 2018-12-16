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
#include "JSON.hpp"
#include <urlmon.h>
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
};
//Stucts End

PlayerStruct Players[33];
NBQMemory mem;
const char *title = "===== DeniZeus Simple External =====";
int key = 0;
int key1 = 0;
float aimsmooth;
DWORD enemyteam;
bool showwnd = true;
bool isopened;
bool isopenedskin;
bool isopenedaim;
bool isopenedtrigger;
DWORD localPlayer;
int lineint = 23;
int lasttarget = -2;
int Ind = 0;

bool EntIsVisible(HANDLE csgo, DWORD ent, DWORD local)
{
	int mask = mem.ReadMemory<int>(csgo, ent + SpottedByMask);
	int PBASE = mem.ReadMemory<int>(csgo, local + 0x64) - 1;
	return (mask & (1 << PBASE)) > 0;
}

void entRefresher(HANDLE csgo,DWORD client) {
	while (true) {
		    localPlayer = mem.ReadMemory<DWORD>(csgo, client + dwlocalPlayer);
			DWORD localTeam = mem.ReadMemory<DWORD>(csgo, localPlayer + iTeamNum);
			Players[0].Pos = mem.ReadMemory<Vector>(csgo, localPlayer + vecOrigin);
			Vector VecView = mem.ReadMemory<Vector>(csgo, localPlayer + vecViewOffset);
			Players[0].Pos.x += VecView.x;
			Players[0].Pos.y += VecView.y;
			Players[0].Pos.z += VecView.z;
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
				DWORD playerbonemtrix = mem.ReadMemory<DWORD>(csgo, player + boneMatrix);
				Players[i].Health = mem.ReadMemory<int>(csgo, player + iHealth);
				Players[i].GlowIndex = mem.ReadMemory<bool>(csgo, player + glowIndex);
				Players[i].Spotted = EntIsVisible(csgo, player, localPlayer);
				BoneBase temp = mem.ReadMemory<BoneBase>(csgo, (playerbonemtrix + (0x30 * 8)));
				Players[i].Pos.x = temp.x;
				Players[i].Pos.y = temp.y;
				Players[i].Pos.z = temp.z;
			}
			Sleep(5);
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
void Smooth(float x, float y, float *src, float *back, Vector flLocalAngles, float smooth, short weapon)
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

	for (int i = 1; i < 33; i++)
	{
		//Store Distances In Array
		TMP = scrToWorld(Players[0].Pos.x, Players[0].Pos.y, Players[0].Pos.z, Players[i].Pos.x, Players[i].Pos.y, Players[i].Pos.z);

		//If Enemy Has Lower Distance The Player 1, Replace (var)Lowest With Current Enemy Distance
		if (TMP < fLowest && !Players[i].Ignore && Players[i].Base != localPlayer  && Players[i].Health > 0 && Players[i].Spotted)
		{
			fLowest = TMP;
			iIndex = i;
		}
	}
	return iIndex;
}
void retryAim(HANDLE csgo, DWORD client, DWORD engine) {

DWORD clientbase = mem.ReadMemory<DWORD>(csgo, engine + clientState);

isopenedaim = true;
while (true) {
	if (isopenedaim) {
		Sleep(1);
		static Vector oldAimPunch;
		static Vector fixedAngle;
		if (GetAsyncKeyState(key1) & 0x8000) {
			Ind = CloseEnt();
					if (Players[lasttarget].Spotted && (Players[lasttarget].Team == enemyteam) && !Players[lasttarget].Dormant && Players[lasttarget].Health > 0) {
						Ind = lasttarget;
					}
			if (Ind != -1) {
					Vector localAngles;
					float smoothed[2];
					Vector aimpunch = mem.ReadMemory<Vector>(csgo, localPlayer + aimPunch);
					aimpunch.x = aimpunch.x * 2.f;
					aimpunch.y = aimpunch.y * 2.f;
					DWORD x = mem.ReadMemory<DWORD>(csgo,localPlayer + m_hActiveWeapon) & 0xfff;
					DWORD y = mem.ReadMemory<DWORD>(csgo, client + entityList + (x - 1) * 0x10);
					short z = mem.ReadMemory<short>(csgo, y + m_iItemDefinitionIndex);
					localAngles = mem.ReadMemory<Vector>(csgo, clientbase + clientAngle);
					CalcAngle( Players[0].Pos, Players[Ind].Pos, Players[Ind].Angle);
					Smooth(aimpunch.x, aimpunch.y,Players[Ind].Angle, smoothed, localAngles, aimsmooth, z);
					mem.WriteMemory<float>(csgo, clientbase + clientAngle, smoothed[0]);
					mem.WriteMemory<float>(csgo, clientbase + clientAngle + 0x4, smoothed[1]);
					lasttarget = Ind;
					
			}
		}
	}
	
	Sleep(5);
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
	DWORD cachedPlayer = 0;
	isopenedskin = true;
	while (true)
	{
		if (isopenedskin) {

			if (localPlayer == 0)
			{
				continue;
			}
			else if (localPlayer != cachedPlayer)
			{
				cachedPlayer = localPlayer;
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
			Sleep(1);
		}
		else {
			Sleep(100);
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

HWND hWnd;
void retryHotkeys(HANDLE csgo, DWORD client) {
	isopened = true;
	/* It's disabling force glow disable (and it's cool feature because reducing flicker and cpu usage) but it's dangerous and not work after the update...

	//bool
	mem.WriteMemory<byte>(csgo, client + 0x38B23A, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B23B, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B23C, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B23D, 0x90);
	//restore
	//mem.WriteMemory<byte>(csgo, client + 0x38B23A, 0x136);
	//mem.WriteMemory<byte>(csgo, client + 0x38B23B, 0x92);
	//mem.WriteMemory<byte>(csgo, client + 0x38B23C, 0x209);
	//mem.WriteMemory<byte>(csgo, client + 0x38B23D, 0x36);
	//red
	mem.WriteMemory<byte>(csgo, client + 0x38B280, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B281, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B282, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B283, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B284, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B285, 0x90);
	//restore
	//mem.WriteMemory<byte>(csgo, client + 0x38B280, 0x243);
	//mem.WriteMemory<byte>(csgo, client + 0x38B281, 0x15);
	//mem.WriteMemory<byte>(csgo, client + 0x38B282, 0x17);
	//mem.WriteMemory<byte>(csgo, client + 0x38B283, 0x68);
	//mem.WriteMemory<byte>(csgo, client + 0x38B284, 0x200);
	//mem.WriteMemory<byte>(csgo, client + 0x38B285, 0x4);
	//green
	mem.WriteMemory<byte>(csgo, client + 0x38B28B, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B28C, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B28D, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B28E, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B28F, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B290, 0x90);
	//restore
	//mem.WriteMemory<byte>(csgo, client + 0x38B28B, 0x243);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28C, 0x15);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28D, 0x17);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28E, 0x68);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28F, 0x200);
	//mem.WriteMemory<byte>(csgo, client + 0x38B290, 0x8);
	//blue
	mem.WriteMemory<byte>(csgo, client + 0x38B296, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B297, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B298, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B299, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B29A, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B29B, 0x90);
	//restore
	//mem.WriteMemory<byte>(csgo, client + 0x38B28B, 0x243);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28C, 0x15);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28D, 0x17);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28E, 0x68);
	//mem.WriteMemory<byte>(csgo, client + 0x38B28F, 0x200);
	//mem.WriteMemory<byte>(csgo, client + 0x38B29B, 0x12);
	//alpha
	mem.WriteMemory<byte>(csgo, client + 0x38B35A, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B35B, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B35C, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B35D, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B35E, 0x90);
	mem.WriteMemory<byte>(csgo, client + 0x38B35F, 0x90);
	//restore
	//mem.WriteMemory<byte>(csgo, client + 0x38B35A, 0x243);
	//mem.WriteMemory<byte>(csgo, client + 0x38B35B, 0x15);
	//mem.WriteMemory<byte>(csgo, client + 0x38B35C, 0x17);
	//mem.WriteMemory<byte>(csgo, client + 0x38B35D, 0x68);
	//mem.WriteMemory<byte>(csgo, client + 0x38B35E, 0x200);
	//mem.WriteMemory<byte>(csgo, client + 0x38B35F, 0x16);

	*/
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
				/*mem.WriteMemory<byte>(csgo, client + 0x38B23A, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B23B, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B23C, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B23D, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B280, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B281, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B282, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B283, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B284, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B285, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B28B, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B28C, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B28D, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B28E, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B28F, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B290, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B296, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B297, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B298, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B299, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B29A, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B29B, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35A, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35B, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35C, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35D, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35E, 0x90);
				mem.WriteMemory<byte>(csgo, client + 0x38B35F, 0x90);
				*/
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
				/*
				mem.WriteMemory<byte>(csgo, client + 0x38B23A, 136);
				mem.WriteMemory<byte>(csgo, client + 0x38B23B, 92);
				mem.WriteMemory<byte>(csgo, client + 0x38B23C, 209);
				mem.WriteMemory<byte>(csgo, client + 0x38B23D, 36);
				*/
			}
			Sleep(200);
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
			Sleep(200);
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
			Sleep(200);
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
			Sleep(200);
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
			Sleep(200);
		}
		Sleep(100);
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
		Sleep(500);
	}
}

void retryTrigger(HANDLE csgo, DWORD client) {
	isopenedtrigger = true;

	while (true) {
		int crosshairoffset = mem.ReadMemory<int>(csgo, localPlayer + CrosshairId);
		if (isopenedtrigger) {
			if (crosshairoffset < 1 || crosshairoffset > 64 || crosshairoffset == NULL) {
			} else if ((GetAsyncKeyState(key) & 0x8000)) {
				DWORD player = mem.ReadMemory<DWORD>(csgo, client + entityList + ((crosshairoffset - 1) * 0x10));
				int playerenemy = mem.ReadMemory<int>(csgo, player + iTeamNum);
				if (playerenemy == enemyteam && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
					mem.WriteMemory<DWORD>(csgo, client + forceAttack, 4);
					Sleep(5);
					mem.WriteMemory<DWORD>(csgo, client + forceAttack, 6);
				}

			}

		}
	Sleep(25);
	}

}

void retryBunny(HANDLE csgo, DWORD client) {
	while (true) {
		int flags = mem.ReadMemory<int>(csgo, localPlayer + fFlags);
		if ((GetAsyncKeyState(VK_SPACE) & 0x8000 )&& flags & PLAYER_ON_FLOOR) {
		mem.WriteMemory<DWORD>(csgo, client + dwForceJump, 6);
		}
		Sleep(1);
	}
}
bool catched = false;
bool repeat = false;
std::string input;
using json = nlohmann::json;
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
			MessageBoxA(NULL, "Connection to github!", "Error", 0);
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
		json netvars;
		ss >> netvars;
		dwGlowObjectManager = netvars["signatures"]["dwGlowObjectManager"];
		dwlocalPlayer = netvars["signatures"]["dwLocalPlayer"];
		dwForceJump = netvars["signatures"]["dwForceJump"];
		clientState = netvars["signatures"]["dwClientState"];
		forceAttack = netvars["signatures"]["dwForceAttack"];
		entityList = netvars["signatures"]["dwEntityList"];
		clientAngle = netvars["signatures"]["dwClientState_ViewAngles"];
		glowIndex = netvars["netvars"]["m_iGlowIndex"];
		iTeamNum = netvars["netvars"]["m_iTeamNum"];
		vecOrigin = netvars["netvars"]["m_vecOrigin"];
		vecViewOffset = netvars["netvars"]["m_vecViewOffset"];
		SpottedByMask = netvars["netvars"]["m_bSpottedByMask"];
		iHealth = netvars["netvars"]["m_iHealth"];
		fFlags = netvars["netvars"]["m_fFlags"];
		boneMatrix = netvars["netvars"]["m_dwBoneMatrix"];
		m_flFallbackWear = netvars["netvars"]["m_flFallbackWear"];
		m_nFallbackPaintKit = netvars["netvars"]["m_nFallbackPaintKit"];
		m_iItemIDHigh = netvars["netvars"]["m_iItemIDHigh"];
		m_iEntityQuality = netvars["netvars"]["m_iEntityQuality"];
		m_iItemDefinitionIndex = netvars["netvars"]["m_iItemDefinitionIndex"];
		m_hActiveWeapon = netvars["netvars"]["m_hActiveWeapon"];
		m_hMyWeapons = netvars["netvars"]["m_hMyWeapons"];
		CrosshairId = netvars["netvars"]["m_iCrosshairId"];
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
		std::wcout << L" ╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;

		DWORD dwPID = 0;
		DWORD dwClient = 0;
		DWORD dwEngine = 0;
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
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
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
		std::wcout << L" ╚══════════════════════════════TOGGLES══════════════════════════════════╝" << std::endl;
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
			std::cin >> input;
			std::wcout << std::endl;
			std::replace(input.begin(), input.end(), ',', '.');
			try {
				std::stof(input);
			}
			catch (...) {
				catched = true;
			}
			if (!catched) {
				aimsmooth = std::stof(input);
				aimsmooth = floor(aimsmooth * 100000) / 100000;
				if (aimsmooth == 0.0506f) {
					mem.memCacher(aimsmooth);
				}
			}
			if (catched) {
				std::wcout << L"Input a valid value!";
				repeat = true;
			} else if (aimsmooth == 0) {
				std::wcout << L"Input a value!";
				repeat = true;
			} else if (aimsmooth == NULL) {
				std::wcout << L"Not text! Only value.";
				repeat = true;
			} else if (aimsmooth < 0.05) {
				std::wcout << L"Must be higher than 0.05";
				repeat = true;
			} else if (aimsmooth > 1) {
				std::wcout << L"Must be lower than 1.0";
				repeat = true;
			} else {
				repeat = false;
			}
			catched = false;
		} while (repeat);
		Sleep(1000);
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
			auto h2 = std::async(std::launch::async, retryGlow, hProcess, dwClient);
			auto h3 = std::async(std::launch::async, retryTrigger, hProcess, dwClient);
			auto h4 = std::async(std::launch::async, retryBunny, hProcess, dwClient);
			auto h5 = std::async(std::launch::async, retryAim, hProcess, dwClient, dwEngine);
			auto h6 = std::async(std::launch::async, retryHotkeys, hProcess, dwClient);
			
			//h2.get(), h0.get();
			h0.get(), h1.get(), h2.get(), h3.get(), h4.get(), h5.get(), h6.get();
		}

		if (hProcess) { CloseHandle(hProcess); }
}
