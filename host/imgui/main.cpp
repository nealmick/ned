/*
	File: main.cpp
	Description: NEDitor main entry point
*/
#include "ned.h"

int main()
{
	Ned ned;
	if (!ned.initialize())
		return -1;
	ned.run();
	return 0;
}
