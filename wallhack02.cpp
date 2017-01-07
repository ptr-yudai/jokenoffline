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
VOID WINAPI my_DrawIndexedPrimitive(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
VOID WINAPI my_EndScene(LPDIRECT3DDEVICE9);

DWORD *vTable;
DWORD EndScene_hook;
DWORD EndScene_ret;
DWORD DrawIndexedPrimitive_hook;
DWORD DrawIndexedPrimitive_ret;

/*
	�e�N�X�`���𐶐�����
*/
bool GenerateCham(IDirect3DDevice9 * pDevice, LPDIRECT3DTEXTURE9 *texture, DWORD colorARGB) {
	if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, texture, NULL)))
		return false;
	WORD color16 = ((WORD)((colorARGB >> 28) & 0xF) << 12)
		| (WORD)(((colorARGB >> 20) & 0xF) << 8)
		| (WORD)(((colorARGB >> 12) & 0xF) << 4)
		| (WORD)(((colorARGB >> 4) & 0xF) << 0);
	D3DLOCKED_RECT d3dlr;
	(*texture)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;
	for (int xy = 0; xy < 8 * 8; xy++) *pDst16++ = color16;
	(*texture)->UnlockRect(0);
	return true;
}

/*
	DrawIndexPrimitive : �t�b�N���̏���
*/
__declspec(naked) void hook_DrawIndexedPrimitive()
{
	static LPDIRECT3DDEVICE9 pDevice;
	static D3DPRIMITIVETYPE pType;
	static INT BaseVertexIndex;
	static UINT MinIndex;
	static UINT NumVertices;
	static UINT StartIndex;
	static UINT PrimitiveCount;

	// pDevice���擾����
	__asm {
		pushad
		pushfd
		mov esi, [ebp + 0x08]
		mov pDevice, esi;
	}
	// �e��������擾����
	__asm {
		mov esi, [ebp + 0x0C]
		mov pType, esi
		mov esi, [ebp + 0x10]
		mov BaseVertexIndex, esi
		mov esi, [ebp + 0x14]
		mov MinIndex, esi
		mov esi, [ebp + 0x18]
		mov NumVertices, esi
		mov esi, [ebp + 0x1C]
		mov StartIndex, esi
		mov esi, [ebp + 0x20]
		mov PrimitiveCount, esi
	}
	// �p�ӂ����������Ăяo��
	my_DrawIndexedPrimitive(pDevice, pType, BaseVertexIndex, MinIndex, NumVertices, StartIndex, PrimitiveCount);
	// �㏑���������߂����s���Ă��猳�̏ꏊ�ɖ߂�
	__asm {
		popfd
		popad
		mov esi, [ebp + 0x8]
		test esi, esi
		jmp DrawIndexedPrimitive_ret;
	}
}
VOID WINAPI my_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
	LPDIRECT3DVERTEXBUFFER9 Stream_Data;
	UINT Stride, offset;
	LPDIRECT3DTEXTURE9 chameleon;

	if (pDevice->GetStreamSource(0, &Stream_Data, &offset, &Stride) == D3D_OK) {
		Stream_Data->Release();
	}

	if ((Stride == 32 && (
		NumVertices == 245 || 
		NumVertices == 232 ||
		NumVertices == 256))) {
		// �e�N�X�`����ݒ肷��
		GenerateCham(pDevice, &chameleon, D3DCOLOR_ARGB(255, 254, 0, 0));
		pDevice->SetTexture(0, chameleon);
	}
}

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
* �������t�b�N����
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
	/*
		DrawIndexPrimitive���\�b�h�̒��Ԃ�detour���쐬����
			EndScene+30h : 8B75 08 : mov esi, [ebp + 8h]
			EndScene+33h : 85f6    : test esi, esi
			--> jmp [hook_DrawIndexPrimitive]
	*/
	DrawIndexedPrimitive_hook = vTable[82] + 0x30;				// mid-function hook
	DrawIndexedPrimitive_ret = DrawIndexedPrimitive_hook + 5;	// return address
	MakeMFH((PBYTE)DrawIndexedPrimitive_hook, (DWORD)hook_DrawIndexedPrimitive, 3 + 2);
}

/*
	���ԃt�b�N���쐬����
*/
VOID MakeMFH(PBYTE hookAddr, DWORD jmpAddr, DWORD size)
{
	DWORD oldProtect, relativeAddr;
	VirtualProtect(hookAddr, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	relativeAddr = (DWORD)(jmpAddr - (DWORD)hookAddr) - 5;	// ���΃A�h���X�ɕϊ�
	// E9 XX XX XX XX : jmp [XXXXXXXX]
	*hookAddr = 0xE9;
	*((DWORD *)(hookAddr + 1)) = relativeAddr;
	// �~NOP
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
* �G���g���[�|�C���g
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
