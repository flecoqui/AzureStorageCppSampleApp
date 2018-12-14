// utils.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "utils.h"

extern "C" {
	UTILS_API int Multiply(int x, int y)
	{
		return x*y+1;
	}
}
