#pragma once
#ifndef WINMM_WRAPPER_HEADER_H
#define WINMM_WRAPPER_HEADER_H
#include "winmm_stubs.h"
#include "winmm_ini.h"
#include "winmm_finder.h"

// Macro define
#define dprintf_nl(...) if (fh) {fflush(NULL); fprintf(fh, __VA_ARGS__); fputc('\n', fh); fflush(NULL);}
#define AddFlagString(str) if (msgDebug[0] == 0) { strcpy(msgDebug, str); } else { strcat (strcat (msgDebug, "|"), str); }
#define AddExtendString(str) if (msgDbgExtend[0] == 0) { strcpy (msgDbgExtend, str); } else { strcat(strcat(msgDbgExtend, ", "), str); }
#define MCI_TMSF_TO_MILLISECS(param) (((int)MCI_TMSF_FRAME(param) * 40 / 3) % 1000) + ((int)MCI_TMSF_SECOND(param) * 1000) + ((int)MCI_TMSF_MINUTE(param) * 60000)
#define MCI_MSF_TO_MILLISECS(param) (((int)MCI_MSF_FRAME(param) * 40 / 3) % 1000) + ((int)MCI_MSF_SECOND(param) * 1000) + ((int)MCI_MSF_MINUTE(param) * 60000)
#define MCI_MILLISECS_TO_TMSF(ms)  MCI_MAKE_TMSF(currentStatus.trackCurrent, (ms / 60000), ((ms / 1000) % 60), (((ms % 1000) * 3 / 40) % 75))
#define MCI_MILLISECS_TO_MSF(ms) MCI_MAKE_MSF((ms / 60000), ((ms / 1000) % 60), (((ms % 1000) * 3 / 40) % 75))
#define MCI_TMSF_PRINT_TMSF_BUFFER(buffer,param) sprintf(buffer, "%02d:%02d:%02d:%02d", MCI_TMSF_TRACK(param), MCI_TMSF_MINUTE(param), MCI_TMSF_SECOND(param), MCI_TMSF_FRAME(param));
#define MCI_TMSF_PRINT_MSF_BUFFER(buffer,param) sprintf(buffer, "%02d:%02d:%02d", MCI_TMSF_MINUTE(param), MCI_TMSF_SECOND(param), MCI_TMSF_FRAME(param));
#define MCI_MSF_PRINT_MSF_BUFFER(buffer,param) sprintf(buffer, "%02d:%02d:%02d", MCI_MSF_MINUTE(param), MCI_MSF_SECOND(param), MCI_MSF_FRAME(param));
#define MCI_MILLISECS_TO_FRAME(ms) ((ms % 1000) * 3 / 40) % 75
#define ADD_TMSF_PRINT_BUFFER(buffer,param) sprintf(buffer, "Track%02d[%d:%02d.%02d]", MCI_TMSF_TRACK(param), MCI_TMSF_MINUTE(param), MCI_TMSF_SECOND(param), MCI_TMSF_FRAME(param)); AddExtendString(buffer)
#define ADD_MSF_PRINT_BUFFER(buffer,param) sprintf(buffer, "StartTo[%d:%02d.%02d]", MCI_MSF_MINUTE(param), MCI_MSF_SECOND(param), MCI_MSF_FRAME(param)); AddExtendString(buffer)
#define ADD_MS_PRINT_BUFFER(buffer,param) sprintf(buffer, "%dms", param); AddExtendString(buffer)
#define MS_PRINT_MSF_BUFFER(buffer,ms) sprintf(buffer, "%d:%02d.%02d", (ms / 60000), ((ms / 1000) % 60), (((ms % 1000) * 3 / 40) % 75));
#define MS_PRINT_TMSF_BUFFER(buffer,track,ms) sprintf(buffer, "Track%02d[%d:%02d.%02d]", track, (ms / 60000), ((ms / 1000) % 60), (((ms % 1000) * 3 / 40) % 75));
#define MSF_PRINT_MSF_BUFFER(buffer,param) sprintf(buffer, "%d:%02d.%02d", MCI_MSF_MINUTE(param), MCI_MSF_SECOND(param), MCI_MSF_FRAME(param));
#define TMSF_PRINT_MSF_BUFFER(buffer,param) sprintf(buffer, "%d:%02d.%02d", MCI_TMSF_MINUTE(param), MCI_TMSF_SECOND(param), MCI_TMSF_FRAME(param));


// Operating status Struct for receive a response
typedef struct {
	BOOL isUpdated;					// Process handshake complete or not
	BOOL notifyPlayback;			// Delayed Playback Notifications (0: not working, 1: wait play)
	DWORD playMode;					// Current playback status (0: playing, 1: stopped, 2: paused)
	DWORD timeFormat;				// set Time format for notify
	int trackFrom;					// Last set start track number
	int trackTo;					// Last set end track number
	DWORD timePlayLast;				// Last play time (when pause-resume used)
	DWORD timePlayStart;			// Play start time (GetTickCount - Current Playtime)
	int trackFromTime;				// Last set start time
	int trackToTime;				// Last set end time
	DWORD trackCount;				// Number of tracks (0-255)
	int trackCurrent;				// Last played Track
	WORD rightVolume;				// Right volume level (0-65535)
	WORD leftVolume;				// Left volume level (0-65535)
	DWORD trackTimes[0x80];			// Playtime by tracks (in milliseconds)
	char trackNames[0x80][0x20];	// Track Filename
}MM_STATUS;


// Evaluate mciSendCommands request values struct
typedef struct {
	DWORD playMode;					// Current playback status (0: playing, 1: stopped, 2: paused)
	BOOL notifySuccess;				// Immediate Command Success Notifications
	BOOL notifyWait;				// Immediate Command Success Notifications
	MCIDEVICEID IDDevice;			// Values for mciSendCommandsA (MCIDEVICEID)
	UINT uMsg;						// Values for mciSendCommandsA (UINT)
	DWORD_PTR fdwCommand;			// Values for mciSendCommandsA (DWORD_PTR)
	DWORD_PTR dwParam;				// Values for mciSendCommandsA (DWORD_PTR)
	DWORD_PTR dwCallback;			// Callback HWND for Playback Notifications
}MM_STATUS_REQUEST;


// Mutex Handle
typedef struct {
	HANDLE sendCommand;				// SendCommand Mutex
	HANDLE toProcSendMsg;			// Send Mailslot Message Mutex for IPC Process
	HANDLE fromProcSendMsg;			// Receive Mailslot Message Mutex for IPC Process
	HANDLE procMutex;				// exe process mailslot mutex
}MCI_MUTEX;


// Server Handle
typedef struct {
	HANDLE startup;					// Startup handle
	HANDLE watchdogIPC;				// Check IPC Process handle
	HANDLE timeServer;				// Time server handle
	HANDLE receiveServer;			// IPC Receive server handle
}MCI_SERVER;


// Delayed Callback sturcts
typedef struct {
	UINT msg;
	DWORD dwMilliseconds;
}MCI_CALLBACK;


// Initialize variables
static INI_STATUS iniStatus{};					// winmm.ini Preference data struct
static MM_REQUEST modeRequest{};				// IPC Request struct
static MM_RESPONSE statusResponse{};			// IPC Response struct
static MM_STATUS currentStatus{					// Current status struct
	0, 0, 0, MCI_FORMAT_MSF,
	0, 0, 0, 0, 0, 0, 0, 0,
	0xFFFF, 0xFFFF, {0,} };
static MM_STATUS_REQUEST requestStatus{};		// Command message request struct
static MCI_MUTEX mciMutex{};					// Mutex handle struct
static MCI_SERVER mciServer{};					// MCI Server handle struct
static DWORD procIdentifier = 0;				// hWnd value
static DWORD magicDeviceID = 0xBEEF;			// Virtual Device ID Value
static DWORD isExeRunnung = 0;					// Check exe running
static unsigned char mciAliasCount = 2;			// Named Alias counter
static char mciNamedAlias[0x40][MAX_PATH]{		// Named cdaudio Alias array
	"all", "cdaudio", 0, 
};
CRITICAL_SECTION mciSendStringSession;			// SendString Critical Session
static char currentPath[MAX_PATH];				// Running current path
static char musicPath[MAX_PATH];				// Music folder
static char musicFileExt[8]{};					// sub log message for mciSend function
static DWORD playtimeDelay = 0x80;				// Estimated latency for mci play
static DWORD cdaudioDelay = 0x200;				// Estimated latency for real CD Audio device
static const DWORD globalDelayTime = 0x10;		// Estimated latency for IPC communication
static const DWORD globalTimeout = 0x800;


// Initialize IPC variables
static const char dllServerName[] = "winmmwrppr";		// DLL mailslot Server name
static const char procServerName[] = "cdaudioplr";		// EXE mailslot Server name
static const char mutexSignature[] = "Mutex";			// Mutex name signiture
static const char mailslotSignature[] = "Mailslot";		// Mailslot server name signiture
static char dllServer[MAX_PATH];
static char procServer[MAX_PATH];
static char dllMutex[MAX_PATH];
static char procMutex[MAX_PATH];
static char musicPlayerPath[MAX_PATH];


// for debug logging
FILE* fh = NULL;								// debugging log file
static BOOL debugOffOnce = 0;					// debug logging off once value (Prevent excessive logging)
static char debugMessage[0x20];					// debug value (Prevent excessive logging)
static char lastDebugMessage[0x20];				// Stored last debug value (Prevent excessive logging)
static BOOL isCDRequest;						// check cdaudio device reqeust (Prevent excessive logging)
static char msgDebug[0x40]{};					// main log message for mciSend function
static char msgDbgExtend[0x80]{};				// sub log message for mciSend function


// winmm wrapper function
MCIERROR WINAPI fake_mciSendCommandA(MCIDEVICEID IDDevice, UINT uMsg, DWORD_PTR fdwCommand, DWORD_PTR dwParam);
MCIERROR WINAPI fake_mciSendStringA(LPCSTR lpCommand, LPSTR lpReturnString, UINT cchReturn, HWND hwndCallback);
UINT WINAPI fake_auxGetNumDevs();
MMRESULT WINAPI fake_auxGetDevCapsA(UINT_PTR uDeviceID, LPAUXCAPSA lpCaps, UINT cbCaps);
MMRESULT WINAPI fake_auxGetVolume(UINT uDeviceID, LPDWORD lpdwVolume);
MMRESULT WINAPI fake_auxSetVolume(UINT uDeviceID, DWORD dwVolume);


// Notification and return functions
void WrapperInitialize();
void PlayNotifyServer();
void ReceiveIPCMessageServer();
HANDLE StartCDAudioProcess(LPSTR exeFileStart);
void WatchdogCDAudioProcess();
BOOL SendIPCMessage();
void DelayedCallback(UINT msg, DWORD dwMilliseconds);
void SendCommonNotify(UINT uMsg);
MCIERROR WINAPI returnSendCommand(int msgNum);

inline BOOL CloseHandleSafe(HANDLE& h) { HANDLE tmp = h; h = NULL; return tmp && tmp != INVALID_HANDLE_VALUE ? CloseHandle(tmp) : TRUE; }


// Named mciSendCommand
static const char mciCmds[][0x10] = {
	"MCI_OPEN", "MCI_CLOSE", "MCI_ESCAPE", "MCI_PLAY", "MCI_SEEK", "MCI_STOP", "MCI_PAUSE", "MCI_INFO", "MCI_GETDEVCAPS",
	"MCI_SPIN", "MCI_SET", "MCI_STEP", "MCI_RECORD", "MCI_SYSINFO", "MCI_BREAK", "MCI_SAVE", "MCI_STATUS",
	"MCI_CUE",
	"MCI_REALIZE", "MCI_WINDOW", "MCI_PUT", "MCI_WHERE", "MCI_FREEZE",
	"MCI_UNFREEZE", "MCI_LOAD", "MCI_CUT", "MCI_COPY", "MCI_PASTE", "MCI_UPDATE", "MCI_RESUME", "MCI_DELETE"
};
static const char mciOpenType[][0x20] = {
	"DEVTYPE_VCR", "DEVTYPE_VIDEODISC", "DEVTYPE_OVERLAY", "DEVTYPE_CD_AUDIO", "DEVTYPE_DAT", "DEVTYPE_SCANNER",
	"DEVTYPE_ANIMATION", "DEVTYPE_DIGITAL_VIDEO", "DEVTYPE_OTHER", "DEVTYPE_WAVEFORM_AUDIO", "DEVTYPE_SEQUENCER"
};
static const char mciTimeFormat[][0x20] = {
	"TIME_FORMAT_MILLISECONDS", "TIME_FORMAT_HMS", "TIME_FORMAT_MSF", "TIME_FORMAT_FRAMES", "TIME_FORMAT_SMPTE_24", "TIME_FORMAT_SMPTE_25",
	"TIME_FORMAT_SMPTE_30", "TIME_FORMAT_SMPTE_30DROP", "TIME_FORMAT_BYTES", "TIME_FORMAT_SAMPLES", "TIME_FORMAT_TMSF"
};
#endif