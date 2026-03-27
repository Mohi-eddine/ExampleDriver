#include "Entry.h"

/////////////////////////////////////////////////////////////////
//global vars
PE_INFO MyDriverInfo = { 0 };
PBYTE* MyDriverOriginalNtHeaders = NULL;//allocate memory and copy them from the PE_INFO passed by the mapper 
PDRIVER_OBJECT MyDriverObject = NULL;//will get set inside DriverEntry0 if created successfully

/////////////////////////////////////////////////////////////////
//static vars
static LPWSTR GlobalName = NULL;
static BOOLEAN IsDriverLoaded = FALSE;
static PDRIVER_DISPATCH IopInvalidDeviceRequest = NULL;

/////////////////////////////////////////////////////////////////
//functions

NTSTATUS DispatchCreate(PDEVICE_OBJECT device_obj, PIRP irp)
{
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0LL;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT device_obj, PIRP irp) 
{
	UNREFERENCED_PARAMETER(device_obj);
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0LL;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DispatchControl(PDEVICE_OBJECT DeviceObj, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObj);
	DbgTextStart("DispatchControl");

	NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
	PIO_STACK_LOCATION CurrentIrpStack = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = sizeof(ULONG64); //an example size , the actual size will depend on the request and the caller should check it before trying to read the data
	ULONG64* pCallData = (ULONG64*)Irp->AssociatedIrp.SystemBuffer;

	volatile ULONG CtlCode = 0;

	do
	{
		if (MyDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] != DispatchControl)
		{
			MyDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;
			break;
		}

		if (!pCallData)
		{
			DBGPrint("[-] DispatchControl-> (NTSTATUS: 0x%X) Error: pCallData is NULL.\n", STATUS_INVALID_PARAMETER);
			NtStatus = STATUS_INVALID_PARAMETER;
			break;
		}

		CtlCode = CurrentIrpStack->Parameters.DeviceIoControl.IoControlCode;
		switch (CtlCode)
		{
		case CTL_CODE_EXAMPLE:
		{
			/*
			* Do something with the data and prepare the response by filling the appropriate fields in the structure pointed by pCallData
			*/
			NtStatus = STATUS_SUCCESS;
			break;
		}
		default:
			break;
		}
	} while (FALSE);

	Irp->IoStatus.Status = NtStatus;
	if (NT_SUCCESS(NtStatus))
	{
		DBGPrint("[+] DispatchControl-> IOCTL Finished Successfully.\n");
	}
	else
	{
		DBGPrint("[+] DispatchControl-> (NTSTATUS: %X) IOCTL Finished With A Failure.\n", NtStatus);
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	DbgTextEnd("DispatchControl");

	return NtStatus;
}

VOID UnloadDriver(PDRIVER_OBJECT pDriverObj)
{
	DbgTextStart("UnloadDriver");
	
	for (int FunctionIdx = 0; FunctionIdx <= IRP_MJ_MAXIMUM_FUNCTION; FunctionIdx++) //set all MajorFunction's to IopInvalidDeviceRequest
	{
		pDriverObj->MajorFunction[FunctionIdx] = IopInvalidDeviceRequest;
	}
	if (MyDriverOriginalNtHeaders)
	{
		ExFreePool(MyDriverOriginalNtHeaders);
		MyDriverOriginalNtHeaders = NULL;
	}
	pDriverObj->DriverStart = NULL;
	pDriverObj->DriverSize = 0;

	IoDeleteDevice(pDriverObj->DeviceObject);
	IoDeleteDriver(pDriverObj);//dereference the driver object
	pDriverObj->Flags |= 1;

	DbgTextEnd("UnloadDriver");
}

//this is the actual entry point of the driver , it will get called by the I/O manager after we create the driver object using IoCreateDriver inside _DriverEntry
NTSTATUS DriverEntry0(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DbgTextStart("DriverEntry0");
	MyDriverObject = pDriverObj;
	//fill the Driver Start and Size so it can be used later when unloading the driver
	pDriverObj->DriverStart = MyDriverInfo.ImageBase;
	pDriverObj->DriverSize = (ULONG)MyDriverInfo.ImageSize;

	WCHAR _DeviceName[MAX_PATH] = { 0 };
	wcscpy_s(_DeviceName, MAX_PATH, L"\\Device\\");
	wcscat_s(_DeviceName, MAX_PATH, GlobalName);

	DBGPrint("[+] DriverEntry0 -> DeviceName: %ls\n", _DeviceName);

	UNICODE_STRING DeviceName;
	RtlInitUnicodeString(&DeviceName, _DeviceName);
	PDEVICE_OBJECT pDeviceObj = NULL;
	NTSTATUS NtStatus = IoCreateDevice(pDriverObj, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObj);
	if (!NT_SUCCESS(NtStatus))
	{
		DBGPrint("[-] DriverEntry0 -> (NTSTATUS: 0x%X) Failed To Create Driver Device.\n", NtStatus);
		DbgTextEnd("DriverEntry0");
		return NtStatus;
	}
	DBGPrint("[+] DriverEntry0 -> pDeviceObj: %p.\n", pDeviceObj);;
	DBGPrint("[+] DriverEntry0 -> Successfully Created Driver Device.\n");
	/*
		no more symbolic links use \\Device\\GlobalName to access the driver from usermode.
	*/
	SetFlag(pDeviceObj->Flags, DO_BUFFERED_IO); //set DO_BUFFERED_IO bit to 1
	if (!IopInvalidDeviceRequest)
	{
		IopInvalidDeviceRequest = pDriverObj->MajorFunction[IRP_MJ_CREATE];//by default IoCreateDriver sets all MajorFunction's to IopInvalidDeviceRequest
	}

	//then set supported functions to appropriate handlers
	pDriverObj->MajorFunction[IRP_MJ_CREATE] = DispatchCreate; //link our io create function
	pDriverObj->MajorFunction[IRP_MJ_CLOSE] = DispatchClose; //link our io close function
	pDriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl; //link our control code handler
	pDriverObj->DriverUnload = UnloadDriver;
	ClearFlag(pDeviceObj->Flags, DO_DEVICE_INITIALIZING); //set DO_DEVICE_INITIALIZING bit to 0 (we are done initializing)

	IsDriverLoaded = NT_SUCCESS(NtStatus);
	DbgTextEnd("DriverEntry0");
	return NtStatus;
}

NTSTATUS _DriverEntry(PDRIVER_ENTRY_PARAMS UserParams, LPWSTR Name)
{
	NTSTATUS NtResult = STATUS_UNSUCCESSFUL;
	DbgTextStart("DriverEntry");
	if (!UserParams || !UserParams->PeInfo)
	{
		DBGPrint("[-] DriverEntry -> No User Params were provided.\n");
		DbgTextEnd("DriverEntry");
		return STATUS_UNSUCCESSFUL;
	}

	if (!Name || !wcslen(Name))
	{
		DBGPrint("[-] DriverEntry -> No Driver Name was provided.\n");
		DbgTextEnd("DriverEntry");
		return STATUS_UNSUCCESSFUL;
	}

	memcpy(&MyDriverInfo, UserParams->PeInfo, sizeof(PE_INFO));
	if (!UserParams->PeInfo->IsNtHeadersSkipped)
	{
		MyDriverOriginalNtHeaders = (PBYTE*)ExAllocatePool2(POOL_FLAG_NON_PAGED, PAGE_SIZE, 'AUPE');
		if (!MyDriverOriginalNtHeaders)
		{
			DBGPrint("[-] DriverEntry -> Failed To Allocate Memory For Driver Original Nt Headers.\n");
			DbgTextEnd("DriverEntry");
			return STATUS_MEMORY_NOT_ALLOCATED;
		}
		PVOID ImageBase = UserParams->PeInfo->ImageBase;
		if (!ImageBase)
		{
			ExFreePool(MyDriverOriginalNtHeaders);
			MyDriverOriginalNtHeaders = NULL;
			DBGPrint("[-] DriverEntry -> The Supplied Driver Image Base Is Null.\n");
			DbgTextEnd("DriverEntry");
			return STATUS_UNSUCCESSFUL;
		}
		memcpy(MyDriverOriginalNtHeaders, ImageBase, PAGE_SIZE);
		memset(ImageBase, 0, PAGE_SIZE);//clear the current nt headers
	}

	PEPROCESS ParentProc = NULL;
	KAPC_STATE ApcState;
	HANDLE ParentProcId = UserParams->ParentProcId;
	if (ParentProcId && ParentProcId != INVALID_HANDLE_VALUE)
	{
		NtResult = PsLookupProcessByProcessId(ParentProcId, &ParentProc);
		if (!NT_SUCCESS(NtResult))
		{
			if (MyDriverOriginalNtHeaders)
			{
				ExFreePool(MyDriverOriginalNtHeaders);
				MyDriverOriginalNtHeaders = NULL;
			}
			DBGPrint("[-] DriverEntry -> Failed To Get Parent Process.\n");
			DbgTextEnd("DriverEntry");
			return STATUS_UNSUCCESSFUL;
		}
		ObDereferenceObject(ParentProc);
		//attach to the parent proc
		KeStackAttachProcess(ParentProc, &ApcState);
	}

	do
	{
		//access any other params provided by the caller through UserParams->Params array if needed here 
		//the driver name can also be found at UserParams->DriverName;
		GlobalName = Name;
		WCHAR _DriverName[MAX_PATH] = { 0 };
		wcscpy_s(_DriverName, MAX_PATH, L"\\Driver\\");
		if(wcscat_s(_DriverName, MAX_PATH, Name) != 0)
		{
			DBGPrint("[-] DriverEntry -> Failed To Construct The Driver Name.\n");
			break;
		}
	
		DBGPrint("[+] DriverEntry -> DriverName: %ls\n", _DriverName);

		UNICODE_STRING DriverName = {0};
		RtlInitUnicodeString(&DriverName, _DriverName);

		DBGPrint("[+] DriverEntry-> Creating Driver switching To DriverEntry0.\n");
		NtResult = IoCreateDriver(&DriverName, &DriverEntry0);
		if (!NT_SUCCESS(NtResult))
		{
			if (MyDriverObject)//will get set inside DriverEntry0 if created successfully 
			{
				IoDeleteDriver(MyDriverObject);
			}
			DBGPrint("[-] DriverEntry -> (NTSTATUS: 0x%X) Failed To Create Driver.\n", NtResult);
		}

	} while (FALSE);

	if (ParentProcId && ParentProcId != INVALID_HANDLE_VALUE)
	{
		KeUnstackDetachProcess(&ApcState);
	}

	if (!NT_SUCCESS(NtResult))
	{
		/*
		* clear any other stuff here if needed before returning
		*/
		if (MyDriverOriginalNtHeaders)
		{
			PVOID Temp = MyDriverOriginalNtHeaders;
			MyDriverOriginalNtHeaders = NULL;
			ExFreePool(Temp);
		}
	}
	GlobalName = NULL;
	DbgTextEnd("DriverEntry");

	return NtResult;
}