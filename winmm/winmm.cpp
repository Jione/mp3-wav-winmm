/*
  Copyright (c) 2012 Toni Spets <toni.spets@iki.fi>

  Permission to use, copy, modify, and distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
  ________________________________________________________________________

  This code has been modified to work with the stand-alone cdaudioplr.exe
  to fix issues with cdaudio playback in old games starting with Win Vista.
  Edits by: DD (2020)
  ________________________________________________________________________

  Changed to new code by u/jione (2024)
  All code has been rewritten and the internal workings have been changed.

  New:   Code refactoring
  New:   Interlock all fake functions using Mutex
  Fixed: Resolves a problem that occurs when the exe file for music player
         is in the same folder.
  Fixed: Fixed an issue where auxGetVolume always returned a value of 0
         rather than the current volume.
  Fixed: Fixed an issue with auxGetDevCapsA returning the supported volume
         to mono sound.
  Fixed: Fixed issue with inappropriate MM_MCINOTIFY Callback notification
         in MCI_PLAY.
  Fixed: Fixed issue with inappropriate MCI_MODE_PLAY return after play is
         complete in MCI_STATUS.
  ________________________________________________________________________
*/

#include "winmm.h"
#include <stdlib.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, PVOID pvReserved) {
    HookInitialize(hModule, dwReason);

    if (dwReason == DLL_PROCESS_ATTACH) {
        HANDLE CriticalMutex = CreateMutexA(NULL, FALSE, "WINMMDLLWRAPPER");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CriticalMutex = OpenMutexA(NULL, FALSE, "WINMMDLLWRAPPER");
        }
        for (int i = 0; i < 30; i++) {
            if (CriticalMutex != NULL) break;
            Sleep(10);
            CriticalMutex = OpenMutexA(NULL, FALSE, "WINMMDLLWRAPPER");
        }
        if (CriticalMutex != NULL) {
            if (WaitForSingleObject(CriticalMutex, 2000) != WAIT_TIMEOUT)
            {
                mciMutex.sendCommand = CreateMutexA(NULL, FALSE, NULL);
                InitializeCriticalSection(&mciSendStringSession);
                mciServer.startup = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WrapperInitialize, NULL, 0, NULL);
            }
            ReleaseMutex(CriticalMutex);
            CloseHandleSafe(CriticalMutex);
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        if (procIdentifier) {
            if (isExeRunnung) {
                modeRequest.requestMode = WPR_CLOSE;
                currentStatus.playMode = -1;
                SendIPCMessage();
            }
            CloseHandleSafe(mciServer.startup);
            CloseHandleSafe(mciServer.watchdogIPC);
            CloseHandleSafe(mciServer.timeServer);
            CloseHandleSafe(mciServer.receiveServer);
            CloseHandleSafe(mciMutex.sendCommand);
            CloseHandleSafe(mciMutex.toProcSendMsg);
            CloseHandleSafe(mciMutex.fromProcSendMsg);
            DeleteCriticalSection(&mciSendStringSession);
        }
    }
    return TRUE;
}


// WINMM.DLL Wrapper Initializer
void WrapperInitialize() {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    char ProcessPath[MAX_PATH]{};

    // ini Loader
    LoadInitialize(&iniStatus);

    // Check for start up
    GetCurrentDirectory(MAX_PATH, currentPath);
    snprintf(musicPlayerPath, MAX_PATH, "%s\\%s", currentPath, iniStatus.musicPlayer);
    procIdentifier = -1;

    // Get Window handle (Wait 1secs)
    for (int i = 0; i < 0x400; i++) {
        procIdentifier = ~(DWORD)GetCurrentHWND();
        if (procIdentifier != -1) {
            procIdentifier = ~procIdentifier;
            break;
        }
        Sleep(1);
    }

    // Check running process is Music player or _inmm server program
    GetParentPath(ProcessPath);
    if ((procIdentifier == -1) || (strstr(ProcessPath, iniStatus.musicPlayer) != 0) || (strstr(ProcessPath, "_inmmserv.exe") != 0)) {
        procIdentifier = 0;
        ReleaseMutex(mciMutex.sendCommand);
        return;
    }

    // Debug on
    if (iniStatus.useDebugDLL) { fh = fopen("winmm.log", "w"); }
    memcpy(&currentStatus.rightVolume, &iniStatus.rightVolume, sizeof(DWORD));
    currentStatus.playMode = 1;
    currentStatus.isUpdated = WPR_UPDATE_INIT;

    snprintf(musicPath, MAX_PATH, "%s\\%s", currentPath, iniStatus.musicFolder);
    MusicFileFinder(musicPath, iniStatus.musicNameFirst, musicFileExt, &currentStatus.trackCount, (DWORD_PTR*)currentStatus.trackTimes, currentStatus.trackNames);

    // Mailslot Server name
    // dllServer: winmmwrppr_fedcba98, procServer: cdaudioplr_Mailslot
    sprintf(dllServer, "\\\\.\\Mailslot\\%s_%08x", dllServerName, procIdentifier);
    sprintf(procServer, "\\\\.\\Mailslot\\%s_%s", procServerName, mailslotSignature);

    // Mutex for Mailslot
    mciMutex.toProcSendMsg = CreateMutexA(NULL, FALSE, NULL);
    mciMutex.fromProcSendMsg = CreateMutexA(NULL, FALSE, NULL);

    // IPC Handshake
    mciServer.watchdogIPC = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WatchdogCDAudioProcess, NULL, 0, NULL);
    mciServer.receiveServer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReceiveIPCMessageServer, NULL, 0, NULL);

    if (strncmp(musicFileExt, "cdaudio", 7) == 0) { playtimeDelay = cdaudioDelay; }

    // Wait running 
    for (int i = 0; i < 30; i++) {
        if (isExeRunnung) { break; }
        Sleep(10);
    }

    // Try 3 times for connect
    for (int i = 0; i < 3; i++) {
        Sleep(playtimeDelay);
        if (currentStatus.isUpdated != WPR_UPDATED) {
            currentStatus.isUpdated = WPR_UPDATE_INIT;
            modeRequest.requestMode = WPR_NOTIFY | WPR_SET_VOLUME;
            memcpy(&modeRequest.setRightVolume, &currentStatus.rightVolume, sizeof(DWORD));
            if (SendIPCMessage() == TRUE) { break; }
        }
        else {
            break;
        }
    }
    if (currentStatus.isUpdated != WPR_UPDATED) { currentStatus.isUpdated = WPR_WAIT_UPDATE; }

    // If not using custom DevID, set DeviceID
    if (!iniStatus.useCustomDevice) {
        MCI_OPEN_PARMS mciOpenParms{};
        mciOpenParms.lpstrDeviceType = "waveaudio";
        int MCIERRret = 0;
        if (MCIERRret = relay_mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_WAIT, (DWORD)(LPVOID)&mciOpenParms)) { //Needs MCI_WAIT to run in time!
            magicDeviceID = iniStatus.magicDeviceID;
            dprintf_nl("Failed to open wave device! Using %d as cdaudio id.", magicDeviceID);
        }
        else {
            magicDeviceID = mciOpenParms.wDeviceID;
            dprintf_nl("Wave device opened succesfully using cdaudio ID %d for emulation.", magicDeviceID);
        }
    }
    dprintf_nl("Process Identifier(PID): %08X, Window Handle(hWnd): %08X", GetCurrentProcessId(), procIdentifier);
    dprintf_nl("Path: %s",ProcessPath);

    // Setting value if update failed
    if (currentStatus.trackCount == 0) {
        dprintf_nl("\n**********Track files not detected. This may work incorrectly.**********");
        currentStatus.isUpdated = WPR_NOT_UPDATED;
        currentStatus.trackCount = 35; // Fake 35 Tracks
        currentStatus.trackTimes[0] = 3600000; // Fake All tracks 1 hour
        for (int i = 1; i < (sizeof(currentStatus.trackTimes) / sizeof(DWORD)); i++) {
            currentStatus.trackTimes[i] = 150000; // Fake 2min 30sec per track
        }
    }

    dprintf_nl("\nCommand name\t\tDeviceID\tCMD(hex)\tCMD(string)\t\tFlags\t\t\t\tParam & Return");
    mciServer.timeServer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlayNotifyServer, NULL, 0, NULL);
    ReleaseMutex(mciMutex.sendCommand);
}


// run Exe process
HANDLE StartCDAudioProcess(LPSTR exeFileStart) {
    if (!iniStatus.autoStartExec) return NULL;
    FILE* fp = NULL;
    HANDLE exeHandle = NULL;

    if ((fp = fopen(musicPlayerPath, "rb")) != NULL) {
        fclose(fp);
        if ((fp = fopen(musicPlayerPath, "r+b")) != NULL) {
            fclose(fp);
            flushall();
            system(exeFileStart);
            for (int i = 0; i < 10; i++) {
                if (exeHandle = FindProcess(iniStatus.musicPlayer)) {
                    break;
                }
                Sleep(100);
            }
            if (exeHandle) {
                // Second run
                if (currentStatus.isUpdated != WPR_UPDATE_INIT) {
                    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
                    if (currentStatus.isUpdated == WPR_NOT_UPDATED) {
                        modeRequest.requestMode = WPR_NOTIFY | WPR_SET_VOLUME;
                    }
                    if (!currentStatus.playMode) {
                        modeRequest.requestMode |= WPR_PLAY;
                        modeRequest.setTrackFrom = currentStatus.trackCurrent;
                        modeRequest.setTrackFromTime = currentStatus.timePlayLast;
                        modeRequest.setTrackToTime = currentStatus.trackToTime;
                        if (currentStatus.trackCurrent > currentStatus.trackTo) {
                            modeRequest.setTrackTo = currentStatus.trackTo;
                        }
                        else {
                            modeRequest.setTrackTo = currentStatus.trackCurrent;
                        }
                    }
                    modeRequest.hWndProcID = procIdentifier;
                    strncpy(modeRequest.musicPath, currentPath, sizeof(modeRequest.musicPath));
                    memcpy(&modeRequest.setRightVolume, &currentStatus.rightVolume, sizeof(DWORD));
                    SendIPCMessage();
                    ReleaseMutex(mciMutex.sendCommand);
                }
            }
        }
    }
    return exeHandle;
}


// Check Exe process
void WatchdogCDAudioProcess() {
    HANDLE exeHandle = NULL;
    char exeFileStart[MAX_PATH]{};
    sprintf(exeFileStart, "start /b \"%s\" \"%s\"", iniStatus.musicPlayer, musicPlayerPath);
    exeHandle = FindProcess(iniStatus.musicPlayer);

    while (1) {
        if (FindProcess(iniStatus.musicPlayer) == NULL) {
            isExeRunnung = 0;
            if (!exeHandle) {
                if (exeHandle = StartCDAudioProcess(exeFileStart)) {
                    isExeRunnung = 1;
                }
            }
        }
        else {
            if (!isExeRunnung) {
                WaitForSingleObject(mciMutex.sendCommand, INFINITE);
                snprintf(musicPath, MAX_PATH, "%s\\%s", currentPath, iniStatus.musicFolder);
                MusicFileFinder(musicPath, iniStatus.musicNameFirst, musicFileExt, &currentStatus.trackCount, (DWORD_PTR*)currentStatus.trackTimes, currentStatus.trackNames);
                modeRequest.requestMode = WPR_NOTIFY | WPR_SET_VOLUME;
                modeRequest.hWndProcID = procIdentifier;
                SendIPCMessage();
                ReleaseMutex(mciMutex.sendCommand);
            }
            isExeRunnung = 1;
        }
        Sleep(1000);
    }
}

// IPC Mailslot message receiving thread
BOOL SendIPCMessage() {
    if (modeRequest.requestMode == 0) { return TRUE; }
    DWORD writtenBytes = 0;
    DWORD cbMessage, cMessage;
    BOOL fResult;
    HANDLE Mailslot;

    // try 3 times for open Mailslot
    Mailslot = CreateFile(procServer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ((Mailslot == INVALID_HANDLE_VALUE) || (Mailslot == NULL)) {
        if (currentStatus.isUpdated == WPR_UPDATE_INIT) {
            for (int i = 0; i < 3; i++) {
                if ((Mailslot != INVALID_HANDLE_VALUE) && (Mailslot != NULL)) break;
                Sleep(1);
                Mailslot = CreateFile(procServer, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            }
        }
        memset(&modeRequest, 0, sizeof(MM_REQUEST));
        return FALSE;
    }

    WaitForSingleObject(mciMutex.toProcSendMsg, INFINITE);
    if (!modeRequest.hWndProcID) modeRequest.hWndProcID = (DWORD)GetCurrentHWND();
    if (modeRequest.requestMode >= WPR_GET_VOLUME) { modeRequest.requestMode |= WPR_NOTIFY; }

    // full request if notify message
    if (modeRequest.requestMode & WPR_NOTIFY) {
        modeRequest.hWndProcID = procIdentifier;
        memcpy(&modeRequest.setRightVolume, &currentStatus.rightVolume, sizeof(DWORD));
        strncpy(modeRequest.musicPath, currentPath, sizeof(modeRequest.musicPath));
        if (strncmp(musicFileExt, "cdaudio", 7) == 0) {
            strcpy(modeRequest.musicPath, "cdaudio");
        }
        else if (musicFileExt[0]) {
            snprintf(modeRequest.musicPath, sizeof(modeRequest.musicPath), "%s\\%s*%s", musicPath, iniStatus.musicNameFirst, musicFileExt);
        }
        else {
            strcpy(modeRequest.musicPath, "noaudio");
        }
    }

    // Write message for old cdplayer
    if (modeRequest.requestMode & WPR_PLAY) {
        snprintf(modeRequest.msgString, (sizeof(modeRequest.msgString) - 1), "%d mci_from", currentStatus.trackCurrent);
        if (strncmp(musicFileExt, "cdaudio", 7) == 0) {
            strcpy(modeRequest.musicPath, "cdaudio");
        }
        else if ((musicFileExt[0]) && currentStatus.trackNames[currentStatus.trackCurrent][0]) {
            snprintf(modeRequest.musicPath, (sizeof(modeRequest.musicPath) - 1), "%s\\%s", musicPath, currentStatus.trackNames[currentStatus.trackCurrent]);
            dprintf_nl("Play Request: %s", modeRequest.musicPath); // debug
        }
        else {
            modeRequest.requestMode = WPR_STOP;
        }
    }

    if ((modeRequest.requestMode & WPR_STOP) || (modeRequest.requestMode & WPR_PAUSE)) {
        if (!currentStatus.playMode) {
            strcpy(modeRequest.msgString, "0 mci_stop");
        }
        else {
            CloseHandle(Mailslot);
            ReleaseMutex(mciMutex.toProcSendMsg);
            memset(&modeRequest, 0, sizeof(MM_REQUEST));
            return TRUE;
        }
    }
    else if (modeRequest.requestMode & WPR_CLOSE) {
        if ((iniStatus.autoCloseExec) && (currentStatus.playMode == -1)) {
            modeRequest.requestMode |= WPR_EXIT;
            strcpy(modeRequest.msgString, "0 exit");
        }
        else if (currentStatus.playMode == 0) {
            strcpy(modeRequest.msgString, "0 mci_stop");
        }
        else {
            CloseHandle(Mailslot);
            ReleaseMutex(mciMutex.toProcSendMsg);
            memset(&modeRequest, 0, sizeof(MM_REQUEST));
            return TRUE;
        }
    }
    else if (modeRequest.requestMode & WPR_SET_VOLUME) {
        int aux_vol;
        memcpy(&aux_vol, &currentStatus.rightVolume, sizeof(int));
        snprintf(modeRequest.msgString, sizeof(modeRequest.msgString), "%d aux_vol", aux_vol);
    }

    // debug
    //dprintf_nl("Server: %s\t\tMode: %08X\t\tSend: %s", procServer, modeRequest.requestMode, modeRequest.msgString);

    // Send request message
    WriteFile(Mailslot, &modeRequest, sizeof(MM_REQUEST), &writtenBytes, NULL);
    if (writtenBytes != sizeof(MM_REQUEST)) { dprintf_nl("> Error: Request failed"); }

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

    // Wait for cdplayer
    if (modeRequest.requestMode & WPR_PLAY) {
        currentStatus.timePlayStart = GetTickCount64() + playtimeDelay - globalDelayTime;
        Sleep(playtimeDelay);
    }
    else if ((modeRequest.requestMode & WPR_STOP) || (modeRequest.requestMode & WPR_PAUSE) || (modeRequest.requestMode & WPR_CLOSE)) {
        Sleep(playtimeDelay);
    }
    else if (modeRequest.requestMode & WPR_SET_VOLUME) {
        Sleep(globalDelayTime);
    }

    memset(&modeRequest, 0, sizeof(MM_REQUEST));
    ReleaseMutex(mciMutex.toProcSendMsg);
    return TRUE;
}


// IPC Mailslot message receiving thread
void ReceiveIPCMessageServer() {
    HANDLE Mailslot;
    DWORD readBytes = 0;

    // try 300 times for create Mailslot
    Mailslot = CreateMailslotA(dllServer, 0, MAILSLOT_WAIT_FOREVER, NULL);
    for (int i = 0; i < 300; i++) {
        if ((Mailslot != INVALID_HANDLE_VALUE) && (Mailslot != NULL)) { break; }
        Sleep(1);
        Mailslot = CreateMailslotA(dllServer, 0, MAILSLOT_WAIT_FOREVER, NULL);
    }
    if ((Mailslot == INVALID_HANDLE_VALUE) || (Mailslot == NULL)) { return; }

    while (1) {
        WaitForSingleObject(mciMutex.fromProcSendMsg, INFINITE);
        if (ReadFile(Mailslot, &statusResponse, sizeof(MM_RESPONSE), &readBytes, NULL)) {
            if (readBytes == sizeof(MM_RESPONSE)) {
                WaitForSingleObject(mciMutex.sendCommand, INFINITE);
                currentStatus.timePlayStart = statusResponse.timePlayStart;
                //memcpy(&currentStatus.rightVolume, &statusResponse.rightVolume, sizeof(DWORD));
                currentStatus.isUpdated = WPR_UPDATED;
                ReleaseMutex(mciMutex.sendCommand);
            }
        }
        ReleaseMutex(mciMutex.fromProcSendMsg);
        Sleep(1);
    }
}


// Play status update thread
// Real-time play status check and notify when stopped
void PlayNotifyServer() {
    static int buffer;
    while (1) {
        if (!currentStatus.playMode) {
            buffer = GetTickCount64() - currentStatus.timePlayStart;
            if (buffer < 0) { currentStatus.timePlayLast = 0; }
            else { currentStatus.timePlayLast = buffer; }

            if ((currentStatus.trackCurrent > currentStatus.trackTo) 
                || ((currentStatus.trackCurrent == currentStatus.trackTo) && ((currentStatus.timePlayLast + playtimeDelay) >= currentStatus.trackToTime))
               )
            {
                dprintf_nl("current: track%02d, currenttime: %d, to: track%02d, totime: %d", currentStatus.trackCurrent, currentStatus.timePlayLast, currentStatus.trackTo, currentStatus.trackToTime);
                currentStatus.timePlayLast = currentStatus.trackToTime;
                WaitForSingleObject(mciMutex.sendCommand, INFINITE);
                memset(&modeRequest, 0, sizeof(MM_REQUEST));
                modeRequest.requestMode = WPR_STOP;
                if (currentStatus.notifyPlayback == 1) {
                    requestStatus.dwCallback = procIdentifier;
                    requestStatus.notifySuccess = 1;
                    currentStatus.notifyPlayback = 0;
                    SendIPCMessage();
                    dprintf_nl("SendMessageA\t\t%08X\t%08X\tMM_NOTIFY\t\tNOTIFY_SUCCESSFUL\t\tTrack%02d", magicDeviceID, (DWORD)GetCurrentHWND(), currentStatus.trackCurrent);
                    SendCommonNotify(NULL);
                }
                else if (!currentStatus.notifyPlayback) {
                    SendIPCMessage();
                    // Added support for older apps that rely on system messages (KOEI games)
                    dprintf_nl("SendMessageB\t\t%08X\t%08X\tMM_NOTIFY\t\tNOTIFY_SUCCESSFUL\t\tTrack%02d", magicDeviceID, (DWORD)GetCurrentHWND(), currentStatus.trackCurrent);
                    PostMessageA(GetCurrentHWND(), MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, magicDeviceID);
                }
                else {
                    currentStatus.notifyPlayback = 0;
                }
                currentStatus.playMode = 1;
                ReleaseMutex(mciMutex.sendCommand);
            }
            else if ((currentStatus.timePlayLast + playtimeDelay) >= currentStatus.trackTimes[currentStatus.trackCurrent]) {
                WaitForSingleObject(mciMutex.sendCommand, INFINITE);
                memset(&modeRequest, 0, sizeof(MM_REQUEST));
                if ((currentStatus.trackCurrent == currentStatus.trackTo) ||
                    ((++currentStatus.trackCurrent == currentStatus.trackTo) && (currentStatus.trackToTime == 0))) {
                    modeRequest.requestMode = WPR_STOP;
                    currentStatus.timePlayLast = currentStatus.trackToTime;
                    if (currentStatus.notifyPlayback == 1) {
                        requestStatus.dwCallback = procIdentifier;
                        requestStatus.notifySuccess = 1;
                        currentStatus.notifyPlayback = 0;
                        SendIPCMessage();
                        dprintf_nl("SendMessageC\t\t%08X\t%08X\tMM_NOTIFY\t\tNOTIFY_SUCCESSFUL\t\tTrack%02d", magicDeviceID, (DWORD)GetCurrentHWND(), currentStatus.trackCurrent);
                        SendCommonNotify(NULL);
                    }
                    else if (!currentStatus.notifyPlayback) {
                        SendIPCMessage();
                        // Added support for older apps that rely on system messages (KOEI games)
                        dprintf_nl("SendMessageD\t\t%08X\t%08X\tMM_NOTIFY\t\tNOTIFY_SUCCESSFUL\t\tTrack%02d", magicDeviceID, (DWORD)GetCurrentHWND(), currentStatus.trackCurrent);
                        PostMessageA(GetCurrentHWND(), MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, magicDeviceID);
                    }
                    else {
                        currentStatus.notifyPlayback = 0;
                    }
                    currentStatus.playMode = 1;
                }
                else {
                    currentStatus.timePlayLast = 0;
                    modeRequest.requestMode = WPR_SET_FROM | WPR_SET_TO | WPR_PLAY;
                    modeRequest.setTrackFrom = currentStatus.trackCurrent;
                    modeRequest.setTrackFromTime = 0;
                    modeRequest.setTrackTo = currentStatus.trackCurrent;
                    if (currentStatus.trackCurrent == currentStatus.trackTo) {
                        modeRequest.setTrackToTime = -1;
                    }
                    else {
                        modeRequest.setTrackToTime = currentStatus.trackToTime;
                    }
                    SendIPCMessage();
                }
                ReleaseMutex(mciMutex.sendCommand);
            }
        }
        Sleep(1);
    }
}

void DelayedCallback(UINT msg, DWORD dwMilliseconds) {
    Sleep(dwMilliseconds);
    PostMessageA(GetCurrentHWND(), MM_MCINOTIFY, msg, magicDeviceID);
}

// Send message to parent process for MCI Notification
void SendCommonNotify(UINT uMsg) {
    MCI_CALLBACK callBack{ MCI_NOTIFY_SUCCESSFUL, playtimeDelay };
    if (((requestStatus.playMode == currentStatus.playMode) && (!requestStatus.notifyWait)) || (uMsg < MCI_OPEN)) {
        callBack.dwMilliseconds = 1;
    }
    if (requestStatus.dwCallback == NULL) { requestStatus.notifySuccess = 0; }
    if (!currentStatus.playMode && (currentStatus.notifyPlayback == 1)) {
        if (requestStatus.notifySuccess) {
            AddFlagString("NOTIFY");
            currentStatus.notifyPlayback = -1;
            if (uMsg == MCI_NOTIFY_FAILURE) {
                AddExtendString("NOTIFY_FAILURE");
                callBack.msg = MCI_NOTIFY_FAILURE;
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DelayedCallback, &callBack, 0, NULL);
            }
            else {
                AddExtendString("NOTIFY_SUPERSEDED");
                callBack.msg = MCI_NOTIFY_SUPERSEDED;
                CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DelayedCallback, &callBack, 0, NULL);
            }
        }
        else if ((uMsg == MCI_STOP) || (uMsg == MCI_SEEK) || (uMsg == MCI_CLOSE)) {
            currentStatus.notifyPlayback = -1;
            AddExtendString("NOTIFY_ABORTED");
            PostMessageA(GetCurrentHWND(), MM_MCINOTIFY, MCI_NOTIFY_ABORTED, magicDeviceID);
        }
    }
    else if (requestStatus.notifySuccess) {
        AddFlagString("NOTIFY");
        if (uMsg == MCI_PLAY) {
            currentStatus.notifyPlayback = 1;
        }
        else if (uMsg == MCI_NOTIFY_FAILURE) {
            AddExtendString("NOTIFY_FAILURE");
            callBack.msg = MCI_NOTIFY_FAILURE;
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DelayedCallback, &callBack, 0, NULL);
        }
        else {
            AddExtendString("NOTIFY_SUCCESSFUL");
            PostMessageA(GetCurrentHWND(), MM_MCINOTIFY, MCI_NOTIFY_SUCCESSFUL, magicDeviceID);
        }
    }
}

// Write debugging message and IPC request function
MCIERROR WINAPI returnSendCommand(int msgNum) {
    BOOL resualt = MMSYSERR_NOERROR;
    if (isCDRequest) {
        if ((modeRequest.requestMode & WPR_PLAY) && ((currentStatus.trackCount < modeRequest.setTrackFrom) || (modeRequest.setTrackFrom <= 0))) {
            SendCommonNotify(MCI_NOTIFY_FAILURE);
            resualt = MMSYSERR_ERROR;
        }
        else if (!((modeRequest.requestMode & WPR_STOP) || (modeRequest.requestMode & WPR_PAUSE)) || !currentStatus.playMode) {
            // Send IPC message
            if (currentStatus.isUpdated < WPR_NOT_UPDATED) { modeRequest.requestMode |= WPR_NOTIFY; }
            SendIPCMessage();
            if (!(modeRequest.requestMode & WPR_CLOSE) || requestStatus.notifySuccess) { SendCommonNotify(requestStatus.uMsg); }
            currentStatus.playMode = requestStatus.playMode;
        }
        else {
            SendCommonNotify(requestStatus.uMsg);
        }
    }

    // return when if not logging
    if (!fh) return resualt;

    if (!debugOffOnce) {
        sprintf(debugMessage, "%08X%08X%08X", requestStatus.IDDevice, requestStatus.uMsg, requestStatus.fdwCommand);
        if (strcmp(debugMessage, lastDebugMessage) != 0) {
            static char buffer[0x100];
            static const char nameSendCommand[] = "mciSendCommandA";
            int i = 12, j;
            strcpy(buffer, "%s\t\t%p\t%p\t%s");
            if (msgNum < 0) {
                for (j = strlen(mciCmds[~msgNum]) / 8; j < 3; j++) {
                    buffer[i++] = '\t';
                }
            }
            else {
                for (j = strlen(mciCmds[msgNum]) / 8; j < 3; j++) {
                    buffer[i++] = '\t';
                }
            }
            buffer[i++] = 0;
            strcat(buffer, "%s");
            if (msgDbgExtend[0] != 0) {
                for (j = strlen(msgDebug) / 8; j < 4; j++) {
                    buffer[++i] = '\t';
                }
                buffer[++i] = 0;
            }
            strcat(buffer, "%s");

            if (msgNum < 0) {
                dprintf_nl("********Unsupported commands, relay to original function********");
                dprintf_nl(buffer, nameSendCommand, requestStatus.IDDevice, requestStatus.uMsg, mciCmds[~msgNum], msgDebug, msgDbgExtend);
                dprintf_nl("****************************************************************");
            }
            else {
                dprintf_nl(buffer, nameSendCommand, requestStatus.IDDevice, requestStatus.uMsg, mciCmds[msgNum], msgDebug, msgDbgExtend);
            }
            strcpy(lastDebugMessage, debugMessage);
        }
    }
    else { debugOffOnce = 0; }
    return resualt;
}


// MCI Command parsing and Information Return Function
MCIERROR WINAPI fake_mciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam) {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_mciSendCommandA(IDDevice, uMsg, fdwCommand, dwParam);
    }

    // init variables
    int i, msgNum = uMsg;
    char buffer[0x80]{ 0 };
    DWORD timePos = 0;
    BOOL resualt;
    isCDRequest = msgDebug[0] = msgDbgExtend[0] = 0;
    memset(&modeRequest, 0, sizeof(MM_REQUEST));
    requestStatus.playMode = currentStatus.playMode;
    requestStatus.IDDevice = IDDevice;
    requestStatus.uMsg = uMsg;
    requestStatus.fdwCommand = fdwCommand;
    requestStatus.dwParam = dwParam;
    if (dwParam) {
        requestStatus.dwCallback = (static_cast<LPMCI_GENERIC_PARMS>((LPVOID)dwParam))->dwCallback;
    }

    // Check command Message
    if (uMsg < MCI_SAVE) { msgNum -= MCI_OPEN; }
    else if (uMsg < MCI_CUE) { msgNum -= MCI_OPEN + 1; }
    else if (uMsg == MCI_CUE) { msgNum = 17; }
    else if (uMsg < MCI_LOAD) { msgNum -= MCI_REALIZE + 18; }
    else { msgNum -= MCI_LOAD + 24; }

    // Check common Flags
    if (fdwCommand & MCI_NOTIFY) {
        requestStatus.notifySuccess = TRUE;
    }
    else {
        requestStatus.notifySuccess = FALSE;
    }
    if (fdwCommand & MCI_WAIT) {
        requestStatus.notifyWait = TRUE;
        AddFlagString("WAIT");
    }
    else {
        requestStatus.notifyWait = FALSE;
    }
    if (fdwCommand & MCI_FROM) { AddFlagString("FROM"); }
    if (fdwCommand & MCI_TO) { AddFlagString("TO"); }
    if (fdwCommand & MCI_TRACK) { AddFlagString("TRACK"); }

    // Check OPEN CDAudio device
    if (uMsg == MCI_OPEN) {
        if (!dwParam) {
            if (debugOffOnce) { dprintf_nl("mciSendCommandA\t\t%08X\t%08X\t%s\t\t%08X\t\t\tError: Invalid Param", IDDevice, uMsg, mciCmds[msgNum], fdwCommand); }
            else { debugOffOnce = 0; }
            ReleaseMutex(mciMutex.sendCommand);
            return MCIERR_NULL_PARAMETER_BLOCK;
        }
        LPMCI_OPEN_PARMS Params = static_cast<LPMCI_OPEN_PARMS>((LPVOID)dwParam);

        if (fdwCommand & MCI_OPEN_TYPE) {
            if (fdwCommand & MCI_OPEN_TYPE_ID) {
                AddFlagString("TYPE_ID");
                strcpy(msgDbgExtend, mciOpenType[((DWORD)Params->lpstrDeviceType - MCI_DEVTYPE_VCR)]);
            }
            else {
                AddFlagString("TYPE");
                if ((DWORD)Params->lpstrDeviceType != NULL) {
                    strcpy(msgDbgExtend, Params->lpstrDeviceType);
                }
            }
        }
        if (fdwCommand & MCI_OPEN_ELEMENT) {
            if (fdwCommand & MCI_OPEN_ELEMENT_ID) {
                AddFlagString("ELEMENT_ID");
                sprintf(buffer, "%08X", (DWORD)Params->lpstrElementName);
                AddExtendString(buffer);
            }
            else {
                AddFlagString("ELEMENT");
                if ((DWORD)Params->lpstrElementName != NULL) {
                    AddExtendString(Params->lpstrElementName);
                }
            }
        }
        if (fdwCommand & MCI_OPEN_ALIAS) {
            AddFlagString("ALIAS");
            if ((DWORD)Params->lpstrAlias != NULL) {
                AddExtendString(Params->lpstrAlias);
            }
        }

        if (fdwCommand & MCI_OPEN_SHAREABLE) { AddFlagString("SHAREABLE"); }

        if ((strstr(msgDbgExtend, "cdaudio") != 0) || (strstr(msgDbgExtend, "CD_AUDIO") != 0)) {
            isCDRequest = 1;
            Params->wDeviceID = magicDeviceID;
            modeRequest.requestMode |= WPR_OPEN;
            resualt = returnSendCommand(msgNum);
            ReleaseMutex(mciMutex.sendCommand);
            return resualt;
        }
    }
    else if ((IDDevice == magicDeviceID) || (IDDevice == 0) || (IDDevice == 0xFFFFFFFF)) {
        isCDRequest = 1;
        if (!dwParam) {
            switch (uMsg) {
            case MCI_PLAY:
            case MCI_PAUSE:
            case MCI_STOP:
            case MCI_RESUME:
            case MCI_CLOSE:
                break;
            default:
                if (debugOffOnce) { dprintf_nl("mciSendCommandA\t\t%08X\t%08X\t%s\t\t%08X\t\t\tError: Invalid Param", IDDevice, uMsg, mciCmds[msgNum], fdwCommand); }
                else { debugOffOnce = 0; }
                ReleaseMutex(mciMutex.sendCommand);
                return MCIERR_NULL_PARAMETER_BLOCK;
            }
        }
        // MCI_PLAY Command
        if (uMsg == MCI_PLAY) {
            LPMCI_PLAY_PARMS Params{};
            if (dwParam) {
                Params = static_cast<LPMCI_PLAY_PARMS>((LPVOID)dwParam);
            }

            currentStatus.timePlayStart = GetTickCount64() + playtimeDelay;
            requestStatus.playMode = 0;
            modeRequest.requestMode |= WPR_PLAY | WPR_SET_FROM | WPR_SET_TO;

            if (fdwCommand & MCI_FROM) {
                currentStatus.timePlayLast = 0;
                if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                    currentStatus.trackCurrent = currentStatus.trackFrom = modeRequest.setTrackFrom = MCI_TMSF_TRACK(Params->dwFrom);
                    currentStatus.trackFromTime = modeRequest.setTrackFromTime = MCI_TMSF_TO_MILLISECS(Params->dwFrom);
                    ADD_TMSF_PRINT_BUFFER(buffer, Params->dwFrom);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                    if (currentStatus.trackFrom) {
                        modeRequest.setTrackFrom = currentStatus.trackFrom;
                    }
                    else {
                        modeRequest.setTrackFrom = currentStatus.trackFrom = currentStatus.trackCurrent;
                    }
                    currentStatus.trackFromTime = modeRequest.setTrackFromTime = MCI_MSF_TO_MILLISECS(Params->dwFrom);
                    ADD_MSF_PRINT_BUFFER(buffer, Params->dwFrom);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                    if (currentStatus.trackFrom) {
                        modeRequest.setTrackFrom = currentStatus.trackFrom;
                    }
                    else {
                        modeRequest.setTrackFrom = currentStatus.trackFrom = currentStatus.trackCurrent;
                    }
                    currentStatus.trackFromTime = modeRequest.setTrackFromTime = Params->dwFrom;
                    ADD_MS_PRINT_BUFFER(buffer, Params->dwFrom);
                }
            }
            else {
                currentStatus.timePlayStart = GetTickCount64() - currentStatus.timePlayLast + playtimeDelay;
                if (currentStatus.trackFrom) {
                    modeRequest.setTrackFrom = currentStatus.trackFrom;
                    modeRequest.setTrackFromTime = currentStatus.trackFromTime;
                }
                else {
                    modeRequest.setTrackFrom = currentStatus.trackFrom = currentStatus.trackCurrent;
                    modeRequest.setTrackFromTime = currentStatus.trackFromTime = currentStatus.timePlayLast;
                }
            }

            if (fdwCommand & MCI_TO) {
                if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                    currentStatus.trackTo = modeRequest.setTrackTo = MCI_TMSF_TRACK(Params->dwTo);
                    currentStatus.trackToTime = modeRequest.setTrackToTime = MCI_TMSF_TO_MILLISECS(Params->dwTo);
                    ADD_TMSF_PRINT_BUFFER(buffer, Params->dwTo);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                    modeRequest.setTrackTo = currentStatus.trackTo = currentStatus.trackCount;
                    currentStatus.trackToTime = modeRequest.setTrackToTime = MCI_MSF_TO_MILLISECS(Params->dwTo);
                    ADD_MSF_PRINT_BUFFER(buffer, Params->dwTo);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                    modeRequest.setTrackTo = currentStatus.trackTo = currentStatus.trackCount;
                    currentStatus.trackToTime = modeRequest.setTrackToTime = Params->dwTo;
                    ADD_MS_PRINT_BUFFER(buffer, Params->dwTo);
                }
            }
            else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                modeRequest.setTrackToTime = currentStatus.trackToTime = currentStatus.trackTimes[currentStatus.trackCount];
                modeRequest.setTrackTo = currentStatus.trackTo = currentStatus.trackCount;
            }
            else {
                modeRequest.setTrackToTime = currentStatus.trackToTime = currentStatus.trackTimes[modeRequest.setTrackFrom];
                modeRequest.setTrackTo = currentStatus.trackTo = modeRequest.setTrackFrom;
            }

            if (modeRequest.setTrackFrom > modeRequest.setTrackTo) {
                modeRequest.setTrackTo = modeRequest.setTrackFrom;
                currentStatus.trackTo = currentStatus.trackFrom;
                currentStatus.trackToTime = currentStatus.trackTimes[modeRequest.setTrackTo];
            }
            else if ((modeRequest.setTrackFrom == modeRequest.setTrackTo) && (modeRequest.setTrackFromTime > modeRequest.setTrackToTime)) {
                modeRequest.setTrackToTime = currentStatus.trackTimes[modeRequest.setTrackTo];
            }
        }

        // MCI_STATUS Command (return required)
        else if (uMsg == MCI_STATUS) {
            LPMCI_STATUS_PARMS Params = static_cast<LPMCI_STATUS_PARMS>((LPVOID)dwParam);
            DWORD trackInfo = 0;
            if (fdwCommand & MCI_TRACK) {
                trackInfo = Params->dwTrack;
                sprintf(msgDbgExtend, "Track%02d", trackInfo);
            }
            Params->dwReturn = 0;

            if (fdwCommand & MCI_STATUS_ITEM) {
                AddFlagString("ITEM");

                // Play mode status
                if (Params->dwItem == MCI_STATUS_MODE) {
                    AddFlagString("MODE");
                    AddExtendString("Status:");

                    if (currentStatus.playMode == 0) {
                        Params->dwReturn = MCI_MODE_PLAY;
                        strcpy(buffer, "Playing");
                    }
                    else if (currentStatus.playMode == 1) {
                        Params->dwReturn = MCI_MODE_STOP;
                        strcpy(buffer, "Stopped");
                    }
                    else {
                        Params->dwReturn = MCI_MODE_PAUSE;
                        strcpy(buffer, "Paused");
                    }
                }
                // Current track status
                else if (Params->dwItem == MCI_STATUS_CURRENT_TRACK) {
                    AddFlagString("CURRENT_TRACK");
                    if (!currentStatus.isUpdated) { modeRequest.requestMode |= WPR_GET_CURRENT_TRACK; }
                    AddExtendString("CurrentTrack:");
                    Params->dwReturn = currentStatus.trackCurrent;
                    sprintf(buffer, "%d", Params->dwReturn);
                }
                // Track Length status (Track0 = All tracks)
                else if (Params->dwItem == MCI_STATUS_LENGTH) {
                    AddFlagString("LENGTH");
                    if (!currentStatus.isUpdated) { modeRequest.requestMode |= WPR_GET_LENGTH; }
                    // AddExtendString("Length:");
                    AddExtendString("");
                    if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                        Params->dwReturn = currentStatus.trackTimes[trackInfo];
                        MS_PRINT_MSF_BUFFER(buffer, Params->dwReturn);
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                        Params->dwReturn = MCI_MILLISECS_TO_MSF(currentStatus.trackTimes[trackInfo]);
                        MSF_PRINT_MSF_BUFFER(buffer, Params->dwReturn);
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                        Params->dwReturn = MCI_MILLISECS_TO_TMSF(currentStatus.trackTimes[trackInfo]);
                        TMSF_PRINT_MSF_BUFFER(buffer, Params->dwReturn);
                    }
                }
                // Track type status
                else if (Params->dwItem == MCI_CDA_STATUS_TYPE_TRACK) {
                    AddFlagString("CDA_TYPE_TRACK");
                    if (!currentStatus.isUpdated) { modeRequest.requestMode |= WPR_GET_LENGTH; }
                    AddExtendString("Type:");
                    if (currentStatus.trackTimes[trackInfo] == 0) {
                        Params->dwReturn = MCI_CDA_TRACK_OTHER;
                        strcpy(buffer, "MCI_CDA_TRACK_OTHER");
                    }
                    else {
                        Params->dwReturn = MCI_CDA_TRACK_AUDIO;
                        strcpy(buffer, "MCI_CDA_TRACK_AUDIO");
                    }
                }
                // Media present status
                else if (Params->dwItem == MCI_STATUS_MEDIA_PRESENT) {
                    AddFlagString("MEDIA_PRESENT");
                    Params->dwReturn = TRUE;
                    strcpy(buffer, "TRUE");
                }
                // Track count status
                else if (Params->dwItem == MCI_STATUS_NUMBER_OF_TRACKS) {
                    AddFlagString("NUMBER_OF_TRACKS");
                    if (!currentStatus.isUpdated) { modeRequest.requestMode |= WPR_GET_COUNT; }
                    AddExtendString("TrackCount:");
                    Params->dwReturn = currentStatus.trackCount;
                    sprintf(buffer, "%d", Params->dwReturn);
                }
                // Position of Track status (Track[num] = Track position, Track0 = Current position)
                else if (Params->dwItem == MCI_STATUS_POSITION) {
                    AddFlagString("POSITION");
                    if (!currentStatus.isUpdated) { modeRequest.requestMode |= WPR_GET_CURRENT_POS; }
                    AddExtendString("Pos:");
                    if (fdwCommand & MCI_TRACK) {
                        for (i = 0; i < trackInfo; i++) {
                            timePos += currentStatus.trackTimes[i];
                        }
                    }
                    else {
                        timePos = currentStatus.timePlayLast;
                    }

                    if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                        Params->dwReturn = timePos;
                        MS_PRINT_MSF_BUFFER(buffer, timePos);
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                        Params->dwReturn = MCI_MILLISECS_TO_MSF(timePos);
                        MS_PRINT_MSF_BUFFER(buffer, timePos);
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                        Params->dwReturn = MCI_MILLISECS_TO_TMSF(timePos);
                        MS_PRINT_TMSF_BUFFER(buffer, currentStatus.trackCurrent, timePos);
                    }
                }
                // Device status
                else if (Params->dwItem == MCI_STATUS_READY) {
                    AddFlagString("READY");
                    Params->dwReturn = TRUE;
                    strcpy(buffer, "TRUE");
                }
                // Time format status
                else if (Params->dwItem == MCI_STATUS_TIME_FORMAT) {
                    AddFlagString("TIME_FORMAT");
                    AddExtendString("TimeFormat:");
                    Params->dwReturn = currentStatus.timeFormat;
                    strcpy(buffer, mciTimeFormat[currentStatus.timeFormat]);
                }
                // Media start position status
                else if (Params->dwItem == MCI_STATUS_START) {
                    AddFlagString("START");
                    AddExtendString("TrackStart:");
                    if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                        Params->dwReturn = 2001;
                        strcpy(buffer, "2001ms");
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                        Params->dwReturn = MCI_MAKE_MSF(0, 2, 0);
                        strcpy(buffer, "0:02.00");
                    }
                    else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                        Params->dwReturn = MCI_MAKE_TMSF(1, 0, 0, 0);
                        strcpy(buffer, "Track01[00:00.00]");
                    }
                }

                strcat(msgDbgExtend, buffer);
                if (modeRequest.requestMode) {
                    modeRequest.requestMode = NULL; // remove request for performance
                    strcat(msgDbgExtend, "(Fake)");
                }
            }
        }

        // MCI_SET Command
        else if (uMsg == MCI_SET) {
            LPMCI_SET_PARMS Params = static_cast<LPMCI_SET_PARMS>((LPVOID)dwParam);
            
            if (fdwCommand & MCI_SET_TIME_FORMAT) {
                AddFlagString("TIME_FORMAT");
                currentStatus.timeFormat = Params->dwTimeFormat;
                strcpy(msgDbgExtend, mciTimeFormat[currentStatus.timeFormat]);
            }

            if (fdwCommand & MCI_SET_AUDIO) {
                AddFlagString("AUDIO");
                DWORD dwVolume = -1;

                if (fdwCommand & MCI_SET_ON) {
                    if (fdwCommand & MCI_SET_AUDIO_LEFT) { AddFlagString("LEFT_ON"); }
                    else if (fdwCommand & MCI_SET_AUDIO_RIGHT) { AddFlagString("RIGHT_ON"); }
                    else { AddFlagString("ALL_ON"); }

                    WORD audioVol = currentStatus.leftVolume;
                    if (audioVol == 0) { audioVol = 0xFFFF; }
                    else if (audioVol < currentStatus.rightVolume) { audioVol = currentStatus.rightVolume; }
                    if (fdwCommand != MCI_SET_AUDIO_LEFT) currentStatus.rightVolume = audioVol;
                    if (fdwCommand != MCI_SET_AUDIO_RIGHT) currentStatus.leftVolume = audioVol;
                }
                else {
                    if (fdwCommand & MCI_SET_AUDIO_LEFT) { AddFlagString("LEFT_OFF"); }
                    else if (fdwCommand & MCI_SET_AUDIO_RIGHT) { AddFlagString("RIGHT_OFF"); }
                    else { AddFlagString("ALL_OFF"); }
                    if (fdwCommand != MCI_SET_AUDIO_LEFT) currentStatus.rightVolume = 0;
                    if (fdwCommand != MCI_SET_AUDIO_RIGHT) currentStatus.leftVolume = 0;
                }

                memcpy(&dwVolume, &currentStatus.rightVolume, sizeof(DWORD));
                fake_auxSetVolume(0, dwVolume);
            }
            if (fdwCommand & MCI_SET_DOOR_OPEN) { AddFlagString("DOOR_OPEN"); }
            if (fdwCommand & MCI_SET_DOOR_CLOSED) { AddFlagString("DOOR_CLOSED"); }
            //else { AddFlagString("VIDEO"); }
        }

        // MCI_SEEK Command
        else if (uMsg == MCI_SEEK) {
            LPMCI_SEEK_PARMS Params = static_cast<LPMCI_SEEK_PARMS>((LPVOID)dwParam);
            modeRequest.requestMode |= WPR_STOP | WPR_SET_TO | WPR_SET_FROM;
            requestStatus.playMode = 1;

            if (fdwCommand & MCI_SEEK_TO_START) {
                AddFlagString("TO_START");
                currentStatus.trackCurrent = currentStatus.trackTo = currentStatus.trackFrom = currentStatus.trackToTime = currentStatus.trackFromTime = 0;
            }
            else if (fdwCommand & MCI_SEEK_TO_END) {
                AddFlagString("TO_END");
                currentStatus.trackCurrent = currentStatus.trackTo = currentStatus.trackFrom = modeRequest.setTrackTo = modeRequest.setTrackFrom = currentStatus.trackCount;
                currentStatus.trackToTime = currentStatus.trackFromTime = modeRequest.setTrackToTime = modeRequest.setTrackFromTime = -1;
            }
            else if (fdwCommand & MCI_TO) {
                if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                    currentStatus.trackCurrent = currentStatus.trackTo = currentStatus.trackFrom = modeRequest.setTrackTo = modeRequest.setTrackFrom = MCI_TMSF_TRACK(Params->dwTo);
                    currentStatus.trackToTime = currentStatus.trackFromTime = modeRequest.setTrackToTime = modeRequest.setTrackFromTime = MCI_TMSF_TO_MILLISECS(Params->dwTo);
                    ADD_TMSF_PRINT_BUFFER(buffer, Params->dwTo);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                    currentStatus.trackCurrent = 0;
                    currentStatus.trackToTime = currentStatus.trackFromTime = modeRequest.setTrackToTime = modeRequest.setTrackFromTime = MCI_MSF_TO_MILLISECS(Params->dwTo);
                    ADD_MSF_PRINT_BUFFER(buffer, Params->dwTo);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                    currentStatus.trackCurrent = 0;
                    currentStatus.trackToTime = currentStatus.trackFromTime = modeRequest.setTrackToTime = modeRequest.setTrackFromTime = Params->dwTo;
                    ADD_MS_PRINT_BUFFER(buffer, Params->dwTo);
                }
            }
        }

        // MCI_GETDEVCAPS Command
        else if (uMsg == MCI_GETDEVCAPS) {
            LPMCI_GETDEVCAPS_PARMS Params = static_cast<LPMCI_GETDEVCAPS_PARMS>((LPVOID)dwParam);
            Params->dwReturn = FALSE;

            if (fdwCommand & MCI_GETDEVCAPS_ITEM) {
                AddFlagString("ITEM");

                if ((Params->dwItem == MCI_GETDEVCAPS_CAN_PLAY) || (Params->dwItem == MCI_GETDEVCAPS_CAN_EJECT) || (Params->dwItem == MCI_GETDEVCAPS_HAS_AUDIO)) {
                    Params->dwReturn = TRUE;
                }
                else if (Params->dwItem == MCI_GETDEVCAPS_DEVICE_TYPE) {
                    Params->dwReturn = MCI_DEVTYPE_CD_AUDIO;
                }
            }
        }

        // MCI_INFO Command
        else if (uMsg == MCI_INFO) {
            LPMCI_INFO_PARMS Params = static_cast<LPMCI_INFO_PARMS>((LPVOID)dwParam);

            if (fdwCommand & MCI_INFO_PRODUCT) {
                AddFlagString("PRODUCT");
                AddExtendString("Product:\"CD Audio\"");
                memcpy((LPVOID)(Params->lpstrReturn), (LPVOID) & "CD Audio", 9);
            }
            else if (fdwCommand & MCI_INFO_MEDIA_IDENTITY) {
                AddFlagString("IDENTITY");
                AddExtendString("ID:\"12345678\"");
                memcpy((LPVOID)(Params->lpstrReturn), (LPVOID) & "12345678", 9);
            }
        }

        // MCI_SYSINFO Command (for Heavy Gear, Battlezone2, Interstate 76)
        else if (uMsg == MCI_SYSINFO) {
            LPMCI_SYSINFO_PARMSA Params = static_cast<LPMCI_SYSINFO_PARMSA>((LPVOID)dwParam);

            if (fdwCommand & MCI_SYSINFO_QUANTITY) {
                AddFlagString("QUANTITY");
                AddExtendString("Qty:1");
                memcpy((LPVOID)(Params->lpstrReturn), (LPVOID) & "1", 2); // quantity = 1
            }
            else if ((fdwCommand & MCI_SYSINFO_NAME) || (fdwCommand & MCI_SYSINFO_INSTALLNAME)) {
                AddFlagString("NAME");
                AddExtendString("Name:\"cdaudio\"");
                memcpy((LPVOID)(Params->lpstrReturn), (LPVOID) & "cdaudio", 8); // name = cdaudio 
            }
        }

        // MCI_PAUSE Command
        else if (uMsg == MCI_PAUSE) {
            modeRequest.requestMode |= WPR_PAUSE;
            currentStatus.timePlayLast = GetTickCount64() - currentStatus.timePlayStart;
            requestStatus.playMode = 2;
        }

        // MCI_STOP Command
        else if (uMsg == MCI_STOP) {
            modeRequest.requestMode |= WPR_STOP;
            currentStatus.trackTo = currentStatus.trackFrom = currentStatus.trackToTime = currentStatus.trackFromTime = 0;
            requestStatus.playMode = 1;
        }

        // MCI_RESUME Command
        else if (uMsg == MCI_RESUME) {
            modeRequest.requestMode |= WPR_RESUME;
            currentStatus.timePlayStart = GetTickCount64() - currentStatus.timePlayLast + playtimeDelay;
            requestStatus.playMode = 1;
        }

        // MCI_CLOSE Command
        else if (uMsg == MCI_CLOSE) {
            currentStatus.timeFormat = MCI_FORMAT_MSF; // reset time format
            modeRequest.requestMode |= WPR_CLOSE;
            currentStatus.timePlayLast = 0;
            requestStatus.playMode = 1;
        }

        // Parse command failed
        else {
            returnSendCommand(~msgNum);
            ReleaseMutex(mciMutex.sendCommand);
            return relay_mciSendCommandA(IDDevice, uMsg, fdwCommand, dwParam);
        }

        resualt = returnSendCommand(msgNum);
        ReleaseMutex(mciMutex.sendCommand);

        // Wait for MCI_PLAY + MCI_WAIT Command
        if ((uMsg == MCI_PLAY) && (fdwCommand & MCI_WAIT)) {
            while (currentStatus.playMode == 0) { Sleep(1); }
        }

        return resualt;
    }
    else {
        if (fh) {
            sprintf(msgDebug, "%08X", fdwCommand);
            strcpy(msgDbgExtend, "No description (not a CD Device)");
        }
    }

    // Relay the original mciSendCommandA when there are other devices
    returnSendCommand(msgNum);
    ReleaseMutex(mciMutex.sendCommand);
    return relay_mciSendCommandA(IDDevice, uMsg, fdwCommand, dwParam);
}


// MCI Command parsing and Information Return Function
// If information is needed, it will be call fake_mciSendCommandA function.
MCIERROR WINAPI fake_mciSendStringA(LPCSTR lpCommand, LPSTR lpReturnString, UINT cchReturn, HWND hwndCallback) {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_mciSendStringA(lpCommand, lpReturnString, cchReturn, hwndCallback);
    }
    EnterCriticalSection(&mciSendStringSession);
    static const char CDDeviceName[] = "cdaudio";
    static char cmdStrings[0x200];
    static char cmdName[0x40];
    static char cmdDevice[MAX_PATH];
    static char cmdMessage[0x200];
    static char buffer[64];
    static char returnBuffer[0x200];
    static DWORD MCI_Return;
    DWORD_PTR fdwCommand = 0;
    int i, aliasPos, isCDDevice, dwFrom, dwTo, dwVolume;

    // Initialize buffer
    buffer[0] = cmdDevice[0] = cmdName[0] = cmdStrings[0] = returnBuffer[0] = MCI_Return = 0;

    // Change command string into lower case
    strcpy(cmdStrings, lpCommand);
    for (i = 0; cmdStrings[i]; i++) {
        cmdStrings[i] = tolower(cmdStrings[i]);
    }
    cmdStrings[i++] = 0x20; // Add last space for sscanf
    cmdStrings[i] = 0;
    isCDDevice = 0;

    // Get command name
    if (sscanf(cmdStrings, "%s %s %*s", cmdName, cmdDevice) == 2) {
        i = strlen(cmdName) + strlen(cmdDevice) + 2;
        strcpy(cmdMessage, &cmdStrings[i]);

        // Evaluate Alias is CD Device
        for (aliasPos = 0; aliasPos < mciAliasCount; aliasPos++) {
            if (strcmp(mciNamedAlias[aliasPos], cmdDevice) == 0) {
                isCDDevice = 1;
                break;
            }
        }

        // Check open type
        if ((!isCDDevice) && ((strncmp(cmdName, "open", 4) == 0) && strstr(cmdMessage, "type cdaudio"))) {
            strcpy(mciNamedAlias[mciAliasCount++], cmdDevice);
        }
    }

    if (isCDDevice) {
        dwFrom = dwTo = 0;
        debugOffOnce = 1;

        // Check common Flags
        if (strstr(cmdMessage, "notify")) {
            fdwCommand |= MCI_NOTIFY;
            requestStatus.notifySuccess = TRUE;
        }
        else {
            requestStatus.notifySuccess = FALSE;
        }
        if (strstr(cmdMessage, "wait")) { fdwCommand |= MCI_WAIT; }
        if (sscanf(cmdMessage, "%*s from %d ", &dwFrom) == 1) { fdwCommand |= MCI_FROM; }
        if (sscanf(cmdMessage, "%*s to %d ", &dwTo) == 1) { fdwCommand |= MCI_TO; }
        requestStatus.dwCallback = (DWORD_PTR)hwndCallback;

        // "open" Command
        if (strcmp(cmdName, "open") == 0) {
            static MCI_OPEN_PARMS Params;
            if (strstr(cmdMessage, "alias")) {
                if (sscanf(cmdStrings, "%*salias %s %*s", mciNamedAlias[mciAliasCount]) == 1) {
                    mciAliasCount++;
                }
            }
            memset(&Params, 0, sizeof(MCI_OPEN_PARMS));
            fdwCommand |= MCI_OPEN_TYPE;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            Params.lpstrDeviceType = CDDeviceName;
            fake_mciSendCommandA(magicDeviceID, MCI_OPEN, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "play" Command
        else if (strcmp(cmdName, "play") == 0) {
            static MCI_PLAY_PARMS Params;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            Params.dwFrom = dwFrom;
            Params.dwFrom = dwTo;
            fake_mciSendCommandA(magicDeviceID, MCI_PLAY, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "stop" Command
        else if (strcmp(cmdName, "stop") == 0) {
            static MCI_GENERIC_PARMS Params;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            fake_mciSendCommandA(magicDeviceID, MCI_STOP, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "pause" Command
        else if (strcmp(cmdName, "pause") == 0) {
            static MCI_GENERIC_PARMS Params;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            fake_mciSendCommandA(magicDeviceID, MCI_PAUSE, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "close" Command
        else if (strcmp(cmdName, "close") == 0) {
            static MCI_GENERIC_PARMS Params;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            if (strstr(CDDeviceName, cmdDevice) == NULL) { mciNamedAlias[aliasPos][0] = 0; }
            fake_mciSendCommandA(magicDeviceID, MCI_CLOSE, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "set" Command
        else if (strstr(cmdName, "set")) {
            static MCI_SET_PARMS Params;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            Params.dwAudio = 0;
            Params.dwTimeFormat = 0;

            // "setaudio" or "set alias audio"
            if (strstr(cmdName, "audio") || strstr(cmdMessage, "audio ")) {
                if (strstr(cmdMessage, " on ") || strstr(cmdMessage, " off "))
                {
                    fdwCommand |= MCI_SET_AUDIO;

                    if (strstr(cmdMessage, " on ")) { fdwCommand |= MCI_SET_ON; }
                    else { fdwCommand |= MCI_SET_OFF; }

                    if (strstr(cmdMessage, " left ")) { fdwCommand |= MCI_SET_AUDIO_LEFT; }
                    else if (strstr(cmdMessage, " right ")) { fdwCommand |= MCI_SET_AUDIO_RIGHT; }
                    else { fdwCommand |= MCI_SET_AUDIO_ALL; }

                    fake_mciSendCommandA(magicDeviceID, MCI_CLOSE, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
                }
                else if (sscanf(cmdMessage, "%*svolume to %d ", &dwVolume) == 1) {
                    dwVolume = (float)dwVolume * 0.001 * 0xFFFF;

                    if (strstr(cmdMessage, "left volume to ") != NULL) {
                        currentStatus.rightVolume = dwVolume;
                    }
                    if (strstr(cmdMessage, "right volume to ") != NULL) {
                        currentStatus.leftVolume = dwVolume;
                    }
                    memcpy(&dwVolume, &currentStatus.rightVolume, sizeof(DWORD));
                    fake_auxSetVolume(magicDeviceID, dwVolume);
                    SendCommonNotify(MCI_SET);
                }
            }

            // "set time format"
            if (sscanf(cmdMessage, "%*stime format %s", buffer) == 1) {
                if (strstr(buffer, "milliseconds") || strstr(buffer, "ms")) { Params.dwTimeFormat = MCI_FORMAT_MILLISECONDS; }
                else if (strstr(buffer, "tmsf")) { Params.dwTimeFormat = MCI_FORMAT_TMSF; }
                else if (strstr(buffer, "msf")) { Params.dwTimeFormat = MCI_FORMAT_MSF; }
                fdwCommand |= MCI_SET_TIME_FORMAT;
                fake_mciSendCommandA(magicDeviceID, MCI_SET, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
            }
        }

        // "status" Command
        else if (strcmp(cmdName, "status") == 0) {
            static MCI_STATUS_PARMS Params;
            fdwCommand |= MCI_STATUS_ITEM;
            Params.dwCallback = (DWORD_PTR)hwndCallback;
            Params.dwTrack = 0;
            //dprintf_nl("%s,%08x", cmdMessage, strstr(cmdMessage, "media present"));
            if (strstr(cmdMessage, "mode")) {
                if (currentStatus.playMode == 0) { strcpy(returnBuffer, "playing"); }
                else if (currentStatus.playMode == 1) { strcpy(returnBuffer, "Stopped"); }
                else { strcpy(returnBuffer, "paused"); }
                SendCommonNotify(MCI_STATUS);
            }
            else if (strstr(cmdMessage, "media present") || strstr(cmdMessage, "ready") || strstr(cmdMessage, "index on")) {
                strcpy(returnBuffer, "true");
                MCI_Return = TRUE;
                SendCommonNotify(MCI_STATUS);
            }
            else if (strstr(cmdMessage, "time format")) {
                if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) { strcpy(returnBuffer, "milliseconds"); }
                else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) { strcpy(returnBuffer, "tmsf"); }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) { strcpy(returnBuffer, "msf"); }
                SendCommonNotify(MCI_STATUS);
            }
            else if (strstr(cmdMessage, "type track")) {
                Params.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
                fake_mciSendCommandA(magicDeviceID, MCI_STATUS, fdwCommand, (DWORD_PTR)(LPVOID)&Params);

                if (Params.dwReturn & MCI_CDA_TRACK_AUDIO) { strcpy(returnBuffer, "audio"); }
                else { strcpy(returnBuffer, "other"); }
            }
            else if (strstr(cmdMessage, "number of tracks")) {
                Params.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
                fake_mciSendCommandA(magicDeviceID, MCI_STATUS, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
                sprintf(returnBuffer, "%d", Params.dwReturn);
            }
            else if (strstr(cmdMessage, "current track")) {
                Params.dwItem = MCI_STATUS_CURRENT_TRACK;
                fake_mciSendCommandA(magicDeviceID, MCI_STATUS, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
                sprintf(returnBuffer, "%d", Params.dwReturn);
            }
            else if (strstr(cmdMessage, "length")) {
                Params.dwItem = MCI_STATUS_LENGTH;
                if (sscanf(cmdMessage, "length track %d", &Params.dwTrack) == 1) { fdwCommand |= MCI_TRACK; }
                fake_mciSendCommandA(magicDeviceID, MCI_STATUS, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
                if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                    sprintf(returnBuffer, "%d", Params.dwReturn);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                    MCI_MSF_PRINT_MSF_BUFFER(returnBuffer, Params.dwReturn);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                    MCI_TMSF_PRINT_MSF_BUFFER(returnBuffer, Params.dwReturn);
                }
            }
            else if (strstr(cmdMessage, "position")) {
                if (strstr(cmdMessage, "start position")) { Params.dwItem = MCI_STATUS_START; }
                else {
                    Params.dwItem = MCI_STATUS_POSITION;
                    if (sscanf(cmdMessage, "position track %d", &Params.dwTrack) == 1) { fdwCommand |= MCI_TRACK; }
                }

                fake_mciSendCommandA(magicDeviceID, MCI_STATUS, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
                if (currentStatus.timeFormat == MCI_FORMAT_MILLISECONDS) {
                    sprintf(returnBuffer, "%d", Params.dwReturn);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_MSF) {
                    MCI_MSF_PRINT_MSF_BUFFER(returnBuffer, Params.dwReturn);
                }
                else if (currentStatus.timeFormat == MCI_FORMAT_TMSF) {
                    MCI_TMSF_PRINT_TMSF_BUFFER(returnBuffer, Params.dwReturn);
                }
            }
            else { SendCommonNotify(MCI_STATUS); }
        }

        // "seek" Command
        else if (strcmp(cmdName, "seek") == 0) {
            static MCI_SEEK_PARMS Params;
            if (sscanf(cmdMessage, " to %s", buffer) == 1) {
                if (strstr(buffer, "start")) { fdwCommand |= MCI_SEEK_TO_START; }
                else if (strstr(buffer, "end")) { fdwCommand |= MCI_SEEK_TO_START; }
            }
            fake_mciSendCommandA(magicDeviceID, MCI_SEEK, fdwCommand, (DWORD_PTR)(LPVOID)&Params);
        }

        // "capability" Command (MCI_GETDEVCAPS SendString equivalent)
        else if (strcmp(cmdName, "capability") == 0) {
            if (strstr(cmdMessage, "can ") || strstr(cmdMessage, "has ")) {
                MCI_Return = TRUE;
                strcpy(returnBuffer, "true");
            }
            else if (strstr(cmdMessage, "device type")) { strcpy(returnBuffer, "cdaudio"); }
            SendCommonNotify(MCI_GETDEVCAPS);
        }

        // "sysinfo" Command
        else if (strcmp(cmdName, "sysinfo") == 0) {
            if (strstr(cmdMessage, "quantity")) {
                strcpy(returnBuffer, "1");
            }
            // Example: "sysinfo cdaudio name 1 open" returns "cdaudio" or the alias.
            else if ((strstr(cmdMessage, "name") && strstr(cmdMessage, "open")) || strstr(cmdMessage, "installname")) {
                strcpy(returnBuffer, CDDeviceName);
            }
            SendCommonNotify(MCI_SYSINFO);
        }

        // "info" Command
        else if (strcmp(cmdName, "info") == 0) {
            if (strstr(cmdMessage, "identity")) { strcpy(returnBuffer, "12345678"); }
            else if (strstr(cmdMessage, "product")) { strcpy(returnBuffer, "CD Audio"); }
            SendCommonNotify(MCI_INFO);
        }

        // unknown Command
        else {
            dprintf_nl("********Unsupported commands, relay to original function********");
            dprintf_nl("mciSendStringA\t\t%08X(hWnd)\t%08X(Retn)\t\"%s\"   -->   [Relay to original]", (DWORD)hwndCallback, cchReturn, lpCommand);
            dprintf_nl("****************************************************************");
            ReleaseMutex(mciMutex.sendCommand);
            LeaveCriticalSection(&mciSendStringSession);
            return relay_mciSendStringA(lpCommand, lpReturnString, cchReturn, hwndCallback);
        }

        if ((DWORD)lpReturnString != 0) { strncpy(lpReturnString, returnBuffer, cchReturn); }
    }
    else {
        // not CD Audio device
        dprintf_nl("mciSendStringA\t\t%08X(hWnd)\t%08X(Retn)\t\"%s\"   -->   [Relay to original]", (DWORD)hwndCallback, cchReturn, lpCommand);
        ReleaseMutex(mciMutex.sendCommand);
        LeaveCriticalSection(&mciSendStringSession);
        return relay_mciSendStringA(lpCommand, lpReturnString, cchReturn, hwndCallback);
    }

    debugOffOnce = 0;
    dprintf_nl("mciSendStringA\t\t%08X(hWnd)\t%08X(Retn)\t\"%s\"   -->   \"%s\"", (DWORD)hwndCallback, cchReturn, lpCommand, returnBuffer);
    ReleaseMutex(mciMutex.sendCommand);
    LeaveCriticalSection(&mciSendStringSession);
    return MCI_Return;
}

UINT WINAPI fake_auxGetNumDevs() {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_auxGetNumDevs();
    }

    dprintf_nl("auxGetNumDevs\t\t(null)\t\t(null)\t\t(null)\t\t\treturnNumDevs\t\t\tNumDevs:1");
    ReleaseMutex(mciMutex.sendCommand);
    return 1;
}
MMRESULT WINAPI fake_auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT cbCaps) {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_auxGetDevCapsA(uDeviceID, lpCaps, cbCaps);
    }

    lpCaps->wMid = 2; // MM_CREATIVE
    lpCaps->wPid = 401; // MM_CREATIVE_AUX_CD
    lpCaps->vDriverVersion = 1;
    strcpy(lpCaps->szPname, "ogg-winmm virtual CD");
    lpCaps->wTechnology = AUXCAPS_CDAUDIO;
    lpCaps->dwSupport = AUXCAPS_VOLUME | AUXCAPS_LRVOLUME;
    dprintf_nl(
        "auxGetDevCapsA\t\t%08X\twMid=%d\t\twPid=%03d\t\tvDriverVersion=%d\t\tszPname=%s",
        uDeviceID, lpCaps->wMid, lpCaps->wPid, lpCaps->vDriverVersion, lpCaps->szPname
    );
    ReleaseMutex(mciMutex.sendCommand);
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI fake_auxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume) {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_auxGetVolume(uDeviceID, lpdwVolume);
    }

    memcpy(&*lpdwVolume, &currentStatus.rightVolume, sizeof(DWORD));
    dprintf_nl(
        "auxGetVolume\t\t%08X\t%08X\tGetVolume\t\tlpdwVolume\t\t\tL:%3.1f%%, R:%3.1f%%",
        uDeviceID, *lpdwVolume, ((float)LOWORD(*lpdwVolume) / (float)0xFFFF * 100.0), ((float)HIWORD(*lpdwVolume) / (float)0xFFFF * 100.0)
    );
    ReleaseMutex(mciMutex.sendCommand);
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI fake_auxSetVolume(UINT uDeviceID, DWORD dwVolume) {
    WaitForSingleObject(mciMutex.sendCommand, INFINITE);
    if (!procIdentifier) {
        ReleaseMutex(mciMutex.sendCommand);
        return relay_auxSetVolume(uDeviceID, dwVolume);
    }

    memcpy(&currentStatus.rightVolume, &dwVolume, sizeof(DWORD));
    memcpy(&modeRequest.setRightVolume, &dwVolume, sizeof(DWORD));
    modeRequest.requestMode = WPR_SET_VOLUME;
    SendIPCMessage();

    dprintf_nl(
        "auxSetVolume\t\t%08X\t%08X\tSetVolume\t\tdwVolume\t\t\tL:%3.1f%%, R:%3.1f%%",
        uDeviceID, dwVolume, ((float)LOWORD(dwVolume) / (float)0xFFFF * 100.0), ((float)HIWORD(dwVolume) / (float)0xFFFF * 100.0)
    );
    ReleaseMutex(mciMutex.sendCommand);
    return MMSYSERR_NOERROR;
}