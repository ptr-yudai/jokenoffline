# Mid-Function HookによるWallhack
<div style="text-align: right">情報工学科3年<br>藤原　裕大</div>

## 概要
本記事では、WindowsでDirect3Dを採用したアプリケーションに対してMid-Function Hookを行うことにより、簡易的なWallhackを実装する。対象とするアプリケーションはDirect3D9のサンプルとして付属するプログラムで、Wallhackにより特定のオブジェクトのテクスチャを赤色に変更する。なお、オブジェクトを最前面に描画する処理については本記事では実装しない。

## 注意
### [1] お願い
本記事ではチート行為として使用できる技術について説明するが、法律で許可される範囲で実験するように注意されたい。また、本記事はそのような行為を助長するためでなく、この分野が非常に幅広い技術を応用していることから、スキルアップのための教材として優れていると考えたために記事とした。実際に各自の環境で本記事の内容を試す場合は、自分でビルドしたものなど、必ずWallhackが許可されたアプリケーションに試すこと。
### [2] 本記事の対象者
本記事からWallhackについて十分に理解することは難しいかもしれないが、分からない部分はインターネットで調べながら読んでほしい。その上で、次のような前提知識がある方に読みやすく書くつもりだ。
- Windowsに慣れている
- C(C++)がある程度読める（特にポインタ）
- アセンブリがある程度読める（Intel x86）
- DLLについて知っている
- エンディアンについて知っている

## 原理
### Wallhack
**Wallhack**とは、3Dゲーム（特にFirst Person Shooting）に対するチート行為の一種である。Wallhackでは一般的に、Direct3DやOpenGLなどのAPIをフックすることにより、プログラムに任意の処理を割り込ませる。それにより、敵キャラクターなどの特定のオブジェクトを常に最前面に描画するように設定し、敵の位置を把握しやすいようにする。また、本記事で実装するように、特定のオブジェクトが目立つようにテクスチャを派手なものに変更することを、カメレオン（Chameleon）から取って**Chams**と呼ぶ。なお、本記事では後者についてのみ実装する。

<div style="text-align: center;">
    <img src="https://i.imgur.com/VbDQ7XS.jpg" width="70%"> </img>
    <p>[1]Wallhackされた3Dゲームの例</p>
</div>

### 仮想関数テーブル
C言語などでコンパイルするとき、関数は実行ファイル上で絶対的なアドレスに配置される。一方クラスを使用すると、変数と同じ扱いになるため、メソッドはメモリ上のランダムな位置に展開されることになる。（関数も実行時にはランダムな位置に配置されるが、これは容易に特定できる。）そこでプログラムがメソッドにアクセスするとき、仮想関数テーブル（**vTable**）と呼ばれる表を参照してメソッドのアドレスを特定する。

### DLLインジェクション
外部から、あるプロセスの中で特定の処理を実行させたいとき、1つの方法として**DLLインジェクション**と呼ばれる手法がある。これは、任意の処理を実装したDLLを特定のプロセスに読み込ませることにより、そのプロセスの内部で処理を実行する方法である。Windowsの場合、CreateRemoteThreadという関数が用意されており、これは特定のプロセス上で、引数が1つの関数を呼び出すことができるという不思議な関数である。一方LoadLibrary関数は、DLLのパスを引数とする1引数の関数で、その関数を呼び出したプロセスにDLLをロードする。したがって、CreateRemoteThread関数を使い特定のプロセスでLoadLibrary関数を実行させることにより、任意のDLLをロードすることができる。これがWindowsにおける一般的なDLLインジェクションの方法である。

### フック
あるプログラムの特定の関数やメソッドの呼出時に任意の動作を実行させたいとき、その処理から任意のコードにジャンプするように書き換えることを「**フックする**」と呼ぶ。特に、処理の途中で自分の用意したコードにジャンプするような機械語を挿入する技術を**detours**(迂回)などと呼ぶ。例えば以下のような処理があったとする。
```
function:
76FA13F2: push ebp
76FA13F3: mov ebp, esp
76FA13F5: push esi
76FA13F6: mov esi, [ebp + 8]
76FA13F9: cmp esi, -C
76FA13FC: jnb 76FA45FB
...
```
これを次のように変更することで、元のプログラムに影響を与えずに任意のコードへジャンプすることができる。（jmpの設置場所と機械語の命令長に注意してnopなどを配置すること。）
```
function:
76FA13F2: push ebp
76FA13F3: mov ebp, espA
76FA13F5: push esi
76FA13F6: jmp [hook_function]
76FA13FB: nop
76FA13FC: jnb 76FA45FB
...

hook_function:
06201000: [My Program]
06201XXX: mov esi, [ebp + 8]
06201XXX: cmp esi, -C
06201XXX: jmp 76FA13FC
```
このように、処理の途中で任意の処理にジャンプさせるようなフックを**Mid-Function Hook**と呼ぶ。なお、レジスタを破壊してしまう場合は、hook_functionにpushadやpushfdを利用してスタックに退避しておくと安全である。
DLLインジェクションは任意の処理を他プロセスで実行するのに対して、フックは既存の処理が呼び出されたときに任意の処理を割込ませるという違いがある。したがって、フックでは関数やメソッドが呼び出されたときの引数が取得できるという利点もある。

## 前提条件
本記事で使用する環境について、特に重要な部分を表1に示した。

|情報|実験環境|
|:-:|:-----:|
|OS|Windows10 x86|
|コンパイラ|**Visual C++ 2015**|
|エディタ|Visual Studio 2015|
|使用ライブラリ|DirectX 3D (June 2010)|
|使用ツール|うさみみハリケーン、DebugView、IDA Pro|
|Wallhack対象|DirectXのサンプルプログラム|

Visual C++は特にインラインアセンブリに非常に長けており、直感的にアセンブリを書くことができるため、今回のような低レイヤな部分を扱うときには大変便利である。また、今回のプログラムはWindowsのバージョンよりもDirect3Dのバージョンに大きく関わるため、各環境によってプログラムの細かい部分が違ってくることに注意してほしい。（本記事のプログラムは上記環境で実装したが、Windows7 x86でもWallhackに成功した。）

## EndSceneメソッドをフックする
Direct3Dでは、レンダリングの終了を知らせるEndSceneメソッドがあり、これは画面を更新するごとに呼び出される。この章では、EndSceneをフックすることで、先に特定の文字列を最前面に描画してから本来のEndScene機能を呼び出すように処理を変更する。

### [0] デバッグ方法
今回はDLLインジェクションにより外部でコードを実行する。このようなときに簡単にプログラムの状況を確認できるのがOutputDebugString関数である。例えばDWORD型の値を確認する場合、次のようなコードを挿入する。
```
TCHAR str[64] = TEXT("");
wsprintf(str, TEXT("0x%x"), value);
OutputDebugString(str);
```
これを確認するのがDebugViewというツールである。このツールではOutputDebugStringにより出力された文字列を取得して表示できる。

### [1] DLLインジェクションを試す
まずは自分でDLLを作り、それを対象のプロセスにロードできるかを確認する。今回はDLLを作るだけで、インジェクションはうさみみハリケーン（<span>http://hp.vector.co.jp/authors/VA028184/#TOOL</span>）というツールを使用する。
新規DLLプロジェクトの作成方法は各自調べてもらいたいが、Win32プロジェクトにすること、空のプロジェクトにすることだけは注意してほしい。プロジェクトに適当なファイル名でcppファイルを作成する。本記事では全ての処理をこのcppファイルに記述する。以下がDLLインジェクション確認用の最低限のプログラムである。
```
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hModule,
                    DWORD fdwReason,
                    LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        MessageBox(NULL,
                   TEXT("Injection Test"),
                   TEXT("Debug"),
                   MB_OK);
	}
	return TRUE;
}
```
プログラムについて詳しくは説明しないが、DLLがロードされるとメッセージボックスが表示されるコードである。プロジェクトをビルドするとDLLが出力される。ロード対象のプロセスとうさみみハリケーンを起動し、うさみみハリケーンで対象プロセスにアタッチする。メニューバーの「デバッグ」から「DLLのロードとアンロード」を選ぶ。

<div style="text-align: center;">
    <img src="https://i.imgur.com/8ZjalU5.png" width="70%"></img>
    <p>[2]うさみみハリケーン DLLのロードとアンロード</p>
</div>

DLLパスに先程コンパイルしたDLLを選択し、「ロード実行」ボタンでDLLをロードしてくれる。ロードと同時に、次のようにメッセージボックスが表示されたらDLLインジェクションは成功である。

<div style="text-align: center;">
    <img src="https://i.imgur.com/UCVb508.png"> </img>
    <p>[3]DLLインジェクションにより任意の処理を実行できた</p>
</div>

またここでは記述していないが、Visual StudioのプロジェクトのプロパティでインクルードディレクトリやライブラリディレクトリにDirect3Dのパスを追加しておく必要がある。先頭に次のライブラリ使用宣言を記述してもエラーにならなければ正しくパスは設定できている。
```
#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
```
なお、パスを設定するとき、もしくはプロジェクトをビルドするときに、プロパティとビルドがそれぞれReleaseもしくはDebugで統一されていないとエラーになる。また、本記事では全て32bitを想定しているので、x86用にビルドすること。

### [2]vTableを探索する
DLLインジェクションが成功したということは、それ以降は通常のプログラム同様に対象プロセスの内部を操作することができる。そこでまず最初に必要なのが、EndSceneのアドレスを持つIDirect3DDevice9クラスのvTableである。vTableさえ手に入れば、vTableからメソッドのアドレスが全て取得できる。残念ながらvTableの場所は分からないため、今回はこれを探索するプログラムを書いた。Direct3Dのライブラリ内にはvTableを参照するコードとして次のような処理が存在する。
```
...
33C0          : xor eax, eax
C760 XXXXXXXX : mov [esi], d3d9.dll+XXXXXXXX ; vTable
8986 XXXXXXXX : mov [esi+XXXXXXXX], eax
8986 XXXXXXXX : mov [esi+XXXXXXXX], eax
...
```
C760の後の4バイトがvTableのアドレスである。このような機械語を持つパターンを探索する方法が一般的であるので、本記事でもこのコードを探索した。なお、このコードはd3d9.dllがロードされた先に存在し、そのアドレスはGetModuleHandle関数により分かるので、メモリのごく一部を探索するだけで見つかる。以下にvTableを探索する関数を示す。なお、hD3DはGetModuleHandleにより取得したd3d9.dllのロード先アドレスである。また、変数vTalbeはDWORD型で、グローバル変数として定義している。
```
VOID GetVTableAddr(DWORD hD3D)
{
    // モジュールのロード先から探索する
    for (DWORD i = 0; i < 0x128000; i++) {
        BOOL match = *(WORD*)(hD3D + i) == (WORD)0x06C7
            && *(WORD*)(hD3D + i + 6) == (WORD)0x8689
            && *(WORD*)(hD3D + i + 12) == (WORD)0x8689;
        if (match) {
            DWORD oldProtection;
            VirtualProtect(&vTable,
                           4, 
                           PAGE_EXECUTE_READWRITE,
                           &oldProtection);
            CopyMemory(&vTable, (void*)(hD3D + i + 2), 4);
            VirtualProtect(&vTable,
                           4,
                           oldProtection,
                           &oldProtection);
            break;
        }
    }
}
```
これによりvTableが取得できるが、テーブルのポインタがどのメソッドを表しているかはIDA Proなどでd3d9.dllを解析することにより容易に分かる。例えば今回フックするEndSceneの場合、IDA ProでEndSceneという名前を調べると、vTable上の関数が並んでいる。（これよりEndSceneはvTable[42]でアクセスできることが分かる。）

<div style="text-align: center;">
    <img src="https://i.imgur.com/mlsnvv8.png" width="70%"></img>
    <p>[4]IDA Proが見つけたvTableの構造</p>
</div>

また、このような情報は検索するとすぐに出てくるが、環境によって大きく異なる場合もあるので、自分の環境でも確認しておくことを推奨する。

### [3]メソッドをフックする
vTableが取得できたら、次にメソッドをフックする。今回はEndScene(vTable[42])をフックする。まずはvTable[42]の値をDebugViewなどで調べて、そのアドレスにうさみみハリケーンでアクセスする。うさみみハリケーンには逆アセンブルツールが付いている。メニューバーの「デバッグ」の「選択範囲を逆アセンブル」でEndSceneの処理を見てみる。
```
EndScene:
6A 20       : push 0x20
B8 XXXXXXXX : mov eax, XXXXXXXX
E8 XXXXXXXX : call XXXXXXXX
8B7D 08     : mov edi, [ebp + 0x8]
33F6        : xor esi, esi
85FF        : test edi, edi
...
```
これは環境によって大きく異なるので必ず自分で確認する必要がある。今回はこれを次のように書き換える。なお、dwEndScene_retはこの場合「test edi, edi」のアドレスである。
```
EndScene:
6A 20       : push 0x20
B8 XXXXXXXX : mov eax, XXXXXXXX
E8 XXXXXXXX : call XXXXXXXX
E9 XXXXXXXX : jmp hook_EndScene
85FF        : test edi, edi
...

hook_EndScene:
;;
;; Put New EndScene Here
;;
8B7D 08     : mov edi, [ebp + 0x8]
33F6        : xor esi, esi
E9 XXXXXXXX : jmp dwEndScene_ret
```
jmpを入れる場所は必ず最低5バイトは上書きする必要がある。今回はちょうど5バイトを上書きしたが、例えば6バイトを潰す必要がある場合はnopなどで埋めると良い。また、原理で説明したように元の処理を迂回先に記述する必要がある。さらに、今回は必要無いが、jmpを挿入する場所によってはpushadやpushfdなどを使用してレジスタの状態を保存しておかないと、新しいEndSceneでレジスタが破壊されてプロセスがクラッシュする場合もある。
さて、まずはjmpコードを挿入するプログラムを以下に示す。なお、hookAddrはフックする関数のポインタ、jmpAddrはジャンプ先のアドレス、sizeはjmpで潰す命令のコード長を指定する。また、jmp命令のジャンプ先アドレスは、機械語ではjmp命令からの相対アドレスになることに注意されたい。
```
VOID MakeMFH(PBYTE hookAddr, DWORD jmpAddr, DWORD size)
{
    DWORD oldProtect, relativeAddr;
    VirtualProtect(hookAddr,
                   size,
                   PAGE_EXECUTE_READWRITE,
                   &oldProtect);
    // 相対アドレスに変換
    relativeAddr = (DWORD)(jmpAddr - (DWORD)hookAddr) - 5;
    // E9 XX XX XX XX : jmp [XXXXXXXX]
    *hookAddr = 0xE9;
    *((DWORD *)(hookAddr + 1)) = relativeAddr;
    // NOPで埋める
    for (DWORD x = 0x5; x < size; x++) *(hookAddr + x) = 0x90;
    VirtualProtect(hookAddr,
                   size,
                   oldProtect,
                   &oldProtect);
    return;
}
```
今回は少しでも汎用的にするために、上書きする命令が5バイトでなくても動作するように5バイト目以降はnopで埋めるようにした。
次に、フックによりジャンプする先の処理を以下に示す。「\_\_delcspec(naked)」を記述すると、コンパイラが通常関数の先頭に入れる余計な命令を入れないようにできる。
```
__declspec(naked) void hook_EndScene()
{
    static LPDIRECT3DDEVICE9 pDevice;
    // pDeviceを取得する
    __asm {
        mov edi, [ebp + 0x8]
        mov pDevice, edi;
    }
    // 用意した処理を呼び出す
    my_EndScene(pDevice);
    // 上書きした命令を実行してから元の場所に戻る
    __asm {
        mov edi, [ebp + 0x8]
        xor esi, esi
        jmp EndScene_ret;
    }
}
```
このプログラムから分かるように、Visual Studioのインラインアセンブリは非常に直感的で、変数も直接アセンブリに書くことが許される。EndSceneメソッドを持つクラスであるIDirect3DDevice9を指すポインタは、この時[ebp+0x8]に存在する。これについては、同様のコードは調べればいくらでも見つかる上、この引数の場所に関しては私の知る限りx86では環境に依存せずebp+0x8である。(ちなみに第一引数はebp+0xC、第二引数はebp+0x10と続く。)このフックさえ正しく書ければ最大の山は乗り越えたであろう。（多くの場合はここでつまずく。）次の節ではmy_EndSceneについて説明する。

### [4] 任意の文字列を描画する
my_EndScene関数では取得したpDeviceを元に、普通のDirect3Dプログラミング同様に様々な処理を実行できる。今回は文字列を描画するので、フォントを生成して文字を書く処理を用意した。
```
VOID WINAPI my_EndScene(LPDIRECT3DDEVICE9 pDevice)
{
    LPD3DXFONT pFont;
    D3DXCreateFont(pDevice,
                   18,
                   0,
                   FW_BOLD,
                   1,
                   0,
                   DEFAULT_CHARSET,
                   OUT_DEFAULT_PRECIS,
                   DEFAULT_QUALITY,
                   DEFAULT_PITCH | FF_DONTCARE,
                   TEXT("Arial"),
                   &pFont);
    // 文字列を描画
    RECT rect;
    SetRect(&rect, 32, 32, 32, 32);
    pFont->DrawText(NULL,
                    TEXT("Wallhack is enabled."),
                    -1,
                    &rect,
                    DT_NOCLIP | DT_LEFT,
                    D3DCOLOR_ARGB(255, 255, 000, 000));
    pFont->Release();
}
```

### [5] 組み合わせる
ここまでで作成した関数はDllMainからは呼び出さない。新しくスレッドを作成する。
```
VOID MyHook()
{
    // d3d9.dllをメモリに展開する
    DWORD hD3D = (DWORD)GetModuleHandle(TEXT("d3d9.dll"));
    if (hD3D == NULL) return;
    // vTableのアドレスを探索する
    GetVTableAddr(hD3D);
    /*
        EndSceneメソッドの中間にdetourを作成する
            EndScene+0Ch : 8B7D 08 : mov edi, [ebp + 8h]
            EndScene+0Fh : 33f6    : xor esi, esi
            --> jmp [hook_EndScene]
    */
    EndScene_hook = vTable[42] + 0xC;
    EndScene_ret = EndScene_hook + 5;
    MakeMFH((PBYTE)EndScene_hook, (DWORD)hook_EndScene, 3 + 2);
}

BOOL WINAPI DllMain(HINSTANCE hModule,
                    DWORD fdwReason,
                    LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL,
                     NULL,
                     (LPTHREAD_START_ROUTINE)MyHook,
                     NULL,
                     NULL,
                     NULL);
    }
    return TRUE;
}
```
これら全てをまとめたプログラムは次のURLからダウンロードできる。

<div style="text-align: center;">
    <img src="https://i.imgur.com/7LhlgoF.png"></img>
    <p>https://github.com/ptr-yudai/jokenoffline/blob/master/wallhack01.cpp</p>
</div>

早速実行してみよう。対象とするプログラムはDirect3Dのサンプルにある「ShadowMap.exe」である。DLLインジェクションにより作成したDLLをロードさせると、次のようにフックが成功していることが確認できる。

<div style="text-align: center;">
    <img src="https://i.imgur.com/OHwKT8K.png" width="70%"></img>
    <p>[5]EndSceneのフックによる文字列の描画</p>
</div>

図から分かるように、my_EndSceneが実行されて「Wallhack is enabled.」という文字列が左上に大きく表示されている。（図にするために文字サイズは大きくした。）

## Wallhackする
ここまではEndSceneをフックして、できるだけ簡単にフックが成功していることを確認した。ここからはWallhackの本題である「特定のオブジェクトのテクスチャを目立つ色に変更する」という処理を作る。

### [1] DIPをフックする
DIPとはDrawIndexedPrimitiveメソッドのことで、物体をレンダリングするためのメソッドである。Direct3Dをフックする方法によるWallhackでは必ずといってよいほどDrawIndexedPrimitiveメソッドがフックされるので、DIPという略称がしばしば使われる。
さて、DIPのフックもEndSceneと基本的に変わらない。vTableからのオフセットを調べ、そのアドレスを逆アセンブルしてMid-Function Hookのための計画を立てる。私の環境ではvTable[82]にDrawIndexedPrimitiveメソッドがあった。（D3D9ならどの環境でも同じ場所にあるだろう。）また、DrawIndexedPrimitiveメソッドは次のようになっていた。
```
DIP+00h : 8BFF    : mov edi, edi
DIP+02h : 55      : push ebp
...
DIP+2Dh : 8965 F0 : mov [ebp-0x10], esp
DIP+30h : 8B75 08 : mov esi, [ebp+0x8]
DIP+33h : 85F6    : test esi, esi
...
```
個人的な流儀で「mov reg, [ebp+0x8]」のところ（pDeviceを取得したところ）にjmpを配置するようにしているが、実行される部分であればどこでもよい。私の環境ではDIPの先頭から0x30の地点にこれがあるので、ここをjmpで上書きする。DIP+30hは3バイトで、DIP+33hは2バイトなので、これで丁度jmp命令の5バイトが上書きされる。（つまり、nopは必要ない。）前に作成した関数を使用すると、フックは次のように書ける。
```
DrawIndexedPrimitive_hook = vTable[82] + 0x30;
DrawIndexedPrimitive_ret = DrawIndexedPrimitive_hook + 5;
MakeMFH((PBYTE)DrawIndexedPrimitive_hook,
        (DWORD)hook_DrawIndexedPrimitive,
        3 + 2);
```
次にhook_DrawIndexedPrimitive関数を作るのだが、ここで注意したいのが、DIPは引数を取るということである。具体的にはDIPは次のように定義される。
```
HRESULT DrawIndexedPrimitive(
    D3DPRIMITIVETYPE Type,
    INT BaseVertexIndex,
    UINT MinIndex,
    UINT NumVertices,
    UINT StartIndex,
    UINT PrimitiveCount
);
```
以前に説明したようにebp+0x8がpDeviceで、第一引数はebp+0xCから始まる。これを考慮したhook_DrawIndexedPrimitive関数は次のようになる。
```
__declspec(naked) void hook_DrawIndexedPrimitive()
{
    static LPDIRECT3DDEVICE9 pDevice;
    static D3DPRIMITIVETYPE pType;
    static INT BaseVertexIndex;
    static UINT MinIndex;
    static UINT NumVertices;
    static UINT StartIndex;
    static UINT PrimitiveCount;
    // pDeviceを取得する
    __asm {
        mov esi, [ebp + 0x08]
        mov pDevice, esi;
    }
    // 各種引数を取得する
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
    // 用意した処理を呼び出す
    my_DrawIndexedPrimitive(pDevice,
                          pType,
                          BaseVertexIndex,
                          MinIndex,
                          NumVertices,
                          StartIndex,
                          PrimitiveCount);
    // 上書きした命令を実行してから元の場所に戻る
    __asm {
        mov esi, [ebp + 0x8]
        test esi, esi
        jmp DrawIndexedPrimitive_ret;
    }
}
```
ところで、このようにアセンブリを書けない方は、Microsoft Researchの提供するDetoursというライブラリを使用することをおすすめする。Detoursでは関数1つでフックが完了する。ただ、今回は具体的な構造を処理を理解してもらいたかったため手動フックした。

### [2] オブジェクトを特定する
これでDIPをフックできるので、あとはpDeviceを使用して特定のオブジェクトのテクスチャを赤色に変更する処理を書けばよい。ここで問題なのが、DrawIndexedPrimitiveメソッドにはオブジェクトを特定できるようなIDなどは引数にないということである。そこで一般的に使用されるのが、引数のNumVertices、PrimitiveCountやpDeviceの持つGetStreamSourceメソッドである。NumVerticesは使用される頂点数である。頂点数はたいていオブジェクトによって異なるため、これを利用してオブジェクトを区別できる。また、GetStreamSourceにより取得できるStrideという変数もオブジェクトの区別によく使われる。さて、次に重要なのは、あるオブジェクトのNumVerticesやStrideを取得することである。これには、一度my_DrawIndexedPrimitiveでOutputDebugStringなどを使ってログを取る方法などがある。
```
TCHAR str[64] = TEXT("");
wsprintf(str,
         TEXT("Stride=%d && NumVertices=%d"),
         Stride,
         NumVertices);
OutputDebugString(str);
```
DebugViewなどでこのログを見るだけでも、オブジェクト特定に繋がる。ちなみにFPSの人間など、部位が細かく分けられているオブジェクトでは複数のNumVerticesを指定しないと部分的にテクスチャを変更してしまう。今回のShadowMap.exeでぐるぐる回っている飛行機をログから特定したところ、Strideは32、NumVerticesは200代の値であった。（今回は飛行機の大部分を赤色に変更するが、細かい部分は元のテクスチャのままである。）

### [3] Wallhackの仕上げ
前節の結果より、my_DrawIndexedPrimitive関数にWallhackのメイン処理を書く。まず、NumVerticesからオブジェクトを特定したら、取得したpDeviceのSetTextureメソッドによりテクスチャを設定する。以下にテクスチャを作成する関数と、my_DrawIndexedPrimitive関数を示す。
```
/*
    テクスチャを生成する
*/
bool GenerateCham(IDirect3DDevice9 * pDevice,
                  LPDIRECT3DTEXTURE9 *texture,
                  DWORD colorARGB)
{
    if (FAILED(pDevice->CreateTexture(8, 8, 1, 0,
                                      D3DFMT_A4R4G4B4,
                                      D3DPOOL_MANAGED,
                                      texture,
                                      NULL)))
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

VOID WINAPI my_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 pDevice,
                                    D3DPRIMITIVETYPE Type,
                                    INT BaseVertexIndex,
                                    UINT MinIndex,
                                    UINT NumVertices,
                                    UINT StartIndex,
                                    UINT PrimitiveCount)
{
    LPDIRECT3DVERTEXBUFFER9 Stream_Data;
    UINT Stride, offset;
    LPDIRECT3DTEXTURE9 chameleon;

    if (pDevice->GetStreamSource(0,
                                 &Stream_Data,
                                 &offset,
                                 &Stride) == D3D_OK) {
        Stream_Data->Release();
    }

    if ((Stride == 32 && (
        NumVertices == 245 || 
        NumVertices == 232 ||
        NumVertices == 256))) {
        // テクスチャを設定する
        GenerateCham(pDevice,
                     &chameleon,
                     D3DCOLOR_ARGB(255, 254, 0, 0));
        pDevice->SetTexture(0, chameleon);
    }
}
```
これら全てをまとめたプログラムは次のURLからダウンロードできる。

<div style="text-align: center;">
    <img src="https://i.imgur.com/yUjiQhi.png)"></img>
    <p>https://github.com/ptr-yudai/jokenoffline/blob/master/wallhack02.cpp</p>
</div>

DLLインジェクションにより作成したDLLをロードさせると、図では確認しにくいが、飛行機のテクスチャが一色に染まった。

<div style="text-align: center;">
    <img src="https://i.imgur.com/IMz5xoT.png" width="70%"></img>
    <p>[6]DIPのフックによるテクスチャの変更</p>
</div>

## 不正対策
特にオンラインゲームなど、一人の不正が不特定多数のプレイヤーに影響を与えてしまう場合、ゲームガードと呼ばれる不正対策機構が使用される。ゲームガードはインジェクションの検知以外にも、あらゆるチートに対する防御機構を備えている。しかし、実際にはゲームガードが実行されるマシンはチート利用者の手中にあるため、必ずチート利用者が有利になってしまう。そのため、現状ではいかにゲームガードの解析や回避を面倒にするかが研究されている。（パッカーや難読化。）

<div style="text-align: center;">
    <img src="https://i.imgur.com/OtWQxps.png"></img>
    <p>[7]ゲームガードに不正行為が検知された例</p>
</div>

ゲームガードに検知されないコードはUD(Undetected)などと呼ばれてインターネットに出回っている。もちろんゲームガードの解析や回避は利用規約によって禁止されているので、決して試さないこと。

## 参考文献
"Mid-Function Hook, that big deal?", <https://www.unknowncheats.me/forum/c-and-c-/67884-mid-function-hook-deal.html>, 2016/01/05アクセス
"Special Force D3D9 (Wallhack, XQZ, Crosshair)", <http://www.unknowncheats.me/forum/other-fps-games/71550-special-force-d3d9-wallhack-xqz-crosshair.html>, 2016/01/04アクセス
"Injectable Mid Function Hook", <http://www.unknowncheats.me/forum/d3d-tutorials-and-source/68900-injectable-mid-function-hook.html>, 2016/01/06
"IDirect3DDevice9 インターフェイス", <https://msdn.microsoft.com/ja-jp/library/cc324252.aspx>, 2016/01/05アクセス
