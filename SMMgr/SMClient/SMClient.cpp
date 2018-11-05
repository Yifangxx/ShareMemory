// SMClient.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
using namespace std;
#include "ShareMemMgr.h"

int main()
{
	TShareMemMgr smMgr;
	auto sm = smMgr.CreateShm("ShareMemory");
	char msg[64];
	int count = 0;
	while (true)
	{
		//cout << "请输入：" << endl;
		//cin >> msg;
		sprintf_s(msg, "%d\n", count);
		//strcat_s(msg, "\n");
		//cout << msg;
		sm->Write(msg, 64, DWORD(TShareMemoryCommand::SM_CMD_NORMAL));
		count++;
		Sleep(200);
	}

    return 0;
}

