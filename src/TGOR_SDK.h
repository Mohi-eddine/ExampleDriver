#pragma once

//TGOR Structs 
#ifndef DRIVER_ENTRY_PARAMS_COUNT 
#define DRIVER_ENTRY_PARAMS_COUNT (10ul)
#endif

#define TGOR_SUCESS(Result) ((Result) >= 0)

typedef struct _declspec(align(8)) _PE_INFO
{
	PBYTE ImageBase;
	SIZE_T ImageSize;
	BOOLEAN IsNtHeadersSkipped;
}PE_INFO, * PPE_INFO;

typedef struct _declspec(align(8)) _DRIVER_ENTRY_PARAMS
{
	HANDLE ParentProcId;//if the caller is another process executing as parent , it should supply it own PID for driver entry to be able to access its memory space
	PPE_INFO PeInfo;
	WCHAR DriverName[MAX_PATH];
	ULONG64 Params[DRIVER_ENTRY_PARAMS_COUNT];
}DRIVER_ENTRY_PARAMS, * PDRIVER_ENTRY_PARAMS;