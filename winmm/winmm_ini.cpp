#include <windows.h>
#include <stdio.h>
#include "winmm_ini.h"

// ini Filename
static const char iniFile[] = "winmm.ini";

// Write ini value to int
BOOL WritePrivateProfileIntA(LPCSTR lpszSection, LPCSTR lpszEntry, int val, LPCSTR lpszFileName) {
    char tmp[12];
    wsprintf(tmp, "%d", val);
    return WritePrivateProfileStringA(lpszSection, lpszEntry, tmp, lpszFileName);
}

BOOL WriteNewINIFile() {
    FILE* iniFp;
    if ((iniFp = fopen(iniFile, "w")) != NULL) {
        fprintf(iniFp,
            "# Setting for Virtual CD-Audio MCI device\n"
            "# Basically, Disable option is 0 and Enable option is 1.\n"
            "#\n"
            "# MCIDeviceID: Alternate MCI DeviceID. (default: 48879, It works if the UseCustomMCIDeviceID is set to 1)\n"
            "# auxVolume: Volume level (0-100), no change in system if negative value. (default: -1)\n"
            "\n"
            "[WINMM]\n"
            "AutoStartExec=1\n"
            "AutoCloseExec=1\n"
            "UseCustomMCIDeviceID=0\n"
            "MCIDeviceID=48879\n"
            "auxVolume=-1\n"
            "UseDLLDebugLog=0\n"
            "MusicPlayer=winmm_player.exe\n"
            "MusicFolder=music\n"
            "MusicNameFirst=track\n"
            "\n"
            "[PLAYER]\n"
            "MinimizeStartup=1\n"
            "OverrideVolume=100\n"
            "UseExeDebugLog=0\n"
            "UseLastCore=1\n"
        );
        fclose(iniFp);
        return TRUE;
    }
    return FALSE;
}

#ifdef WINMM_DLL_INI
#include "winmm_stubs.h"

// winmm.ini Preference data struct
static INI_STATUS dllDefault{
    0xBEEF,     //magicDeviceID
    0,          //useCustomDevice
    1,          //autoStartExec
    1,          //autoCloseExec
    0xFFFF,     //volumeLeft
    0xFFFF,     //volumeRight
    0,          //useDebugDLL
    "winmm_player.exe"
    "music"
    "track"
};

// Load winmm.ini file for DLL
BOOL LoadInitialize(INI_STATUS* iniStatus) {
    char iniFilename[MAX_PATH]{};
    char currentPath[MAX_PATH]{};
    FILE* iniFp = NULL;
    int volumeLevel = -1;
    DWORD volume = 0;

    GetCurrentDirectory(MAX_PATH, currentPath);
    sprintf(iniFilename, "%s\\%s", currentPath, iniFile);
    memcpy(iniStatus, &dllDefault, sizeof(INI_STATUS));

    if ((iniFp = fopen(iniFile, "rb")) != NULL) {
        fclose(iniFp);
        iniStatus->autoStartExec = GetPrivateProfileIntA("WINMM", "AutoStartExec", dllDefault.autoStartExec, iniFilename);
        iniStatus->autoCloseExec = GetPrivateProfileIntA("WINMM", "AutoCloseExec", dllDefault.autoCloseExec, iniFilename);
        iniStatus->useCustomDevice = GetPrivateProfileIntA("WINMM", "UseCustomMCIDeviceID", dllDefault.useCustomDevice, iniFilename);
        iniStatus->magicDeviceID = GetPrivateProfileIntA("WINMM", "MCIDeviceID", dllDefault.magicDeviceID, iniFilename);
        volumeLevel = GetPrivateProfileIntA("WINMM", "auxVolume", volumeLevel, iniFilename);
        iniStatus->useDebugDLL = GetPrivateProfileIntA("WINMM", "UseDLLDebugLog", dllDefault.useDebugDLL, iniFilename);
        GetPrivateProfileStringA("WINMM", "MusicPlayer", dllDefault.musicPlayer, iniStatus->musicPlayer, MAX_PATH, iniFilename);
        GetPrivateProfileStringA("WINMM", "MusicFolder", dllDefault.musicFolder, iniStatus->musicFolder, MAX_PATH, iniFilename);
        GetPrivateProfileStringA("WINMM", "MusicNameFirst", dllDefault.musicNameFirst, iniStatus->musicNameFirst, MAX_PATH, iniFilename);

        // Rewrite Profile when Debug on
        if (iniStatus->useDebugDLL && ((iniFp = fopen(iniFile, "r+b")) != NULL)) {
            fclose(iniFp);
            WritePrivateProfileIntA("WINMM", "AutoStartExec", iniStatus->autoStartExec, iniFilename);
            WritePrivateProfileIntA("WINMM", "AutoCloseExec", iniStatus->autoCloseExec, iniFilename);
            WritePrivateProfileIntA("WINMM", "UseCustomMCIDeviceID", iniStatus->useCustomDevice, iniFilename);
            WritePrivateProfileIntA("WINMM", "MCIDeviceID", iniStatus->magicDeviceID, iniFilename);
            WritePrivateProfileIntA("WINMM", "auxVolume", volumeLevel, iniFilename);
            WritePrivateProfileIntA("WINMM", "UseDLLDebugLog", iniStatus->useDebugDLL, iniFilename);
            WritePrivateProfileStringA("WINMM", "MusicPlayer", iniStatus->musicPlayer, iniFilename);
            WritePrivateProfileStringA("WINMM", "MusicFolder", iniStatus->musicFolder, iniFilename);
            WritePrivateProfileStringA("WINMM", "MusicNameFirst", iniStatus->musicNameFirst, iniFilename);
        }

        if (volumeLevel >= 0 && volumeLevel <= 100) {
            iniStatus->leftVolume = iniStatus->rightVolume = ((float)volumeLevel / 100.0) * 0xFFFF;
            memset(&volume, iniStatus->rightVolume, sizeof(DWORD));
            relay_waveOutSetVolume(0, volume);
        }
        else {
            if (relay_waveOutGetVolume(0, (DWORD*)&volumeLevel) == MMSYSERR_NOERROR) {
                iniStatus->leftVolume = iniStatus->rightVolume = volumeLevel;
            }
            else {
                iniStatus->leftVolume = iniStatus->rightVolume = 0xFFFF;
                relay_waveOutSetVolume(0, (DWORD)0xFFFFFFFF);
            }
        }
    }
    else if (!WriteNewINIFile()) { return FALSE; }
    return TRUE;
}
#endif

#ifdef WINMM_EXE_INI
// winmm.ini Preference data struct
static INI_STATUS exeDefault{
    1,          //minimizeStartup
    1.0,        //overrideVolume
    0,          //useDebugEXE
    1,          //useLastCore
};

// Load winmm.ini file for EXE
BOOL LoadInitialize(INI_STATUS* iniStatus) {
    char iniFilename[MAX_PATH]{};
    char currentPath[MAX_PATH]{};
    FILE* iniFp = NULL;
    int volumeLevel = 100;

    GetCurrentDirectory(MAX_PATH, currentPath);
    sprintf(iniFilename, "%s\\%s", currentPath, iniFile);
    memcpy(iniStatus, &exeDefault, sizeof(INI_STATUS));

    if ((iniFp = fopen(iniFile, "rb")) != NULL) {
        fclose(iniFp);
        iniStatus->useLastCore = GetPrivateProfileIntA("PLAYER", "UseLastCore", exeDefault.useLastCore, iniFilename);
        iniStatus->minimizeStartup = GetPrivateProfileIntA("PLAYER", "MinimizeStartup", exeDefault.minimizeStartup, iniFilename);
        volumeLevel = GetPrivateProfileIntA("PLAYER", "OverrideVolume", volumeLevel, iniFilename);
        iniStatus->useDebugEXE = GetPrivateProfileIntA("PLAYER", "UseExeDebugLog", exeDefault.useDebugEXE, iniFilename);

        // Rewrite Profile when Debug on
        if (iniStatus->useDebugEXE && ((iniFp = fopen(iniFile, "r+b")) != NULL)) {
            fclose(iniFp);
            WritePrivateProfileIntA("PLAYER", "MinimizeStartup", iniStatus->minimizeStartup, iniFilename);
            WritePrivateProfileIntA("PLAYER", "OverrideVolume", volumeLevel, iniFilename);
            WritePrivateProfileIntA("PLAYER", "UseExeDebugLog", iniStatus->useDebugEXE, iniFilename);
            WritePrivateProfileIntA("PLAYER", "UseLastCore", iniStatus->useLastCore, iniFilename);
        }

        if (volumeLevel >= 0 && volumeLevel <= 100) {
            iniStatus->overrideVolume = (float)volumeLevel / 100.0;
        }
        else {
            iniStatus->overrideVolume = 1.0;
        }
    }
    else if (!WriteNewINIFile()) { return FALSE; }
    return TRUE;
}

// Write winmm.ini file for EXE
BOOL UpdateProfile(INI_STATUS* iniStatus) {
    char iniFilename[MAX_PATH]{};
    char currentPath[MAX_PATH]{};
    FILE* iniFp = NULL;
    int volumeLevel = iniStatus->overrideVolume * 100.0;

    GetCurrentDirectory(MAX_PATH, currentPath);
    sprintf(iniFilename, "%s\\%s", currentPath, iniFile);

    if (iniStatus->useDebugEXE && ((iniFp = fopen(iniFile, "r+b")) != NULL)) {
        fclose(iniFp);
        WritePrivateProfileIntA("PLAYER", "MinimizeStartup", iniStatus->minimizeStartup, iniFilename);
        WritePrivateProfileIntA("PLAYER", "OverrideVolume", volumeLevel, iniFilename);
        WritePrivateProfileIntA("PLAYER", "UseExeDebugLog", iniStatus->useDebugEXE, iniFilename);
        WritePrivateProfileIntA("PLAYER", "UseLastCore", iniStatus->useLastCore, iniFilename);
        return TRUE;
    }
    else {
        return FALSE;
    }
}
#endif