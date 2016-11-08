#include <iostream>

#include <windows.h>

LPSTR GetError()
{
	LPSTR errorText = NULL;

	FormatMessageA(
		// use system message tables to retrieve error text
		FORMAT_MESSAGE_FROM_SYSTEM
		// allocate buffer on local heap for error text
		| FORMAT_MESSAGE_ALLOCATE_BUFFER
		// Important! will fail otherwise, since we're not 
		// (and CANNOT) pass insertion parameters
		| FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&errorText,  // output 
		0, // minimum size for output buffer
		NULL);   // arguments - see not

	return errorText;
}

void TestSection()
{
	std::cout << "Testuje Sekcje!" << std::endl;
	HANDLE Handle = OpenFileMapping(FILE_MAP_WRITE, FALSE, L"Global\\vRouter");

	if (Handle == NULL || Handle == INVALID_HANDLE_VALUE)
	{
		std::cout << "Error, bad HANDLE: " << std::endl;
		std::cout << "Error: " << GetError() << std::endl;
		return;
	}

	//PVOID Section = MapViewOfFile(Handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	PVOID Section = MapViewOfFile(Handle, FILE_MAP_READ, 0, 0, 0);

	if (Section == NULL)
	{
		std::cout << "Error, bad pointer: " << std::endl;
		std::cout << "Error: " << GetError() << std::endl;
		return;
	}

	std::cout << "Pointer is: " << (int*)Section << std::endl;
	std::cout << "Got " << (int)((char*)Section)[0] << "=" << ((char*)Section)[0]<< ", dobrze? :(" << std::endl;
}

int main()
{
    HANDLE hFile;
    DWORD dwReturn;
	WCHAR buff[80 / sizeof(WCHAR)];

	std::cout << "Trying..." << std::endl;

    hFile = CreateFile(L"\\\\.\\vRouter", 
            GENERIC_READ | GENERIC_WRITE, 0, NULL, 
            OPEN_EXISTING, 0, NULL);

    if(hFile != INVALID_HANDLE_VALUE)
    {
        WriteFile(hFile, L"Hello from user mode!", 
                  sizeof(L"Hello from user mode!"), &dwReturn, NULL);
		ReadFile(hFile, buff, 80, &dwReturn, NULL);
        CloseHandle(hFile);
		std::wcout << L"Success :) Got: " << buff << L"!!" << std::endl;
    }
	else
	{
		std::wcout << L"Failure :( Blad to: " << GetError() << std::endl;
	}

	TestSection();
    
    return 0;
}