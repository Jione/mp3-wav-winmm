#pragma once
#ifndef WINMM_STUBS_HEADER_H
#define WINMM_STUBS_HEADER_H
#define WIN32_LEAN_AND_MEAN
#define EXTERNC extern "C"
#define NAKED __declspec(naked)
#define EXPORT __declspec(dllexport)
#define ALCPP EXPORT NAKED
#define ALSTD EXTERNC EXPORT NAKED void __stdcall
#define ALCFAST EXTERNC EXPORT NAKED void __fastcall
#define ALCDECL EXTERNC NAKED void __cdecl

#include <windows.h>
#include <stdio.h>
//#pragma comment (lib, "winmm.lib")
#include <mmsystem.h>

BOOL HookInitialize(HMODULE hModule, DWORD dwReason);
HWND GetCurrentHWND();
BOOL GetParentPath(LPTSTR lpszProcessPath);
HANDLE FindProcess(LPTSTR pExeName);

MCIERROR WINAPI relay_mciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam);
MCIERROR WINAPI relay_mciSendStringA(LPCSTR lpCommand, LPSTR lpReturnString, UINT cchReturn, HWND hwndCallback);
UINT WINAPI relay_auxGetNumDevs();
MMRESULT WINAPI relay_auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT cbCaps);
MMRESULT WINAPI relay_auxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume);
MMRESULT WINAPI relay_auxSetVolume(UINT uDeviceID, DWORD dwVolume);
MMRESULT WINAPI relay_waveOutGetVolume(UINT uDeviceID, LPDWORD lpdwVolume);
MMRESULT WINAPI relay_waveOutSetVolume(UINT uDeviceID, DWORD dwVolume);
#endif