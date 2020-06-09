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
#include <commctrl.h>
#include <tchar.h>
#include <stdlib.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Wintrust.lib")

// values returned by the program to indicate the status of the verification
#define VERIFICATION_STATUS_OK					0
#define VERIFICATION_STATUS_INVALID_SIGNATURE	1
#define VERIFICATION_STATUS_NO_SIGNATURE		2
#define VERIFICATION_STATUS_FILE_ACCESS_ERROR	3
#define VERIFICATION_STATUS_NO_PROVIDER			4
#define VERIFICATION_STATUS_UNKNOWN_ERROR		5

#define SIG_STATES_SIZE 100
#define SIG_DATA_ARRAY_NUM_SUB_ARRAYS 3 // Number of maximum available signatures in file, TODO: rewrite to support infinite number of sigs
#define SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS 18 // Number of data fields per one signature, including counter-signature TODO: rewrite to support separate array for counter-sigs
#define SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING 260
#define SIG_DATA_NUM_OF_COLUMNS 10
 
struct FileSigData {
	int totalNumberOfSignatures;
	int numSecondarySigs;
	bool primarySigExists;
	bool secondarySigExists;
	bool sigVerifyStates[SIG_STATES_SIZE];
	wchar_t*** sigDataArray;
};

// SigInfoBackEnd.cpp
bool ProcessData(_TCHAR* rawFilePath, struct FileSigData * fileSigData);
int VerifyAllSignatures(WINTRUST_DATA sWintrustData, GUID guidAction, bool* sigVerifyStates, DWORD* numSecondarySigs, bool* primarySigExists, bool* secondarySigExists);
int FillSigData(WINTRUST_DATA sWintrustData, bool sigVerifyStates[], DWORD sigNumber, wchar_t*** sigDataArray);
int VerifyPrimarySignature(WINTRUST_DATA sWintrustData, GUID guidAction, bool sigVerfyStates[]);
void VerifySecondarySignatures(DWORD numSecondarySigs, WINTRUST_DATA sWintrustData, GUID guidAction, bool sigVerifyStates[]);
void InitWintrustData(_TCHAR* fileName, WINTRUST_DATA* sWintrustData, WINTRUST_FILE_INFO* sWintrustFileInfo, WINTRUST_SIGNATURE_SETTINGS* SignatureSettings, DWORD dwFlags);
bool InitCryptProviderStructs(WINTRUST_DATA* sWintrustData, CRYPT_PROVIDER_DATA** psProvData, CRYPT_PROVIDER_SGNR** psProvSigner, CRYPT_PROVIDER_CERT** psProvCert);

//SigInfoGUIAux.cpp
wchar_t ** ParseListFile(wchar_t* fileName, int * numLines);
bool IsFileNameAbsolute(wchar_t* fileName);
bool FileIsListFile(wchar_t * fileName);
bool InitFileOpenDialog(HWND hwnd);
bool InitImportData(HWND hwnd);
bool InitFileAddAndCheckDialog(HWND hwndMain);
bool InitContextManu(HWND hwnd, WPARAM wParam);
bool InitMainDialog(HWND hwnd);
bool InitViewDialog(HWND hwnd, LPARAM lParam);
bool ListBoxCopy(HWND hwnd);
bool CheckLastAddedItem(HWND hwndMain, LVITEM lvItem);
bool CheckSelectedItem(HWND hwnd);
bool AddCheckedData(struct FileSigData* fileSigData, wchar_t * filePath, int itemNumber, HWND hwnd, LVITEM lvItem);
bool ResizeDialog(HWND hwnd);
void DeleteCharsInString(LPTSTR szThumbprint, int num, wchar_t ch);
bool IsOs64Bit(void);
bool DisableWow64RedirectionIfSystem32Bit(PVOID oldValue);
bool RevertWow64RedirectionIfPreviouslyDisabled(PVOID oldValue);
wchar_t * CutPathFromFile(wchar_t* path);

// SigInfoDataGetters.cpp
LPTSTR CreateSignatureTypeString(bool sigVerifyStates[], DWORD sigNumber, DWORD sigStatus);
LPTSTR CreateSigStatusString(bool sigStates[], DWORD sigNumber);
LPTSTR GetSignatureDate(CRYPT_PROVIDER_SGNR* psProvSigner);
LPTSTR GetSigSubjectOrIssuer(PCCERT_CONTEXT pCertCtx, DWORD dwFlags);
LPTSTR GetCertSerialNumber(PCCERT_CONTEXT pCertContext);
LPTSTR GetCertIssuerName(PCCERT_CONTEXT pCertContext);
LPTSTR GetCertThumbprint(PCCERT_CONTEXT pCertContext);
LPTSTR GetNotBeforeDate(PCCERT_CONTEXT pCertContext);
LPTSTR GetNotAfterDate(PCCERT_CONTEXT pCertContext);
LPTSTR GetTimestampDate(CRYPT_PROVIDER_SGNR* psProvSigner);
LPTSTR GetCertSubjectName(PCCERT_CONTEXT pCertContext);