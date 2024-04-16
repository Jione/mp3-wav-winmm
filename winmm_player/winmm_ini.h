#pragma once
#ifndef WINMM_INI_HEADER_H
#define WINMM_INI_HEADER_H

#define WINMM_EXE_INI                   // EXE INI

// Mailslot IPC
// Request mode values
#define WPR_NOTIFY 0x1					// Live dll server notify
#define WPR_PLAY 0x2					// Play track
#define WPR_STOP 0x4					// Stop track
#define WPR_PAUSE 0x8					// Pause track
#define WPR_RESUME 0x10					// Resume track
#define WPR_CLOSE 0x20					// Close device
#define WPR_SET_FROM 0x40				// Play or Seek from track
#define WPR_SET_TO 0x80					// Play or Seek to track
#define WPR_SET_VOLUME 0x100			// Change volume to requested value
#define WPR_GET_VOLUME 0x200			// Request current volume value (reserved)
#define WPR_GET_PLAYMODE 0x400			// Request current playback status (reserved)
#define WPR_GET_COUNT 0x800				// Request track count (reserved)
#define WPR_GET_LENGTH 0x1000			// Request track length (reserved)
#define WPR_GET_CURRENT_TRACK 0x2000	// Request track number currently playing (reserved)
#define WPR_GET_CURRENT_POS 0x4000		// Request current playing track position (reserved)
#define WPR_OPEN 0x10000				// Request open device for server start
#define WPR_EXIT 0x20000				// Exit cdaudio program

#define WPR_UPDATE_INIT -1				// Initialize update
#define WPR_NOT_UPDATED 0				// Never updated yet
#define WPR_UPDATED 0x1					// Updated
#define WPR_WAIT_UPDATE 0x2				// Waiting for update

// Mailslot IPC
// Struct for mode requests
// Limited to 256 bytes or less for compatibility with older versions
typedef struct {
	char msgString[0x18];			// Request message string (Reserved for older version compatibility)
	DWORD hWndProcID;				// Identifier for request verification
	DWORD requestMode;				// Bitmask mode value for request (WPR_COMMAND)
	int setTrackFrom;				// Request to play or seek from set track number
	int setTrackFromTime;			// Request to play or seek from set time
	int setTrackTo;					// Request to play or seek to set track number (reserved)
	int setTrackToTime;				// Request to play or seek to set time
	WORD setRightVolume;			// Request to set right volume level (0-65535)
	WORD setLeftVolume;				// Request to set left volume level (0-65535)
	char musicPath[0xCA];			// Music file path for the requested process
} MM_REQUEST;

// Mailslot IPC
// Struct for response data
typedef struct {
	DWORD timePlayStart;			// Play start time (GetTickCount - Current Playtime)
	int trackCurrent;				// Last played Track
	int trackFromTime;				// Last set start time
	int trackToTime;				// Last set end time
	WORD rightVolume;				// Right volume level (0-65535)
	WORD leftVolume;				// Left volume level (0-65535)
} MM_RESPONSE;
#endif

// DLL ini Loader
#ifdef WINMM_DLL_INI
// winmm.ini
// DLL Preference struct
typedef struct {
    DWORD magicDeviceID;
    BOOL useCustomDevice;
    BOOL autoStartExec;
    BOOL autoCloseExec;
    WORD rightVolume;
    WORD leftVolume;
    BOOL useDebugDLL;
    char musicPlayer[MAX_PATH];
    char musicFolder[MAX_PATH];
    char musicNameFirst[MAX_PATH];
}INI_STATUS;

BOOL LoadInitialize(INI_STATUS* iniStatus);
#endif

// EXE ini Loader
#ifdef WINMM_EXE_INI
// winmm.ini
// EXE Preference struct
typedef struct {
	BOOL minimizeStartup;
	float overrideVolume;
	BOOL useDebugEXE;
    BOOL useLastCore;
}INI_STATUS;

BOOL LoadInitialize(INI_STATUS* iniStatus);
BOOL UpdateProfile(INI_STATUS* iniStatus);
#endif