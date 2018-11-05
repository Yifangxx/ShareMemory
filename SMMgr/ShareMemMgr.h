#pragma once

#include <windows.h>

#define SM_NAME "MgrShareMemory"
#define BUF_SIZE	2048
#define NAME_SIZE 256

#define RS_SUF	"_RS"	//Read Semaphore suffix
#define WS_SUF	"_WS"	//Write Semaphore suffix
#define NE_SUF	"_NE"	//Notify Event suffix

//Share Memory definition
const DWORD SM_CFM_CODE = 0x1111DBFF;	//Confirm code
enum  class TShareMemoryCommand			//Command code
{
	SM_CMD_NONE,
	SM_CMD_NORMAL
};

enum class TShareMemoryStatus
{
	READ ,
	WRITE,
	READWRITE
};

struct TShareMemoryHeader
{
	DWORD		FShareMemorySize;	//AllSize - 16
	DWORD		FWritePos;			//Address+4
	DWORD		FLastWritePos;		//Address+8
	DWORD		FReadPos;			//Address+12
};

struct TShareMemoryDataHeader
{
	DWORD		FConfirmCode;	//Confirm code
	DWORD		FCommandCode;	//Command code
	DWORD		FDataSize;		//Data Size
	BYTE		FReserved[20];	//Reserved
	BYTE		FData[1];		//Data Start Address
};
const int SM_DATAHEADER_SIZE = sizeof(TShareMemoryDataHeader) - 1;

class TShareMemItem
{
public:
	TShareMemItem(const char* shmName, int shmSize = BUF_SIZE);
	~TShareMemItem();

private:
	char				FShareMemoryName[NAME_SIZE];
	HANDLE				FShareMemoryHandle;	
	TShareMemoryHeader* FShareMemoryHeader;		//share memory header
	void*				FShareMemoryAddress;	//valid share memory start adress(offset header)

	HANDLE				FReadSemaphore;
	HANDLE				FWriteSemaphore;
	HANDLE				FNotifyEvent;	

public:
	bool	CheckValid();
	bool	WaitRead(DWORD dwWaitTime = INFINITE);
	bool	Write(void* src, int size, DWORD commandCode);
	bool	Read(void* des, int& size, DWORD& commandCode);
};

class TShareMemMgr
{
public:
	TShareMemMgr();
	~TShareMemMgr();

public:
	TShareMemItem*	CreateShm(const char* shmName);
	void			RemoveShm(TShareMemItem*& shm);
};

