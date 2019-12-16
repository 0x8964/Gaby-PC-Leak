#if (defined(_MSC_VER) && (_MSC_VER < 1900))
#define snprintf _snprintf
#endif
#include"General.h"
#include "d3d9.h"


typedef NxPhysicsSDK* (NX_CALL_CONV * MyNxCreatePhysicsSDK) (NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc& desc, NxSDKCreateError* errorCode);
MyNxCreatePhysicsSDK originalCreateSDK;
NxPhysicsSDK* hookedFunction(NxU32 sdkVersion, NxUserAllocator* allocator, NxUserOutputStream* outputStream, const NxPhysicsSDKDesc& desc, NxSDKCreateError* errorCode)
{
	DetourRemove((PBYTE)originalCreateSDK, (PBYTE)hookedFunction);
	NxPhysicsSDK* SDKPointer = originalCreateSDK(sdkVersion, allocator, outputStream, desc, errorCode);
	if (SDKPointer != NULL)
		hkNxPhysicsSDK *ret = new hkNxPhysicsSDK(&SDKPointer);
	General::hacks.physics = SDKPointer;
	return SDKPointer;
}



int WINAPI DllThread()
{

	HMODULE hMod = LoadLibrary("d3d9.dll");
	oDirect3DCreate9 = (tDirect3DCreate9)DetourFunction((BYTE*)GetProcAddress(hMod, "Direct3DCreate9"), (BYTE*)hkDirect3DCreate9);

	hMod = LoadLibrary("PhysXLoader.dll");
	/*while (hMod == NULL)
	{
		hMod = GetModuleHandle("PhysXLoader.dll");
		Sleep(10);
	}*/

	originalCreateSDK = (MyNxCreatePhysicsSDK)DetourFunction((BYTE*)GetProcAddress(hMod, "NxCreatePhysicsSDK"), (BYTE*)hookedFunction);
	return 0;
}

class CTools
{

public:
	void HideDLL(HINSTANCE hModule);
	void EraseHeader(HINSTANCE hModule);
} Tools;

void CTools::EraseHeader(HINSTANCE hModule)
{
	/*
	* func to erase headers, by Croner.
	* keep in mind you wont be able to load
	* any resources after you erase headers.
	*/

	PIMAGE_DOS_HEADER pDoH;
	PIMAGE_NT_HEADERS pNtH;
	DWORD i, ersize, protect;

	if (!hModule) return;

	// well just to make clear what we doing
	pDoH = (PIMAGE_DOS_HEADER)(hModule);

	pNtH = (PIMAGE_NT_HEADERS)((LONG)hModule + ((PIMAGE_DOS_HEADER)hModule)->e_lfanew);

	ersize = sizeof(IMAGE_DOS_HEADER);
	if (VirtualProtect(pDoH, ersize, PAGE_READWRITE, &protect))
	{
		for (i = 0; i < ersize; i++)
			*(BYTE*)((BYTE*)pDoH + i) = 0;
	}

	ersize = sizeof(IMAGE_NT_HEADERS);
	if (pNtH && VirtualProtect(pNtH, ersize, PAGE_READWRITE, &protect))
	{
		for (i = 0; i < ersize; i++)
			*(BYTE*)((BYTE*)pNtH + i) = 0;
	}
	return;
}

void CTools::HideDLL(HINSTANCE hModule)
{
	DWORD dwPEB_LDR_DATA = 0;
	_asm
	{
		pushad;
		pushfd;
		mov eax, fs:[30h]
			mov eax, [eax + 0Ch]
			mov dwPEB_LDR_DATA, eax

		InLoadOrderModuleList :
		mov esi, [eax + 0Ch]
			mov edx, [eax + 10h]

		LoopInLoadOrderModuleList :
								  lodsd
								  mov esi, eax
								  mov ecx, [eax + 18h]
								  cmp ecx, hModule
								  jne SkipA
								  mov ebx, [eax]
								  mov ecx, [eax + 4]
								  mov[ecx], ebx
								  mov[ebx + 4], ecx
								  jmp InMemoryOrderModuleList

							  SkipA :
		cmp edx, esi
			jne LoopInLoadOrderModuleList

		InMemoryOrderModuleList :
		mov eax, dwPEB_LDR_DATA
			mov esi, [eax + 14h]
			mov edx, [eax + 18h]

		LoopInMemoryOrderModuleList :
									lodsd
									mov esi, eax
									mov ecx, [eax + 10h]
									cmp ecx, hModule
									jne SkipB
									mov ebx, [eax]
									mov ecx, [eax + 4]
									mov[ecx], ebx
									mov[ebx + 4], ecx
									jmp InInitializationOrderModuleList

								SkipB :
		cmp edx, esi
			jne LoopInMemoryOrderModuleList

		InInitializationOrderModuleList :
		mov eax, dwPEB_LDR_DATA
			mov esi, [eax + 1Ch]
			mov edx, [eax + 20h]

		LoopInInitializationOrderModuleList :
											lodsd
											mov esi, eax
											mov ecx, [eax + 08h]
											cmp ecx, hModule
											jne SkipC
											mov ebx, [eax]
											mov ecx, [eax + 4]
											mov[ecx], ebx
											mov[ebx + 4], ecx
											jmp Finished

										SkipC :
		cmp edx, esi
			jne LoopInInitializationOrderModuleList

		Finished :
		popfd;
		popad;
	}
}

bool WINAPI DllMain(HMODULE hDll, DWORD dwReason, PVOID pvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Tools.HideDLL(hDll);
		Tools.EraseHeader(hDll);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)DllThread, NULL, NULL, NULL);
		return true;
	}

	return false;
}



