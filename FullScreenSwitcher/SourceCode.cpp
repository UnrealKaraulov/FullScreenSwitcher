#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define WINVER _WIN32_WINNT_WINXP
#define NTDDI_VERSION NTDDI_WINXP
#include <SDKDDKVer.h>
#include <windows.h>

using namespace std;

#define IsKeyPressed(key) (( GetAsyncKeyState( key ) >> 16 )==-1)

const long windowstyle = 0x6CF0000;
const long windowexstyle = 0x20040900;
const long fullscreenstyle = 0x96080000;
const long fullscreenexstyle = 0x40808;

HWND war3window = NULL;
BOOL unloadBOOL = FALSE;
BOOL fullscreen = FALSE;
RECT WindowRect;
WINDOWPLACEMENT wpc;

// Разрешение экрана
void GetDesktopResolution( int& horizontal , int& vertical )
{
	horizontal = GetSystemMetrics( SM_CXSCREEN );
	vertical = GetSystemMetrics( SM_CYSCREEN );
}


// Переключение между оконным и полноэкранным режимом
void ToggleFullscreen( )
{
	if ( fullscreen )
	{
		fullscreen = FALSE;
		SetWindowLong( war3window , GWL_EXSTYLE , windowexstyle );
		SetWindowLong( war3window , GWL_STYLE , windowstyle );
		ShowWindow( war3window , SW_SHOWNORMAL );
		SetWindowPlacement( war3window , &wpc );
		SetWindowPos( war3window , HWND_NOTOPMOST ,
					  WindowRect.left , WindowRect.top ,
					  ( WindowRect.right - WindowRect.left ) ,
					  ( WindowRect.bottom - WindowRect.top ) ,
					  SWP_SHOWWINDOW );
	}
	else
	{
		fullscreen = TRUE;
		GetWindowPlacement( war3window , &wpc );
		GetWindowRect( war3window , &WindowRect );
		int xs = 0;
		int ys = 0;

		GetDesktopResolution( xs , ys );
		SetWindowPos( war3window , HWND_TOPMOST ,
					  0 , 0 ,
					  xs ,
					  ys ,
					  SWP_SHOWWINDOW );

		SetWindowLong( war3window , GWL_EXSTYLE , fullscreenexstyle );
		SetWindowLong( war3window , GWL_STYLE , fullscreenstyle );
		ShowWindow( war3window , SW_SHOWMAXIMIZED );
	}

}

// проверить включен ли полноэкранный режим :)
BOOL IsFullScreen( HWND hwnd )
{
	RECT windowRect;
	GetWindowRect( hwnd , &windowRect );
	return windowRect.left == 0 && windowRect.top == 0 && windowRect.right == GetSystemMetrics( SM_CXSCREEN ) && windowRect.bottom == GetSystemMetrics( SM_CYSCREEN );
}

BOOL CALLBACK FindTopWindow( HWND hwnd , LPARAM lParam )
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId( hwnd , &lpdwProcessId );
	if ( lpdwProcessId == lParam )
	{
		war3window = hwnd;
		return FALSE;
	}
	return TRUE;
}

BOOL IsNotGame( int GameDll )
{
	return !( *( int* ) ( ( UINT32 ) GameDll + 0xACF678 ) > 0 || *( int* ) ( ( UINT32 ) GameDll + 0xAB62A4 ) > 0 );
}

unsigned long __stdcall WaitGameStartThread( void * a )
{
	while ( !war3window )
	{
		Sleep( 1000 );
	}

	int GameDll = (int)GetModuleHandle( "Game.dll" );

	if ( GameDll )
	{
		while ( true )
		{
			while ( IsNotGame( GameDll ) )
			{
				Sleep( 50 );
			}

			if ( war3window != GetForegroundWindow( ) )
			{
				ShowWindow( war3window , SW_RESTORE );
				SetWindowPos( war3window , HWND_TOPMOST , 0 , 0 , 0 , 0 , SWP_NOSIZE | SWP_NOMOVE );
				SetWindowPos( war3window , HWND_NOTOPMOST , 0 , 0 , 0 , 0 , SWP_NOSIZE | SWP_NOMOVE );
				SetForegroundWindow( war3window );
				SetFocus( war3window );
				SetActiveWindow( war3window );
			}


			while ( !IsNotGame( GameDll ) )
			{
				Sleep( 50 );
			}

		}
	}
	return 0;
}


unsigned long __stdcall WindowModuleThread( void * a )
{
	int maxwait = 80;

	// Ждать появления окна Warcraft III
	while ( TRUE  )
	{
		Sleep( 100 );
		war3window = FindWindow( "Warcraft III" , NULL );
		if ( !war3window )
		war3window = FindWindow( NULL , "Warcraft III" );
		if ( war3window )
			break;
		maxwait--;
		if ( maxwait < 0 )
		{
			// Тут по идее должен быть поиск окна если это не варкрафт :)
			EnumWindows( FindTopWindow , GetCurrentProcessId( ) );
			if ( !war3window )
			{
				MessageBox( 0 , "Warcraft III window not found!" , "ERROR" , MB_ICONSTOP );
				return 0;
			}
			break;
		}
	}

	// Если вар запущен в полноэкранном режиме то переключение не будет работать. (исправлю в следующей версии)
	if ( IsFullScreen( war3window ) )
	{
		ShowWindow( war3window , SW_MINIMIZE );
		MessageBox( 0 , "ERROR! Warcraft III running in FullScreen mode!\nEnable window mode!(включите оконный режим)." , "ERROR" , MB_ICONSTOP );
		return 0;
	}
	// Включить полноэкранный режим
	ToggleFullscreen( );

	

	
	while ( TRUE )
	{
		// Ждать нажатия ALT без ENTER
		while ( !IsKeyPressed( VK_RMENU ) || IsKeyPressed( VK_RETURN ) )
		{
			Sleep( 50 );
		}
		// Ждать нажатия ALT+ENTER
		while ( !IsKeyPressed( VK_RMENU ) || !IsKeyPressed( VK_RETURN ) )
		{
			Sleep( 50 );
		}
		// Если окно варкрафта вверху то переключить из полноэкранного в экранный
		if ( GetForegroundWindow( ) == war3window )
			ToggleFullscreen( );
		
		while ( IsKeyPressed( VK_RETURN ) )
		{
			Sleep( 100 );
		}
	}

	return 0;
}


HANDLE FullScreenTogglerThreadId = NULL;
HANDLE WaitGameThreadId = NULL;
BOOL WINAPI DllMain( HINSTANCE hiDLL , DWORD reason , LPVOID )
{
	switch ( reason )
	{
		case DLL_PROCESS_ATTACH:
			FullScreenTogglerThreadId = CreateThread( 0 , 0 , WindowModuleThread , 0 , 0 , 0 );
			WaitGameThreadId = CreateThread( 0 , 0 , WaitGameStartThread , 0 , 0 , 0 );
			break;
		case DLL_PROCESS_DETACH:
			TerminateThread( FullScreenTogglerThreadId , 0 );
			TerminateThread( WaitGameThreadId , 0 );
			break;
	}
	return TRUE;
}