/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    MsPassthroughExt.c

Abstract:

    This file contains the implementation of a passthrough filter
    utilizing SxBase.lib.


--*/

#include <Ntstrsafe.h>
#include <wdm.h>

#include "precomp.h"

UCHAR SxExtMajorNdisVersion = NDIS_FILTER_MAJOR_VERSION;
UCHAR SxExtMinorNdisVersion = NDIS_FILTER_MINOR_VERSION;
PWCHAR SxExtFriendlyName = L"Microsoft Sample Passthrough Extension";
PWCHAR SxExtUniqueName = L"{2A06F1CB-1B9B-43A8-ADF7-B7D44A9EE71C}";
PWCHAR SxExtServiceName = L"MsPassthroughExt";
ULONG SxExtAllocationTag = 'tPsM';
ULONG SxExtOidRequestId = 'tPsM';

const WCHAR DeviceName[] = L"\\Device\\vRouter";
const WCHAR DeviceSymLink[] = L"\\DosDevices\\vRouter";
const WCHAR SectionName[] = L"\\BaseNamedObjects\\vRouter";

NTSTATUS
NotImplemented(PDEVICE_OBJECT DriverObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Irp);

	DbgPrint("Called a non-implemented function!");

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
Create(PDEVICE_OBJECT DriverObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Irp);

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
Close(PDEVICE_OBJECT DriverObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(Irp);

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS
Write(PDEVICE_OBJECT DriverObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DriverObject);

	NTSTATUS NtStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION IoStackIrp = NULL;
	PCHAR WriteDataBuffer;
	int printable;

	DbgPrint("Write");

	IoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if (IoStackIrp)
	{
		WriteDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

		if (WriteDataBuffer)
		{
			printable = 0;
			for (ULONG i = 0; i < IoStackIrp->Parameters.Write.Length; i++)
			{
				if (WriteDataBuffer[i] == '\0')
				{
					printable = 1;
				}
			}
			if (printable)
			{
				DbgPrint("Got from the outer space: %ws", WriteDataBuffer);
			}
		}
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = IoStackIrp->Parameters.Write.Length;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;
}

NTSTATUS
Read(PDEVICE_OBJECT DriverObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DriverObject);

	NTSTATUS NtStatus = STATUS_SUCCESS;
	PIO_STACK_LOCATION IoStackIrp = NULL;
	PWCHAR ReadData = L"Hello, I'm your god.";
	UINT BufferSize = sizeof(L"Hello, I'm your god.");
	PWCHAR ReadDataBuffer;

	DbgPrint("Read");

	IoStackIrp = IoGetCurrentIrpStackLocation(Irp);

	if (IoStackIrp)
	{
		ReadDataBuffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

		if (ReadDataBuffer && IoStackIrp->Parameters.Read.Length >= BufferSize)
		{
			RtlCopyMemory(ReadDataBuffer, ReadData, BufferSize);
		}
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = IoStackIrp->Parameters.Write.Length;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;
}

NTSTATUS
CreateDevice(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING _DeviceName;
	UNICODE_STRING _DeviceSymLink;
	PDEVICE_OBJECT DeviceObject = NULL;

	RtlInitUnicodeString(&_DeviceName, DeviceName);
	RtlInitUnicodeString(&_DeviceSymLink, DeviceSymLink);

	NTSTATUS Status = IoCreateDevice(DriverObject, 0, &_DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (Status == STATUS_OBJECT_NAME_COLLISION)
	{
		DbgPrint("Failed to create device, because of collisions...\r\n");
	}
	if (Status == STATUS_SUCCESS)
	{
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
			DriverObject->MajorFunction[i] = NotImplemented;

		DriverObject->MajorFunction[IRP_MJ_CREATE] = Create;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = Close;
		DriverObject->MajorFunction[IRP_MJ_WRITE] = Write;
		DriverObject->MajorFunction[IRP_MJ_READ] = Read;

		DeviceObject->Flags |= DO_DIRECT_IO;
		DeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

		Status = IoCreateSymbolicLink(&_DeviceSymLink, &_DeviceName);
	}

	return Status;
}


NTSTATUS
CreateSection()
{
	NTSTATUS status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Section;
	ULONG Attributes = OBJ_KERNEL_HANDLE | OBJ_FORCE_ACCESS_CHECK | OBJ_OPENIF;
	UNICODE_STRING _SectionName;
	PVOID BaseAddress = NULL;
	SIZE_T ViewSize = 0;
	LARGE_INTEGER MaxSize;
	MaxSize.QuadPart = 320;
	//PVOID SecurityDescriptor;

	DbgPrint("Creating a section\r\n");

	RtlInitUnicodeString(&_SectionName, SectionName);

	//SecurityDescriptor = ExAllocatePool(PagedPool, PAGE_SIZE);
	//RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

	InitializeObjectAttributes(&ObjectAttributes, &_SectionName, Attributes, NULL, NULL);

	status = ZwCreateSection(&Section, SECTION_ALL_ACCESS, &ObjectAttributes, &MaxSize, PAGE_READWRITE, SEC_COMMIT, NULL);


	if (status != STATUS_SUCCESS)
	{
		DbgPrint("Failed creating a section, error code: %lx\r\n", status);
		DbgPrint("Falling back to opening an existing section...\r\n");
		status = ZwOpenSection(&Section, SECTION_ALL_ACCESS, &ObjectAttributes);
		if (status != STATUS_SUCCESS)
		{
			DbgPrint("Failed open a section, error code: %lx\r\n", status);
			return status;
		}
	}

	status = ZwMapViewOfSection(Section, ZwCurrentProcess(), &BaseAddress, 0, 80, NULL, &ViewSize, ViewShare, 0, PAGE_READWRITE);
	if (status != STATUS_SUCCESS)
	{
		DbgPrint("Failed creating a mapping, error code: %lx\r\n", status);
		return status;
	}

	ZwClose(&Section);

	RtlFillMemory(BaseAddress, 10, 'S');

	DbgPrint("Succeeded creating a mapping, sample: %c\r\n", ((char*)BaseAddress)[0]);

	return STATUS_SUCCESS;
}


NDIS_STATUS
SxExtInitialize(PDRIVER_OBJECT DriverObject)
{
	DbgPrint("SxExtInitialize");

	CreateDevice(DriverObject);
	CreateSection();

    return NDIS_STATUS_SUCCESS;
}


VOID
SxExtUninitialize(PDRIVER_OBJECT DriverObject)
{
	DbgPrint("SxExtUninitialize");

	UNICODE_STRING DosDeviceName;

	RtlInitUnicodeString(&DosDeviceName, DeviceSymLink);
	IoDeleteSymbolicLink(&DosDeviceName);

	IoDeleteDevice(DriverObject->DeviceObject);

    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreateSwitch(
    PSX_SWITCH_OBJECT Switch,
    PNDIS_HANDLE *ExtensionContext
    )
{
	DbgPrint("SxExtCreateSwitch");
    UNREFERENCED_PARAMETER(Switch);
    
    *ExtensionContext = NULL;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtDeleteSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
	DbgPrint("SxExtDeleteSwitch");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtActivateSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
	DbgPrint("SxExtActivateSwitch");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtRestartSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
	DbgPrint("SxExtRestartSwitch");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);

    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_ 
VOID
SxExtPauseSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
	DbgPrint("SxExtPauseSwitch");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreatePort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
	DbgPrint("SxExtCreatePort");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtUpdatePort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
	DbgPrint("SxExtUpdatePort");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreateNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
	DbgPrint("SxExtCreateNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtConnectNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
	DbgPrint("SxExtConnectNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtUpdateNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
	DbgPrint("SxExtUpdateNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDisconnectNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
	DbgPrint("SxExtDisconnectNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDeleteNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
	DbgPrint("SxExtDeleteNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtTeardownPort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
	DbgPrint("SxExtTeardownPort");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDeletePort(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
	DbgPrint("SxExtDeletePort");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtSaveNic(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    PULONG BytesWritten,
    PULONG BytesNeeded
    )
{
	DbgPrint("SxExtSaveNic");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    *BytesWritten = 0;
    *BytesNeeded = 0;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtSaveNicComplete(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    )
{
	DbgPrint("SxExtSaveNicComplete");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    return;
}


_Use_decl_annotations_  
NDIS_STATUS
SxExtNicRestore(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    PULONG BytesRestored
    )
{
	DbgPrint("SxExtNicRestore");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    *BytesRestored = 0;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtNicRestoreComplete(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    )
{
	DbgPrint("SxExtNicRestoreComplete");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtAddSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    )
{
	DbgPrint("SxExtAddSwitchProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtUpdateSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    )
{
	DbgPrint("SxExtUpdateSwitchProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
BOOLEAN
SxExtDeleteSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_DELETE_PARAMETERS SwitchProperty
    )
{
	DbgPrint("SxExtDeleteSwitchProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return FALSE;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtAddPortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
{
	DbgPrint("SxExtAddPortProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtUpdatePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
{
	DbgPrint("SxExtUpdatePortProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
BOOLEAN
SxExtDeletePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_DELETE_PARAMETERS PortProperty
    )
{
	DbgPrint("SxExtDeletePortProperty");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return FALSE;
}


_Use_decl_annotations_
BOOLEAN
SxExtQuerySwitchFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_FEATURE_STATUS_PARAMETERS SwitchFeatureStatus,
    PULONG BytesNeeded
    )
{
	DbgPrint("SxExtQuerySwitchFeatureStatus");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchFeatureStatus);
    UNREFERENCED_PARAMETER(BytesNeeded);
    
    return FALSE;
}


_Use_decl_annotations_
BOOLEAN
SxExtQueryPortFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_FEATURE_STATUS_PARAMETERS PortFeatureStatus,
    PULONG BytesNeeded
    )
{
	DbgPrint("SxExtQueryPortFeatureStatus");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortFeatureStatus);
    UNREFERENCED_PARAMETER(BytesNeeded);
    
    return FALSE;
}
    

_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicRequest(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_OID_REQUEST OidRequest,
    PNDIS_SWITCH_PORT_ID SourcePortId,
    PNDIS_SWITCH_NIC_INDEX SourceNicIndex,
    PNDIS_SWITCH_PORT_ID DestinationPortId,
    PNDIS_SWITCH_NIC_INDEX DestinationNicIndex
    )
{
	DbgPrint("SxExtProcessNicRequest");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicRequestComplete(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_OID_REQUEST OidRequest,
    NDIS_SWITCH_PORT_ID SourcePortId,
    NDIS_SWITCH_NIC_INDEX SourceNicIndex,
    NDIS_SWITCH_PORT_ID DestinationPortId,
    NDIS_SWITCH_NIC_INDEX DestinationNicIndex,
    NDIS_STATUS Status
    )
{
	DbgPrint("SxExtProcessNicRequestComplete");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    UNREFERENCED_PARAMETER(Status);
    
    return NDIS_STATUS_SUCCESS;
}
    

_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_STATUS_INDICATION StatusIndication,
    NDIS_SWITCH_PORT_ID SourcePortId,
    NDIS_SWITCH_NIC_INDEX SourceNicIndex
    )
{
	DbgPrint("SxExtProcessNicStatus");
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(StatusIndication);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtStartNetBufferListsIngress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG SendFlags
    )
{
	DbgPrint("SxExtStartNetBufferListsIngress");
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibSendNetBufferListsIngress(Switch,
                                   NetBufferLists,
                                   SendFlags,
                                   0);
}


_Use_decl_annotations_ 
VOID
SxExtStartNetBufferListsEgress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG NumberOfNetBufferLists,
    ULONG ReceiveFlags
    )
{
	DbgPrint("SxExtStartNetBufferListsEgress");
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibSendNetBufferListsEgress(Switch,
                                  NetBufferLists,
                                  NumberOfNetBufferLists,
                                  ReceiveFlags);
}


_Use_decl_annotations_
VOID
SxExtStartCompleteNetBufferListsEgress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG ReturnFlags
    )
{
	DbgPrint("SxExtStartCompleteNetBufferListsEgress");
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibCompleteNetBufferListsEgress(Switch,
                                      NetBufferLists,
                                      ReturnFlags);
}


_Use_decl_annotations_
VOID
SxExtStartCompleteNetBufferListsIngress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG SendCompleteFlags
    )
{
	DbgPrint("SxExtStartCompleteNetBufferListsIngress");
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibCompleteNetBufferListsIngress(Switch,
                                       NetBufferLists,
                                       SendCompleteFlags);
}
    
