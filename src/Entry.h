#pragma once

#include "includes.h"

/////////////////////////////////////////////////////////////////
//macros
#define CTL_CODE_EXAMPLE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1270, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)

/////////////////////////////////////////////////////////////////
//global vars
extern PE_INFO MyDriverInfo;
extern PBYTE* MyDriverOriginalNtHeaders;//allocate memory and copy them from the PE_INFO passed by the mapper
extern PDRIVER_OBJECT MyDriverObject;

/////////////////////////////////////////////////////////////////
// functions

NTKERNELAPI
NTSTATUS
NTAPI
IoCreateDriver(
	PUNICODE_STRING DriverName,
	PDRIVER_INITIALIZE
	InitializationFunction);

inline
VOID
NTAPI
IoDeleteDriver(
	PDRIVER_OBJECT DriverObject)
{
	ObMakeTemporaryObject(DriverObject);
	ObDereferenceObject(DriverObject);
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS DispatchControl(PDEVICE_OBJECT DeviceObj, PIRP Irp);
VOID UnloadDriver(PDRIVER_OBJECT pDriverObj);

//this is the actual entry point of the driver , it will get called by the I/O manager after we create the driver object using IoCreateDriver inside _DriverEntry
NTSTATUS DriverEntry0(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING RegistryPath);
NTSTATUS _DriverEntry(PDRIVER_ENTRY_PARAMS UserParams, LPWSTR Name);

