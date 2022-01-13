// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "vectors.h"
#include "drawing.h"
#include <string>
#include <sstream>
#include <iostream>

#define STRING(num) #num
#define PLAYER_BASE 0x50F4F4
#define PLAYERS_NUM 0x50F500
#define HEALTH 0xF8
#define TEAMMATE 0x32C

/* (3D coordinates) x (mvpMatrix) = (camera 좌표계) */
/* camera 좌표의 x,y 로 플레이어의 위치를, z 로 원근을 표시 */
float mvpMatrix[16]; // viewMatirx => camera coordinate system 을 위한 것
int SinglePlayStart = 1;
int MultiPlayStart = 0;
DWORD viewMatrixAddr = 0x501AE8;

bool WorldToScreen(Vec3 enemyPos, Vec2 &screen, int windowWidth, int windowHeight) {
	Vec4 clip_coords;
	// 동차 좌표계이므로 w 는 1
	clip_coords.x = enemyPos.x * mvpMatrix[0] + enemyPos.y * mvpMatrix[4] + enemyPos.z * mvpMatrix[8] + mvpMatrix[12];
	clip_coords.y = enemyPos.x * mvpMatrix[1] + enemyPos.y * mvpMatrix[5] + enemyPos.z * mvpMatrix[9] + mvpMatrix[13];
	clip_coords.z = enemyPos.x * mvpMatrix[2] + enemyPos.y * mvpMatrix[6] + enemyPos.z * mvpMatrix[10] + mvpMatrix[14];
	clip_coords.w = enemyPos.x * mvpMatrix[3] + enemyPos.y * mvpMatrix[7] + enemyPos.z * mvpMatrix[11] + mvpMatrix[15];

	if (clip_coords.w < 0.1f)
		return false;

	Vec3 NDC;
	NDC.x = clip_coords.x / clip_coords.w;
	NDC.y = clip_coords.y / clip_coords.w;
	NDC.z = clip_coords.z / clip_coords.w;
	
	// http://www.songho.ca/opengl/gl_transform.html Window Coordinates
	screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
	screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
	return true;
}

int MainThread() {
	/* GDI ESP 의 Flicking 현상은 자연스러운 것 */
	HWND hwndAC_Client = FindWindow(0, ("AssaultCube")); // AC 윈도우 얻어옴
	if (!hwndAC_Client) {
		MessageBox(nullptr, "Can't Get AC Window Handle", "Error", MB_OK);
		return 0;
	}
	DWORD myTeam = *(DWORD*)(*(DWORD*)(PLAYER_BASE)+TEAMMATE);
	while (true) {
		HBRUSH Enemy_Brush = CreateSolidBrush(RGB(255, 0, 0));
		HBRUSH Team_Brush = CreateSolidBrush(RGB(0, 255, 0));
		DWORD enemyList = *(DWORD*)(PLAYER_BASE + 0x4); // 나를 제외한 사람 객체의 배열
		DWORD amountOfPlayers = *(DWORD*)(PLAYERS_NUM);
		
		/* 아무도 없을 시 바로 종료 */
		if (amountOfPlayers <= 1) {
			MessageBox(NULL, "No Players", "Error", MB_OK);
			return 0;
		}

		HDC hdcAC_client = GetDC(hwndAC_Client);
		memcpy(&mvpMatrix, (PBYTE*)(viewMatrixAddr), sizeof(mvpMatrix));

		/* Get Enemy Entity Pos */
		for (DWORD i = SinglePlayStart; i < amountOfPlayers; i++) {
			Vec2 vEnemy, vEnemyHead;
			DWORD enemy = *(DWORD*)(enemyList + 0x4 * i);

			/* 아무도 없을 시 바로 종료 */
			if (enemy == NULL)
				continue;

			/* Get Enemy Pos */
			float x = *(float*)(enemy + 0x34);
			float y = *(float*)(enemy + 0x38);
			float z = *(float*)(enemy + 0x3c);
			Vec3 enemyPos = { x,y,z };

			/* Get Enemy Head Pos*/
			float x_head = *(float*)(enemy + 0x4);
			float y_head = *(float*)(enemy + 0x8);
			float z_head = *(float*)(enemy + 0xc);
			Vec3 enemyHeadPos = { x_head, y_head, z_head };

			if (WorldToScreen(enemyPos, vEnemy, 1280, 720)) {
				if (WorldToScreen(enemyHeadPos, vEnemyHead, 1280, 720)) {
					DWORD isTeam = *(DWORD*)(enemy + TEAMMATE);
					DWORD health = *(DWORD*)(enemy + HEALTH);
					if (isTeam == myTeam) {
						DrawRect(hdcAC_client, vEnemyHead.x - 20, vEnemyHead.y - 10, 30, vEnemyHead.y - vEnemy.y, 3, Team_Brush);
					}
					else {
						DrawRect(hdcAC_client, vEnemyHead.x - 20, vEnemyHead.y - 10, 30, vEnemyHead.y - vEnemy.y, 3, Enemy_Brush);
						DrawStr(hdcAC_client, vEnemy.x-20, vEnemy.y, "HP", 20);
						if(health>0 && health<=100)
							DrawStr(hdcAC_client, vEnemy.x+10, vEnemy.y, std::to_string(health).c_str(), 20);
						else
							DrawStr(hdcAC_client, vEnemy.x + 10, vEnemy.y, "0", 20);
					}
				}
			}
		}
		// 반복하는 동안의 메모리 누수 방지
		DeleteObject(Enemy_Brush);
		DeleteObject(Team_Brush);
		DeleteObject(hdcAC_client);
		Sleep(1);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	/*
		일종의 DLL Life Cycle
		LoadLibrary 혹은 CreateRemoteThread 시에 사용됨.
	*/
	switch (ul_reason_for_call)
	{
		/* DLL 이 프로세스에 매핑 될 때 */
		case DLL_PROCESS_ATTACH: {
			//MessageBox(nullptr, L"injection success", L"dll injection", MB_OK);
			//DisableThreadLibraryCalls(hModule); // Don't post DLL_THREAD_ATTACH/DLL_THREAD_DETACH to DllMain
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MainThread, NULL, NULL, NULL);
			//MainThread();
		}
		/* 프로세스에서 스레드가 생성될 때 */
		case DLL_THREAD_ATTACH:
		/* 프로세스에서 스레드가 소멸될 때 */
		case DLL_THREAD_DETACH:
		/* DLL 이 프로세스에서 분리될 때 */
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

/*
	std::ostringstream stream;
	stream << enemy;
	std::string str = stream.str();
	MessageBox(nullptr, str.c_str(), "dll injection", MB_OK);
	*/