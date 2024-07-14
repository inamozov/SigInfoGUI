#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include <stdio.h>
#include <windows.h>
#include <Softpub.h>
#include <Wincrypt.h>
#include <tchar.h>
#include <stdlib.h>
#include "SigInfo.h"
#include "resource.h"

#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Wintrust.lib")

typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

void DeleteCharsInString(LPTSTR szThumbprint, wchar_t ch) {
  int j = 0;
  for (int i = 0; i < _tcslen(szThumbprint); i++) {
	if (szThumbprint[i] != ch) {
	  szThumbprint[j++] = szThumbprint[i];
	}
  }
  szThumbprint[j] = 0;
}

bool IsOs64Bit(void) {

	BOOL bIsWow64 = FALSE;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process) {
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64)) {
			_tprintf(_T("IsWow64Process failed with error %x\n"), GetLastError());
		}
	}

	return bIsWow64;
}

bool DisableWow64RedirectionIfSystem32Bit(PVOID oldValue) {

	bool disabledOsRedir = false;

	if (Wow64DisableWow64FsRedirection(&oldValue))
		disabledOsRedir = true;

	return disabledOsRedir;
}

bool RevertWow64RedirectionIfPreviouslyDisabled(PVOID oldValue) {

	bool revertedOsRedir = false;

	if (Wow64RevertWow64FsRedirection(oldValue))
		revertedOsRedir = true;

	return revertedOsRedir;
}