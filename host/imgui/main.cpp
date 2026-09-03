/*
	File: main.cpp
	Description: NEDitor main entry point
*/
#include "host.h"

int main()
{
	AppHost ned;
	if (!ned.initialize())
		return -1;
	ned.run();
	return 0;
}
