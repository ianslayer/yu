#include "yu.h"
#include "renderer/renderer.h"
#include "core/system.h"

#include <stdio.h>

int main()
{
	yu::InitYu();


	MSG msg = {0};
	while( WM_QUIT != msg.message )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			//if( GetMessage( &msg, NULL, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		yu::SwapFrameBuffer();
	}

	return 0;
}