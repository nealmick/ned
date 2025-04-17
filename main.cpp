/*
	File: main.cpp
	Description: NED text editor main entry point.
*/

#include "ned.h"
int main()
{
	Ned ned;

	if (!ned.initialize())
	{
		return -1;
	}
	// hello world t
	std::cout << "Starting NED..." << '\n';
	ned.run();
	return 0;
}
