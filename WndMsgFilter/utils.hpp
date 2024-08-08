#pragma once

#include <format>
#include <string>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define LOG_DEBUG(msg, ...) OutputDebugStringW(std::format(L"[WndMsgFilter] " msg, __VA_ARGS__).c_str())

struct SrwExclusiveLock {
    SRWLOCK* theLock;

    SrwExclusiveLock(SRWLOCK& lock) noexcept
        : theLock{ &lock }
    {
        AcquireSRWLockExclusive(theLock);
    }

    ~SrwExclusiveLock() {
        ReleaseSRWLockExclusive(theLock);
    }
};

struct SrwSharedLock {
    SRWLOCK* theLock;

    SrwSharedLock(SRWLOCK& lock) noexcept
        : theLock{ &lock }
    {
        AcquireSRWLockShared(theLock);
    }

    ~SrwSharedLock() {
        ReleaseSRWLockShared(theLock);
    }
};

inline std::wstring GetLastErrorStr() noexcept {
    LPWSTR msgBuf;
    size_t size;

    // https://stackoverflow.com/a/17387176
    DWORD errId = ::GetLastError();
    if (errId == 0)
        return {};

    msgBuf = nullptr;
    size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&msgBuf), 0, NULL);

    std::wstring msg(msgBuf, size);
    LocalFree(msgBuf);
    return msg;
}
