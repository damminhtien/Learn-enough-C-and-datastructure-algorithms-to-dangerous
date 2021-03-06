// TelnetServerUsingWSAAS.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TelnetServerUsingWSAAS.h"
#include "stdlib.h"
#include "stdio.h"
#include "winsock2.h"

#define MAX_LOADSTRING 100
#define WM_SOCKET WM_USER + 1

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
CRITICAL_SECTION CriticalSection;
SOCKADDR_IN clientAddr;
SOCKET listener;
SOCKET client;
SOCKET clients[64];
int numClients = 0;
char *addr[1024];
char clientsID[64][100];
int ret;
char buf[256];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

void deleteClient(int i) {
	clients[i] = clients[numClients - 1];
	numClients--;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(9000);

	bind(listener, (SOCKADDR *)&addr, sizeof(addr));
	listen(listener, 5);

	for (int i = 0; i < 64; i++) memcpy(clientsID[i], "", 1);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TELNETSERVERUSINGWSAAS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TELNETSERVERUSINGWSAAS));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TELNETSERVERUSINGWSAAS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TELNETSERVERUSINGWSAAS);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   WSAAsyncSelect(listener, hWnd, WM_SOCKET, FD_ACCEPT);

   CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
	   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL,
	   10, 10, 160, 350, hWnd, (HMENU)IDC_LIST_CLIENT, GetModuleHandle(NULL), NULL);

   CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
	   WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL,
	   180, 10, 400, 350, hWnd, (HMENU)IDC_LIST_MESSAGE, GetModuleHandle(NULL), NULL);

   CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("BUTTON"), TEXT("OK"),
	   WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
	   420, 360, 150, 40, hWnd, (HMENU)IDC_BUTTON_SEND, GetModuleHandle(NULL), NULL);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam))
		{
			closesocket((SOCKET)wParam);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		if (WSAGETSELECTEVENT(lParam) == FD_ACCEPT)
		{
			int clientAddrLen = sizeof(clientAddr);
			SOCKET client = accept((SOCKET)wParam, (SOCKADDR *)&clientAddr, &clientAddrLen);

			char buf[256];
			strcpy(buf, inet_ntoa(clientAddr.sin_addr));
			strcat(buf, ":");

			char bport[16];
			_itoa(clientAddr.sin_port, bport, 10);
			strcat(buf, bport);

			addr[numClients] = (char *)malloc(64);
			memcpy(addr[numClients], buf, strlen(buf) + 1);

			SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_ADDSTRING, 0, (LPARAM)buf);

			WSAAsyncSelect(client, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);

			clients[numClients] = client;
			numClients++;
		}
		else if (WSAGETSELECTEVENT(lParam) == FD_READ)
		{
			for (int i = 0; i < numClients; i++) {
				// if client didn't login
				if (!strcmp("", clientsID[i])) {
					ret = recv(clients[i], buf, sizeof(buf), 0);
					if (ret <= 0) {
						deleteClient(i);
						return -1;
					}
					buf[ret] = 0;
					char username[31];
					memcpy(username, &buf, ret - 1);
					username[ret - 1] = '\0';
					send(client, "Password: ", 11, 0);
					ret = recv(clients[i], buf, sizeof(buf), 0);
					if (ret <= 0)
						return -1;
					buf[ret] = 0;
					char password[31];
					memcpy(password, &buf, ret - 1);
					password[ret - 1] = '\0';
					char filename[] = "c:\\Users\\USER\\source\\repos\\DamMinhTien_20156599_Tuan8\\TelnetServerUsingWSAAS\\db.txt";
					//open and get the file handle
					InitializeCriticalSection(&CriticalSection);
					EnterCriticalSection(&CriticalSection);
					FILE* fh;
					fopen_s(&fh, filename, "r");
					//check if file exists
					if (fh == NULL) {
						printf("File does not exists");
						return 0;
					}
					//read line by line
					const size_t line_size = 255;
					char* line = (char*)malloc(line_size);
					while (fgets(line, line_size, fh) != NULL) {
						int len = strlen(line);
						for (int j = 0; j < len; j++) {
							if (line[j] == 32) {
								char c_username[31];
								memcpy(c_username, &line[0], j);
								c_username[j] = '\0';
								char c_password[31];
								memcpy(c_password, &line[j + 1], len - j - 2);
								c_password[len - j - 2] = '\0';
								if (strcmp(username, c_username) == 0 && strcmp(password, c_password) == 0) {
									memcpy(clientsID[i], username, strlen(username) + 1);
									break;
								}
							}
						}
					}
					// free(fh);
					free(line);
					LeaveCriticalSection(&CriticalSection);
					if (!strcmp("", clientsID[i])) {
						send(clients[i], "Login error, Username: ", 24, 0);
					}
					else {
						send(clients[i], "Command: ", 9, 0);
					}
				}
				else {
					// client logged in
					InitializeCriticalSection(&CriticalSection);
					EnterCriticalSection(&CriticalSection);
					ret = recv(client, buf, sizeof(buf), 0);
					if (ret <= 0)
						return -1;
					buf[ret - 1] = 0;
					strcat(buf, " > c:\\Users\\USER\\source\\repos\\DamMinhTien_20156599_Tuan8\\TelnetServerUsingWSAAS\\output.txt");
					printf("%s", buf);
					system(buf);
					send(clients[i], "Command: ", 9, 0);
					LeaveCriticalSection(&CriticalSection);
				}
			}
		}
	}
	break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
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
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

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
