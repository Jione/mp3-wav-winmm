#include <windows.h>
#include <stdio.h>
#include "winmm_finder.h"
#include "winmm_stubs.h"

#define MCI_TMSF_TO_MILLISECS(param) (((int)MCI_TMSF_FRAME(param) * 40 / 3) % 1000) + ((int)MCI_TMSF_SECOND(param) * 1000) + ((int)MCI_TMSF_MINUTE(param) * 60000)
#define MCI_MSF_TO_MILLISECS(param) (((int)MCI_MSF_FRAME(param) * 40 / 3) % 1000) + ((int)MCI_MSF_SECOND(param) * 1000) + ((int)MCI_MSF_MINUTE(param) * 60000)

DWORD playtimeDelay = 0x80 * 2;

// WAVE Header struct
typedef struct {
    char riff[4];				// signature RIFF string
    DWORD overallSize;		    // overall size of file in bytes
    char wave[4];				// WAVE string
    char fmtChunkMarker[4];	    // fmt string with trailing null char
    DWORD lengthOfFmt;			// length of the format data
    WORD formatType;			// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
    WORD channels;			    // number of channels
    DWORD sampleRate;			// sampling rate (blocks per second)
    DWORD byterate;				// SampleRate * NumChannels * BitsPerSample/8
    WORD blockAlign;			// NumChannels * BitsPerSample/8
    WORD bitsPerSample;	        // bits per sample, 8- 8bits, 16- 16 bits etc
    char dataChunkHeader[4];	// DATA string or FLLR string
    DWORD dataSize;				// NumSamples * NumChannels *
} WAVEHEADER;

// MPEG Bitrate index table
static const DWORD idxBitrate[][3][16]{
    {   // v2.5
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},     // v2.5 Layer-1
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},          // v2.5 Layer-2
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}           // v2.5 Layer-3
    },
    {   // reserved version
        {0,},{0,},{0,}
    },
    {   // v2.0 (ISO/IEC 13818-3)
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0},     // v2.0 Layer-1
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0},          // v2.0 Layer-2
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0}           // v2.0 Layer-3
    },
    {   // v1.0 (ISO/IEC 11172-3)
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},  // v1.0 Layer-1
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},     // v1.0 Layer-2
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}       // v1.0 Layer-3
    }
};

// MPEG Frequency index table
static const DWORD idxFrequency[][4]{
    {11025, 12000, 8000, 0},    // v2.5
    {0,},                       // reserved version
    {22050, 24000, 16000, 0},   // v2.0
    {44100, 48000, 32000, 0}    // v1.0
};

// MPEG Samples per Frame / 8
static const DWORD idxCoefficients[][3] =
{
    {12000, 144000, 72000},   // v2.5 { Layer1, Layer2, Layer3 }
    {0, 0, 0},                // reserved version
    {12000, 144000, 72000},   // v2.0 { Layer1, Layer2, Layer3 }
    {12000, 144000, 144000},  // v1.0 { Layer1, Layer2, Layer3 }
};

DWORD GetWavePlaytime(LPCSTR waveFile) {
    WAVEHEADER waveHeader{};
    FILE* wavFp = NULL;
    DWORD totalLen = 0;
    wavFp = fopen(waveFile, "r");
    if (wavFp == NULL) { return 0; }
    fread(&waveHeader, 1, sizeof(WAVEHEADER), wavFp);
    fseek(wavFp, 0, SEEK_END);
    totalLen = ftell(wavFp) - sizeof(WAVEHEADER);
    fclose(wavFp);
    totalLen = waveHeader.dataSize / (((float)waveHeader.sampleRate / 8000.0) * (waveHeader.channels * waveHeader.bitsPerSample));
    if (totalLen > playtimeDelay) totalLen -= playtimeDelay;
    return totalLen;
}


DWORD GetMP3Playtime(LPCSTR mp3file) {
    FILE* mp3Fp;
    BYTE fileBuff[0x1000]{};
    DWORD idxVer, idxLay, idxBit, idxFreq, valBit, valFreq, valCffi ,paddLen, frameLen;
    DWORD totalLen, dwordBuff;
    int seekPos, buffPos;
    float durationTime;

    dwordBuff = 0;
    durationTime = 0;

    // File load
    mp3Fp = fopen(mp3file, "r");
    if (mp3Fp == NULL) { return 0; }

    // get total length
    seekPos = 0;
    buffPos = sizeof(fileBuff);
    fseek(mp3Fp, 0, SEEK_END);
    totalLen = ftell(mp3Fp);
    valCffi = valBit = idxFreq = idxVer = idxLay = frameLen = 0;
    while (seekPos < totalLen) {
        if (sizeof(fileBuff) <= buffPos) {
            fseek(mp3Fp, seekPos, SEEK_SET);
            if ((totalLen - seekPos) < sizeof(fileBuff)) {
                fread(fileBuff, sizeof(BYTE), (totalLen - seekPos), mp3Fp);
                seekPos = totalLen;
            }
            else {
                fread(fileBuff, sizeof(BYTE), sizeof(fileBuff), mp3Fp);
                seekPos += sizeof(fileBuff);
            }
            buffPos %= sizeof(fileBuff);
        }
        dwordBuff <<= 8;
        dwordBuff |= fileBuff[buffPos];

        // fourcc header filter
        if (((HIWORD(dwordBuff) & 0xFFE0) != 0xFFE0)
            || ((idxVer = (dwordBuff >> 19) & 3) == 1)          // version id
            || ((idxLay = 3 - ((dwordBuff >> 17) & 3)) == 0)    // layer type
            || ((idxBit = (dwordBuff >> 12) & 15) == 15)        // bitrate index
            || ((idxFreq = (dwordBuff >> 10) & 3) == 3)         // frequency index
           )
        {
            buffPos++;
            continue;
        }
        paddLen = (dwordBuff >> 9) & 1;                         // padding
        valBit = idxBitrate[idxVer][idxLay][idxBit];
        valFreq = idxFrequency[idxVer][idxFreq];
        valCffi = idxCoefficients[idxVer][idxLay];
        frameLen = (valBit * valCffi / valFreq) - 4;

        if (!HIWORD(frameLen)) {
            if (valBit) { durationTime += (float)((frameLen + 4) * 8) / (float)valBit; }
            frameLen += paddLen;
            buffPos += frameLen;
            dwordBuff = 0;
        }
    }
    fclose(mp3Fp);
    if (durationTime > (float)playtimeDelay) durationTime -= (float)playtimeDelay;
    return durationTime;
}


BOOL GetCDAudioPlaytime(LPDWORD trackCount, DWORD_PTR trackTimes[0x80]) {
    MCI_OPEN_PARMS mciOpenParams{};
    MCI_STATUS_PARMS mciStatusParams{};
    MCI_SET_PARMS mciSetParams{};
    MCI_GENERIC_PARMS mciGenericParams{};
    DWORD totalLen, mciDevID, mciTimeformat;
    BOOL resault = FALSE;
    mciOpenParams.lpstrDeviceType = "cdaudio";
    if (!relay_mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE | MCI_WAIT, (DWORD)(LPVOID)&mciOpenParams)) {
        mciDevID = mciOpenParams.wDeviceID;
        
        // Get timeformat
        mciStatusParams.dwItem = MCI_STATUS_TIME_FORMAT;
        relay_mciSendCommandA(mciDevID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParams);
        mciTimeformat = mciStatusParams.dwReturn;

        // Set TMSF timeformat
        mciSetParams.dwTimeFormat = MCI_FORMAT_TMSF;
        relay_mciSendCommandA(mciDevID, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (DWORD)(LPVOID)&mciSetParams);

        // Get Tracks
        mciStatusParams.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
        relay_mciSendCommandA(mciDevID, MCI_STATUS, MCI_STATUS_ITEM | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParams);
        if (mciStatusParams.dwReturn) {
            *trackCount = mciStatusParams.dwReturn;

            // Get first track length
            memset(&mciSetParams, 0, sizeof(MCI_SET_PARMS));
            mciSetParams.dwTimeFormat = MCI_FORMAT_MSF;
            relay_mciSendCommandA(mciDevID, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (DWORD)(LPVOID)&mciSetParams);

            memset(&mciStatusParams, 0, sizeof(MCI_STATUS_PARMS));
            mciStatusParams.dwItem = MCI_STATUS_LENGTH;
            mciStatusParams.dwTrack = 1;
            relay_mciSendCommandA(mciDevID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParams);
            totalLen = trackTimes[1] = MCI_MSF_TO_MILLISECS(mciStatusParams.dwReturn);

            memset(&mciSetParams, 0, sizeof(MCI_SET_PARMS));
            mciSetParams.dwTimeFormat = MCI_FORMAT_TMSF;
            relay_mciSendCommandA(mciDevID, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (DWORD)(LPVOID)&mciSetParams);

            // Get tracks length
            for (int i = 2; i <= *trackCount; i++) {
                memset(&mciStatusParams, 0, sizeof(MCI_STATUS_PARMS));
                mciStatusParams.dwItem = MCI_STATUS_LENGTH;
                mciStatusParams.dwTrack = i;
                relay_mciSendCommandA(mciDevID, MCI_STATUS, MCI_STATUS_ITEM | MCI_TRACK | MCI_WAIT, (DWORD)(LPVOID)&mciStatusParams);
                totalLen += trackTimes[i] = MCI_TMSF_TO_MILLISECS(mciStatusParams.dwReturn);
            }

            if (totalLen) {
                trackTimes[0] = totalLen;
                resault = TRUE;
            }
        }

        if (mciTimeformat != MCI_FORMAT_TMSF) {
            memset(&mciSetParams, 0, sizeof(MCI_SET_PARMS));
            mciSetParams.dwTimeFormat = mciTimeformat;
            relay_mciSendCommandA(mciDevID, MCI_SET, MCI_SET_TIME_FORMAT | MCI_WAIT, (DWORD)(LPVOID)&mciSetParams);
        }
        relay_mciSendCommandA(0, MCI_CLOSE, MCI_WAIT, (DWORD)(LPVOID)&mciGenericParams);
    }
    return resault;
}


void MusicFileFinder(LPCSTR folderPath, LPCSTR fileStartname, LPSTR fileExt, LPDWORD trackCount, DWORD_PTR trackTimes[0x80], char trackNames[0x80][0x20]) {
    WIN32_FIND_DATA fileInfo;
    HANDLE hFind = NULL;
    DWORD trackNo = 0;
    DWORD totalLen = 0;
    char searchExt[MAX_PATH]{};
    char fileName[MAX_PATH]{};

    fileExt[0] = 0;
    *trackCount = 0;

    snprintf(searchExt, MAX_PATH, "%s\\%s*.wav", folderPath, fileStartname);
    hFind = FindFirstFile(searchExt, &fileInfo);
    if ((hFind == INVALID_HANDLE_VALUE) || (hFind == NULL)) {
        snprintf(searchExt, MAX_PATH, "%s\\%s*.mp3", folderPath, fileStartname);
        hFind = FindFirstFile(searchExt, &fileInfo);
    }
    else {
        // WAV File found
        strcpy(fileExt, "wav");
    }

    if ((hFind == INVALID_HANDLE_VALUE) || (hFind == NULL)) {
        snprintf(searchExt, MAX_PATH, "%s\\%s*.mid", folderPath, fileStartname);
        hFind = FindFirstFile(searchExt, &fileInfo);
        if ((hFind == INVALID_HANDLE_VALUE) || (hFind == NULL)) {
            // Searching CD Audio device
            if (GetCDAudioPlaytime(trackCount, trackTimes)) {
                strcpy(fileExt, "cdaudio");
            }
            return;
        }
        // MIDI File found
        strcpy(fileExt, "mid");
        return;
    }
    else if (fileExt[0] == 0) {
        // MP3 File found
        strcpy(fileExt, "mp3");
    }

    snprintf(searchExt, MAX_PATH, "%s%s*%s", fileStartname, "%d", fileExt);
    do {
        if (fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        strcpy(fileName, fileInfo.cFileName);
        for (int i = 0; fileName[i]; i++) {
            fileName[i] = tolower(fileName[i]);
        }

        if ((sscanf(fileName, searchExt, &trackNo) == 1) && (trackNo > 0) && (trackNo < 0x80)) {
            snprintf(fileName, MAX_PATH, "%s\\%s", folderPath, fileInfo.cFileName);
            if (trackNo > *trackCount) { *trackCount = trackNo; }
            if (strncmp(fileExt, "wav", 3) == 0) {
                totalLen += trackTimes[trackNo] = GetWavePlaytime(fileName);
                strncpy(trackNames[trackNo], fileInfo.cFileName, 0x20);
            }
            else if (strncmp(fileExt, "mp3", 3) == 0) {
                totalLen += trackTimes[trackNo] = GetMP3Playtime(fileName);
                strncpy(trackNames[trackNo], fileInfo.cFileName, 0x20);
            }
        }
    } while (FindNextFile(hFind, &fileInfo) != 0);
    FindClose(hFind);
    if (totalLen) { trackTimes[0] = totalLen; }
}