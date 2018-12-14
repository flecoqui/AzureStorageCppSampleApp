// tools.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "tools.h"

extern "C" {

	TOOLS_API int DoIt(int x)
	{
		return Multiply(x,x);
	}

}
