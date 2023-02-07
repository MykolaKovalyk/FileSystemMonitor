#include <fltKernel.h>
#include <suppress.h>
#include <ntddk.h>




#define Log(message, ...) DbgPrintEx(0, 0, message, __VA_ARGS__)




NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registryPath);

NTSTATUS DriverUnload(FLT_FILTER_UNLOAD_FLAGS flags);


FLT_POSTOP_CALLBACK_STATUS PostCreateCallback(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS filterObjects, PVOID* completionContext, FLT_POST_OPERATION_FLAGS flags);


NTSTATUS ConnectNotify(PFLT_PORT clientPort, PVOID serverportCookie, PVOID context, ULONG size, PVOID connectionCookie);

NTSTATUS DisconnectNotify(PVOID connectionCookie);

NTSTATUS MessageNotify(PVOID portCookie, PVOID inputBuffer, ULONG inputBufferLength, PVOID outputBuffer, ULONG outputBufferLength, PULONG returnedLength);




PFLT_FILTER filter = NULL;
PFLT_PORT serverPort = NULL;
PFLT_PORT clientPort = NULL;

UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\FSFD");

LARGE_INTEGER timeout;



const FLT_OPERATION_REGISTRATION callbacks[] =
{
	{IRP_MJ_CREATE, 0, NULL, PostCreateCallback},
	{IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION registrationParams =
{
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	callbacks,
	DriverUnload,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};





NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(registryPath);


	NTSTATUS status;
	PSECURITY_DESCRIPTOR securityDescriptor = NULL;
	OBJECT_ATTRIBUTES attributes;

	timeout.QuadPart = -500 * 10 * 1000;


	status = FltRegisterFilter(driver, &registrationParams, &filter);

	if (!NT_SUCCESS(status)) return status;



	Log("FileSystemMonitorDriver: driver was loaded.");



	status = FltBuildDefaultSecurityDescriptor(&securityDescriptor, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status))
	{
		FltUnregisterFilter(filter);
		return status;
	}

	InitializeObjectAttributes(&attributes, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, securityDescriptor);
	status = FltCreateCommunicationPort(filter, &serverPort, &attributes, NULL, ConnectNotify, DisconnectNotify, MessageNotify, 1);

	FltFreeSecurityDescriptor(securityDescriptor);

	if (!NT_SUCCESS(status))
	{
		FltUnregisterFilter(filter);
		return status;
	}



	Log("FileSystemMonitorDriver: communication port was opened.");



	status = FltStartFiltering(filter);

	if (!NT_SUCCESS(status))
	{
		FltCloseCommunicationPort(serverPort);
		FltUnregisterFilter(filter);
	}



	Log("FileSystemMonitorDriver: filtering was initiated.");



	return status;
}

NTSTATUS DriverUnload(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);

	Log("FileSystemMonitorDriver: driver unload sequence initiated.");

	FltCloseCommunicationPort(serverPort);
	FltUnregisterFilter(filter);

	return STATUS_SUCCESS;
}


FLT_POSTOP_CALLBACK_STATUS PostCreateCallback(PFLT_CALLBACK_DATA data, PCFLT_RELATED_OBJECTS filterObjects, PVOID* completionContext, FLT_POST_OPERATION_FLAGS flags)
{
	UNREFERENCED_PARAMETER(completionContext);
	UNREFERENCED_PARAMETER(filterObjects);
	UNREFERENCED_PARAMETER(flags);




	if (!clientPort)
		return FLT_POSTOP_FINISHED_PROCESSING;

	ULONG disposition = data->Iopb->Parameters.Create.Options >> 24 & 0xFF;

	if (
		disposition != FILE_SUPERSEDE &&
		disposition != FILE_CREATE &&
		disposition != FILE_OPEN_IF &&
		disposition != FILE_OVERWRITE_IF)
		return FLT_POSTOP_FINISHED_PROCESSING;




	NTSTATUS status;

	PUNICODE_STRING processName = NULL;
	PUNICODE_STRING fileName = NULL;




	PEPROCESS process = FltGetRequestorProcess(data);
	status = SeLocateProcessImageName(process, &processName);


	PFLT_FILE_NAME_INFORMATION fileInfo;
	status = FltGetFileNameInformation(data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE, &fileInfo);

	if (NT_SUCCESS(status))
		fileName = &fileInfo->Name;




	unsigned short filePathSize = fileName ? fileName->Length : 0;

	if (filePathSize > 0)
	{
		unsigned short processPathSize = processName ? processName->Length : 0;
		unsigned int PID = FltGetRequestorProcessId(data);


		int overallSize =
			sizeof(PID) +
			sizeof(processPathSize) +
			sizeof(filePathSize) +
			processPathSize +
			filePathSize;


		void* messageBuffer = ExAllocatePool2(POOL_FLAG_NON_PAGED | POOL_FLAG_UNINITIALIZED, overallSize, 7 /* just a random tag */);

		if (messageBuffer != NULL)
		{
			RtlCopyMemory(messageBuffer, &PID, sizeof(PID));
			RtlCopyMemory((char*)messageBuffer + sizeof(PID), &processPathSize, sizeof(processPathSize));
			RtlCopyMemory((char*)messageBuffer + sizeof(PID) + sizeof(processPathSize), &filePathSize, sizeof(filePathSize));


			if (processPathSize)
				RtlCopyMemory(
					(char*)messageBuffer
					+ sizeof(PID)
					+ sizeof(processPathSize)
					+ sizeof(filePathSize),

					processName->Buffer,
					processPathSize);

			if (filePathSize)
				RtlCopyMemory(
					(char*)messageBuffer
					+ sizeof(PID)
					+ sizeof(processPathSize)
					+ sizeof(filePathSize)
					+ processPathSize,

					fileName->Buffer,
					filePathSize);


			FltSendMessage(filter, &clientPort, messageBuffer, overallSize, NULL, 0, &timeout);
			ExFreePool(messageBuffer);
		}
	}




	ExFreePool(processName);

	if (NT_SUCCESS(status))
		FltReleaseFileNameInformation(fileInfo);


	return FLT_POSTOP_FINISHED_PROCESSING;
}


NTSTATUS ConnectNotify(PFLT_PORT port, PVOID serverportCookie, PVOID context, ULONG size, PVOID connectionCookie)
{
	UNREFERENCED_PARAMETER(serverportCookie);
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(connectionCookie);

	Log("FileSystemMonitorDriver: connection was established.");

	clientPort = port;
	return STATUS_SUCCESS;
}

NTSTATUS DisconnectNotify(PVOID connectionCookie)
{
	UNREFERENCED_PARAMETER(connectionCookie);

	FltCloseClientPort(filter, &clientPort);

	Log("FileSystemMonitorDriver: client disconnected.");

	return STATUS_SUCCESS;
}

NTSTATUS MessageNotify(PVOID portCookie, PVOID inputBuffer, ULONG inputBufferLength, PVOID outputBuffer, ULONG outputBufferLength, PULONG returnedLength)
{
	UNREFERENCED_PARAMETER(portCookie);
	UNREFERENCED_PARAMETER(inputBuffer);
	UNREFERENCED_PARAMETER(inputBufferLength);
	UNREFERENCED_PARAMETER(outputBuffer);
	UNREFERENCED_PARAMETER(outputBufferLength);
	UNREFERENCED_PARAMETER(returnedLength);


	return STATUS_SUCCESS;
}



