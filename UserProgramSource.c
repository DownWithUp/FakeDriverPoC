#include <windows.h>
#include <winternl.h>
#include <stdio.h>

void main()
{
  OBJECT_ATTRIBUTES objDevice;
  IO_STATUS_BLOCK   ioBlock;
  UNICODE_STRING    usDevice;
  NTSTATUS          ntRet;
  HANDLE            hDevice;
  BYTE bIn[] = {
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
    0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE,
  };

  RtlInitUnicodeString(&usDevice, L"\\Device\\RandomDevice");
  InitializeObjectAttributes(&objDevice, &usDevice, 0, 0, 0);
  ntRet = NtCreateFile(&hDevice, FILE_READ_ACCESS | FILE_WRITE_ACCESS, &objDevice, &ioBlock, NULL, 0, 0, OPEN_EXISTING, 0, NULL, 0);
  if (!ntRet)
  {
    printf("[i] Got a handle to the device: %X\n", hDevice);
    getchar();
    DeviceIoControl(hDevice, 0xDEADC0DE, &bIn, sizeof(bIn), NULL, 0, NULL, NULL);
  }
  else
  {
    printf("[!] NTSTATUS Error: %X\n", ntRet);
  }
  getchar();
  return;
}
