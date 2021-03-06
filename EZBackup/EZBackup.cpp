// playlistMerge.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <Windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <string>
#include <thread>

#include "EZBackup.h"



//------------------------------------------------------------------------------------------------
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_EZBACKUP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EZBACKUP));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
//------------------------------------------------------------------------------------------------
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EZBACKUP));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_EZBACKUP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
//------------------------------------------------------------------------------------------------
//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable
					   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	mainWindow = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, winWidth, winHeight, NULL, NULL, hInstance, NULL);
	if (!mainWindow)
	{
		return FALSE;
	}

	ShowWindow(mainWindow, nCmdShow);
	UpdateWindow(mainWindow);

	return TRUE;
}
//------------------------------------------------------------------------------------------------
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_CREATE:
		CreateWindow("BUTTON", "Destination", WS_VISIBLE | WS_CHILD, baseX, baseY, buttonW, buttonH, hWnd, (HMENU)(FILE_OPEN_1), NULL, 0);
		CreateWindow("BUTTON", "Source", WS_VISIBLE | WS_CHILD, baseX, baseY * 4, buttonW, buttonH, hWnd, (HMENU)(FILE_OPEN_2), NULL, 0);
		mergeButton = CreateWindow("BUTTON", "MERGE!!", WS_VISIBLE | WS_CHILD, baseX, baseY * 8, buttonW, buttonH, hWnd, (HMENU)(MERGE_FILES), NULL, 0);
		CreateWindow("Edit", destPath.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER, baseX + buttonW + 20, baseY, MAX_PATH * 8, 20, hWnd, (HMENU)FILE_PATH_1, NULL, NULL);
		CreateWindow("Edit", srcPath.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER, baseX + buttonW + 20, baseY * 4, MAX_PATH * 8, 20, hWnd, (HMENU)FILE_PATH_2, NULL, NULL);
		UpdateStatusMessage("waiting...");
		break;

	case WM_COMMAND:

		//do file open
		if (LOWORD(wParam) == FILE_OPEN_1)
		{
			//use file browser
			destPath = BrowseFolder("C:\\");
			//set the string to what the file broser gave us so we can see it on screen
			SetDlgItemText(hWnd, FILE_PATH_1, destPath.c_str());
		}
		if (LOWORD(wParam) == FILE_OPEN_2)
		{
			srcPath = BrowseFolder("C:\\");
			//set the string to what the file broser gave us so we can see it on screen
			SetDlgItemText(hWnd, FILE_PATH_2, srcPath.c_str());
		}
		if (LOWORD(wParam) == MERGE_FILES)
		{
			char buffer[MAX_PATH];
			//get the data in the input filed for master file
			GetDlgItemText(hWnd, FILE_PATH_1, buffer, MAX_PATH);
			//fill string with data in text box (incase they typed in a path)
			destPath = (char *)buffer;

			char buffer2[MAX_PATH];
			GetDlgItemText(hWnd, FILE_PATH_2, buffer2, MAX_PATH);
			//fill string with data in text box (incase they typed in a path)
			srcPath = (char *)buffer2;

			if (!doesRepoExist(destPath))
				createRepo(destPath, numJobs);
			else
			{	
				if(destPath[destPath.size()] != '\\')
					curRepoFile = (destPath + "\\"+REPO_NAME);
				else curRepoFile = (destPath + REPO_NAME);
				string output;
				
				openDBMultiThread(numJobs);

				if (!dbConnection[0].openDataBase(curRepoFile))
				{
					MessageBox(NULL, "coudldnt open your old repo file", "Finihsed", MB_OK);
					exit(-1);
				}
				
			}

			EnableWindow(mergeButton, false);

			thread doWork([]()
			{
				UpdateStatusMessage("started");
				vector<string> diskPAths;

				MyFileDirDll::clearDirTree();
				UpdateStatusMessage("find all files and dirs in " + srcPath);
				MyFileDirDll::addDirTree(srcPath, 10);
				MyFileDirDll::dumpTreeToVector("", diskPAths, false);

				checkRepo(diskPAths);

				char msg[30];
				sprintf(msg, "num dupes ignored: %d", numDupes);
				MessageBox(NULL, msg, "Finihsed", MB_OK);
			});
			doWork.detach();

		

		}

		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		//TextOutA(hdc,10,10,"hello world",11);
		TextOutA(hdc, baseX + buttonW + 20, baseY * 8, curMessage.c_str(), curMessage.size());
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
//------------------------------------------------------------------------------------------------
// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		break;
	}
	return (INT_PTR)FALSE;
}
