// SMMgr.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
using namespace std;

#include "ShareMemMgr.h"
int main()
{
	TShareMemMgr smMgr;
	auto sm = smMgr.CreateShm("ShareMemory");
	char msg[1024];
	while (sm->WaitRead())
	{
		int size = 64;
		DWORD command;
		while(sm->Read(msg, size, command))
			cout << msg;
	}
    return 0;
}

