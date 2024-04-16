/*
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
  PARTICULAR PURPOSE.

  (c) 2020 DD
  ________________________________________________________________________
  cdaudio player using MCI Strings with the
  objective of fixing regression issue in cdaudio
  playback starting with Windows Vista. Mainly the
  lack of a working mode update after playing has
  finished (missing MCI_NOTIFY_SUCCESSFUL msg).

  Rewritten in 2022.
  ________________________________________________________________________
  When cdaudioplr.exe is auto-started by the wrapper it may inherit the CPU
  affinity of the game program. A particularly problematic case is the original
  Midtown Madness game executable which runs in a high priority class and sets
  the player to run on the first CPU core (single core affinity). This results
  in the player hanging unless the mouse cursor is moved around.

  Set cdaudioplr.exe to run in high priority:
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
  ________________________________________________________________________
  Changed to new code by u/jione (2024)
  All code has been rewritten and the internal workings have been changed.

  New:   Code refactoring
  New:   Supports tray icon function and moves to tray icon when minimize
  New:   Support for multi-application execution (incomplete)
  Modify:Override function to master volume function by connecting auxSet-
		 Volume to mciSendString setaudio (incomplete)
  ________________________________________________________________________
*/
#include "winmm_player.h"
#include "resource.h"

#define IDT_CDAPLR_TIMER 1

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	WNDCLASSEX wndClass{};
	MSG cbMessage;
	SYSTEM_INFO sysInfo;

	// ini load
	LoadInitialize(&iniStatus);
	guiVolume.selectLevel = iniStatus.overrideVolume * 10.0;

	// Checks that program is not already running
	snprintf(procMutex, MAX_PATH, "%s%s", procServerName, mutexSignature);
	playerMutex.procMutex = CreateMutexA(NULL, TRUE, procMutex);
	if (GetLastError() == ERROR_ALREADY_EXISTS) { return 0; }

	// Start debugging
	if (iniStatus.useDebugEXE) {
		fh = fopen("winmm_player.log", "w");
	}
	dprintf_nl("Beginning of debug log");

	// Set affinity to last CPU core
	if (iniStatus.useLastCore) {
		GetSystemInfo(&sysInfo);
		SetProcessAffinityMask(GetCurrentProcess(), sysInfo.dwNumberOfProcessors);
	}

	// Register and create window information
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = szClassName;
	wndClass.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0);

	if (!RegisterClassEx(&wndClass)) {
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		szClassName,
		"cdaudio-winmm player",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 180,
		NULL, NULL, hInstance, NULL
	);

	if (hWnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Start minimized
	SendMessageA(hWnd, WM_TRAY_SETUP, NULL, NULL);
	if (iniStatus.minimizeStartup) { ShowWindow(hWnd, SW_HIDE); }	// Start with tray icon.
	else { ShowWindow(hWnd, SW_SHOWNOACTIVATE); }					// Start with inactive window.
	UpdateWindow(hWnd);


	// Set timer orr set a named timer
	if (!iniStatus.useLastCore) {
		SetTimer(hWnd, IDT_CDAPLR_TIMER, 10, NULL);	// Use a named timer so we can handle its message.
	}

	// Make mutex
	sprintf(procServer, "\\\\.\\Mailslot\\%s_%s", procServerName, mailslotSignature);
	playerMutex.playCommand = CreateMutexA(NULL, FALSE, NULL);
	playerMutex.toDLLSendMsg = CreateMutexA(NULL, FALSE, NULL);
	playerMutex.fromDLLSendMsg = CreateMutexA(NULL, FALSE, NULL);

	// Start threads
	playerServer.receiveServer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveIPCMessageServer, NULL, 0, NULL);
	playerServer.watchdogPlay = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WatchdogPlaytime, NULL, 0, NULL);

	// Message Loop
	while (GetMessage(&cbMessage, NULL, 0, 0) > 0) {
		TranslateMessage(&cbMessage);
		DispatchMessage(&cbMessage);
	}

	// running after destroyed window
	CloseUsedHandle();

	// debug logging 
	dprintf_nl("End of debug log.");
	if (fh) { fclose(fh); }
	return cbMessage.wParam;
}

// Destroyed window handle
void CloseUsedHandle() {
	ReleaseMutexSafe(playerMutex.procMutex);
	ReleaseMutexSafe(playerMutex.playCommand);
	ReleaseMutexSafe(playerMutex.fromDLLSendMsg);
	ReleaseMutexSafe(playerMutex.toDLLSendMsg);
	CloseHandleSafe(playerMutex.procMutex);
	CloseHandleSafe(playerMutex.playCommand);
	CloseHandleSafe(playerMutex.fromDLLSendMsg);
	CloseHandleSafe(playerMutex.toDLLSendMsg);
	CloseHandleSafe(playerServer.receiveServer);
	CloseHandleSafe(playerServer.watchdogPlay);
}

void WatchdogPlaytime() {
	PLAY_INFO infoMap{};
	int i;
	DWORD szMap, dwTick, dwMillisecs = 0;
	char cmdString[MAX_PATH]{}, printString[MAX_PATH]{};
	char returnString[0x80]{};
	MM_RESPONSE responseData{};
	while(1) {
		szMap = PIDMap.size();
		if (szMap) {
			auto iter = PIDMap.begin();
			for (i = 0; i < szMap; i++) {
				infoMap = iter->second;
				if (infoMap.lastStatus == 0) {
					mciSendStr(cmdString, returnString, "status %08x position", infoMap.pid);
					if (strncmp(infoMap.lastPlayFile, "cdaudio", 7)) {
						if (!sscanf(returnString, "%d", &dwMillisecs)) {
							dwMillisecs = 0;
						}
					}
					else {
						int t = 0, m = 0, s = 0, f = 0;
						if (sscanf(returnString, "%d:%d:%d:%d", &t, &m, &s, &f) == 3) {
							f = s;
							s = m;
							m = t;
						}
						dwMillisecs = (m * 60000) + (s * 1000) + (f * 40 / 3);
					}
					dwMillisecs -= dwTick = GetTickCount64();
					if (dwMillisecs < 0x480) {
						memset(&responseData, 0, sizeof(responseData));
						responseData.timePlayStart = dwTick;
						//memcpy(&responseData.rightVolume, &infoMap.lastVolume, sizeof(DWORD));
					}
					//SendIPCMessage(infoMap.pid, &responseData);
				}
				iter++;
			}
			memset(printString, 0, sizeof(printString));
			snprintf(
				printString, sizeof(printString),
				"Running Process: %d\nStatus: %s\nType: %s\nLast play: Track%02d",
				szMap,
				infoMap.lastStatus ? "Stopped" : "Playing",
				strncmp(infoMap.lastPlayFile, "cdaudio", 7) ? "wav/mp3" : "CD Audio",
				infoMap.lastTrack
			);
			SetWindowTextA(hEdit, printString);
		}
		else {
			memset(printString, 0, sizeof(printString));
			snprintf(printString, sizeof(printString), "Wait MCI Process...");
			SetWindowTextA(hEdit, printString);
		}
		Sleep(1000);
	}
}

// IPC Mailslot message receiving thread
void ReceiveIPCMessageServer() {
	HANDLE Mailslot;
	MM_REQUEST modeRequest{};
	DWORD readBytes = 0;

	// try 300 times for create Mailslot
	Mailslot = CreateMailslotA(procServer, 0, MAILSLOT_WAIT_FOREVER, NULL);
	for (int i = 0; i < 300; i++) {
		if ((Mailslot != INVALID_HANDLE_VALUE) && (Mailslot != NULL)) { break; }
		Sleep(1);
		Mailslot = CreateMailslotA(procServer, 0, MAILSLOT_WAIT_FOREVER, NULL);
	}
	if ((Mailslot == INVALID_HANDLE_VALUE) || (Mailslot == NULL)) {
		dprintf_nl("Unable to create receive server");
		return;
	}

	while (1) {
		if (WaitForSingleObject(playerMutex.fromDLLSendMsg, globalTimeout) == WAIT_TIMEOUT) {
			dprintf_nl("********Timeout ReceiveIPCMessageServer, Check Deadlock********");
		}
		if (ReadFile(Mailslot, &modeRequest, sizeof(MM_REQUEST), &readBytes, NULL)) {
			if (readBytes == sizeof(MM_REQUEST)) {

				if ((modeRequest.requestMode & WPR_NOTIFY) || (modeRequest.requestMode & WPR_OPEN)) {
					RegisterPID(modeRequest.hWndProcID, modeRequest.setRightVolume, modeRequest.setLeftVolume);
				}

				if (modeRequest.requestMode & WPR_SET_VOLUME) {
					DWORD vol;
					memcpy(&vol, &modeRequest.setRightVolume, sizeof(DWORD));
					MusicVolumeControl(modeRequest.hWndProcID, vol);
				}

				if (modeRequest.requestMode & WPR_CLOSE) {
					UnregisterPID(modeRequest.hWndProcID);
					if ((modeRequest.requestMode & WPR_EXIT) && PIDMap.empty()) {
						PostMessageA(hWnd, WM_CLOSE, 0, 0);
					}
				}
				else if (modeRequest.requestMode & WPR_PLAY) {
					if (modeRequest.setTrackFrom < modeRequest.setTrackTo) {
						MusicPlay(modeRequest.hWndProcID, modeRequest.musicPath, modeRequest.setTrackFrom, modeRequest.setTrackFromTime, 0);
					}
					else if (modeRequest.setTrackFromTime < static_cast<int>(modeRequest.setTrackToTime)) {
						MusicPlay(modeRequest.hWndProcID, modeRequest.musicPath, modeRequest.setTrackFrom, modeRequest.setTrackFromTime, 0);
					}
					else {
						MusicPlay(modeRequest.hWndProcID, modeRequest.musicPath, modeRequest.setTrackFrom, modeRequest.setTrackFromTime, modeRequest.setTrackToTime);
					}

				}
				else if (modeRequest.requestMode & (WPR_STOP | WPR_PAUSE)) {
					MusicStop(modeRequest.hWndProcID);
				}
			}
		}
		ReleaseMutex(playerMutex.fromDLLSendMsg);
		Sleep(1);
	}
}


// IPC Mailslot message receiving thread
BOOL SendIPCMessage(DWORD procIdentifier, MM_RESPONSE* responseData) {
	char dllServer[MAX_PATH];
	DWORD writtenBytes = 0;
	DWORD cbMessage, cMessage;
	BOOL fResult;
	HANDLE Mailslot;

	sprintf(dllServer, "\\\\.\\Mailslot\\%s_%08x", dllServerName, procIdentifier);

	// try 3 times for open Mailslot
	Mailslot = CreateFile(dllServer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ((Mailslot == INVALID_HANDLE_VALUE) || (Mailslot == NULL)) {
		for (int i = 0; i < 3; i++) {
			if ((Mailslot != INVALID_HANDLE_VALUE) && (Mailslot != NULL)) break;
			Sleep(1);
			Mailslot = CreateFile(dllServer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		return FALSE;
	}

	if (WaitForSingleObject(playerMutex.toDLLSendMsg, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout SendIPCMessage, Check Deadlock********");
	}

	// Send request message
	WriteFile(Mailslot, responseData, sizeof(MM_RESPONSE), &writtenBytes, NULL);
	if (writtenBytes != sizeof(MM_RESPONSE)) { dprintf_nl("> Error: Request failed"); }

	// Check read message
	fResult = GetMailslotInfo(Mailslot, NULL, &cbMessage, &cMessage, NULL);
	while (fResult && (cbMessage != MAILSLOT_NO_MESSAGE)) {
		fResult = GetMailslotInfo(Mailslot, NULL, &cbMessage, &cMessage, NULL);
		Sleep(1);
	}
	if (!fResult) {
		dprintf_nl("GetMailslotInfo failed with %d.\n", GetLastError());
	}
	CloseHandle(Mailslot);
	ReleaseMutex(playerMutex.toDLLSendMsg);
	return TRUE;
}

// Preparing an Open MCI Driver for Music Playback
void RegisterPID (DWORD procIdentifier, WORD rightVol, WORD leftVol) {
	if (WaitForSingleObject(playerMutex.playCommand, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout RegisterPID, Check Deadlock********");
	}
	if (PIDMap.find(procIdentifier) != PIDMap.end()) {
		dprintf_nl("id:%08x is already loaded", procIdentifier);
		dprintf_nl("reload data....");
		UnregisterPID(procIdentifier);
	}

	PLAY_INFO playInfo{};
	PIDMap.insert({ procIdentifier, playInfo });
	auto iterTrack = PIDMap.find(procIdentifier);
	iterTrack->second.pid = procIdentifier;
	iterTrack->second.lastStatus = -1;
	iterTrack->second.lastVolume = (rightVol << 0x10) | leftVol;
	ReleaseMutex(playerMutex.playCommand);
}

// Close all MCI drivers assigned to procID and unregister map
void UnregisterPID(DWORD procIdentifier) {
	if (WaitForSingleObject(playerMutex.playCommand, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout UnregisterPID, Check Deadlock********");
	}
	auto iterTrack = PIDMap.find(procIdentifier);
	if (iterTrack == PIDMap.end()) {
		ReleaseMutex(playerMutex.playCommand);
		return;
	}
	if (iterTrack->second.lastStatus != -1) {
		char cmdString[MAX_PATH]{};
		char returnString[0x80]{};
		if (iterTrack->second.lastStatus == 0) {
			mciSendStr(cmdString, returnString, "stop %08x", procIdentifier);
		}
		mciSendStr(cmdString, returnString, "close %08x wait", procIdentifier);
	}
	PIDMap.erase(procIdentifier);
	ReleaseMutex(playerMutex.playCommand);
}


// Music Play Command Functions
BOOL MusicPlay(DWORD procIdentifier, LPCSTR trackPath, DWORD trackNum, DWORD startTime, DWORD endTime) {
	if (WaitForSingleObject(playerMutex.playCommand, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout MusicPlay, Check Deadlock********");
	}
	auto iterTrack = PIDMap.find(procIdentifier);
	if (iterTrack == PIDMap.end()) {
		dprintf_nl("Music play: Not initialized procID(%08X), request: track%02d, start: %dms, end: %dms", procIdentifier, trackNum, startTime, endTime);
		ReleaseMutex(playerMutex.playCommand);
		return FALSE;
	}

	char cmdString[MAX_PATH]{};
	char returnString[0x80]{};
	BOOL isCDAudio;
	isCDAudio = strncmp(trackPath, "cdaudio", 7) ? 0 : 1;
	iterTrack->second.lastTrack = trackNum;

	if (iterTrack->second.lastStatus == -1) {
		if (isCDAudio) {
			mciSendStr(cmdString, returnString, "open cdaudio alias %08x wait", procIdentifier);
			mciSendStr(cmdString, returnString, "set %08x time format tmsf wait", procIdentifier);
		}
		else {
			mciSendStr(cmdString, returnString, "open \"%s\" type mpegvideo alias %08x", trackPath, procIdentifier);
			mciSendStr(cmdString, returnString, "set %08x time format milliseconds", procIdentifier);
		}
	}
	else {
		if (iterTrack->second.lastStatus == 0) {
			mciSendStr(cmdString, returnString, "stop %08x", procIdentifier);
		}
		if (!isCDAudio) {
			if (strcmp(iterTrack->second.lastPlayFile, trackPath)) {
				mciSendStr(cmdString, returnString, "close %08x", procIdentifier);
				mciSendStr(cmdString, returnString, "open \"%s\" type mpegvideo alias %08x", trackPath, procIdentifier);
				mciSendStr(cmdString, returnString, "set %08x time format milliseconds", procIdentifier);
			}
		}
	}

	memcpy(iterTrack->second.lastPlayFile, trackPath, sizeof(iterTrack->second.lastPlayFile));
	iterTrack->second.lastStatus = 0;
	MusicVolumeControl(procIdentifier, iterTrack->second.lastVolume);

	if (isCDAudio) {
		char start[0x10]{}, end[0x10]{};
		MS_TO_MSF_BUFFER(start, startTime);
		if ((static_cast<int>(endTime) <= 0) || (startTime >= endTime)) {
			mciSendStr(cmdString, returnString, "play %08x from %d:%s to %d:0:0:0", procIdentifier, trackNum, start, (trackNum + 1));
		}
		else {
			MS_TO_MSF_BUFFER(end, endTime);
			mciSendStr(cmdString, returnString, "play %08x from %d:%s to %d:%s", procIdentifier, trackNum, start, trackNum, end);
		}
	}
	else {
		if ((static_cast<int>(endTime) <= 0) || (startTime >= endTime)) {
			mciSendStr(cmdString, returnString, "play %08x from %d", procIdentifier, startTime);
		}
		else {
			mciSendStr(cmdString, returnString, "play %08x from %d to %d", procIdentifier, startTime, endTime);
		}
	}
	ReleaseMutex(playerMutex.playCommand);
}

// Music Stop Command Functions
BOOL MusicStop(DWORD procIdentifier) {
	if (WaitForSingleObject(playerMutex.playCommand, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout MusicStop, Check Deadlock********");
	}
	auto iterTrack = PIDMap.find(procIdentifier);
	if (iterTrack == PIDMap.end()) {
		dprintf_nl("Music stop: Not initialized procID(%08X)", procIdentifier);
		ReleaseMutex(playerMutex.playCommand);
		return FALSE;
	}
	char cmdString[MAX_PATH]{};
	char returnString[0x80]{};

	if (iterTrack->second.lastStatus == 0) {
		mciSendStr(cmdString, returnString, "stop %08x", procIdentifier);
		iterTrack->second.lastStatus = 1;
	}
	ReleaseMutex(playerMutex.playCommand);
}

// Music Volume Control Command Functions
BOOL MusicVolumeControl(DWORD procIdentifier, DWORD dwVolume) {
	if (WaitForSingleObject(playerMutex.playCommand, globalTimeout) == WAIT_TIMEOUT) {
		dprintf_nl("********Timeout MusicVolumeControl, Check Deadlock********");
	}
	auto iterTrack = PIDMap.find(procIdentifier);
	if (iterTrack == PIDMap.end()) {
		dprintf_nl("Volume control: Not initialized procID(%08X)", procIdentifier);
		ReleaseMutex(playerMutex.playCommand);
		return FALSE;
	}
	char cmdString[MAX_PATH]{};
	char returnString[0x80]{};

	iterTrack->second.lastVolume = dwVolume;

	if (!guiVolume.isForced && !iterTrack->second.lastStatus) {
		if (HIWORD(iterTrack->second.lastVolume) == LOWORD(iterTrack->second.lastVolume)) {
			int vol = (float)LOWORD(iterTrack->second.lastVolume) / (float)0xFFFF * 1000;
			snprintf(cmdString, sizeof(cmdString), "setaudio %08x volume to %d", procIdentifier, vol);
			mciSendStringA(cmdString, returnString, sizeof(returnString), NULL);
		}
		else {
			int leftVol = (float)LOWORD(iterTrack->second.lastVolume) / (float)0xFFFF * 1000;
			snprintf(cmdString, sizeof(cmdString), "setaudio %08x left volume to %d", procIdentifier, leftVol);
			mciSendStringA(cmdString, returnString, sizeof(returnString), NULL);
			int rightVol = (float)HIWORD(iterTrack->second.lastVolume) / (float)0xFFFF * 1000;
			snprintf(cmdString, sizeof(cmdString), "setaudio %08x right volume to %d", procIdentifier, rightVol);
			mciSendStringA(cmdString, returnString, sizeof(returnString), NULL);
		}
	}
	ReleaseMutex(playerMutex.playCommand);
}

/**********************************************************************************************/
/* Callback Functions */
/**********************************************************************************************/

// Add tray icon
void OnBnClickedTrayAdd(HWND hwnd, HICON hIcon) {
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.uID = IDI_MYICON;
	nid.dwInfoFlags = NIIF_NONE;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	nid.hWnd = hwnd;
	nid.hIcon = hIcon;
	nid.uCallbackMessage = WM_TRAY_MENU;
	nid.uTimeout = 3000;
	lstrcpy(nid.szInfoTitle, "WinMM Audio Player");
	lstrcpy(nid.szInfo, "WinMM Audio Player is running on tray...");
	lstrcpy(nid.szTip, "WinMM Audio Player");

	Shell_NotifyIcon(NIM_ADD, &nid);
}

// Catch messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:

		// Static text display
		hEdit = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD | SS_LEFT, 0, 0, 100, 100, hwnd, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);

		HMENU hMenu, hSubMenu, hPopupMenu;

		hMenu = CreateMenu();

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, IDR_FILE_EXIT, "E&xit");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, "&File");

		hSubMenu = CreatePopupMenu();
		hPopupMenu = CreatePopupMenu();
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL0, "&0%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL1, "&10%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL2, "&20%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL3, "&30%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL4, "&40%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL5, "&50%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL6, "&60%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL7, "&70%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL8, "&80%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL9, "&90%");
		AppendMenu(hPopupMenu, MF_STRING, IDR_MENU_VOL10, "&100%");
		AppendMenu(hSubMenu, MF_STRING | MF_POPUP, (UINT64)hPopupMenu, "&Set Volume");
		AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hSubMenu, MF_STRING, IDR_MENU_MUTE, "&Mute");
		AppendMenu(hSubMenu, MF_STRING, IDR_MENU_FORCED, "&Forced Volume");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, "&Volume");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, IDR_VIEW_WDIR, "&Running from...");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, "&View");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, IDR_HELP_INST, "&Instructions");
		AppendMenu(hSubMenu, MF_STRING, IDR_HELP_ABOUT, "&About");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT64)hSubMenu, "&Help");

		SetMenu(hwnd, hMenu);

		uShellRestart = RegisterWindowMessage("TaskbarCreated");
		break;

	case WM_SIZE:
		switch (LOWORD(wParam)) {
		case SIZE_MINIMIZED:
			ShowWindow(hwnd, SW_HIDE);
			break;
		default:
			hEdit = GetDlgItem(hwnd, IDC_MAIN_EDIT);
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
			break;
		}
		break;

	case WM_INITMENUPOPUP:
		CheckMenuItem((HMENU)wParam, IDR_MENU_MUTE, MF_BYCOMMAND | (guiVolume.isMute ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuItem((HMENU)wParam, IDR_MENU_FORCED, MF_BYCOMMAND | (guiVolume.isForced ? MF_CHECKED : MF_UNCHECKED));
		CheckMenuRadioItem((HMENU)wParam, IDR_MENU_VOL0, IDR_MENU_VOL10, (guiVolume.selectLevel + IDR_MENU_VOL0), MF_BYCOMMAND);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDR_FILE_EXIT:
			PostMessageA(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDR_MENU_VOL0:
		case IDR_MENU_VOL1:
		case IDR_MENU_VOL2:
		case IDR_MENU_VOL3:
		case IDR_MENU_VOL4:
		case IDR_MENU_VOL5:
		case IDR_MENU_VOL6:
		case IDR_MENU_VOL7:
		case IDR_MENU_VOL8:
		case IDR_MENU_VOL9:
		case IDR_MENU_VOL10:
			guiVolume.selectLevel = LOWORD(wParam) - IDR_MENU_VOL0;
			guiVolume.dwVolume = (((float)guiVolume.selectLevel / 10.0) * (DWORD)0xFFFF);
			guiVolume.dwVolume += (guiVolume.dwVolume << 16);
			if (!guiVolume.isMute) { waveOutSetVolume(NULL, guiVolume.dwVolume); }
			iniStatus.overrideVolume = (float)guiVolume.selectLevel / 10.0;
			UpdateProfile(&iniStatus);
			break;
		case IDR_MENU_MUTE:
			if (guiVolume.isMute ^= 1) {
				guiVolume.isForced = 0;
				waveOutSetVolume(NULL, 0);
			}
			else {
				waveOutSetVolume(NULL, guiVolume.dwVolume);
			}
			break;
		case IDR_MENU_FORCED:
			waveOutSetVolume(NULL, guiVolume.dwVolume);
			if (guiVolume.isForced ^= 1) {
				guiVolume.isMute = 0;
			}
			break;
		case IDR_VIEW_WDIR:
			char buff[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, buff);
			MessageBox(hwnd, buff, "cdaudio-winmm player is running from:", MB_OK | MB_ICONINFORMATION);
			break;
		case IDR_HELP_INST:
			MessageBox(
				hwnd,
				TEXT(
					"1. Place winmm.dll wrapper\n"
					"    into the game folder.\n"
					"2. Place cdaudioplr.exe\n"
					"    into the game folder.\n"
					"3. Run the game normally.\n"
					"\n"
					"Additional tips:\n"
					"\n"
					"- You can also start cdaudioplr.exe\n"
					"manually before running the game.\n"
					"\n"
					"- mp3 or wav tracks can be placed\n"
					"in 'music' folder (track02 ...)"
				),
				TEXT("Instructions"),
				MB_OK
			);
			break;
		case IDR_HELP_ABOUT:
			MessageBox(
				hwnd,
				TEXT(
					"cdaudio-winmm player\n"
					"version 2.0.0 (a) 2024\n"
					"\n"
					"Restores track repeat\n"
					"and volume control\n"
					"in Vista and later."
				),
				TEXT("About"),
				MB_OK
			);
			break;
		case IDR_TRAY_SHOW:
			ShowWindow(hwnd, SW_SHOWNORMAL);
			SetForegroundWindow(hwnd);
			break;
		case IDR_TRAY_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TRAY_SETUP:
		OnBnClickedTrayAdd(hwnd, LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON)));
		break;
	case WM_TRAY_MENU:
		switch ((UINT)lParam) {
		case WM_LBUTTONUP:
			ShowWindow(hwnd, SW_SHOWNORMAL);
			SetForegroundWindow(hwnd);
			break;
		case WM_RBUTTONUP:
			HMENU hSubMenu;
			POINT trackPt;
			hSubMenu = CreatePopupMenu();
			AppendMenu(hSubMenu, MF_STRING, IDR_TRAY_SHOW, "&Open");
			AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hSubMenu, MF_STRING, IDR_TRAY_EXIT, "&Exit");

			GetCursorPos(&trackPt);
			SetForegroundWindow(hwnd);
			TrackPopupMenu(hSubMenu, TPM_LEFTALIGN, trackPt.x, trackPt.y, 0, hwnd, NULL);
			break;
		}
		break;
	default:
		if (msg == uShellRestart) {
			OnBnClickedTrayAdd(hwnd, nid.hIcon);
		}
		else {
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
	}
	return 0;
}