#include "stdafx.h"

#include <windows.h>

#include "ShareMemMgr.h"


TShareMemItem::TShareMemItem(const char* shmName, int shmSize /* = BUF_SIZE */)
	:FShareMemorySize(shmSize), FWritePos(0), FReadPos(0), FLastWritePos(0)
	,FNotifyEvent(NULL), FReadSemaphore(NULL), FWriteSemaphore(NULL)
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
	sprintf_s(name, "%s%s", shmName, WS_SUF);
	FNotifyEvent = CreateEvent(NULL, TRUE, FALSE, name);

	HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, FShareMemorySize, FShareMemoryName);

	if (hMapFile)
	{
		FShareMemoryHandle = hMapFile;
		FShareMemoryAddress = MapViewOfFile(FShareMemoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, FShareMemorySize);
	}
}

TShareMemItem::~TShareMemItem()
{
	CloseHandle(FNotifyEvent);
	CloseHandle(FWriteSemaphore);
	CloseHandle(FReadSemaphore);

	if (FShareMemoryAddress)
		UnmapViewOfFile(FShareMemoryAddress);
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

bool TShareMemItem::Read(void* des, int& size)
{
	auto ret = WaitForSingleObject(FReadSemaphore, INFINITE);
	switch (ret)
	{
	case WAIT_OBJECT_0:
		break;
	default:
		return false;
	}

	bool canRead = false;
	if (FLastWritePos > 0)
	{
		if (FLastWritePos - FReadPos > size)
		{
			canRead = true;
		}
		else
		{
			if (FWritePos > size)
			{
				FLastWritePos = 0;
				FReadPos = 0;
			}
		}
	}
	else
	{
		if (FWritePos - FReadPos > size)
		{
			canRead = true;
		}
	}

	if (canRead)
	{
		memcpy(des, static_cast<char*>(FShareMemoryAddress) + FReadPos, size);
		FReadPos += size;		
	}
	ReleaseSemaphore(FReadSemaphore, 1, NULL);
	return canRead;
}

int TShareMemItem::ReadTo(void* des, const char ch)
{
	int size = 0;

	return size;
}

bool TShareMemItem::Write(void* src, int size)
{
	auto ret = WaitForSingleObject(FWriteSemaphore, INFINITE);
	switch (ret)
	{
	case WAIT_OBJECT_0:
		break;
	default:
		return false;
	}

	bool canWrite = false;
	if (FLastWritePos != 0)
	{
		if (FReadPos - FWritePos > size)
			canWrite = true;
	}
	else
	{
		if (FShareMemorySize - FWritePos > size)
		{
			canWrite = true;
		}
		else
		{
			if (FReadPos > size)
			{
				FLastWritePos = FWritePos;
				FWritePos = 0;
				canWrite = true;
			}			
		}
	}

	if (canWrite)
	{
		memcpy(static_cast<char*>(FShareMemoryAddress) + FWritePos, src, size);
		FWritePos += size;
		SetEvent(FNotifyEvent);
	}
	ReleaseSemaphore(FWriteSemaphore, 1, NULL);
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
