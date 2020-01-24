#include <ntifs.h>
#include <ntddk.h>

typedef unsigned char BYTE;

#pragma warning(disable : 4152)

extern NTKERNELAPI NTSTATUS ObCreateObject(
  IN KPROCESSOR_MODE      ObjectAttributesAccessMode OPTIONAL,
  IN POBJECT_TYPE         ObjectType,
  IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
  IN KPROCESSOR_MODE      AccessMode,
  IN PVOID                Reserved,
  IN ULONG                ObjectSizeToAllocate,
  IN ULONG                PagedPoolCharge OPTIONAL,
  IN ULONG                NonPagedPoolCharge OPTIONAL,
  OUT PVOID               *Object
);

extern NTKERNELAPI NTSTATUS ObInsertObject(
  IN PVOID          Object,
  IN PACCESS_STATE  PassedAccessState OPTIONAL,
  IN ACCESS_MASK    DesiredAccess,
  IN ULONG          AdditionalReferences,
  OUT PVOID         *ReferencedObject OPTIONAL,
  OUT PHANDLE       Handle
);

extern POBJECT_TYPE IoDriverObjectType;

NTSTATUS DriverUnload(PDRIVER_OBJECT DriverObject) 
{
  UNREFERENCED_PARAMETER(DriverObject);
  return(STATUS_SUCCESS);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) 
{
  OBJECT_ATTRIBUTES objDriver;
  UNICODE_STRING    usFakeDriver;
  UNICODE_STRING    usFakeDevice;
  PDRIVER_OBJECT    pFakeDriver;
  PDEVICE_OBJECT    pFakeDevice;
  HANDLE            hNewDriver;
  PVOID             pFunctionPool;
  PVOID             pIofCompleteRequest;
  ULONG             ulObjectSize;
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = &DriverUnload;
  RtlInitUnicodeString(&usFakeDriver, L"\\Driver\\RandomDriver");
  RtlInitUnicodeString(&usFakeDevice, L"\\Device\\RandomDevice");
  InitializeObjectAttributes(&objDriver, &usFakeDriver, OBJ_PERMANENT | OBJ_CASE_INSENSITIVE, 0, 0);
  ulObjectSize = (sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION)); // From WRK
  ObCreateObject(KernelMode, *(POBJECT_TYPE*)IoDriverObjectType, &objDriver, KernelMode, NULL, ulObjectSize, 0, 0, &pFakeDriver);
  ObInsertObject(pFakeDriver, 0, 1i64, 0, NULL, &hNewDriver);
  ZwClose(hNewDriver);
  /*
    Minimum code required to handle IOCTLs:
    sub rsp, 0x28q
    mov rcx, rdx				; IRP
    xor edx, edx				; IO_NO_INCREMENT
    mov rax, 0xAAAAAAAAAAAAAAAA ; To be replaced with nt!IofCompleteRequest
    call rax
    add rsp, 0x28
    ret
  */
  BYTE bHandlerCode[26] = {
    0x48, 0x83, 0xEC, 0x28, 0x48, 0x89, 0xD1, 0x31, 0xD2, 0x48, 0xB8, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xFF, 0xD0, 0x48, 0x83, 0xC4,
    0x28, 0xC3
  };
  pFunctionPool = ExAllocatePool(NonPagedPool, 0x100);
  memset(pFunctionPool, 0x00, 0x100);
  pIofCompleteRequest = &IofCompleteRequest;
  memmove(&bHandlerCode[11], &pIofCompleteRequest, 8);
  memmove(pFunctionPool, &bHandlerCode, sizeof(bHandlerCode));
  
  for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
  {
    pFakeDriver->MajorFunction[i] = (PDRIVER_DISPATCH)pFunctionPool;
  }
  DbgPrint("pExePool: %I64X\n", pFunctionPool);
  pFakeDriver->Flags &= DO_BUFFERED_IO;
  pFakeDriver->FastIoDispatch = NULL;
  IoCreateDevice(pFakeDriver, 0, &usFakeDevice, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pFakeDevice);
  pFakeDevice->Flags = DO_DEVICE_HAS_NAME;
  /*
    Now a fake driver and device exist and a user mode program can interact with them
  */
  return(STATUS_SUCCESS);
}
