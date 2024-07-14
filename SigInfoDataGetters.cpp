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

LPTSTR CreateSignatureTypeString(bool sigStates[], DWORD sigNumber, DWORD sigStatus) {

	DWORD strLength = MAX_PATH;
	LPTSTR sigTypeString = (LPTSTR)LocalAlloc(0, strLength * sizeof(TCHAR));
		
	if (sigStatus == 1 && sigNumber == 0)
		_tcscpy_s(sigTypeString, strLength, _T("Primary Signature"));
	else if (sigStatus == 1 && sigNumber != 0)
		_tcscpy_s(sigTypeString, strLength, _T("Secondary Signature"));
	else if (sigStatus == 0 && sigNumber == 0)
		_tcscpy_s(sigTypeString, strLength, _T("Primary Counter-Signature"));
	else
		_tcscpy_s(sigTypeString, strLength, _T("Secondary Counter-Signature"));
	/*
	if (sigStates[sigNumber])
		_tcscat_s(statusString, strLength, _T("Verified"));
	else
		_tcscat_s(statusString, strLength, _T("Not Verified"));
	*/
	return sigTypeString;
}

LPTSTR CreateSigStatusString(bool sigStates[], DWORD sigNumber) {

	DWORD strLength = MAX_PATH;
	LPTSTR statusString = (LPTSTR)LocalAlloc(0, strLength * sizeof(TCHAR));

	if (sigStates[sigNumber])
		_tcscpy_s(statusString, strLength, _T("Verified"));
	else
		_tcscpy_s(statusString, strLength, _T("Not Verified"));

	return statusString;
}
LPTSTR GetSignatureDate(CRYPT_PROVIDER_SGNR* psProvSigner) {

	FILETIME localFt;
	SYSTEMTIME sysTime;
	DWORD strLength = MAX_PATH;

	_TCHAR* workStrMiddle = (_TCHAR*)malloc(strLength* sizeof(TCHAR));
	_TCHAR* workStrFinal = (_TCHAR*)malloc(strLength* sizeof(TCHAR));

	LPTSTR sigDateRet = (LPTSTR)LocalAlloc(0, strLength * sizeof(TCHAR));

	if (FileTimeToLocalFileTime(&psProvSigner->sftVerifyAsOf, &localFt)
	  && FileTimeToSystemTime(&localFt, &sysTime)) {
	  _stprintf_s(sigDateRet, strLength, _T("%02u/%02u/%04u %02u:%02u:%02u"), sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
	}
	else {
	  _tcscpy_s(sigDateRet, strLength, _T("INVALID"));
	}
	free(workStrMiddle);
	free(workStrFinal);

	return sigDateRet;
}

LPTSTR GetSigSubjectOrIssuer(PCCERT_CONTEXT pCertCtx, DWORD dwFlags) {
	DWORD dwStrType;
	DWORD dwCount;
	DWORD dwType;
	LPTSTR szSubjectRDN = NULL;

	dwStrType = CERT_X500_NAME_STR;
	dwType = CERT_NAME_RDN_TYPE;
	//dwFlags = {0, 1}: 0 - Subject, 1 - Issuer

	dwCount = CertGetNameString(pCertCtx, dwType, dwFlags, &dwStrType, NULL, 0);

	if (dwCount) {
		szSubjectRDN = (LPTSTR)LocalAlloc(0, dwCount * sizeof(TCHAR));
		CertGetNameString(pCertCtx, dwType, dwFlags, &dwStrType, szSubjectRDN, dwCount);
	}

	return szSubjectRDN;
}

LPTSTR GetCertSerialNumber(PCCERT_CONTEXT pCertContext) {

	DWORD dwData = pCertContext->pCertInfo->SerialNumber.cbData;
	DWORD strLength = MAX_PATH;

	_TCHAR* workStrMiddle = (_TCHAR*)malloc(strLength * sizeof(TCHAR));
	_TCHAR* workStrFinal = (_TCHAR*)malloc(strLength * sizeof(TCHAR));

	LPTSTR certSNRet = NULL;

	for (DWORD n = 0; n < dwData; n++) {
		_stprintf_s(workStrMiddle, strLength, L"%02x", pCertContext->pCertInfo->SerialNumber.pbData[dwData - (n + 1)]);
		if (n == 0)
			_tcscpy_s(workStrFinal, strLength, workStrMiddle);
		else
			_tcscat_s(workStrFinal, strLength, workStrMiddle);
	}

	certSNRet = (LPTSTR)LocalAlloc(0, strLength * sizeof(TCHAR));

	_tcscpy_s(certSNRet, strLength, workStrFinal);

	free(workStrMiddle);
	free(workStrFinal);

	return certSNRet;
}

LPTSTR GetCertIssuerName(PCCERT_CONTEXT pCertContext) {

	DWORD dwData;
	LPTSTR szIssuerName = NULL;

	// 1. Get Issuer name size.
	if (!(dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, NULL, 0))) {
		_tprintf(_T("CertGetNameString failed.\n"));
	}

	// 2. Allocate memory for Issuer name.
	szIssuerName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
	if (!szIssuerName) {
		_tprintf(_T("Unable to allocate memory for Issuer Name.\n"));
	}

	// 3. Get Issuer name.
	if (!(CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, NULL, szIssuerName, dwData))) {
		_tprintf(_T("CertGetNameString failed.\n"));
	}

	return szIssuerName;
}

LPTSTR GetCertThumbprint(PCCERT_CONTEXT pCertContext) {

	BYTE* pvData = NULL;
	DWORD cbSize = 0, cbHash = 0, dest = 0;
	LPTSTR szThumbprint = NULL;

	// 1. Get Thumbprint size.
	if (!CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, NULL, &cbSize)) {
		_tprintf(_T("CertGetCertificateContextProperty failed with %x\n"), GetLastError());
	}

	// 2. Get Thumbprint data.
	pvData = (BYTE*)malloc(cbSize);
	cbHash = cbSize;
	if (!CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, pvData, &cbSize)) {
		_tprintf(_T("CertGetCertificateContextProperty failed with %x\n"), GetLastError());
	}

	// 3. Convert binary data to string.
	if (!CryptBinaryToString(pvData, cbHash, CRYPT_STRING_HEX, NULL, &dest)) {
		_tprintf(_T("CryptBinaryToString failed with %x\n"), GetLastError());
	}

	szThumbprint = (LPTSTR)LocalAlloc(0, (dest+1) * sizeof(TCHAR));

	if (!CryptBinaryToString(pvData, cbHash, CRYPT_STRING_HEX, szThumbprint, &dest)) {
		_tprintf(_T("CryptBinaryToString failed with %x\n"), GetLastError());
	}

	return szThumbprint;
}

LPTSTR GetNotBeforeDate(PCCERT_CONTEXT pCertContext) {

	FILETIME ftNBefore = pCertContext->pCertInfo->NotBefore;
	SYSTEMTIME stNBefore;

	DWORD strLength = MAX_PATH;

	_TCHAR* strNotBefore = (_TCHAR*)LocalAlloc(0, strLength * sizeof(TCHAR));

	if (FileTimeToSystemTime(&ftNBefore, &stNBefore)) {
	  _stprintf_s(strNotBefore, strLength, _T("%02u/%02u/%04u"), stNBefore.wDay, stNBefore.wMonth, stNBefore.wYear);
	}
	else {
	  _tcscpy_s(strNotBefore, strLength, _T("INVALID"));
	}

	return strNotBefore;
}

LPTSTR GetNotAfterDate(PCCERT_CONTEXT pCertContext) {

	FILETIME ftNAfter = pCertContext->pCertInfo->NotAfter;
	SYSTEMTIME stNAfter;

	DWORD strLength = MAX_PATH;

	_TCHAR* strNotAfter = (_TCHAR*)LocalAlloc(0, strLength * sizeof(TCHAR));

	if (FileTimeToSystemTime(&ftNAfter, &stNAfter)) {
		_stprintf_s(strNotAfter, strLength, _T("%02u/%02u/%04u"), stNAfter.wDay, stNAfter.wMonth, stNAfter.wYear);
	}
	else {
	  _tcscpy_s(strNotAfter, strLength, _T("INVALID"));
	}

	return strNotAfter;
}

LPTSTR GetTimestampDate(CRYPT_PROVIDER_SGNR* psProvSigner) {

  FILETIME localFt;
  SYSTEMTIME sysTime;
  DWORD strLength = MAX_PATH;

  LPTSTR tsDateRet = (LPTSTR)LocalAlloc(0, strLength * sizeof(TCHAR));

  if (FileTimeToLocalFileTime(&psProvSigner->pasCounterSigners[0].sftVerifyAsOf, &localFt)
	&& FileTimeToSystemTime(&localFt, &sysTime)) {
	_stprintf_s(tsDateRet, strLength, _T("%02u/%02u/%04u %02u:%02u:%02u"), sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

  }
  else {
	_tcscpy_s(tsDateRet, strLength, _T("INVALID"));
  }
  return tsDateRet;
}

LPTSTR GetCertSubjectName(PCCERT_CONTEXT pCertContext) {

	DWORD dwData;
	LPTSTR szSubjectName = NULL;

	// 1. Get Subject name size.
	if (!(dwData = CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0))) {
		_tprintf(_T("CertGetNameString failed.\n"));
	}

	// 2. Allocate memory for Subject name.
	szSubjectName = (LPTSTR)LocalAlloc(LPTR, dwData * sizeof(TCHAR));
	if (!szSubjectName) {
		_tprintf(_T("Unable to allocate memory for Issuer Name.\n"));
	}

	// 3. Get Subject name.
	if (!(CertGetNameString(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szSubjectName, dwData))) {
		_tprintf(_T("CertGetNameString failed.\n"));
	}

	return szSubjectName;
}