/////////////////////////////////////////////
//                                         //
// Minimizing C++ Win32 App To System Tray //
//                                         //
// You found this at bobobobo's weblog,    //
// https://bobobobo.wordpress.com           //
//                                         //
// Creation date:  Mar 30/09               //
// Last modified:  Mar 30/09               //
//                                         //
/////////////////////////////////////////////

// GIVING CREDIT WHERE CREDIT IS DUE!!
// Thanks ubergeek!  http://www.gidforums.com/t-5815.html

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <thread>
#include "pedal.h"

#ifdef UNICODE
#define stringcopy wcscpy
#else
#define stringcopy strcpy
#endif

#define ID_TRAY_APP_ICON                5000
#define ID_TRAY_DETECT_CONTEXT_MENU_ITEM  3001
#define ID_TRAY_TOGGLE_CONTEXT_MENU_ITEM  3002
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define WM_TRAYICON ( WM_USER + 1 )


UINT WM_TASKBARCREATED = 0 ;

HWND g_hwnd ;
HMENU g_menu ;
HICON icon_on;
HICON icon_off;

NOTIFYICONDATA g_notifyIconData ;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

void InitNotifyIconData(){
	memset( &g_notifyIconData, 0, sizeof( NOTIFYICONDATA ) ) ;

	g_notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	g_notifyIconData.hWnd = g_hwnd;
	g_notifyIconData.uID = ID_TRAY_APP_ICON;
	g_notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_notifyIconData.uCallbackMessage = WM_TRAYICON;
	icon_on = (HICON)LoadImage( NULL, TEXT("icon_on.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE  ) ;
	icon_off = (HICON)LoadImage( NULL, TEXT("icon_off.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE  ) ;
	g_notifyIconData.hIcon = icon_on;
	stringcopy(g_notifyIconData.szTip, TEXT("Kubo pedal! - ON"));
}

void changeStatus(bool status){
	if(status){
		g_notifyIconData.hIcon = icon_on;
		stringcopy(g_notifyIconData.szTip, TEXT("Kubo pedal! - ON"));
	} else {
		g_notifyIconData.hIcon = icon_off;
		stringcopy(g_notifyIconData.szTip, TEXT("Kubo pedal! - OFF"));
	}
	Shell_NotifyIcon(NIM_MODIFY, &g_notifyIconData);
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR args, int iCmdShow){
	TCHAR className[] = TEXT( "kuba pedal" );

	WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated") ;

	// Console
	//AllocConsole();
	//AttachConsole( GetCurrentProcessId() ) ;
	//freopen( "CON", "w", stdout ) ;

	init();
	std::thread t1(pedal);
	//mute(true);

	WNDCLASSEX wnd = { 0 };

	wnd.hInstance = hInstance;
	wnd.lpszClassName = className;
	wnd.lpfnWndProc = WndProc;
	wnd.style = CS_HREDRAW | CS_VREDRAW ;
	wnd.cbSize = sizeof (WNDCLASSEX);

	wnd.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wnd.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
	wnd.hCursor = LoadCursor (NULL, IDC_ARROW);
	wnd.hbrBackground = (HBRUSH)COLOR_APPWORKSPACE ;

	if(!RegisterClassEx(&wnd)){
		FatalAppExit( 0, TEXT("Couldn't register window class!") );
	}

	g_hwnd = CreateWindowEx (
		0, className,
		TEXT( "Using the system tray" ),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		400, 400,
		NULL, NULL,
		hInstance, NULL
	);


	CreateWindow( TEXT("static"), TEXT("right click the system tray icon to close"), WS_CHILD | WS_VISIBLE | SS_CENTER,
                  0, 0, 400, 400, g_hwnd, 0, hInstance, NULL ) ;

	// Initialize the NOTIFYICONDATA structure once
	InitNotifyIconData();
	Shell_NotifyIcon(NIM_ADD, &g_notifyIconData);


	//ShowWindow (g_hwnd, iCmdShow);

	MSG msg ;
	while (GetMessage (&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}


	// Once you get the quit message, before exiting the app,
	// clean up and remove the tray icon
	if(!IsWindowVisible( g_hwnd )){
		Shell_NotifyIcon(NIM_DELETE, &g_notifyIconData);
	}

	uninit();
	exit(msg.wParam);

	return msg.wParam;
}


LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message){
		case WM_CREATE:
			g_menu = CreatePopupMenu();

			AppendMenu(g_menu, MF_STRING, ID_TRAY_DETECT_CONTEXT_MENU_ITEM,  TEXT( "Detect" ) );
			AppendMenu(g_menu, MF_STRING | ((toggleMute)? MF_CHECKED : MF_UNCHECKED), ID_TRAY_TOGGLE_CONTEXT_MENU_ITEM,  TEXT( "Toggle Mute" ) );
			AppendMenu(g_menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM,  TEXT( "Exit" ) );
			break;

		case WM_TRAYICON: {
			if (lParam == WM_RBUTTONDOWN){
				// Get current mouse position.
				POINT curPoint ;
				GetCursorPos(&curPoint) ;

				// should SetForegroundWindow according
				// to original poster so the popup shows on top
				SetForegroundWindow(hwnd);

				// TrackPopupMenu blocks the app until TrackPopupMenu returns
				printf("calling track\n");
				UINT clicked = TrackPopupMenu(
					g_menu,
					TPM_RETURNCMD | TPM_NONOTIFY,
					curPoint.x,
					curPoint.y,
					0,
					hwnd,
					NULL
				);

				if (clicked == ID_TRAY_EXIT_CONTEXT_MENU_ITEM){
					// quit the application.
					mute(false);
					PostQuitMessage( 0 ) ;
				} else if(clicked == ID_TRAY_DETECT_CONTEXT_MENU_ITEM){
					detect = true;
				} else if(clicked == ID_TRAY_TOGGLE_CONTEXT_MENU_ITEM){
					printf("toggle change");
					ModifyMenu(g_menu, ID_TRAY_TOGGLE_CONTEXT_MENU_ITEM, MF_BYCOMMAND | MF_STRING | ((toggleToggleMute())? MF_CHECKED : MF_UNCHECKED), 0, TEXT( "Toggle Mute" ));
				}
			}
		} break;

		case WM_DESTROY:
			printf( "DESTROY!!\n" ) ;
			PostQuitMessage (0);
			break;

	}

	return DefWindowProc( hwnd, message, wParam, lParam ) ;
}
