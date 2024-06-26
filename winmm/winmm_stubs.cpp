/*
  Generated by AheadLib hijack dll Source Code Generator.

  Referenced code
  AheadLib: https://github.com/strivexjun/AheadLib-x86-x64
  ogg-winmm(Original): https://github.com/hifi-unmaintained/ogg-winmm
  cdaudio-winmm(MP3-Wav): https://github.com/dippy-dipper/cdaudio-winmm
  cdaudio-winmm(CD Player): https://github.com/YELLO-belly/cdaudio-winmm

  Added: Rewrote the original u/hifi's winmm.def linker
  Added: Support for binary patched programs known as "_inmm" (Behavior when changing name winmm.dll to _inmm.dll)
  Added: More added original u/YELLO-belly's function relay for the fake-functions
  Added: Configure standalone execution environment with DLL replication function (Behavior when winmm.win32.dll exists)
  Added: Add Current hWnd Finder with GetCurrentHWND() function
  Added: Add exe Path Finder with GetParentPath(LPTSTR) function
  Added: Add named exe Process Running Checker with FindProcess(LPTSTR) function
*/

#include "winmm_stubs.h"
#include <tlhelp32.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

PVOID pfn_mciExecute;
PVOID pfn_CloseDriver;
PVOID pfn_DefDriverProc;
PVOID pfn_DriverCallback;
PVOID pfn_DrvGetModuleHandle;
PVOID pfn_GetDriverModuleHandle;
PVOID pfn_NotifyCallbackData;
PVOID pfn_OpenDriver;
PVOID pfn_PlaySound;
PVOID pfn_PlaySoundA;
PVOID pfn_PlaySoundW;
PVOID pfn_SendDriverMessage;
PVOID pfn_WOW32DriverCallback;
PVOID pfn_WOW32ResolveMultiMediaHandle;
PVOID pfn_WOWAppExit;
PVOID pfn_aux32Message;
PVOID pfn_auxGetDevCapsA;
PVOID pfn_auxGetDevCapsW;
PVOID pfn_auxGetNumDevs;
PVOID pfn_auxGetVolume;
PVOID pfn_auxOutMessage;
PVOID pfn_auxSetVolume;
PVOID pfn_joy32Message;
PVOID pfn_joyConfigChanged;
PVOID pfn_joyGetDevCapsA;
PVOID pfn_joyGetDevCapsW;
PVOID pfn_joyGetNumDevs;
PVOID pfn_joyGetPos;
PVOID pfn_joyGetPosEx;
PVOID pfn_joyGetThreshold;
PVOID pfn_joyReleaseCapture;
PVOID pfn_joySetCapture;
PVOID pfn_joySetThreshold;
PVOID pfn_mci32Message;
PVOID pfn_mciDriverNotify;
PVOID pfn_mciDriverYield;
PVOID pfn_mciFreeCommandResource;
PVOID pfn_mciGetCreatorTask;
PVOID pfn_mciGetDeviceIDA;
PVOID pfn_mciGetDeviceIDFromElementIDA;
PVOID pfn_mciGetDeviceIDFromElementIDW;
PVOID pfn_mciGetDeviceIDW;
PVOID pfn_mciGetDriverData;
PVOID pfn_mciGetErrorStringA;
PVOID pfn_mciGetErrorStringW;
PVOID pfn_mciGetYieldProc;
PVOID pfn_mciLoadCommandResource;
PVOID pfn_mciSendCommandA;
PVOID pfn_mciSendCommandW;
PVOID pfn_mciSendStringA;
PVOID pfn_mciSendStringW;
PVOID pfn_mciSetDriverData;
PVOID pfn_mciSetYieldProc;
PVOID pfn_mid32Message;
PVOID pfn_midiConnect;
PVOID pfn_midiDisconnect;
PVOID pfn_midiInAddBuffer;
PVOID pfn_midiInClose;
PVOID pfn_midiInGetDevCapsA;
PVOID pfn_midiInGetDevCapsW;
PVOID pfn_midiInGetErrorTextA;
PVOID pfn_midiInGetErrorTextW;
PVOID pfn_midiInGetID;
PVOID pfn_midiInGetNumDevs;
PVOID pfn_midiInMessage;
PVOID pfn_midiInOpen;
PVOID pfn_midiInPrepareHeader;
PVOID pfn_midiInReset;
PVOID pfn_midiInStart;
PVOID pfn_midiInStop;
PVOID pfn_midiInUnprepareHeader;
PVOID pfn_midiOutCacheDrumPatches;
PVOID pfn_midiOutCachePatches;
PVOID pfn_midiOutClose;
PVOID pfn_midiOutGetDevCapsA;
PVOID pfn_midiOutGetDevCapsW;
PVOID pfn_midiOutGetErrorTextA;
PVOID pfn_midiOutGetErrorTextW;
PVOID pfn_midiOutGetID;
PVOID pfn_midiOutGetNumDevs;
PVOID pfn_midiOutGetVolume;
PVOID pfn_midiOutLongMsg;
PVOID pfn_midiOutMessage;
PVOID pfn_midiOutOpen;
PVOID pfn_midiOutPrepareHeader;
PVOID pfn_midiOutReset;
PVOID pfn_midiOutSetVolume;
PVOID pfn_midiOutShortMsg;
PVOID pfn_midiOutUnprepareHeader;
PVOID pfn_midiStreamClose;
PVOID pfn_midiStreamOpen;
PVOID pfn_midiStreamOut;
PVOID pfn_midiStreamPause;
PVOID pfn_midiStreamPosition;
PVOID pfn_midiStreamProperty;
PVOID pfn_midiStreamRestart;
PVOID pfn_midiStreamStop;
PVOID pfn_mixerClose;
PVOID pfn_mixerGetControlDetailsA;
PVOID pfn_mixerGetControlDetailsW;
PVOID pfn_mixerGetDevCapsA;
PVOID pfn_mixerGetDevCapsW;
PVOID pfn_mixerGetID;
PVOID pfn_mixerGetLineControlsA;
PVOID pfn_mixerGetLineControlsW;
PVOID pfn_mixerGetLineInfoA;
PVOID pfn_mixerGetLineInfoW;
PVOID pfn_mixerGetNumDevs;
PVOID pfn_mixerMessage;
PVOID pfn_mixerOpen;
PVOID pfn_mixerSetControlDetails;
PVOID pfn_mmDrvInstall;
PVOID pfn_mmGetCurrentTask;
PVOID pfn_mmTaskBlock;
PVOID pfn_mmTaskCreate;
PVOID pfn_mmTaskSignal;
PVOID pfn_mmTaskYield;
PVOID pfn_mmioAdvance;
PVOID pfn_mmioAscend;
PVOID pfn_mmioClose;
PVOID pfn_mmioCreateChunk;
PVOID pfn_mmioDescend;
PVOID pfn_mmioFlush;
PVOID pfn_mmioGetInfo;
PVOID pfn_mmioInstallIOProcA;
PVOID pfn_mmioInstallIOProcW;
PVOID pfn_mmioOpenA;
PVOID pfn_mmioOpenW;
PVOID pfn_mmioRead;
PVOID pfn_mmioRenameA;
PVOID pfn_mmioRenameW;
PVOID pfn_mmioSeek;
PVOID pfn_mmioSendMessage;
PVOID pfn_mmioSetBuffer;
PVOID pfn_mmioSetInfo;
PVOID pfn_mmioStringToFOURCCA;
PVOID pfn_mmioStringToFOURCCW;
PVOID pfn_mmioWrite;
PVOID pfn_mmsystemGetVersion;
PVOID pfn_mod32Message;
PVOID pfn_mxd32Message;
PVOID pfn_sndPlaySoundA;
PVOID pfn_sndPlaySoundW;
PVOID pfn_tid32Message;
PVOID pfn_timeBeginPeriod;
PVOID pfn_timeEndPeriod;
PVOID pfn_timeGetDevCaps;
PVOID pfn_timeGetSystemTime;
PVOID pfn_timeGetTime;
PVOID pfn_timeKillEvent;
PVOID pfn_timeSetEvent;
PVOID pfn_waveInAddBuffer;
PVOID pfn_waveInClose;
PVOID pfn_waveInGetDevCapsA;
PVOID pfn_waveInGetDevCapsW;
PVOID pfn_waveInGetErrorTextA;
PVOID pfn_waveInGetErrorTextW;
PVOID pfn_waveInGetID;
PVOID pfn_waveInGetNumDevs;
PVOID pfn_waveInGetPosition;
PVOID pfn_waveInMessage;
PVOID pfn_waveInOpen;
PVOID pfn_waveInPrepareHeader;
PVOID pfn_waveInReset;
PVOID pfn_waveInStart;
PVOID pfn_waveInStop;
PVOID pfn_waveInUnprepareHeader;
PVOID pfn_waveOutBreakLoop;
PVOID pfn_waveOutClose;
PVOID pfn_waveOutGetDevCapsA;
PVOID pfn_waveOutGetDevCapsW;
PVOID pfn_waveOutGetErrorTextA;
PVOID pfn_waveOutGetErrorTextW;
PVOID pfn_waveOutGetID;
PVOID pfn_waveOutGetNumDevs;
PVOID pfn_waveOutGetPitch;
PVOID pfn_waveOutGetPlaybackRate;
PVOID pfn_waveOutGetPosition;
PVOID pfn_waveOutGetVolume;
PVOID pfn_waveOutMessage;
PVOID pfn_waveOutOpen;
PVOID pfn_waveOutPause;
PVOID pfn_waveOutPrepareHeader;
PVOID pfn_waveOutReset;
PVOID pfn_waveOutRestart;
PVOID pfn_waveOutSetPitch;
PVOID pfn_waveOutSetPlaybackRate;
PVOID pfn_waveOutSetVolume;
PVOID pfn_waveOutUnprepareHeader;
PVOID pfn_waveOutWrite;
PVOID pfn_wid32Message;
PVOID pfn_wod32Message;

typedef MCIERROR(WINAPI* func_mciSendCommandA)(MCIDEVICEID, UINT, DWORD, DWORD);
typedef MCIERROR(WINAPI* func_mciSendStringA)(LPCSTR, LPSTR, UINT, HWND);
typedef UINT(WINAPI* func_GetNumDevs)();
typedef MMRESULT(WINAPI* func_GetDevCapsA)(UINT_PTR, LPAUXCAPS, UINT);
typedef MMRESULT(WINAPI* func_GetVolume)(UINT, LPDWORD);
typedef MMRESULT(WINAPI* func_SetVolume)(UINT, DWORD);

static func_mciSendCommandA f_mciSendCommandA;
static func_mciSendStringA f_mciSendStringA;
static func_GetNumDevs f_auxGetNumDevs;
static func_GetDevCapsA f_auxGetDevCapsA;
static func_GetVolume f_auxGetVolume;
static func_SetVolume f_auxSetVolume;
static func_GetVolume f_waveOutGetVolume;
static func_SetVolume f_waveOutSetVolume;

static HMODULE g_OldModule = NULL; // Original Module handle
static HWND ParentPID = NULL; // Parent PID handle
static char ParentPath[MAX_PATH]{}; // Parent PID handle
static HWND CurrentHWND = NULL; // Current Window handle
static HANDLE FindHWNDProc = NULL; // Find Current Window handle
static HANDLE FindParentProc = NULL; // Find Parent PID handle


inline BOOL CloseHandleSafe(HANDLE& h) { HANDLE tmp = h; h = NULL; return tmp && tmp != INVALID_HANDLE_VALUE ? CloseHandle(tmp) : TRUE; }

ALCDECL fake_nullReturn(void) {
	__asm RETN
}

void WINAPI Free() {
	if (g_OldModule) {
		FreeLibrary(g_OldModule);
	}
}

void WINAPI Copy() {
	TCHAR tzPath[MAX_PATH];
	TCHAR tzTemp[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, tzTemp);
	lstrcat(tzTemp, TEXT("\\winmm.win32.dll"));
	GetSystemDirectory(tzPath, MAX_PATH);
	lstrcat(tzPath, TEXT("\\winmm.dll"));
	CopyFileA(tzPath, tzTemp, FALSE);
}

BOOL WINAPI Load() {
	TCHAR tzPath[MAX_PATH];
	FILE* fp;

	GetCurrentDirectory(MAX_PATH, tzPath);
	lstrcat(tzPath, TEXT("\\winmm.win32.dll"));
	fp = fopen(tzPath, "rb");
	if (fp == NULL) {
		GetSystemDirectory(tzPath, MAX_PATH);
		lstrcat(tzPath, TEXT("\\winmm.dll"));
		g_OldModule = LoadLibrary(tzPath);
		// CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Copy, NULL, 0, NULL);
	}
	else {
		fclose(fp);
		g_OldModule = LoadLibrary(tzPath);
		if ((g_OldModule == NULL) || (GetProcAddress(g_OldModule, "mciSendStringA") == NULL)) {
			Free();
			GetSystemDirectory(tzPath, MAX_PATH);
			lstrcat(tzPath, TEXT("\\winmm.dll"));
			g_OldModule = LoadLibrary(tzPath);
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Copy, NULL, 0, NULL);
		}
	}

	return (g_OldModule != NULL);
}


FARPROC WINAPI GetAddress(PCSTR pszProcName) {
	FARPROC fpAddress;

	fpAddress = GetProcAddress(g_OldModule, pszProcName);
	if (fpAddress == NULL) {
		return (FARPROC)fake_nullReturn;
	}
	return fpAddress;
}

ULONG ProcIDFromWnd(HWND hwnd) {
	ULONG idProc;
	GetWindowThreadProcessId(hwnd, &idProc);
	return idProc;
}

HWND GetWinHandle(ULONG pid) {
	HWND tempHwnd = FindWindow(NULL, NULL);

	while (tempHwnd != NULL) {
		if (GetParent(tempHwnd) == NULL) 
			if (pid == ProcIDFromWnd(tempHwnd))
				return tempHwnd;
		tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT);
	}
	return NULL;
}

void SetWinHandle() {
	while (CurrentHWND == NULL) {
		CurrentHWND = GetWinHandle(GetCurrentProcessId());
	}
}

HWND GetCurrentHWND() {
	if (!IsWindow(CurrentHWND)) {
		FindHWNDProc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SetWinHandle, NULL, 0, NULL);
		if (FindHWNDProc != NULL) {
			WaitForSingleObject(FindHWNDProc, 0x80);
			CloseHandleSafe(FindHWNDProc);
		}
	}
	return CurrentHWND;
}

void SetParentPath() {
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	do {
		Sleep(1);
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	} while (hProc == NULL);
	GetModuleFileNameExA(hProc, NULL, ParentPath, MAX_PATH);
	CloseHandleSafe(hProc);
}

BOOL GetParentPath(LPTSTR lpszProcessPath) {
	WaitForSingleObject(FindParentProc, 1000);
	if (!ParentPath[0]) return FALSE;
	strcpy(lpszProcessPath, ParentPath);
	return TRUE;
}

HANDLE FindProcess(LPTSTR pExeName) {
	if (pExeName == NULL || strlen(pExeName) == 0) return NULL;
	HANDLE hProc = NULL;
	PROCESSENTRY32 pe32 = {};

	hProc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	for (int i = 0; i < 10; i++) {
		if (hProc != INVALID_HANDLE_VALUE) break;
		Sleep(10);
		hProc = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	}
	if (hProc == INVALID_HANDLE_VALUE)
		return NULL;

	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hProc, &pe32)) {
		do {
			if (strstr(pe32.szExeFile, pExeName) != 0)
				return (HANDLE)pe32.th32ProcessID;
		} while (Process32Next(hProc, &pe32));
	}
	return NULL;
}

BOOL WINAPI NsGetAddress() {
	pfn_mciExecute = GetAddress("mciExecute");
	pfn_CloseDriver = GetAddress("CloseDriver");
	pfn_DefDriverProc = GetAddress("DefDriverProc");
	pfn_DriverCallback = GetAddress("DriverCallback");
	pfn_DrvGetModuleHandle = GetAddress("DrvGetModuleHandle");
	pfn_GetDriverModuleHandle = GetAddress("GetDriverModuleHandle");
	pfn_NotifyCallbackData = GetAddress("NotifyCallbackData");
	pfn_OpenDriver = GetAddress("OpenDriver");
	pfn_PlaySound = GetAddress("PlaySound");
	pfn_PlaySoundA = GetAddress("PlaySoundA");
	pfn_PlaySoundW = GetAddress("PlaySoundW");
	pfn_SendDriverMessage = GetAddress("SendDriverMessage");
	pfn_WOW32DriverCallback = GetAddress("WOW32DriverCallback");
	pfn_WOW32ResolveMultiMediaHandle = GetAddress("WOW32ResolveMultiMediaHandle");
	pfn_WOWAppExit = GetAddress("WOWAppExit");
	pfn_aux32Message = GetAddress("aux32Message");
	pfn_auxGetDevCapsA = GetAddress("auxGetDevCapsA");
	pfn_auxGetDevCapsW = GetAddress("auxGetDevCapsW");
	pfn_auxGetNumDevs = GetAddress("auxGetNumDevs");
	pfn_auxGetVolume = GetAddress("auxGetVolume");
	pfn_auxOutMessage = GetAddress("auxOutMessage");
	pfn_auxSetVolume = GetAddress("auxSetVolume");
	pfn_joy32Message = GetAddress("joy32Message");
	pfn_joyConfigChanged = GetAddress("joyConfigChanged");
	pfn_joyGetDevCapsA = GetAddress("joyGetDevCapsA");
	pfn_joyGetDevCapsW = GetAddress("joyGetDevCapsW");
	pfn_joyGetNumDevs = GetAddress("joyGetNumDevs");
	pfn_joyGetPos = GetAddress("joyGetPos");
	pfn_joyGetPosEx = GetAddress("joyGetPosEx");
	pfn_joyGetThreshold = GetAddress("joyGetThreshold");
	pfn_joyReleaseCapture = GetAddress("joyReleaseCapture");
	pfn_joySetCapture = GetAddress("joySetCapture");
	pfn_joySetThreshold = GetAddress("joySetThreshold");
	pfn_mci32Message = GetAddress("mci32Message");
	pfn_mciDriverNotify = GetAddress("mciDriverNotify");
	pfn_mciDriverYield = GetAddress("mciDriverYield");
	pfn_mciFreeCommandResource = GetAddress("mciFreeCommandResource");
	pfn_mciGetCreatorTask = GetAddress("mciGetCreatorTask");
	pfn_mciGetDeviceIDA = GetAddress("mciGetDeviceIDA");
	pfn_mciGetDeviceIDFromElementIDA = GetAddress("mciGetDeviceIDFromElementIDA");
	pfn_mciGetDeviceIDFromElementIDW = GetAddress("mciGetDeviceIDFromElementIDW");
	pfn_mciGetDeviceIDW = GetAddress("mciGetDeviceIDW");
	pfn_mciGetDriverData = GetAddress("mciGetDriverData");
	pfn_mciGetErrorStringA = GetAddress("mciGetErrorStringA");
	pfn_mciGetErrorStringW = GetAddress("mciGetErrorStringW");
	pfn_mciGetYieldProc = GetAddress("mciGetYieldProc");
	pfn_mciLoadCommandResource = GetAddress("mciLoadCommandResource");
	pfn_mciSendCommandA = GetAddress("mciSendCommandA");
	pfn_mciSendCommandW = GetAddress("mciSendCommandW");
	pfn_mciSendStringA = GetAddress("mciSendStringA");
	pfn_mciSendStringW = GetAddress("mciSendStringW");
	pfn_mciSetDriverData = GetAddress("mciSetDriverData");
	pfn_mciSetYieldProc = GetAddress("mciSetYieldProc");
	pfn_mid32Message = GetAddress("mid32Message");
	pfn_midiConnect = GetAddress("midiConnect");
	pfn_midiDisconnect = GetAddress("midiDisconnect");
	pfn_midiInAddBuffer = GetAddress("midiInAddBuffer");
	pfn_midiInClose = GetAddress("midiInClose");
	pfn_midiInGetDevCapsA = GetAddress("midiInGetDevCapsA");
	pfn_midiInGetDevCapsW = GetAddress("midiInGetDevCapsW");
	pfn_midiInGetErrorTextA = GetAddress("midiInGetErrorTextA");
	pfn_midiInGetErrorTextW = GetAddress("midiInGetErrorTextW");
	pfn_midiInGetID = GetAddress("midiInGetID");
	pfn_midiInGetNumDevs = GetAddress("midiInGetNumDevs");
	pfn_midiInMessage = GetAddress("midiInMessage");
	pfn_midiInOpen = GetAddress("midiInOpen");
	pfn_midiInPrepareHeader = GetAddress("midiInPrepareHeader");
	pfn_midiInReset = GetAddress("midiInReset");
	pfn_midiInStart = GetAddress("midiInStart");
	pfn_midiInStop = GetAddress("midiInStop");
	pfn_midiInUnprepareHeader = GetAddress("midiInUnprepareHeader");
	pfn_midiOutCacheDrumPatches = GetAddress("midiOutCacheDrumPatches");
	pfn_midiOutCachePatches = GetAddress("midiOutCachePatches");
	pfn_midiOutClose = GetAddress("midiOutClose");
	pfn_midiOutGetDevCapsA = GetAddress("midiOutGetDevCapsA");
	pfn_midiOutGetDevCapsW = GetAddress("midiOutGetDevCapsW");
	pfn_midiOutGetErrorTextA = GetAddress("midiOutGetErrorTextA");
	pfn_midiOutGetErrorTextW = GetAddress("midiOutGetErrorTextW");
	pfn_midiOutGetID = GetAddress("midiOutGetID");
	pfn_midiOutGetNumDevs = GetAddress("midiOutGetNumDevs");
	pfn_midiOutGetVolume = GetAddress("midiOutGetVolume");
	pfn_midiOutLongMsg = GetAddress("midiOutLongMsg");
	pfn_midiOutMessage = GetAddress("midiOutMessage");
	pfn_midiOutOpen = GetAddress("midiOutOpen");
	pfn_midiOutPrepareHeader = GetAddress("midiOutPrepareHeader");
	pfn_midiOutReset = GetAddress("midiOutReset");
	pfn_midiOutSetVolume = GetAddress("midiOutSetVolume");
	pfn_midiOutShortMsg = GetAddress("midiOutShortMsg");
	pfn_midiOutUnprepareHeader = GetAddress("midiOutUnprepareHeader");
	pfn_midiStreamClose = GetAddress("midiStreamClose");
	pfn_midiStreamOpen = GetAddress("midiStreamOpen");
	pfn_midiStreamOut = GetAddress("midiStreamOut");
	pfn_midiStreamPause = GetAddress("midiStreamPause");
	pfn_midiStreamPosition = GetAddress("midiStreamPosition");
	pfn_midiStreamProperty = GetAddress("midiStreamProperty");
	pfn_midiStreamRestart = GetAddress("midiStreamRestart");
	pfn_midiStreamStop = GetAddress("midiStreamStop");
	pfn_mixerClose = GetAddress("mixerClose");
	pfn_mixerGetControlDetailsA = GetAddress("mixerGetControlDetailsA");
	pfn_mixerGetControlDetailsW = GetAddress("mixerGetControlDetailsW");
	pfn_mixerGetDevCapsA = GetAddress("mixerGetDevCapsA");
	pfn_mixerGetDevCapsW = GetAddress("mixerGetDevCapsW");
	pfn_mixerGetID = GetAddress("mixerGetID");
	pfn_mixerGetLineControlsA = GetAddress("mixerGetLineControlsA");
	pfn_mixerGetLineControlsW = GetAddress("mixerGetLineControlsW");
	pfn_mixerGetLineInfoA = GetAddress("mixerGetLineInfoA");
	pfn_mixerGetLineInfoW = GetAddress("mixerGetLineInfoW");
	pfn_mixerGetNumDevs = GetAddress("mixerGetNumDevs");
	pfn_mixerMessage = GetAddress("mixerMessage");
	pfn_mixerOpen = GetAddress("mixerOpen");
	pfn_mixerSetControlDetails = GetAddress("mixerSetControlDetails");
	pfn_mmDrvInstall = GetAddress("mmDrvInstall");
	pfn_mmGetCurrentTask = GetAddress("mmGetCurrentTask");
	pfn_mmTaskBlock = GetAddress("mmTaskBlock");
	pfn_mmTaskCreate = GetAddress("mmTaskCreate");
	pfn_mmTaskSignal = GetAddress("mmTaskSignal");
	pfn_mmTaskYield = GetAddress("mmTaskYield");
	pfn_mmioAdvance = GetAddress("mmioAdvance");
	pfn_mmioAscend = GetAddress("mmioAscend");
	pfn_mmioClose = GetAddress("mmioClose");
	pfn_mmioCreateChunk = GetAddress("mmioCreateChunk");
	pfn_mmioDescend = GetAddress("mmioDescend");
	pfn_mmioFlush = GetAddress("mmioFlush");
	pfn_mmioGetInfo = GetAddress("mmioGetInfo");
	pfn_mmioInstallIOProcA = GetAddress("mmioInstallIOProcA");
	pfn_mmioInstallIOProcW = GetAddress("mmioInstallIOProcW");
	pfn_mmioOpenA = GetAddress("mmioOpenA");
	pfn_mmioOpenW = GetAddress("mmioOpenW");
	pfn_mmioRead = GetAddress("mmioRead");
	pfn_mmioRenameA = GetAddress("mmioRenameA");
	pfn_mmioRenameW = GetAddress("mmioRenameW");
	pfn_mmioSeek = GetAddress("mmioSeek");
	pfn_mmioSendMessage = GetAddress("mmioSendMessage");
	pfn_mmioSetBuffer = GetAddress("mmioSetBuffer");
	pfn_mmioSetInfo = GetAddress("mmioSetInfo");
	pfn_mmioStringToFOURCCA = GetAddress("mmioStringToFOURCCA");
	pfn_mmioStringToFOURCCW = GetAddress("mmioStringToFOURCCW");
	pfn_mmioWrite = GetAddress("mmioWrite");
	pfn_mmsystemGetVersion = GetAddress("mmsystemGetVersion");
	pfn_mod32Message = GetAddress("mod32Message");
	pfn_mxd32Message = GetAddress("mxd32Message");
	pfn_sndPlaySoundA = GetAddress("sndPlaySoundA");
	pfn_sndPlaySoundW = GetAddress("sndPlaySoundW");
	pfn_tid32Message = GetAddress("tid32Message");
	pfn_timeBeginPeriod = GetAddress("timeBeginPeriod");
	pfn_timeEndPeriod = GetAddress("timeEndPeriod");
	pfn_timeGetDevCaps = GetAddress("timeGetDevCaps");
	pfn_timeGetSystemTime = GetAddress("timeGetSystemTime");
	pfn_timeGetTime = GetAddress("timeGetTime");
	pfn_timeKillEvent = GetAddress("timeKillEvent");
	pfn_timeSetEvent = GetAddress("timeSetEvent");
	pfn_waveInAddBuffer = GetAddress("waveInAddBuffer");
	pfn_waveInClose = GetAddress("waveInClose");
	pfn_waveInGetDevCapsA = GetAddress("waveInGetDevCapsA");
	pfn_waveInGetDevCapsW = GetAddress("waveInGetDevCapsW");
	pfn_waveInGetErrorTextA = GetAddress("waveInGetErrorTextA");
	pfn_waveInGetErrorTextW = GetAddress("waveInGetErrorTextW");
	pfn_waveInGetID = GetAddress("waveInGetID");
	pfn_waveInGetNumDevs = GetAddress("waveInGetNumDevs");
	pfn_waveInGetPosition = GetAddress("waveInGetPosition");
	pfn_waveInMessage = GetAddress("waveInMessage");
	pfn_waveInOpen = GetAddress("waveInOpen");
	pfn_waveInPrepareHeader = GetAddress("waveInPrepareHeader");
	pfn_waveInReset = GetAddress("waveInReset");
	pfn_waveInStart = GetAddress("waveInStart");
	pfn_waveInStop = GetAddress("waveInStop");
	pfn_waveInUnprepareHeader = GetAddress("waveInUnprepareHeader");
	pfn_waveOutBreakLoop = GetAddress("waveOutBreakLoop");
	pfn_waveOutClose = GetAddress("waveOutClose");
	pfn_waveOutGetDevCapsA = GetAddress("waveOutGetDevCapsA");
	pfn_waveOutGetDevCapsW = GetAddress("waveOutGetDevCapsW");
	pfn_waveOutGetErrorTextA = GetAddress("waveOutGetErrorTextA");
	pfn_waveOutGetErrorTextW = GetAddress("waveOutGetErrorTextW");
	pfn_waveOutGetID = GetAddress("waveOutGetID");
	pfn_waveOutGetNumDevs = GetAddress("waveOutGetNumDevs");
	pfn_waveOutGetPitch = GetAddress("waveOutGetPitch");
	pfn_waveOutGetPlaybackRate = GetAddress("waveOutGetPlaybackRate");
	pfn_waveOutGetPosition = GetAddress("waveOutGetPosition");
	pfn_waveOutGetVolume = GetAddress("waveOutGetVolume");
	pfn_waveOutMessage = GetAddress("waveOutMessage");
	pfn_waveOutOpen = GetAddress("waveOutOpen");
	pfn_waveOutPause = GetAddress("waveOutPause");
	pfn_waveOutPrepareHeader = GetAddress("waveOutPrepareHeader");
	pfn_waveOutReset = GetAddress("waveOutReset");
	pfn_waveOutRestart = GetAddress("waveOutRestart");
	pfn_waveOutSetPitch = GetAddress("waveOutSetPitch");
	pfn_waveOutSetPlaybackRate = GetAddress("waveOutSetPlaybackRate");
	pfn_waveOutSetVolume = GetAddress("waveOutSetVolume");
	pfn_waveOutUnprepareHeader = GetAddress("waveOutUnprepareHeader");
	pfn_waveOutWrite = GetAddress("waveOutWrite");
	pfn_wid32Message = GetAddress("wid32Message");
	pfn_wod32Message = GetAddress("wod32Message");

	f_mciSendCommandA = reinterpret_cast<func_mciSendCommandA>(reinterpret_cast<void*>(pfn_mciSendCommandA));
	f_mciSendStringA = reinterpret_cast<func_mciSendStringA>(reinterpret_cast<void*>(pfn_mciSendStringA));
	f_auxGetNumDevs = reinterpret_cast<func_GetNumDevs>(reinterpret_cast<void*>(pfn_auxGetNumDevs));
	f_auxGetDevCapsA = reinterpret_cast<func_GetDevCapsA>(reinterpret_cast<void*>(pfn_auxGetDevCapsA));
	f_auxGetVolume = reinterpret_cast<func_GetVolume>(reinterpret_cast<void*>(pfn_auxGetVolume));
	f_auxSetVolume = reinterpret_cast<func_SetVolume>(reinterpret_cast<void*>(pfn_auxSetVolume));
	f_waveOutGetVolume = reinterpret_cast<func_GetVolume>(reinterpret_cast<void*>(pfn_waveOutGetVolume));
	f_waveOutSetVolume = reinterpret_cast<func_SetVolume>(reinterpret_cast<void*>(pfn_waveOutSetVolume));

	return TRUE;
}

BOOL HookInitialize(HMODULE hModule, DWORD dwReason) {

	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		Load();
		NsGetAddress();
		FindHWNDProc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SetWinHandle, NULL, 0, NULL);
		FindParentProc = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SetParentPath, NULL, 0, NULL);
	}
	else if (dwReason == DLL_PROCESS_DETACH) {
		CloseHandleSafe(FindHWNDProc);
		CloseHandleSafe(FindParentProc);
		Free();
	}

	return TRUE;
}

ALCDECL fake_mciExecute(void) {
	__asm jmp pfn_mciExecute;
}
ALCDECL fake_CloseDriver(void) {
	__asm jmp pfn_CloseDriver;
}
ALCDECL fake_DefDriverProc(void) {
	__asm jmp pfn_DefDriverProc;
}
ALCDECL fake_DriverCallback(void) {
	__asm jmp pfn_DriverCallback;
}
ALCDECL fake_DrvGetModuleHandle(void) {
	__asm jmp pfn_DrvGetModuleHandle;
}
ALCDECL fake_GetDriverModuleHandle(void) {
	__asm jmp pfn_GetDriverModuleHandle;
}
ALCDECL fake_NotifyCallbackData(void) {
	__asm jmp pfn_NotifyCallbackData;
}
ALCDECL fake_OpenDriver(void) {
	__asm jmp pfn_OpenDriver;
}
ALCDECL fake_PlaySound(void) {
	__asm jmp pfn_PlaySound;
}
ALCDECL fake_PlaySoundA(void) {
	__asm jmp pfn_PlaySoundA;
}
ALCDECL fake_PlaySoundW(void) {
	__asm jmp pfn_PlaySoundW;
}
ALCDECL fake_SendDriverMessage(void) {
	__asm jmp pfn_SendDriverMessage;
}
ALCDECL fake_WOW32DriverCallback(void) {
	__asm jmp pfn_WOW32DriverCallback;
}
ALCDECL fake_WOW32ResolveMultiMediaHandle(void) {
	__asm jmp pfn_WOW32ResolveMultiMediaHandle;
}
ALCDECL fake_WOWAppExit(void) {
	__asm jmp pfn_WOWAppExit;
}
ALCDECL fake_aux32Message(void) {
	__asm jmp pfn_aux32Message;
}
// ignore for winmm wrapper
//ALCDECL fake_auxGetDevCapsA(void) {
//	__asm jmp pfn_auxGetDevCapsA;
//}
ALCDECL fake_auxGetDevCapsW(void) {
	__asm jmp pfn_auxGetDevCapsW;
}
// ignore for winmm wrapper
//ALCDECL fake_auxGetNumDevs(void) {
//	__asm jmp pfn_auxGetNumDevs;
//}
//ALCDECL fake_auxGetVolume(void) {
//	__asm jmp pfn_auxGetVolume;
//}
ALCDECL fake_auxOutMessage(void) {
	__asm jmp pfn_auxOutMessage;
}
// ignore for winmm wrapper
//ALCDECL fake_auxSetVolume(void) {
//	__asm jmp pfn_auxSetVolume;
//}
ALCDECL fake_joy32Message(void) {
	__asm jmp pfn_joy32Message;
}
ALCDECL fake_joyConfigChanged(void) {
	__asm jmp pfn_joyConfigChanged;
}
ALCDECL fake_joyGetDevCapsA(void) {
	__asm jmp pfn_joyGetDevCapsA;
}
ALCDECL fake_joyGetDevCapsW(void) {
	__asm jmp pfn_joyGetDevCapsW;
}
ALCDECL fake_joyGetNumDevs(void) {
	__asm jmp pfn_joyGetNumDevs;
}
ALCDECL fake_joyGetPos(void) {
	__asm jmp pfn_joyGetPos;
}
ALCDECL fake_joyGetPosEx(void) {
	__asm jmp pfn_joyGetPosEx;
}
ALCDECL fake_joyGetThreshold(void) {
	__asm jmp pfn_joyGetThreshold;
}
ALCDECL fake_joyReleaseCapture(void) {
	__asm jmp pfn_joyReleaseCapture;
}
ALCDECL fake_joySetCapture(void) {
	__asm jmp pfn_joySetCapture;
}
ALCDECL fake_joySetThreshold(void) {
	__asm jmp pfn_joySetThreshold;
}
ALCDECL fake_mci32Message(void) {
	__asm jmp pfn_mci32Message;
}
ALCDECL fake_mciDriverNotify(void) {
	__asm jmp pfn_mciDriverNotify;
}
ALCDECL fake_mciDriverYield(void) {
	__asm jmp pfn_mciDriverYield;
}
ALCDECL fake_mciFreeCommandResource(void) {
	__asm jmp pfn_mciFreeCommandResource;
}
ALCDECL fake_mciGetCreatorTask(void) {
	__asm jmp pfn_mciGetCreatorTask;
}
ALCDECL fake_mciGetDeviceIDA(void) {
	__asm jmp pfn_mciGetDeviceIDA;
}
ALCDECL fake_mciGetDeviceIDFromElementIDA(void) {
	__asm jmp pfn_mciGetDeviceIDFromElementIDA;
}
ALCDECL fake_mciGetDeviceIDFromElementIDW(void) {
	__asm jmp pfn_mciGetDeviceIDFromElementIDW;
}
ALCDECL fake_mciGetDeviceIDW(void) {
	__asm jmp pfn_mciGetDeviceIDW;
}
ALCDECL fake_mciGetDriverData(void) {
	__asm jmp pfn_mciGetDriverData;
}
ALCDECL fake_mciGetErrorStringA(void) {
	__asm jmp pfn_mciGetErrorStringA;
}
ALCDECL fake_mciGetErrorStringW(void) {
	__asm jmp pfn_mciGetErrorStringW;
}
ALCDECL fake_mciGetYieldProc(void) {
	__asm jmp pfn_mciGetYieldProc;
}
ALCDECL fake_mciLoadCommandResource(void) {
	__asm jmp pfn_mciLoadCommandResource;
}
// ignore for winmm wrapper
//ALCDECL fake_mciSendCommandA(void) {
//	__asm jmp pfn_mciSendCommandA;
//}
ALCDECL fake_mciSendCommandW(void) {
	__asm jmp pfn_mciSendCommandW;
}
// ignore for winmm wrapper
//ALCDECL fake_mciSendStringA(void) {
//	__asm jmp pfn_mciSendStringA;
//}
ALCDECL fake_mciSendStringW(void) {
	__asm jmp pfn_mciSendStringW;
}
ALCDECL fake_mciSetDriverData(void) {
	__asm jmp pfn_mciSetDriverData;
}
ALCDECL fake_mciSetYieldProc(void) {
	__asm jmp pfn_mciSetYieldProc;
}
ALCDECL fake_mid32Message(void) {
	__asm jmp pfn_mid32Message;
}
ALCDECL fake_midiConnect(void) {
	__asm jmp pfn_midiConnect;
}
ALCDECL fake_midiDisconnect(void) {
	__asm jmp pfn_midiDisconnect;
}
ALCDECL fake_midiInAddBuffer(void) {
	__asm jmp pfn_midiInAddBuffer;
}
ALCDECL fake_midiInClose(void) {
	__asm jmp pfn_midiInClose;
}
ALCDECL fake_midiInGetDevCapsA(void) {
	__asm jmp pfn_midiInGetDevCapsA;
}
ALCDECL fake_midiInGetDevCapsW(void) {
	__asm jmp pfn_midiInGetDevCapsW;
}
ALCDECL fake_midiInGetErrorTextA(void) {
	__asm jmp pfn_midiInGetErrorTextA;
}
ALCDECL fake_midiInGetErrorTextW(void) {
	__asm jmp pfn_midiInGetErrorTextW;
}
ALCDECL fake_midiInGetID(void) {
	__asm jmp pfn_midiInGetID;
}
ALCDECL fake_midiInGetNumDevs(void) {
	__asm jmp pfn_midiInGetNumDevs;
}
ALCDECL fake_midiInMessage(void) {
	__asm jmp pfn_midiInMessage;
}
ALCDECL fake_midiInOpen(void) {
	__asm jmp pfn_midiInOpen;
}
ALCDECL fake_midiInPrepareHeader(void) {
	__asm jmp pfn_midiInPrepareHeader;
}
ALCDECL fake_midiInReset(void) {
	__asm jmp pfn_midiInReset;
}
ALCDECL fake_midiInStart(void) {
	__asm jmp pfn_midiInStart;
}
ALCDECL fake_midiInStop(void) {
	__asm jmp pfn_midiInStop;
}
ALCDECL fake_midiInUnprepareHeader(void) {
	__asm jmp pfn_midiInUnprepareHeader;
}
ALCDECL fake_midiOutCacheDrumPatches(void) {
	__asm jmp pfn_midiOutCacheDrumPatches;
}
ALCDECL fake_midiOutCachePatches(void) {
	__asm jmp pfn_midiOutCachePatches;
}
ALCDECL fake_midiOutClose(void) {
	__asm jmp pfn_midiOutClose;
}
ALCDECL fake_midiOutGetDevCapsA(void) {
	__asm jmp pfn_midiOutGetDevCapsA;
}
ALCDECL fake_midiOutGetDevCapsW(void) {
	__asm jmp pfn_midiOutGetDevCapsW;
}
ALCDECL fake_midiOutGetErrorTextA(void) {
	__asm jmp pfn_midiOutGetErrorTextA;
}
ALCDECL fake_midiOutGetErrorTextW(void) {
	__asm jmp pfn_midiOutGetErrorTextW;
}
ALCDECL fake_midiOutGetID(void) {
	__asm jmp pfn_midiOutGetID;
}
ALCDECL fake_midiOutGetNumDevs(void) {
	__asm jmp pfn_midiOutGetNumDevs;
}
ALCDECL fake_midiOutGetVolume(void) {
	__asm jmp pfn_midiOutGetVolume;
}
ALCDECL fake_midiOutLongMsg(void) {
	__asm jmp pfn_midiOutLongMsg;
}
ALCDECL fake_midiOutMessage(void) {
	__asm jmp pfn_midiOutMessage;
}
ALCDECL fake_midiOutOpen(void) {
	__asm jmp pfn_midiOutOpen;
}
ALCDECL fake_midiOutPrepareHeader(void) {
	__asm jmp pfn_midiOutPrepareHeader;
}
ALCDECL fake_midiOutReset(void) {
	__asm jmp pfn_midiOutReset;
}
ALCDECL fake_midiOutSetVolume(void) {
	__asm jmp pfn_midiOutSetVolume;
}
ALCDECL fake_midiOutShortMsg(void) {
	__asm jmp pfn_midiOutShortMsg;
}
ALCDECL fake_midiOutUnprepareHeader(void) {
	__asm jmp pfn_midiOutUnprepareHeader;
}
ALCDECL fake_midiStreamClose(void) {
	__asm jmp pfn_midiStreamClose;
}
ALCDECL fake_midiStreamOpen(void) {
	__asm jmp pfn_midiStreamOpen;
}
ALCDECL fake_midiStreamOut(void) {
	__asm jmp pfn_midiStreamOut;
}
ALCDECL fake_midiStreamPause(void) {
	__asm jmp pfn_midiStreamPause;
}
ALCDECL fake_midiStreamPosition(void) {
	__asm jmp pfn_midiStreamPosition;
}
ALCDECL fake_midiStreamProperty(void) {
	__asm jmp pfn_midiStreamProperty;
}
ALCDECL fake_midiStreamRestart(void) {
	__asm jmp pfn_midiStreamRestart;
}
ALCDECL fake_midiStreamStop(void) {
	__asm jmp pfn_midiStreamStop;
}
ALCDECL fake_mixerClose(void) {
	__asm jmp pfn_mixerClose;
}
ALCDECL fake_mixerGetControlDetailsA(void) {
	__asm jmp pfn_mixerGetControlDetailsA;
}
ALCDECL fake_mixerGetControlDetailsW(void) {
	__asm jmp pfn_mixerGetControlDetailsW;
}
ALCDECL fake_mixerGetDevCapsA(void) {
	__asm jmp pfn_mixerGetDevCapsA;
}
ALCDECL fake_mixerGetDevCapsW(void) {
	__asm jmp pfn_mixerGetDevCapsW;
}
ALCDECL fake_mixerGetID(void) {
	__asm jmp pfn_mixerGetID;
}
ALCDECL fake_mixerGetLineControlsA(void) {
	__asm jmp pfn_mixerGetLineControlsA;
}
ALCDECL fake_mixerGetLineControlsW(void) {
	__asm jmp pfn_mixerGetLineControlsW;
}
ALCDECL fake_mixerGetLineInfoA(void) {
	__asm jmp pfn_mixerGetLineInfoA;
}
ALCDECL fake_mixerGetLineInfoW(void) {
	__asm jmp pfn_mixerGetLineInfoW;
}
ALCDECL fake_mixerGetNumDevs(void) {
	__asm jmp pfn_mixerGetNumDevs;
}
ALCDECL fake_mixerMessage(void) {
	__asm jmp pfn_mixerMessage;
}
ALCDECL fake_mixerOpen(void) {
	__asm jmp pfn_mixerOpen;
}
ALCDECL fake_mixerSetControlDetails(void) {
	__asm jmp pfn_mixerSetControlDetails;
}
ALCDECL fake_mmDrvInstall(void) {
	__asm jmp pfn_mmDrvInstall;
}
ALCDECL fake_mmGetCurrentTask(void) {
	__asm jmp pfn_mmGetCurrentTask;
}
ALCDECL fake_mmTaskBlock(void) {
	__asm jmp pfn_mmTaskBlock;
}
ALCDECL fake_mmTaskCreate(void) {
	__asm jmp pfn_mmTaskCreate;
}
ALCDECL fake_mmTaskSignal(void) {
	__asm jmp pfn_mmTaskSignal;
}
ALCDECL fake_mmTaskYield(void) {
	__asm jmp pfn_mmTaskYield;
}
ALCDECL fake_mmioAdvance(void) {
	__asm jmp pfn_mmioAdvance;
}
ALCDECL fake_mmioAscend(void) {
	__asm jmp pfn_mmioAscend;
}
ALCDECL fake_mmioClose(void) {
	__asm jmp pfn_mmioClose;
}
ALCDECL fake_mmioCreateChunk(void) {
	__asm jmp pfn_mmioCreateChunk;
}
ALCDECL fake_mmioDescend(void) {
	__asm jmp pfn_mmioDescend;
}
ALCDECL fake_mmioFlush(void) {
	__asm jmp pfn_mmioFlush;
}
ALCDECL fake_mmioGetInfo(void) {
	__asm jmp pfn_mmioGetInfo;
}
ALCDECL fake_mmioInstallIOProcA(void) {
	__asm jmp pfn_mmioInstallIOProcA;
}
ALCDECL fake_mmioInstallIOProcW(void) {
	__asm jmp pfn_mmioInstallIOProcW;
}
ALCDECL fake_mmioOpenA(void) {
	__asm jmp pfn_mmioOpenA;
}
ALCDECL fake_mmioOpenW(void) {
	__asm jmp pfn_mmioOpenW;
}
ALCDECL fake_mmioRead(void) {
	__asm jmp pfn_mmioRead;
}
ALCDECL fake_mmioRenameA(void) {
	__asm jmp pfn_mmioRenameA;
}
ALCDECL fake_mmioRenameW(void) {
	__asm jmp pfn_mmioRenameW;
}
ALCDECL fake_mmioSeek(void) {
	__asm jmp pfn_mmioSeek;
}
ALCDECL fake_mmioSendMessage(void) {
	__asm jmp pfn_mmioSendMessage;
}
ALCDECL fake_mmioSetBuffer(void) {
	__asm jmp pfn_mmioSetBuffer;
}
ALCDECL fake_mmioSetInfo(void) {
	__asm jmp pfn_mmioSetInfo;
}
ALCDECL fake_mmioStringToFOURCCA(void) {
	__asm jmp pfn_mmioStringToFOURCCA;
}
ALCDECL fake_mmioStringToFOURCCW(void) {
	__asm jmp pfn_mmioStringToFOURCCW;
}
ALCDECL fake_mmioWrite(void) {
	__asm jmp pfn_mmioWrite;
}
ALCDECL fake_mmsystemGetVersion(void) {
	__asm jmp pfn_mmsystemGetVersion;
}
ALCDECL fake_mod32Message(void) {
	__asm jmp pfn_mod32Message;
}
ALCDECL fake_mxd32Message(void) {
	__asm jmp pfn_mxd32Message;
}
ALCDECL fake_sndPlaySoundA(void) {
	__asm jmp pfn_sndPlaySoundA;
}
ALCDECL fake_sndPlaySoundW(void) {
	__asm jmp pfn_sndPlaySoundW;
}
ALCDECL fake_tid32Message(void) {
	__asm jmp pfn_tid32Message;
}
ALCDECL fake_timeBeginPeriod(void) {
	__asm jmp pfn_timeBeginPeriod;
}
ALCDECL fake_timeEndPeriod(void) {
	__asm jmp pfn_timeEndPeriod;
}
ALCDECL fake_timeGetDevCaps(void) {
	__asm jmp pfn_timeGetDevCaps;
}
ALCDECL fake_timeGetSystemTime(void) {
	__asm jmp pfn_timeGetSystemTime;
}
ALCDECL fake_timeGetTime(void) {
	__asm jmp pfn_timeGetTime;
}
ALCDECL fake_timeKillEvent(void) {
	__asm jmp pfn_timeKillEvent;
}
ALCDECL fake_timeSetEvent(void) {
	__asm jmp pfn_timeSetEvent;
}
ALCDECL fake_waveInAddBuffer(void) {
	__asm jmp pfn_waveInAddBuffer;
}
ALCDECL fake_waveInClose(void) {
	__asm jmp pfn_waveInClose;
}
ALCDECL fake_waveInGetDevCapsA(void) {
	__asm jmp pfn_waveInGetDevCapsA;
}
ALCDECL fake_waveInGetDevCapsW(void) {
	__asm jmp pfn_waveInGetDevCapsW;
}
ALCDECL fake_waveInGetErrorTextA(void) {
	__asm jmp pfn_waveInGetErrorTextA;
}
ALCDECL fake_waveInGetErrorTextW(void) {
	__asm jmp pfn_waveInGetErrorTextW;
}
ALCDECL fake_waveInGetID(void) {
	__asm jmp pfn_waveInGetID;
}
ALCDECL fake_waveInGetNumDevs(void) {
	__asm jmp pfn_waveInGetNumDevs;
}
ALCDECL fake_waveInGetPosition(void) {
	__asm jmp pfn_waveInGetPosition;
}
ALCDECL fake_waveInMessage(void) {
	__asm jmp pfn_waveInMessage;
}
ALCDECL fake_waveInOpen(void) {
	__asm jmp pfn_waveInOpen;
}
ALCDECL fake_waveInPrepareHeader(void) {
	__asm jmp pfn_waveInPrepareHeader;
}
ALCDECL fake_waveInReset(void) {
	__asm jmp pfn_waveInReset;
}
ALCDECL fake_waveInStart(void) {
	__asm jmp pfn_waveInStart;
}
ALCDECL fake_waveInStop(void) {
	__asm jmp pfn_waveInStop;
}
ALCDECL fake_waveInUnprepareHeader(void) {
	__asm jmp pfn_waveInUnprepareHeader;
}
ALCDECL fake_waveOutBreakLoop(void) {
	__asm jmp pfn_waveOutBreakLoop;
}
ALCDECL fake_waveOutClose(void) {
	__asm jmp pfn_waveOutClose;
}
ALCDECL fake_waveOutGetDevCapsA(void) {
	__asm jmp pfn_waveOutGetDevCapsA;
}
ALCDECL fake_waveOutGetDevCapsW(void) {
	__asm jmp pfn_waveOutGetDevCapsW;
}
ALCDECL fake_waveOutGetErrorTextA(void) {
	__asm jmp pfn_waveOutGetErrorTextA;
}
ALCDECL fake_waveOutGetErrorTextW(void) {
	__asm jmp pfn_waveOutGetErrorTextW;
}
ALCDECL fake_waveOutGetID(void) {
	__asm jmp pfn_waveOutGetID;
}
ALCDECL fake_waveOutGetNumDevs(void) {
	__asm jmp pfn_waveOutGetNumDevs;
}
ALCDECL fake_waveOutGetPitch(void) {
	__asm jmp pfn_waveOutGetPitch;
}
ALCDECL fake_waveOutGetPlaybackRate(void) {
	__asm jmp pfn_waveOutGetPlaybackRate;
}
ALCDECL fake_waveOutGetPosition(void) {
	__asm jmp pfn_waveOutGetPosition;
}
ALCDECL fake_waveOutGetVolume(void) {
	__asm jmp pfn_waveOutGetVolume;
}
ALCDECL fake_waveOutMessage(void) {
	__asm jmp pfn_waveOutMessage;
}
ALCDECL fake_waveOutOpen(void) {
	__asm jmp pfn_waveOutOpen;
}
ALCDECL fake_waveOutPause(void) {
	__asm jmp pfn_waveOutPause;
}
ALCDECL fake_waveOutPrepareHeader(void) {
	__asm jmp pfn_waveOutPrepareHeader;
}
ALCDECL fake_waveOutReset(void) {
	__asm jmp pfn_waveOutReset;
}
ALCDECL fake_waveOutRestart(void) {
	__asm jmp pfn_waveOutRestart;
}
ALCDECL fake_waveOutSetPitch(void) {
	__asm jmp pfn_waveOutSetPitch;
}
ALCDECL fake_waveOutSetPlaybackRate(void) {
	__asm jmp pfn_waveOutSetPlaybackRate;
}
ALCDECL fake_waveOutSetVolume(void) {
	__asm jmp pfn_waveOutSetVolume;
}
ALCDECL fake_waveOutUnprepareHeader(void) {
	__asm jmp pfn_waveOutUnprepareHeader;
}
ALCDECL fake_waveOutWrite(void) {
	__asm jmp pfn_waveOutWrite;
}
ALCDECL fake_wid32Message(void) {
	__asm jmp pfn_wid32Message;
}
ALCDECL fake_wod32Message(void) {
	__asm jmp pfn_wod32Message;
}

// Relay original function
MCIERROR WINAPI relay_mciSendCommandA(MCIDEVICEID a0, UINT a1, DWORD_PTR a2, DWORD_PTR a3) {
	return (*f_mciSendCommandA)(a0, a1, a2, a3);
}
MCIERROR WINAPI relay_mciSendStringA(LPCSTR a0, LPSTR a1, UINT a2, HWND a3) {
	return (*f_mciSendStringA)(a0, a1, a2, a3);
}
UINT WINAPI relay_auxGetNumDevs() {
	return (*f_auxGetNumDevs)();
}
MMRESULT WINAPI relay_auxGetDevCapsA(UINT_PTR a0, LPAUXCAPSA a1, UINT a2) {
	return (*f_auxGetDevCapsA)(a0, a1, a2);
}
MMRESULT WINAPI relay_auxGetVolume(UINT a0, LPDWORD a1) {
	return (*f_auxGetVolume)(a0, a1);
}
MMRESULT WINAPI relay_auxSetVolume(UINT a0, DWORD a1) {
	return (*f_auxSetVolume)(a0, a1);
}
MMRESULT WINAPI relay_waveOutGetVolume(UINT a0, LPDWORD a1) {
	return (*f_waveOutGetVolume)(a0, a1);
}
MMRESULT WINAPI relay_waveOutSetVolume(UINT a0, DWORD a1) {
	return (*f_waveOutSetVolume)(a0, a1);
}
