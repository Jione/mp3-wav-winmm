#pragma once
#ifndef WINMM_WRAPPER_FINDER_H
#define WINMM_WRAPPER_FINDER_H

void MusicFileFinder(LPCSTR folderPath, LPCSTR fileStartname, LPSTR fileExt, LPDWORD trackCount, DWORD_PTR trackTimes[0x80], char trackNames[0x80][0x20]);

#endif