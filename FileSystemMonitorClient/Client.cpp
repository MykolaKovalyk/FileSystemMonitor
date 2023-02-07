#include "windows.h"
#include "fltuser.h"
#include "subauth.h"
#include <iostream>
#include <windows.h>




#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "fltlib.lib")




#define BUFFER_SIZE 2048
#define PORT_ADDR L"\\FSFD"



void* memory = NULL;
HANDLE port = NULL;




extern "C" __declspec(dllexport) int Connect()
{
	HRESULT result = FilterConnectCommunicationPort(PORT_ADDR, 0, NULL, 0, NULL, &port);

	if (FAILED(result))
		return result;

	return S_OK;
}

extern "C" __declspec(dllexport) int Disonnect()
{
	if (CloseHandle(port))
		return S_OK;
	else 
		return GetLastError();
}


extern "C" __declspec(dllexport) int ReceiveMessage(unsigned int* PID, WCHAR * processPath, WCHAR * filePath, int* processPathSize, int* filePathSize)
{
	HRESULT result;

	while(!memory)
		memory = malloc(BUFFER_SIZE);


	result = FilterGetMessage(port, (PFILTER_MESSAGE_HEADER)memory, BUFFER_SIZE, NULL);
	if (FAILED(result))
		return result;



	unsigned long long offset = sizeof(FILTER_MESSAGE_HEADER);



	*PID = *(unsigned int*)((char*)memory + offset);

	short processByteSize = *(USHORT*)((char*)memory + (offset += sizeof(int)));
	short fileByteSize = *(USHORT*)((char*)memory + (offset += sizeof(short)));

	*processPathSize = processByteSize / sizeof(WCHAR);
	*filePathSize = fileByteSize / sizeof(WCHAR);

	RtlCopyMemory(processPath, (PWCH)((char*)memory + (offset += sizeof(short))), processByteSize);
	RtlCopyMemory(filePath, (PWCH)((char*)memory + (offset += processByteSize)), fileByteSize);



	return S_OK;
}