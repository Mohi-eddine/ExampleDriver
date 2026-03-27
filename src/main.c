#include "Entry.h"

NTSTATUS DriverEntry(PDRIVER_ENTRY_PARAMS UserParams, LPWSTR Name);

NTSTATUS DriverEntry(PDRIVER_ENTRY_PARAMS UserParams, LPWSTR Name)
{
	return _DriverEntry(UserParams, Name);
}