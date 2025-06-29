/*
    File: main.cpp
    Description: NEDitor main entry point
*/

#include "ned.h"
int main()
{
    Ned ned;
    if(!ned.initial
    ize())
    {
        return -1;
    }
    std::cout << "Starting NED..." << '\n';
    ned.run();
    return 0;
}

