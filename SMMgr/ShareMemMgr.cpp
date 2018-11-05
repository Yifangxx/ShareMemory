#include "stdafx.h"
#include <iostream>
using namespace std;
#include "ShareMemMgr.h"

//FShareMemorySize(shmSize), FWritePos(0), FReadPos(0), FLastWritePos(0)
TShareMemItem::TShareMemItem(const char* shmName, int shmSize /* = BUF_SIZE */)
	:FShareMemoryHeader(nullptr),
	FNotifyEvent(NULL), FReadSemaphore(NULL), FWriteSemaphore(NULL)
{
	strncpy_s(FShareMemoryName, shmName, strlen(shmName));

	char name[128] = { 0 };
	//ReadSemaphore
	sprintf_s(name, "%s%s", shmName, RS_SUF);
	FReadSemaphore = CreateSemaphore(NULL, 1, 1, name);
	//WriteSemaphore
	sprintf_s(name, "%s%s", shmName, WS_SUF);
	FWriteSemaphore = CreateSemaphore(NULL, 1, 1, name);
	//NotifyEvent
	sprintf_s(name, "%s%s", shmName, NE_SUF);
	FNotifyEvent = CreateEvent(NULL, FALSE, FALSE, name);

	bool existed = false;
	HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, shmSize, FShareMemoryName);
	auto err = GetLastError();
	if (err == ERROR_ALREADY_EXISTS)
	{
		existed = true;
	}
	//OpenFileMapping()
	if (hMapFile)
	{
		FShareMemoryHandle = hMapFile;
		auto startAddress = MapViewOfFile(FShareMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, shmSize);
		FShareMemoryHeader = static_cast<TShareMemoryHeader*>(startAddress);
		if (!existed)
		{
			ZeroMemory(FShareMemoryHeader, sizeof(TShareMemoryHeader));
			FShareMemoryHeader->FShareMemorySize = shmSize - sizeof(TShareMemoryHeader);
		}
		FShareMemoryAddress = static_cast<BYTE*>(startAddress) + sizeof(TShareMemoryHeader);
	}
}

TShareMemItem::~TShareMemItem()
{
	CloseHandle(FNotifyEvent);
	CloseHandle(FWriteSemaphore);
	CloseHandle(FReadSemaphore);

	if (FShareMemoryHeader)
	{
		UnmapViewOfFile((LPVOID)FShareMemoryHeader);
	}
// 	if (FShareMemoryAddress)
// 		UnmapViewOfFile(FShareMemoryAddress);
	if (FShareMemoryHandle)
		CloseHandle(FShareMemoryHandle);
}

bool TShareMemItem::CheckValid()
{
	if (FShareMemoryHandle && FShareMemoryAddress)
		return true;
	return false;
}

bool TShareMemItem::WaitRead(DWORD dwWaitTime)
{
	auto ret = WaitForSingleObject(FNotifyEvent, dwWaitTime);
	switch (ret)
	{
	case WAIT_OBJECT_0:
		return true;
	default:
		return false;
	}
}

bool TShareMemItem::Read(void* des, int& size, DWORD& commandCode)
{
	WaitForSingleObject(FReadSemaphore, INFINITE);

	bool canRead = false;
	if (FShareMemoryHeader->FLastWritePos > 0)
	{
		if (FShareMemoryHeader->FLastWritePos - FShareMemoryHeader->FReadPos >= size)
		{
			canRead = true;
		}
		else
		{
			if (FShareMemoryHeader->FWritePos >= size)
			{
				FShareMemoryHeader->FLastWritePos = 0;
				FShareMemoryHeader->FReadPos = 0;
			}
		}
	}
	else
	{
		if (FShareMemoryHeader->FWritePos - FShareMemoryHeader->FReadPos >= size)
		{
			canRead = true;
		}
	}

	if (canRead)
	{
		TShareMemoryDataHeader* dataHeader = (TShareMemoryDataHeader*)((char*)FShareMemoryAddress + FShareMemoryHeader->FReadPos);
		if (dataHeader->FConfirmCode != SM_CFM_CODE)
		{
			canRead = false;
		}
		else
		{
			commandCode = dataHeader->FCommandCode;
			size = dataHeader->FDataSize;
			memcpy(des, dataHeader->FData, size);
			FShareMemoryHeader->FReadPos += size + SM_DATAHEADER_SIZE;
		}
	}

	ReleaseSemaphore(FReadSemaphore, 1, NULL);
	return canRead;
}

bool TShareMemItem::Write(void* src, int size, DWORD commandCode)
{
	WaitForSingleObject(FWriteSemaphore, INFINITE);

	auto nodeSize = size + SM_DATAHEADER_SIZE;
	bool canWrite = false;
	if (FShareMemoryHeader->FLastWritePos != 0)
	{
		if (FShareMemoryHeader->FReadPos - FShareMemoryHeader->FWritePos > nodeSize)
			canWrite = true;
	}
	else
	{

		if (FShareMemoryHeader->FShareMemorySize - FShareMemoryHeader->FWritePos > nodeSize)
		{
			canWrite = true;
		}
		else
		{
			if (FShareMemoryHeader->FReadPos > nodeSize)
			{
				FShareMemoryHeader->FLastWritePos = FShareMemoryHeader->FWritePos;
				FShareMemoryHeader->FWritePos = 0;
				canWrite = true;
			}			
		}
	}

	if (canWrite)
	{
		TShareMemoryDataHeader* dataHeader = (TShareMemoryDataHeader*)((char*)FShareMemoryAddress + FShareMemoryHeader->FWritePos);
		dataHeader->FConfirmCode = SM_CFM_CODE;
		dataHeader->FCommandCode = commandCode;
		dataHeader->FDataSize = size;
		memcpy(dataHeader->FData, src, size);
		FShareMemoryHeader->FWritePos += nodeSize;
		cout << "Success to Write msg:" << (char*)src << endl;
	}
	ReleaseSemaphore(FWriteSemaphore, 1, NULL);
	SetEvent(FNotifyEvent);
	return canWrite;
}



TShareMemMgr::TShareMemMgr()
{
}


TShareMemMgr::~TShareMemMgr()
{
}

TShareMemItem* TShareMemMgr::CreateShm(const char* shmName)
{
	TShareMemItem* shm = new TShareMemItem(shmName);
	if (!shm->CheckValid())
	{
		RemoveShm(shm);
	}
	return shm;
}

void TShareMemMgr::RemoveShm(TShareMemItem*& shm)
{
	delete shm;
	shm = nullptr;
}
