#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "SigInfo.h"
#include "resource.h"

int workStringLength = MAX_PATH * sizeof(wchar_t*);

bool ListBoxCopy(HWND hwndView) {

	bool retval = false;
	int oneArrayOfDataLength = 0;
	wchar_t* oneArrayOfData;
	wchar_t** viewDataArray = (wchar_t**)malloc(SIG_DATA_NUM_OF_COLUMNS * sizeof(wchar_t*));
	for (int i = 0; i < SIG_DATA_NUM_OF_COLUMNS; i++) {
		viewDataArray[i] = (wchar_t*)malloc(workStringLength * sizeof(TCHAR));
	}

	int itemCount = SendDlgItemMessage(hwndView, IDC_LIST_VIEW, LB_GETCOUNT, 0, 0);

	if (itemCount != LB_ERR) {

		for (int i = 0; i < itemCount; i++) {
			SendDlgItemMessage(hwndView, IDC_LIST_VIEW, LB_GETTEXT, (WPARAM)i, (LPARAM)viewDataArray[i]);
			oneArrayOfDataLength += _tcslen(viewDataArray[i]) + 1;
		}

		oneArrayOfData = (wchar_t*)malloc((oneArrayOfDataLength + 1) * sizeof(wchar_t*));

		for (int i = 0; i < itemCount; i++) {
			if (i == 0) {
				_tcscpy_s(oneArrayOfData, oneArrayOfDataLength + 1, viewDataArray[i]);
				_tcscat_s(oneArrayOfData, oneArrayOfDataLength + 1, L"\n");
			}
			else {
				_tcscat_s(oneArrayOfData, oneArrayOfDataLength + 1, viewDataArray[i]);
				_tcscat_s(oneArrayOfData, oneArrayOfDataLength + 1, L"\n");
			}
		}

		const size_t len = (oneArrayOfDataLength + 1) * sizeof(wchar_t*);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		memcpy(GlobalLock(hMem), oneArrayOfData, len);
		GlobalUnlock(hMem);
		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, hMem);
		CloseClipboard();
		free(oneArrayOfData);
		retval = true;
	}

	for (int i = 0; i < SIG_DATA_NUM_OF_COLUMNS; i++) {
		free(viewDataArray[i]);
	}

	return retval;
}

bool InitViewDialog(HWND hwndView, LPARAM lParam) {// lParam = hwndMain

	bool retval = false;
	LVITEM lvItem;
	int subItemIndex;
	int selectedRowIndex = 0;
	int stringFromColumnLength = workStringLength;
	wchar_t* stringFromColumn = (wchar_t*)malloc(stringFromColumnLength * sizeof(TCHAR));
	wchar_t** viewDataArray = (wchar_t**)malloc(SIG_DATA_NUM_OF_COLUMNS * sizeof(wchar_t*));

	selectedRowIndex = SendDlgItemMessage((HWND)lParam, IDC_LIST1, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);

	memset(&lvItem, 0, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.cchTextMax = stringFromColumnLength;
	lvItem.iItem = selectedRowIndex;
	lvItem.pszText = stringFromColumn;

	for (subItemIndex = 0; subItemIndex < SIG_DATA_NUM_OF_COLUMNS; subItemIndex++) {
		lvItem.iSubItem = subItemIndex;
		SendDlgItemMessage((HWND)lParam, IDC_LIST1, LVM_GETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)& lvItem);
		viewDataArray[subItemIndex] = (wchar_t*)malloc((stringFromColumnLength + 1) * sizeof(wchar_t*));
		_tcscpy_s(viewDataArray[subItemIndex], stringFromColumnLength + 1, lvItem.pszText);
		SendDlgItemMessage(hwndView, IDC_LIST_VIEW, LB_ADDSTRING, 0, (LPARAM)viewDataArray[subItemIndex]);
	}

	free(stringFromColumn);

	for (int i = 0; i < SIG_DATA_NUM_OF_COLUMNS; i++) {
		free(viewDataArray[i]);
	}

	retval = true;

	return retval;
}

bool FileIsListFile(wchar_t* fileName) {

	bool retval = false;
	wchar_t* workFileName = CutPathFromFile(fileName);

	if (!_tcsncmp(workFileName, L"file_list", 9))
		retval = true;

	free(workFileName);

	return retval;
}

wchar_t** ParseListFile(wchar_t* fileName, int* numLines) {

	FILE* fp;
	wchar_t** fileNamesArray;
	unsigned int maxStringLength = MAX_PATH;
	short ch = 0;

	fp = _wfopen(fileName, L"r+t");

	while ((ch = fgetc(fp)) != EOF)
		if (ch == '\n') 
			(*numLines)++;

	fclose(fp);

	if (*numLines) {
		(*numLines)++;
		fp = _wfopen(fileName, L"r+t");
		fileNamesArray = (wchar_t**)malloc((*numLines) * sizeof(wchar_t*));
		
		for (int i = 0; i < (*numLines); i++) {
			fileNamesArray[i] = (wchar_t*)calloc(maxStringLength, sizeof(wchar_t));
			fgetws(fileNamesArray[i], maxStringLength, fp);
			if(i != ((*numLines) - 1))
				fileNamesArray[i][wcslen(fileNamesArray[i]) - 1] = '\0';
			else
				fileNamesArray[i][wcslen(fileNamesArray[i])] = '\0';
		}

		fclose(fp);
		return fileNamesArray;
	}
	else
		return NULL;
}

bool InitFileOpenDialog(HWND hwndMain) {

	bool retval = false;
	int index;
	LVITEM lvItem;
	wchar_t szFile[MAX_PATH * sizeof(wchar_t)];
	OPENFILENAME open;
	ZeroMemory(&open, sizeof(open));
	
	open.lStructSize = sizeof(open);
	open.hwndOwner = hwndMain;
	open.lpstrFile = szFile;
	open.lpstrFile[0] = '\0';
	open.nMaxFile = sizeof(szFile);
	open.lpstrTitle = L"Add...";
	open.lpstrFileTitle = NULL;
	open.lpstrInitialDir = L"C:\\Windows\\Sysnative\\drivers";
	open.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&open)) { // here we get a filename string which is open.lpstrFile

		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask = LVIF_TEXT;
		lvItem.cchTextMax = workStringLength;
		lvItem.iSubItem = 0;
		lvItem.pszText = (LPWSTR)malloc(lvItem.cchTextMax * sizeof(TCHAR));

		if (FileIsListFile(open.lpstrFile)) {
			int numLines = 0;
			wchar_t** fileList = ParseListFile(open.lpstrFile, &numLines);
			
			for (int i = 0; i < numLines; i++) {
				lvItem.iItem = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMCOUNT, 0, 0);
				_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileList[i]);
				SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);
			}
			free(fileList);
			retval = true;
		}
		else {
			lvItem.iItem = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMCOUNT, 0, 0);
			_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, open.lpstrFile); // here we copy filename string to listview struct 
			SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);
			free(lvItem.pszText);
			retval = true;
		}		
	}

	return retval;
}


bool InitContextManu(HWND hwndMain, WPARAM wParam) {

	bool retval = false;

	if ((HWND)wParam == GetDlgItem(hwndMain, IDC_LIST1)) {
		HMENU m_hMenu = CreatePopupMenu();
		POINT cursor;
		GetCursorPos(&cursor);
		AppendMenu(m_hMenu, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDR_MENU2_COPY, L"Copy");
		AppendMenu(m_hMenu, MF_BYCOMMAND | MF_STRING | MF_ENABLED, IDR_MENU2_CHECK, L"Check");
		TrackPopupMenu(m_hMenu, TPM_TOPALIGN | TPM_LEFTALIGN, cursor.x, cursor.y, 0, hwndMain, NULL);
		retval = true;
	}

	return retval;
}

bool InitMainDialog(HWND hwndMain) {

	LVITEM lvItem;
	LVCOLUMN lvCol;
	HWND hList = GetDlgItem(hwndMain, IDC_LIST1);
	bool retval = false;

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	SendMessage(hwndMain, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

	// Configure list view columns
	memset(&lvCol, 0, sizeof(lvCol));
	lvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvCol.cx = 0x69;
	lvCol.pszText = (LPWSTR)malloc(workStringLength*sizeof(WCHAR));

	_tcscpy_s(lvCol.pszText, workStringLength, L"Filename");
	SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Signature");
	SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Status");
	SendMessage(hList, LVM_INSERTCOLUMN, 2, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Sign Date");
	SendMessage(hList, LVM_INSERTCOLUMN, 3, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Subject");
	SendMessage(hList, LVM_INSERTCOLUMN, 4, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Serial");
	SendMessage(hList, LVM_INSERTCOLUMN, 5, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Issuer");
	SendMessage(hList, LVM_INSERTCOLUMN, 6, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Thumbprint");
	SendMessage(hList, LVM_INSERTCOLUMN, 7, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Valid from");
	SendMessage(hList, LVM_INSERTCOLUMN, 8, (LPARAM)& lvCol);
	_tcscpy_s(lvCol.pszText, workStringLength, L"Valid to");
	SendMessage(hList, LVM_INSERTCOLUMN, 9, (LPARAM)& lvCol);

	free(lvCol.pszText);

	// Show
	ShowWindow(hwndMain, SW_NORMAL);
	UpdateWindow(hwndMain);

	retval = true;
	
	return retval;
}

bool InitImportData(HWND hwndMain) {

	bool retval = false;
	int itemCount, numSubItems = 10;
	LVITEM lvItem;
	FILE* fp;
	wchar_t szFile[MAX_PATH * sizeof(wchar_t)];
	wcscpy(szFile, L"import.txt");
	OPENFILENAME save;
	ZeroMemory(&save, sizeof(save));
	wchar_t* saveString = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));

	save.lStructSize = sizeof(save);
	save.hwndOwner = hwndMain;
	save.lpstrFile = szFile;
	save.nMaxFile = sizeof(szFile);
	save.lpstrTitle = L"Import to file...";
	save.lpstrFilter = L"Text Files\0*.txt\0\0";
	save.lpstrFileTitle = NULL;
	save.lpstrInitialDir = L"C:\\";
	save.lpstrDefExt = L"txt";
	save.Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT;

	/*
		This is a simple algorithm
		In order to create an aligned import table, we need to determine the largest string in column to create an aligned import table
	*/
	if (GetSaveFileName(&save)) {

		fp = _wfopen(save.lpstrFile, L"w+t");

		itemCount = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMCOUNT, 0, 0);

		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask = LVIF_TEXT;
		lvItem.cchTextMax = workStringLength;
		lvItem.pszText = saveString;

		for (int i = 0; i < itemCount; i++) {
			lvItem.iItem = i;
			for (int j = 0; j < numSubItems; j++) {	
				lvItem.iSubItem = j;
				SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)& lvItem);

				if (wcslen(saveString) > 0) {

					fputws(saveString, fp);

					if(j == numSubItems - 1)
						fputws(L"\n", fp);
					else {
						lvItem.iSubItem = j + 1;
						SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)& lvItem);
						if(wcslen(saveString) > 0)
							fputws(L" | ", fp);
						else
							fputws(L"\n", fp);
					}						
				}				
			}
		}		

		fclose(fp);
	}

	return retval;
}

/* Add file and then check it */
bool InitFileAddAndCheckDialog(HWND hwndMain) {

	bool retval = false;
	LVITEM lvItem;
	wchar_t szFile[MAX_PATH * 1];
	OPENFILENAME open;
	ZeroMemory(&open, sizeof(open));

	open.lStructSize = sizeof(open);
	open.hwndOwner = hwndMain;
	open.lpstrFile = szFile;
	open.lpstrFile[0] = '\0';
	open.nMaxFile = MAX_PATH;
	open.lpstrTitle = L"Add and check...";
	open.lpstrFileTitle = NULL;
	open.lpstrInitialDir = L"C:\\Windows";
	open.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&open)) { // here we got a filename string which is open.lpstrFile

		memset(&lvItem, 0, sizeof(lvItem));
		lvItem.mask = LVIF_TEXT;
		lvItem.cchTextMax = workStringLength;
		lvItem.iSubItem = 0;
		lvItem.pszText = (LPWSTR)malloc(lvItem.cchTextMax*sizeof(WCHAR));

		if (FileIsListFile(open.lpstrFile)) {
			int numLines = 0;
			wchar_t** fileList = ParseListFile(open.lpstrFile, &numLines);

			for (int i = 0; i < numLines; i++) {
				lvItem.iItem = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMCOUNT, 0, 0);
				_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileList[i]);
				SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);
				if (IsFileNameAbsolute(fileList[i]))
					CheckLastAddedItem(hwndMain, lvItem);
				else {
					wchar_t* errorStr = (wchar_t*)malloc(MAX_PATH * sizeof(wchar_t));
					wcscpy(errorStr, fileList[i]);
					wcscat(errorStr, L" doesn't exist.");
					MessageBox(hwndMain, errorStr, L"Error", MB_OK);
				}
					
			}
			free(fileList);
			retval = true;
		}
		else {
			lvItem.iItem = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMCOUNT, 0, 0);
			_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, open.lpstrFile); 
			SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);
			CheckLastAddedItem(hwndMain, lvItem);
			retval = true;
		}

		free(lvItem.pszText);
	}

	return retval;
}

/* Check every last added item */
bool CheckLastAddedItem(HWND hwndMain, LVITEM lvItem) {

	HWND hList = GetDlgItem(hwndMain, IDC_LIST1);
	int fullCountBefore = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
	bool retval = false;
	FileSigData fileSigData;
	fileSigData.primarySigExists = false;
	fileSigData.secondarySigExists = false;

	if (ProcessData(lvItem.pszText, &fileSigData)) {
		if (AddCheckedData(&fileSigData, lvItem.pszText, lvItem.iItem, hwndMain, lvItem)) {
			int fullCountAfter = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
			if (fullCountBefore != fullCountAfter) {
				retval = true;
			}
			else {
				wchar_t* fileName = CutPathFromFile(lvItem.pszText);
				wchar_t* errorText = (wchar_t*)malloc(workStringLength * sizeof(TCHAR));
				_tcscpy_s(errorText, workStringLength, fileName);
				_tcscat_s(errorText, workStringLength, L" has no signatures.");
				MessageBox(hwndMain, errorText, L"Error", MB_OK);
				free(errorText);
				free(fileName);
			}
		}
		else
			MessageBox(hwndMain, L"Couldn't Add Checked Data", L"Error", MB_OK);
	}
	else
		MessageBox(hwndMain, L"Couldn't Process Data", L"Error", MB_OK);

	memset(&fileSigData, 0, sizeof(fileSigData));

	return retval;
}

bool IsFileNameAbsolute(wchar_t* fileName) {

	if (fileName[1] == 58 && fileName[2] == 92) 
		return true;
	else 
		return false;

}

bool CheckSelectedItem(HWND hwndMain) {

	LVITEM lvItem;
	HWND hList = GetDlgItem(hwndMain, IDC_LIST1);
	int fullCountBefore = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
	int selCount = SendMessage(hList, LVM_GETSELECTEDCOUNT, 0, 0);
	bool retval = false;

	if (selCount != LB_ERR) {
		if (selCount == 1) {

			int index = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);
			
			wchar_t* workString = (wchar_t*)malloc(workStringLength * sizeof(TCHAR));

			memset(&lvItem, 0, sizeof(lvItem));
			lvItem.mask = LVIF_TEXT;
			lvItem.cchTextMax = workStringLength;
			lvItem.iItem = index;
			lvItem.iSubItem = 0;
			lvItem.pszText = workString;

			SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)& lvItem);

			if (IsFileNameAbsolute(lvItem.pszText)) {
				FileSigData fileSigData;
				fileSigData.primarySigExists = false;
				fileSigData.secondarySigExists = false;

				if (ProcessData(lvItem.pszText, &fileSigData)) {
					if (AddCheckedData(&fileSigData, lvItem.pszText, index, hwndMain, lvItem)) {
						int fullCountAfter = SendMessage(hList, LVM_GETITEMCOUNT, 0, 0);
						if (fullCountBefore != fullCountAfter) {
							retval = true;
						}
						else {
							wchar_t* fileName = CutPathFromFile(lvItem.pszText);
							wchar_t* errorText = (wchar_t*)malloc(workStringLength * sizeof(TCHAR));
							_tcscpy_s(errorText, workStringLength, fileName);
							_tcscat_s(errorText, workStringLength, L" has no signatures.");
							MessageBox(hwndMain, errorText, L"Error", MB_OK);
							free(errorText);
							free(fileName);
						}
					}
					else
						MessageBox(hwndMain, L"Couldn't Add Checked Data", L"Error", MB_OK);
				}
				else
					MessageBox(hwndMain, L"Couldn't Process Data", L"Error", MB_OK);

				memset(&fileSigData, 0, sizeof(fileSigData));
			}
			else {
				MessageBox(hwndMain, L"Invalid filename", L"Error", MB_OK);
			}
			
		}
		else
			MessageBox(hwndMain, L"Please select one file", L"Error", MB_OK);
	}
	else
		MessageBox(hwndMain, L"Error getting selected count", L"Error", MB_OK);

	return retval;
}

bool AddCheckedData(struct FileSigData* fileSigData, wchar_t * filePath, int itemNumber, HWND hwnd, LVITEM lvItem) {

	int sigDataIndex = 0;
	int index = itemNumber;	
	int strSize = MAX_PATH;
	wchar_t* fileName = CutPathFromFile(filePath);

	if (fileSigData->primarySigExists) { //we have some signatures in file
		for (int i = 0; i < fileSigData->totalNumberOfSignatures/2; i++) {
			index++;

			memset(&lvItem, 0, sizeof(lvItem));
			lvItem.mask = LVIF_TEXT;
			lvItem.cchTextMax = strSize;
			lvItem.pszText = (LPWSTR)malloc(lvItem.cchTextMax * sizeof(wchar_t *));
			lvItem.iItem = index;

			// 1. Filename
			lvItem.iSubItem = 0;
			_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileName);
			SendDlgItemMessage(hwnd, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);
			// 2. All other data
			while (sigDataIndex < 9) {
				lvItem.iSubItem++;
				_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileSigData->sigDataArray[i][sigDataIndex]);
				SendDlgItemMessage(hwnd, IDC_LIST1, LVM_SETITEM, 0, (LPARAM)& lvItem);
				sigDataIndex++;
			}		
			
			// If counter-signature exists
			if (_tcscmp(fileSigData->sigDataArray[i][9], L"EMPTY") != 0) {
				// Counter-Signature
				index++;
				lvItem.iItem = index;

				// 1. Filename
				lvItem.iSubItem = 0;
				_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileName);
				SendDlgItemMessage(hwnd, IDC_LIST1, LVM_INSERTITEM, 0, (LPARAM)& lvItem);

				// 2. All other data 				
				while (sigDataIndex < 18) {
					lvItem.iSubItem++;
					_tcscpy_s(lvItem.pszText, lvItem.cchTextMax, fileSigData->sigDataArray[i][sigDataIndex]);
					SendDlgItemMessage(hwnd, IDC_LIST1, LVM_SETITEM, 0, (LPARAM)& lvItem);
					sigDataIndex++;
				}
			}
			sigDataIndex = 0;
			free(lvItem.pszText);
		}		
	}

	free(fileName);
	return TRUE;
}

bool ResizeDialog(HWND hwnd) {
	RECT A;
	RECT B;

	GetClientRect(hwnd, &A);
	SetWindowPos(GetDlgItem(hwnd, IDC_LIST1), 0, 14, 14, A.right - 28, A.bottom - 62, SWP_NOZORDER | SWP_NOACTIVATE);
	GetClientRect(GetDlgItem(hwnd, IDC_LIST1), &B);
	for (int i = 0; i < 9; i++)
		SendDlgItemMessage(hwnd, IDC_LIST1, LVM_SETCOLUMNWIDTH, (WPARAM)i, (LPARAM)B.right / 10);

	HWND btnCheck = GetDlgItem(hwnd, IDCHECK);
	GetClientRect(btnCheck, &B);
	SetWindowPos(btnCheck, 0, A.right - 118, A.bottom - 38, B.right, B.bottom, SWP_SHOWWINDOW);

	HWND btnView = GetDlgItem(hwnd, IDVIEW);
	GetClientRect(btnView, &B);
	SetWindowPos(btnView, 0, A.right - 224, A.bottom - 38, B.right, B.bottom, SWP_SHOWWINDOW);

	HWND btnClear = GetDlgItem(hwnd, IDCLEAR);
	GetClientRect(btnClear, &B);
	SetWindowPos(btnClear, 0, A.right - 330, A.bottom - 38, B.right, B.bottom, SWP_SHOWWINDOW);

	return true;
}

wchar_t* CutPathFromFile(wchar_t* path) {

    LPCTSTR fn=PathFindFileName(path);
	return _tcsdup(fn ? fn : _T(""));
}