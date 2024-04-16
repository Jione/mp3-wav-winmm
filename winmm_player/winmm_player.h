#pragma once
#ifndef WINMM_PLAYER_HEADER_H
#define WINMM_PLAYER_HEADER_H
#include <windows.h>
#include <stdio.h>
#include <mmsystem.h>
#include <map>
#pragma comment(lib, "winmm.lib")
#include "winmm_ini.h"

// Macro define
#define dprintf_nl(...) if (fh) {fflush(NULL); fprintf(fh, __VA_ARGS__); fputc('\n', fh); fflush(NULL);}
#define AddFlagString(str) if (msgDebug[0] == 0) { strcpy(msgDebug, str); } else { strcat (strcat (msgDebug, "|"), str); }
#define AddExtendString(str) if (msgDbgExtend[0] == 0) { strcpy (msgDbgExtend, str); } else { strcat(strcat(msgDbgExtend, ", "), str); }
#define mciSendStr(cmdString,returnString,...) snprintf(cmdString, sizeof(cmdString), __VA_ARGS__); dprintf_nl(cmdString); mciSendStringA(cmdString, returnString, sizeof(returnString), NULL)
#define MS_TO_MSF_BUFFER(buffer,ms) sprintf(buffer, "%d:%d:%d", (ms / 60000), ((ms / 1000) % 60), (((ms % 1000) * 3 / 40) % 75));



// Mutex Handle
typedef struct {
	HANDLE procMutex;				// exe process mailslot mutex
	HANDLE playCommand;				// execute command
	HANDLE toDLLSendMsg;			// Send Mailslot Message Mutex for IPC Process
	HANDLE fromDLLSendMsg;			// Receive Mailslot Message Mutex for IPC Process
}PLAYER_MUTEX;


// Server Handle
typedef struct {
	HANDLE watchdogPlay;			// Playback watchdog
	HANDLE receiveServer;			// IPC Receive server handle
}PLAYER_SERVER;


// Server Handle
typedef struct {
	DWORD pid;
	DWORD lastVolume;
	int lastStatus;
	int lastTrack;
	char lastPlayFile[MAX_PATH];
}PLAY_INFO;


// Initialize variables
const char szClassName[] = "WinmmAudioPlayer";
static INI_STATUS iniStatus{};					// winmm.ini Preference data struct
static MM_REQUEST modeRequest{};				// IPC Request struct
static MM_RESPONSE statusResponse{};			// IPC Response struct
static char currentPath[MAX_PATH];				// Running current path
static PLAYER_MUTEX playerMutex{};				// Player Mutex handle struct
static PLAYER_SERVER playerServer{};			// Player Server handle struct
static std::map<DWORD, PLAY_INFO> PIDMap;		// Process ID Map for Multiple Play
static const DWORD globalTimeout = 0x800;


// Initialize IPC variables
static const char dllServerName[] = "winmmwrppr";		// DLL mailslot Server name
static const char procServerName[] = "cdaudioplr";		// EXE mailslot Server name
static const char mutexSignature[] = "Mutex";			// Mutex name signiture
static const char mailslotSignature[] = "Mailslot";		// Mailslot server name signiture
static char procServer[MAX_PATH];
static char procMutex[MAX_PATH];
//static char musicPlayerPath[MAX_PATH];


// for debug logging
FILE* fh = NULL;								// debugging log file
static BOOL debugOffOnce = 0;					// debug logging off once value (Prevent excessive logging)
static char debugMessage[0x20];					// debug value (Prevent excessive logging)
static char lastDebugMessage[0x20];				// Stored last debug value (Prevent excessive logging)
static BOOL isCDRequest;						// check cdaudio device reqeust (Prevent excessive logging)
static char msgDebug[0x40]{};					// main log message for mciSend function
static char msgDbgExtend[0x80]{};				// sub log message for mciSend function

// for Mailslot IPC function
void CloseUsedHandle();
void WatchdogPlaytime();
void ReceiveIPCMessageServer();
BOOL SendIPCMessage(DWORD procIdentifier, MM_RESPONSE* responseData);

// for Music playback
void RegisterPID(DWORD procIdentifier, WORD rightVol, WORD leftVol);
void UnregisterPID(DWORD procIdentifier);
BOOL MusicPlay(DWORD procIdentifier, LPCSTR trackPath, DWORD trackNum, DWORD startTime, DWORD endTime);
BOOL MusicStop(DWORD procIdentifier);
BOOL MusicVolumeControl(DWORD procIdentifier, DWORD dwVolume);

// for win32 mfc callback
// Callback volume struct
typedef struct {
	int selectLevel;
	BOOL isForced;
	BOOL isMute;
	DWORD dwVolume;
}GUI_VOLUME;

HWND hWnd;
HWND hEdit;
static GUI_VOLUME guiVolume{};					// GUI Volume level struct
static UINT uShellRestart;
static NOTIFYICONDATA nid{};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void OnBnClickedTrayAdd(HWND hwnd, HICON hIcon);

inline BOOL CloseHandleSafe(HANDLE& h) { HANDLE tmp = h; h = NULL; return tmp && tmp != INVALID_HANDLE_VALUE ? CloseHandle(tmp) : TRUE; }
inline BOOL ReleaseMutexSafe(HANDLE& h) { HANDLE tmp = h; h = NULL; return tmp && tmp != INVALID_HANDLE_VALUE ? ReleaseMutex(tmp) : TRUE; }
#endif