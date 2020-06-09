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

bool ProcessData(_TCHAR* rawFilePath, struct FileSigData * fileSigData) {

	WINTRUST_DATA sWintrustData = {};
	WINTRUST_FILE_INFO sWintrustFileInfo = {};
	WINTRUST_SIGNATURE_SETTINGS SignatureSettings = {};
	GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	bool sigVerifyStates[SIG_STATES_SIZE] = { false };
	wchar_t*** sigDataArray; // inner function temporary array

	HRESULT hResult;
	DWORD numSecondarySigs = 0;
	int verifyStatus = 0, totalNumberOfMainSignatures = 0, totalNumberOfCounterSignatures = 0;
	bool redirectionDisabled = false, primarySigExists = false, secondarySigExists = false;
	bool retval = false;

	// 0. Get Proper file path by disabling OS redirection
	PVOID oldValue = NULL;
	if (IsOs64Bit())
		redirectionDisabled = DisableWow64RedirectionIfSystem32Bit(oldValue);

	// 1. Initialize Wintrust Data for further verification
	InitWintrustData(rawFilePath, &sWintrustData, &sWintrustFileInfo, &SignatureSettings, WSS_VERIFY_SPECIFIC | WSS_GET_SECONDARY_SIG_COUNT);

	// 2. Verify all Main Signatures in File and store result in sigVerifyStates
	totalNumberOfMainSignatures = VerifyAllSignatures(sWintrustData, guidAction, sigVerifyStates, &numSecondarySigs, &primarySigExists, &secondarySigExists);
	
	// 3. Initialize Signature Data Array according to number of Signatures in file 
	sigDataArray = (wchar_t***)malloc(SIG_DATA_ARRAY_NUM_SUB_ARRAYS * sizeof(wchar_t**));
	for (int i = 0; i < SIG_DATA_ARRAY_NUM_SUB_ARRAYS; ++i) {
		sigDataArray[i] = (wchar_t**)malloc(SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS * sizeof(wchar_t*));
		for (int j = 0; j < SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS; ++j) {
			sigDataArray[i][j] = (wchar_t*)malloc(SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING * sizeof(wchar_t));
			_tcscpy_s(sigDataArray[i][j], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, L"EMPTY");
		}
	}		
    
	// final array that goes to GUI part
	fileSigData->sigDataArray = (wchar_t***)malloc(SIG_DATA_ARRAY_NUM_SUB_ARRAYS * sizeof(wchar_t**));
	for (int i = 0; i < SIG_DATA_ARRAY_NUM_SUB_ARRAYS; ++i) {
		fileSigData->sigDataArray[i] = (wchar_t**)malloc(SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS * sizeof(wchar_t*));
		for (int j = 0; j < SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS; ++j) {
			fileSigData->sigDataArray[i][j] = (wchar_t*)malloc(SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING * sizeof(wchar_t));
		}
	}
	
	fileSigData->totalNumberOfSignatures = totalNumberOfMainSignatures;
	fileSigData->numSecondarySigs = numSecondarySigs;
	fileSigData->primarySigExists = primarySigExists;
	fileSigData->secondarySigExists = secondarySigExists;
	for (int i = 0; i < SIG_STATES_SIZE; i++)
		fileSigData->sigVerifyStates[i] = sigVerifyStates[i];	
	
	// 4. Get All File Signatures Data and store it in sigDataArray
	for (DWORD x = 0; x <= numSecondarySigs; x++) {
		// Clear previous state data from the last call to WinVerifyTrust
		sWintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
		hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);
		// Process and store signature data
		if (S_OK != hResult) {
			verifyStatus = VERIFICATION_STATUS_UNKNOWN_ERROR;
			retval = false;
		}			
		else {
			sWintrustData.hWVTStateData = NULL;
			// Reset dwStateAction as it may have been changed during the last call
			sWintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
			// Get to next signature
			sWintrustData.pSignatureSettings->dwIndex = x;
			hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);
			// Fill Signature Data
			totalNumberOfCounterSignatures += FillSigData(sWintrustData, sigVerifyStates, x, sigDataArray);
			retval = true;
		}
	}

	fileSigData->totalNumberOfSignatures += totalNumberOfCounterSignatures;
	
	for (int i = 0; i < SIG_DATA_ARRAY_NUM_SUB_ARRAYS; i++) {
		for (int j = 0; j < SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS; j++) {
			_tcscpy_s(fileSigData->sigDataArray[i][j], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, sigDataArray[i][j]);
		}
	}

	// Free Wintrust Data
	sWintrustData.dwUIChoice = WTD_UI_NONE;
	sWintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
	hResult = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &guidAction, &sWintrustData);
	if (hResult != S_OK)
		retval = false;

	//free workArray
	for (int i = 0; i < SIG_DATA_ARRAY_NUM_SUB_ARRAYS; i++) {
		for (int j = 0; j < SIG_DATA_ARRAY_NUM_SUB_SUB_ARRAYS; j++) {
			free(sigDataArray[i][j]);
		}
	}

	return retval;
}

int VerifyAllSignatures(WINTRUST_DATA sWintrustData, GUID guidAction, bool* sigVerifyStates, DWORD* numSecondarySigs, bool* primarySigExists, bool* secondarySigExists) {

	HRESULT hResult = S_OK;
	int totalNumberOfSignatures = 0;
	int verifyStatus = 0;

	// 1. Verify and Check Primary Signature for existance
	verifyStatus = VerifyPrimarySignature(sWintrustData, guidAction, sigVerifyStates);

	if (sigVerifyStates[0] == true)
		*primarySigExists = true;		
	
	if (verifyStatus != VERIFICATION_STATUS_NO_SIGNATURE)// signature exists but currupted
		*primarySigExists = true;
	
	// 2. Process Signatures Data
	if (*primarySigExists) {

		totalNumberOfSignatures++;

		// 2.1 Get number of secondary signatures
		*numSecondarySigs = sWintrustData.pSignatureSettings->cSecondarySigs;
		totalNumberOfSignatures += *numSecondarySigs;

		// 2.2 Verify all secondary signatures
		if (numSecondarySigs > 0) {
			*secondarySigExists = true;
			VerifySecondarySignatures(*numSecondarySigs, sWintrustData, guidAction, sigVerifyStates);
		}
	}

	// Clear previous state data from the last call to WinVerifyTrust
	sWintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
	hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);

	return totalNumberOfSignatures;
}

int FillSigData(WINTRUST_DATA sWintrustData, bool sigVerifyStates[], DWORD sigNumber, wchar_t*** sigDataArray) {

	CRYPT_PROVIDER_DATA* psProvData = NULL;
	CRYPT_PROVIDER_SGNR* psProvSigner = NULL;
	CRYPT_PROVIDER_CERT* psProvCert = NULL;
	LPTSTR errorString1 = (LPTSTR)malloc(MAX_PATH);
	LPTSTR errorString2 = (LPTSTR)malloc(MAX_PATH);
	DWORD counterSigners = 0;
	int counterSigNumber = 0;

	if (InitCryptProviderStructs(&sWintrustData, &psProvData, &psProvSigner, &psProvCert)) {
		// 1. Main signature
		// 1.0 Signature Types String
		LPTSTR SigTypeString = CreateSignatureTypeString(sigVerifyStates, sigNumber, 1);
		if (SigTypeString) {
			_tcscpy_s(sigDataArray[sigNumber][0], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, SigTypeString);
			LocalFree(SigTypeString);
		}	

		// 1.1 Signature Status String
		LPTSTR SigStatusString = CreateSigStatusString(sigVerifyStates, sigNumber);
		if (SigStatusString) {
			_tcscpy_s(sigDataArray[sigNumber][1], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, SigStatusString);
			LocalFree(SigStatusString);
		}
		
		// 1.2 Get Signature date
		LPTSTR szSigMainDate = GetSignatureDate(psProvSigner);
		if (szSigMainDate) {
			_tcscpy_s(sigDataArray[sigNumber][2], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szSigMainDate);
			LocalFree(szSigMainDate);
		}
		
		// 1.3 Get Certificate Subject
		LPTSTR szCertMainSubject = GetSigSubjectOrIssuer(psProvCert->pCert, 0);
		if (szCertMainSubject) {
			_tcscpy_s(sigDataArray[sigNumber][3], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szCertMainSubject);
			LocalFree(szCertMainSubject);
		}

		// 1.4 Serial Number
		LPTSTR szCertSerialNum = GetCertSerialNumber(psProvCert->pCert);
		if (szCertSerialNum) {
			_tcscpy_s(sigDataArray[sigNumber][4], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szCertSerialNum);
			LocalFree(szCertSerialNum);
		}

		// 1.5 Issuer
		LPTSTR szIssuerName = GetCertIssuerName(psProvCert->pCert);
		if (szIssuerName) {
			_tcscpy_s(sigDataArray[sigNumber][5], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szIssuerName);
			LocalFree(szIssuerName);
		}

		// 1.6 Thumbprint
		LPTSTR szThumbprint = GetCertThumbprint(psProvCert->pCert);
		if (szThumbprint) {
			DeleteCharsInString(szThumbprint, 0, _T(' '));
			DeleteCharsInString(szThumbprint, 0, _T('\r'));
			DeleteCharsInString(szThumbprint, 0, _T('\n'));
			_tcscpy_s(sigDataArray[sigNumber][6], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szThumbprint);
			LocalFree(szThumbprint);
		}

		// 1.7 Valid from
		LPTSTR szNotBefore = GetNotBeforeDate(psProvCert->pCert);
		if (szNotBefore) {
			_tcscpy_s(sigDataArray[sigNumber][7], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szNotBefore);
			LocalFree(szNotBefore);
		}
		
		// 1.8 Valid to
		LPTSTR szNotAfter = GetNotAfterDate(psProvCert->pCert);
		if (szNotAfter) {
			_tcscpy_s(sigDataArray[sigNumber][8], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szNotAfter);
			LocalFree(szNotAfter);
		}		

		// 2. Timestamp counter-signature
		counterSigners = psProvSigner->csCounterSigners;
		if (counterSigners) {
			counterSigNumber++;
			// 2.0 Status String
			LPTSTR sigTypeString = CreateSignatureTypeString(sigVerifyStates, sigNumber, 0);
			if (sigTypeString) {
				_tcscpy_s(sigDataArray[sigNumber][9], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, sigTypeString);
				LocalFree(sigTypeString);
			}		

			// 2.1 Signature Status String
			LPTSTR SigStatusString = CreateSigStatusString(sigVerifyStates, sigNumber);
			if (SigStatusString) {
				_tcscpy_s(sigDataArray[sigNumber][10], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, SigStatusString);
				LocalFree(SigStatusString);
			}

			// 2.2 Get Signature date
			LPTSTR szSigCounterDate = GetTimestampDate(psProvSigner);
			if (szSigCounterDate) {
				_tcscpy_s(sigDataArray[sigNumber][11], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szSigCounterDate);
				LocalFree(szSigCounterDate);
			}

			//		 We are now getting only first counter-signature due to pasCounterSigners[0].
			//		 We need to get all counter-signatures, passing arg to pasCounterSigners[arg] in cycle.
			//       Thus we need to create a function for getting all counter-signature data. 
			//       The question is if a file can have more than one counter-signature?

			psProvCert = WTHelperGetProvCertFromChain(&psProvSigner->pasCounterSigners[0], 0);
			if (psProvCert) {
				// 2.3 Print timestamp Subject
				LPTSTR szCertCounterSubject = GetSigSubjectOrIssuer(psProvCert->pCert, 0);
				if (szCertCounterSubject) {
					_tcscpy_s(sigDataArray[sigNumber][12], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szCertCounterSubject);
					LocalFree(szCertCounterSubject);
				}				

				// 2.4 Serial Number
				LPTSTR szCertSerialNum = GetCertSerialNumber(psProvCert->pCert);
				if (szCertSerialNum) {
					_tcscpy_s(sigDataArray[sigNumber][13], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szCertSerialNum);
					LocalFree(szCertSerialNum);
				}

				// 2.5 Issuer
				LPTSTR szIssuerName = GetCertIssuerName(psProvCert->pCert);
				if (szIssuerName) {
					_tcscpy_s(sigDataArray[sigNumber][14], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szIssuerName);
					LocalFree(szIssuerName);
				}

				// 2.6 Thumbprint
				LPTSTR szThumbprint = GetCertThumbprint(psProvCert->pCert);
				if (szThumbprint) {
					DeleteCharsInString(szThumbprint, 0, _T(' '));
					DeleteCharsInString(szThumbprint, 0, _T('\r'));
					DeleteCharsInString(szThumbprint, 0, _T('\n'));
					_tcscpy_s(sigDataArray[sigNumber][15], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szThumbprint);
					LocalFree(szThumbprint);
				}

				// 2.7 Valid from
				LPTSTR szNotBefore = GetNotBeforeDate(psProvCert->pCert);
				if (szNotBefore) {
					_tcscpy_s(sigDataArray[sigNumber][16], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szNotBefore);
					LocalFree(szNotBefore);
				}

				// 2.8 Valid to
				LPTSTR szNotAfter = GetNotAfterDate(psProvCert->pCert);
				if (szNotAfter) {
					_tcscpy_s(sigDataArray[sigNumber][17], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, szNotAfter);
					LocalFree(szNotAfter);
				}
			}
			else {
				_tcscpy_s(errorString1, MAX_PATH, L"WTHelperGetProvCertFromChain failed with ");
				_stprintf_s(errorString2, MAX_PATH, L"%x", GetLastError());
				_tcscat_s(errorString1, MAX_PATH, errorString2);
				_tcscat_s(errorString1, MAX_PATH, L"\n");
				_tcscpy_s(sigDataArray[sigNumber][10], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, errorString1);
				free(errorString1);
				free(errorString2);
			}
		}
		else {
			_tcscpy_s(errorString1, MAX_PATH, L"File has ");
			_stprintf_s(errorString2, MAX_PATH, L"%lu", counterSigners);
			_tcscat_s(errorString1, MAX_PATH, errorString2);
			_tcscat_s(errorString1, MAX_PATH, L" countersigners\n");
			_tcscpy_s(sigDataArray[sigNumber][8], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, errorString1);
			free(errorString1);
			free(errorString2);
		}
	}
	else {
		_tcscpy_s(errorString1, MAX_PATH, L"InitCryptProviderStructs failed with ");
		_stprintf_s(errorString2, MAX_PATH, L"%x", GetLastError());
		_tcscat_s(errorString1, MAX_PATH, errorString2);
		_tcscat_s(errorString1, MAX_PATH, L"\n");
		_tcscpy_s(sigDataArray[sigNumber][0], SIG_DATA_ARRAY_NUM_CHARS_IN_ONE_STRING, errorString1);
		free(errorString1);
		free(errorString2);
	}

	return counterSigNumber;
}

int VerifyPrimarySignature(WINTRUST_DATA sWintrustData, GUID guidAction, bool sigVerifyStates[]) {

	HRESULT hResult;
	int status;

	hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);
	
	if (hResult == TRUST_E_NOSIGNATURE)
		status = VERIFICATION_STATUS_NO_SIGNATURE;
	else if (hResult == CRYPT_E_FILE_ERROR)
		status = VERIFICATION_STATUS_FILE_ACCESS_ERROR;
	else if (hResult == TRUST_E_BAD_DIGEST)
		status = VERIFICATION_STATUS_INVALID_SIGNATURE;
	else if (hResult == TRUST_E_PROVIDER_UNKNOWN)
		status = VERIFICATION_STATUS_NO_PROVIDER;
	else if (hResult != S_OK) {
		status = VERIFICATION_STATUS_UNKNOWN_ERROR;
	}		
	else {
		status = VERIFICATION_STATUS_OK;
		sigVerifyStates[0] = true;
	}

	return status;
}

void VerifySecondarySignatures(DWORD numSecondarySigs, WINTRUST_DATA sWintrustData, GUID guidAction, bool sigVerifyStates[]) {

	HRESULT hResult;
	int status;

	for (DWORD x = 1; x <= sWintrustData.pSignatureSettings->cSecondarySigs; x++) {

		// Clear previous state data from the last call to WinVerifyTrust
		sWintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
		hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);

		if (S_OK != hResult)
			status = VERIFICATION_STATUS_UNKNOWN_ERROR;
		else {
			sWintrustData.hWVTStateData = NULL;

			// Reset dwStateAction as it may have been changed during the last call
			sWintrustData.dwStateAction = WTD_STATEACTION_VERIFY;
			sWintrustData.pSignatureSettings->dwIndex = x;

			hResult = WinVerifyTrust(NULL, &guidAction, &sWintrustData);

			if (S_OK != hResult)
				status = VERIFICATION_STATUS_UNKNOWN_ERROR;
			else {
				sigVerifyStates[x] = true;
			}
		}
	}
}

void InitWintrustData(_TCHAR* fileName, WINTRUST_DATA* sWintrustData, WINTRUST_FILE_INFO* sWintrustFileInfo, WINTRUST_SIGNATURE_SETTINGS* SignatureSettings, DWORD dwFlags) {

	sWintrustFileInfo->cbStruct = sizeof(WINTRUST_FILE_INFO);
	sWintrustFileInfo->pcwszFilePath = fileName;
	sWintrustFileInfo->hFile = NULL;

	sWintrustData->cbStruct = sizeof(WINTRUST_DATA);
	sWintrustData->dwUIChoice = WTD_UI_NONE;
	sWintrustData->fdwRevocationChecks = WTD_REVOKE_NONE;
	sWintrustData->dwUnionChoice = WTD_CHOICE_FILE;
	sWintrustData->pFile = sWintrustFileInfo;
	sWintrustData->dwStateAction = WTD_STATEACTION_VERIFY;

	SignatureSettings->cbStruct = sizeof(WINTRUST_SIGNATURE_SETTINGS);
	SignatureSettings->dwFlags = dwFlags;
	SignatureSettings->dwIndex = 0;

	sWintrustData->pSignatureSettings = SignatureSettings;
}

bool InitCryptProviderStructs(WINTRUST_DATA* sWintrustData, CRYPT_PROVIDER_DATA** psProvData, CRYPT_PROVIDER_SGNR** psProvSigner, CRYPT_PROVIDER_CERT** psProvCert) {

	bool retval = false;

	*psProvData = WTHelperProvDataFromStateData(sWintrustData->hWVTStateData);
	if (*psProvData) {
		*psProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA)* psProvData, 0, FALSE, 0);
		if (*psProvSigner) {
			*psProvCert = WTHelperGetProvCertFromChain(*psProvSigner, 0);
			if (*psProvCert) {
				retval = true;
			}
		}
	}

	return retval;
}