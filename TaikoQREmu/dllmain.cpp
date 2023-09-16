// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppClangTidyClangDiagnosticMicrosoftCast
#include <cstdint>
#include <Windows.h>
#include <MinHook.h>
#include <iostream>

enum class State
{
    Ready,
    CopyWait,
    AfterCopy1,
    AfterCopy2,
};

enum class KeyState
{
    Left,
    Push,
    Pushing
};

State gState = State::Ready;
KeyState gKeyState = KeyState::Left;

char __fastcall qrInit(int64_t a1)
{
    // std::cout << "qrInit" << std::endl;
    return 1;
}

char __fastcall qrRead(int64_t a1)
{
    //auto type = *(DWORD*)(a1 + 16);
    // std::cout << "qrRead: type " << type << std::endl;
    *reinterpret_cast<DWORD*>(a1 + 40) = 1;
    *reinterpret_cast<DWORD*>(a1 + 16) = 1;
    return 1;
}

char __fastcall qrClose(int64_t a1)
{
    // std::cout << "qrClose" << std::endl;
    return 1;
}

int64_t __fastcall callQrUnknown(int64_t a1)
{
    // std::cout << "callQrUnknown" << std::endl;
    switch (gState)
    {
    case State::Ready:
    case State::CopyWait:
        {
            // std::cout << "Ready" << std::endl;
            return 1;
        }
    case State::AfterCopy1:
        {
            // std::cout << "AfterCopy1" << std::endl;
            gState = State::AfterCopy2;
            return 1;
        }
    case State::AfterCopy2:
        {
            // std::cout << "AfterCopy2" << std::endl;
            return 2;
        }
    default: // NOLINT(clang-diagnostic-covered-switch-default)
        return 0;
    }
}

bool __fastcall Send1(int64_t, const void*, int64_t)
{
    // std::cout << "Send1" << std::endl;
    return true;
}

bool __fastcall Send2(int64_t a1, char a2)
{
    // std::cout << "Send2" << std::endl;
    return true;
}

bool __fastcall Send3(int64_t a1)
{
    // std::cout << "Send3" << std::endl;
    *(WORD*)(a1 + 88) = 0;
    *(BYTE*)(a1 + 90) = 0;
    return true;
}

bool __fastcall Send4(int64_t a1)
{
    // std::cout << "Send4" << std::endl;
    *(BYTE*)(a1 + 88) = 1;
    *(int64_t*)(a1 + 32) = *(int64_t*)(a1 + 24);
    *(WORD*)(a1 + 89) = 0;
    return true;
}

int64_t __fastcall copy_data(int64_t this_, void* dest, int length)
{
    if (gState == State::CopyWait)
    {
        std::cout << "Copy data, length: " << length << std::endl;
        std::string data =
            "BNTTCNID1";
        memcpy(dest, data.c_str(), data.size() + 1);
        gState = State::AfterCopy1;
        return data.size() + 1;
    }
    // std::cout << "No copy" << std::endl;
    return 0;
}

int gCount = 0;

extern "C" __declspec(dllexport) void Update()
{
    if (GetAsyncKeyState('P') & 0x8000)
    {
        if (gKeyState == KeyState::Left)
        {
            gKeyState = KeyState::Push;
        }
        else
        {
            gKeyState = KeyState::Pushing;
        }
    }
    else
    {
        gKeyState = KeyState::Left;
    }

    if (gState == State::AfterCopy2)
    {
        gCount++;
        if (gCount > 10)
        {
            gCount = 0;
            gState = State::Ready;
        }
    }
    if (gState == State::Ready)
    {
        if (gKeyState == KeyState::Push)
        {
            std::cout << "Insert" << std::endl;
            gState = State::CopyWait;
        }
    }
}

extern "C" __declspec(dllexport) void Init()
{
    // std::cout << "PreInit" << std::endl;
    auto amHandle = reinterpret_cast<uintptr_t>(GetModuleHandle(L"AMFrameWork.dll"));
    MH_Initialize();
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x161B0), qrInit, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x163A0), qrRead, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x16350), qrClose, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x8F60), callQrUnknown, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x16A30), Send1, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x16A00), Send2, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x16990), Send3, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x16940), Send4, nullptr);
    MH_CreateHook(reinterpret_cast<LPVOID>(amHandle + 0x169D0), copy_data, nullptr);
    MH_EnableHook(nullptr);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
