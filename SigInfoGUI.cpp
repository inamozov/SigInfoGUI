#pragma comment(lib, "comctl32.lib")

#include <windows.h>
#include <commctrl.h>
#include "SigInfo.h"
#include "resource.h"

/*
  Summer - Fall 2019
  Author - Ilyas Namozov, i.namozov@list.ru
*/

static HWND hList = NULL;
LVCOLUMN lvCol;
LVITEM lvItem;
PVOID pResizeState = NULL;
HINSTANCE hInstGl;

PVOID oldValue = NULL;
bool redirectionDisabled = false;


INT_PTR CALLBACK ViewDlgProc(HWND hwndView, UINT Message, WPARAM wParam, LPARAM lParam) {

	switch (Message) {
		case WM_INITDIALOG:
			InitViewDialog(hwndView, lParam);
		break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_BUTTON_COPY:
					ListBoxCopy(hwndView);
				break;
				case IDC_BUTTON_CLOSE:
					EndDialog(hwndView, IDOK);
				break;
			}
		break;
		case WM_CLOSE:
			EndDialog(hwndView, 0);
			break;
		default:
			return FALSE;
	} //end switch (Message)
	return TRUE;
}

INT_PTR CALLBACK AboutDlgProc(HWND hwndAbout, UINT Message, WPARAM wParam, LPARAM lParam) {

	switch (Message) {
	case WM_INITDIALOG:
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON1:
			EndDialog(hwndAbout, IDOK);
			break;
		}
	default:
		return FALSE;
	}
	return TRUE;
}

INT_PTR CALLBACK DlgProc(HWND hwndMain, UINT Message, WPARAM wParam, LPARAM lParam) {

	switch (Message) {
		case WM_INITDIALOG:
			InitMainDialog(hwndMain);
		break;
		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR)lParam;

			switch (lpnm->code)
			{
				case LVN_GETINFOTIP:
				{
					LPNMLVGETINFOTIP pGetInfoTip = (LPNMLVGETINFOTIP)lParam;
				}
					break;
				default:
					break;
			}
		}
		break;
		case WM_SIZING:
			ResizeDialog(hwndMain);
		break;
		case WM_SIZE:
			switch (LOWORD(wParam)) {
				case SIZE_MAXIMIZED:
					ResizeDialog(hwndMain);
				break;
				case SIZE_RESTORED:
					ResizeDialog(hwndMain);
				break;
			}
		break;
		case WM_CONTEXTMENU:
			InitContextManu(hwndMain, wParam);
		break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				break;
				case ID_FILE_OPEN:
					InitFileOpenDialog(hwndMain);
				break;
				case ID_FILE_IMPORT:
					InitImportData(hwndMain);
				break;
				case ID_ADD_AND_CHECK:
					InitFileAddAndCheckDialog(hwndMain);
				break;
				case IDCHECK:
					CheckSelectedItem(hwndMain);		
				break;
				case IDR_MENU2_COPY: {
					// Get data from selected row
					int subItemIndex;
					int selectedRowIndex = 0;
					int stringFromColumnLength = MAX_PATH;
					wchar_t* stringFromColumn = (wchar_t*)malloc(stringFromColumnLength * sizeof(TCHAR));
					wchar_t** viewDataArray = (wchar_t**)malloc(SIG_DATA_NUM_OF_COLUMNS * sizeof(wchar_t*));
					int oneArrayOfDataLength = 0;
					int itemCount = SIG_DATA_NUM_OF_COLUMNS;
					wchar_t* oneArrayOfData; 

					selectedRowIndex = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);

					memset(&lvItem, 0, sizeof(lvItem));
					lvItem.mask = LVIF_TEXT;
					lvItem.cchTextMax = stringFromColumnLength;
					lvItem.iItem = selectedRowIndex;
					lvItem.pszText = stringFromColumn;

					for (subItemIndex = 0; subItemIndex < itemCount; subItemIndex++) {
						lvItem.iSubItem = subItemIndex;
						SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)& lvItem);
						viewDataArray[subItemIndex] = (wchar_t*)malloc((stringFromColumnLength + 1) * sizeof(wchar_t*));
						_tcscpy_s(viewDataArray[subItemIndex], stringFromColumnLength + 1, lvItem.pszText);
						oneArrayOfDataLength += stringFromColumnLength + 1;
					}
					free(stringFromColumn);
					oneArrayOfData	= (wchar_t*)malloc((oneArrayOfDataLength + 1) * sizeof(wchar_t*));
					 // copy data
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
				}
				break;
			case IDR_MENU2_CHECK: {
				CheckSelectedItem(hwndMain);
			}
			break;
			case IDVIEW: {
				int selectedItemIndex = SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);
				if (selectedItemIndex < 0) {
					MessageBox(hwndMain, L"Please select one item", L"Error", MB_OK);
				}
				else {
					//DialogBoxParam(hInstGl, MAKEINTRESOURCE(IDD_DIALOGBAR_VIEW), hwndMain, ViewDlgProc, (LPARAM)hwndMain);
					
					HWND hwndView = CreateDialogParam(hInstGl, MAKEINTRESOURCE(IDD_DIALOGBAR_VIEW), hwndMain, ViewDlgProc, (LPARAM)hwndMain);
					if (hwndView)
						ShowWindow(hwndView, SW_SHOW);
					else
						MessageBox(hwndMain, L"CeateDialog returned NULL", L"Error", MB_OK);
					
				}			
			}			
			break;
			case IDCLEAR:
				SendDlgItemMessage(hwndMain, IDC_LIST1, LVM_DELETEALLITEMS, 0, 0);
				break;
			case ID__ABOUT:
				DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGBAR), GetActiveWindow(), AboutDlgProc);
				break;
			case ID_FILE_EXIT:
				PostMessage(hwndMain, WM_CLOSE, 0, 0);
				break;
		}//end switch(LOWORD(wParam))
	break;
	case WM_CLOSE:		
		EndDialog(hwndMain, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	hInstGl = hInstance;

	return DialogBox(hInstGl, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
}