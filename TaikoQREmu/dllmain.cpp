// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppClangTidyClangDiagnosticMicrosoftCast
#include <cstdint>
#include <filesystem>
#include <Windows.h>
#include <MinHook.h>
#include <iostream>
#include <vector>
#include <fstream>

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

enum class Mode
{
    Card,
    Data
};

State gState = State::Ready;
KeyState gKeyState = KeyState::Left;
Mode gMode = Mode::Card;

char __fastcall qrInit(int64_t a1)
{
    // std::cout << "qrInit" << std::endl;
    return 1;
}

char __fastcall qrRead(int64_t a1)
{
    *reinterpret_cast<DWORD*>(a1 + 40) = 1;
    *reinterpret_cast<DWORD*>(a1 + 16) = 1;
    *reinterpret_cast<BYTE*>(a1 + 112) = 0;
    return 1;
}

char __fastcall qrClose(int64_t a1)
{
    // std::cout << "qrClose" << std::endl;
    return 1;
}

int64_t __fastcall callQrUnknown(int64_t a1)
{
    //std::cout << "callQrUnknown, state: " << static_cast<int>(gState) << std::endl;
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
            return 1;
        }
    default: // NOLINT(clang-diagnostic-covered-switch-default)
        return 0;
    }
}

bool __fastcall Send1(int64_t, const void*, int64_t)
{
    //std::cout << "Send1" << std::endl;
    return true;
}

bool __fastcall Send2(int64_t a1, char a2)
{
    //std::cout << "Send2" << std::endl;
    return true;
}

bool __fastcall Send3(int64_t a1)
{
    //std::cout << "Send3" << std::endl;
    *(WORD*)(a1 + 88) = 0;
    *(BYTE*)(a1 + 90) = 0;
    return true;
}

bool __fastcall Send4(int64_t a1)
{
    //std::cout << "Send4" << std::endl;
    *(BYTE*)(a1 + 88) = 1;
    *(int64_t*)(a1 + 32) = *(int64_t*)(a1 + 24);
    *(WORD*)(a1 + 89) = 0;
    return true;
}

std::vector<BYTE> readFileToByteBuffer(const std::string& relativePath) {
    // Check if the file exists
    if (!std::filesystem::exists(relativePath)) {
        throw std::runtime_error("Qr file does not exist.");
    }

    // Read the file into a string
    std::ifstream file(relativePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open the file.");
    }

    const std::string content((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());
    file.close();

    // Calculate text length (assuming it fits into a single byte for this example)
    if (content.size() > 255) {
        throw std::runtime_error("Text is too long");
    }
    BYTE text_length = static_cast<BYTE>(content.size());

    // Create the final byte buffer
    std::vector<BYTE> byteBuffer = {
        0x53, 0x31, 0x32, 0x00, 0x00, 0xff, 0xff, text_length, 0x01, 0x00
    };

    // Append the content bytes
    for (char c : content) {
        byteBuffer.push_back(static_cast<BYTE>(c));
    }

    // Append the ending bytes
    byteBuffer.push_back(0xee);
    byteBuffer.push_back(0xff);

    return byteBuffer;
}

int64_t __fastcall copy_data(int64_t this_, void* dest, int length)
{
    if (gState == State::CopyWait && gMode == Mode::Card)
    {
        std::cout << "Copy data, length: " << length << std::endl;
        const std::string data =
            "BNTTCNID1";
        memcpy(dest, data.c_str(), data.size() + 1);
        gState = State::AfterCopy1;
        return data.size() + 1;
    }
    if (gState == State::CopyWait && gMode == Mode::Data)
    {
        std::cout << "Copy data, length: " << length << std::endl;
        auto data = readFileToByteBuffer("plugins/qr.txt");
        memcpy(dest, data.data(), data.size());
        gState = State::AfterCopy1;
        return data.size();
    }
    //std::cout << "No copy" << std::endl;
    return 0;
}

int gCount = 0;

extern "C" __declspec(dllexport) void Update()
{
    if (GetAsyncKeyState('P') & 0x8000)
    {
        gMode = Mode::Card;
        if (gKeyState == KeyState::Left)
        {
            gKeyState = KeyState::Push;
        }
        else
        {
            gKeyState = KeyState::Pushing;
        }
    }
    else if (GetAsyncKeyState('O') & 0x8000)
    {
        gMode = Mode::Data;
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
