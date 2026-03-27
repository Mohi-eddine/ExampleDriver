#pragma once

#include <stdarg.h>

#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include <wdm.h>
#include <ntimage.h>
#include <ntstrsafe.h>
#include <handleapi.h>

#include "TGOR_SDK.h"

#define DBG_BEGIN_STR "\n########################################################\n<<Begin>> ---> %s <---\n########################################################\n"
#define DBG_END_STR   "########################################################\n<<End>> ---> %s <---\n########################################################\n"

#define DBGPrint(...) KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL,__VA_ARGS__))
#define DbgTextStart(SectionName) DBGPrint(DBG_BEGIN_STR,SectionName)
#define DbgTextEnd(SectionName)	 DBGPrint(DBG_END_STR,SectionName)