/*
	wallhack01.cpp

	- Visual C++�ŃR���p�C������K�v������܂�
	- Direct3D(d3dx9)���K�v�ł�
	- �e��p�����[�^�͊e�����ɍ��킹�Ē������Ă�������
*/
#pragma once
#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

VOID MyHook();
VOID GetVTableAddr(DWORD);
VOID MakeMFH(PBYTE, DWORD, DWORD);
VOID WINAPI my_EndScene(LPDIRECT3DDEVICE9);

DWORD *vTable;
DWORD EndScene_hook;
DWORD EndScene_ret;

/*
	EndScene : �t�b�N���̏���
*/
__declspec(naked) void hook_EndScene()
{
	static LPDIRECT3DDEVICE9 pDevice;
	// pDevice���擾����
	__asm {
		mov edi, [ebp + 0x8]
		mov pDevice, edi;
	}
	// �p�ӂ����������Ăяo��
	my_EndScene(pDevice);
	// �㏑���������߂����s���Ă��猳�̏ꏊ�ɖ߂�
	__asm {
		mov edi, [ebp + 0x8]
		xor esi, esi
		jmp EndScene_ret;
	}
}
VOID WINAPI my_EndScene(LPDIRECT3DDEVICE9 pDevice)
{
	LPD3DXFONT pFont;
	D3DXCreateFont(pDevice, 32, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &pFont);
	// �������`��
	RECT rect;
	SetRect(&rect, 32, 32, 32, 32);
	pFont->DrawText(NULL, TEXT("Wallhack is enabled."), -1, &rect, DT_NOCLIP | DT_LEFT, D3DCOLOR_ARGB(255, 255, 000, 000));
	pFont->Release();
}

/*
	�������t�b�N����
*/
VOID MyHook()
{
	// d3d9.dll���������ɓW�J����
	DWORD hD3D = (DWORD)GetModuleHandle(TEXT("d3d9.dll"));
	if (hD3D == NULL) return;
	// vTable�̃A�h���X��T������
	GetVTableAddr(hD3D);
	/*
		EndScene���\�b�h�̒��Ԃ�detour���쐬����
			EndScene+0Ch : 8B7D 08 : mov edi, [ebp + 8h]
			EndScene+0Fh : 33f6    : xor esi, esi
			--> jmp [hook_EndScene]
	*/
	EndScene_hook = vTable[42] + 0xC;	// mid-function hook
	EndScene_ret = EndScene_hook + 5;	// return address
	MakeMFH((PBYTE)EndScene_hook, (DWORD)hook_EndScene, 3 + 2);
}

/*
	�t�b�N���쐬����
*/
VOID MakeMFH(PBYTE hookAddr, DWORD jmpAddr, DWORD size)
{
	DWORD oldProtect, relativeAddr;
	VirtualProtect(hookAddr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	relativeAddr = (DWORD)(jmpAddr - (DWORD)hookAddr) - 5;	// ���΃A�h���X�ɕϊ�
	// E9 XX XX XX XX : jmp [XXXXXXXX]
	*hookAddr = 0xE9;
	*((DWORD *)(hookAddr + 1)) = relativeAddr;
	// NOP�Ŗ��߂�
	for (DWORD x = 0x5; x < size; x++) *(hookAddr + x) = 0x90;
	VirtualProtect(hookAddr, size, oldProtect, &oldProtect);
	return;
}

/*
	D3D��vTable�̃A�h���X��T������
*/
VOID GetVTableAddr(DWORD hD3D)
{
	// ���W���[���̃��[�h�悩��T������
	for (DWORD i = 0; i < 0x128000; i++) {
		// ���̂悤�ȃp�^�[����T��
		// C7 06 XX XX XX XX : mov [esi], [vTable]
		// 89 86 XX XX XX XX : mov esi, [hoge1]
		// 89 86 XX XX XX XX : mov esi, [hoge2]
		BOOL match = *(WORD*)(hD3D + i) == (WORD)0x06C7
			&& *(WORD*)(hD3D + i + 6) == (WORD)0x8689
			&& *(WORD*)(hD3D + i + 12) == (WORD)0x8689;
		if (match) {
			DWORD oldProtection;
			VirtualProtect(&vTable, 4, PAGE_EXECUTE_READWRITE, &oldProtection);
			CopyMemory(&vTable, (void*)(hD3D + i + 2), 4);
			VirtualProtect(&vTable, 4, oldProtection, &oldProtection);
			break;
		}
	}
}

/*
	�G���g���[�|�C���g
*/
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyHook, NULL, NULL, NULL);
	}
	return TRUE;
}
