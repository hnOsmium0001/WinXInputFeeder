#include "pch.hpp"

#include "utils.hpp"

std::wstring Utf8ToWide(std::string_view utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring result;
    result.resize_and_overwrite(
        len,
        [utf8](wchar_t* buf, size_t bufSize) { return MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), buf, static_cast<int>(bufSize)); });
    return result;
}

std::string WideToUtf8(std::wstring_view wide) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string result;
    result.resize_and_overwrite(
        len,
        [wide](char* buf, size_t bufSize) { return WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), buf, static_cast<int>(bufSize), nullptr, nullptr); });
    return result;

}

static bool GetLastErrorMessage(LPWSTR& msgBuf, size_t& size) noexcept {
    // https://stackoverflow.com/a/17387176
    DWORD errId = ::GetLastError();
    if (errId == 0)
        return false;

    msgBuf = nullptr;
    size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&msgBuf), 0, NULL);

    return true;
}

std::wstring GetLastErrorStr() noexcept {
    LPWSTR msgBuf;
    size_t size;
    if (!GetLastErrorMessage(msgBuf, size))
        return {};

    std::wstring msg(msgBuf, size);
    LocalFree(msgBuf);
    return msg;
}

std::string GetLastErrorStrUtf8() noexcept {
    LPWSTR msgBuf;
    size_t size;
    if (!GetLastErrorMessage(msgBuf, size))
        return {};

    std::string msg(WideToUtf8(std::wstring_view(msgBuf, size)));
    LocalFree(msgBuf);
    return msg;
}

toml::table toml::parse_file(const std::filesystem::path& path) {
    // Modified from toml::parse_file()

    std::ifstream file;
    char fileBuffer[sizeof(void*) * 1024];
    file.rdbuf()->pubsetbuf(fileBuffer, sizeof(fileBuffer));
    // This should use the -W version of CreateFile, etc. because open() takes an overload that handles fs::path directly
    // Unlike toml++, which doesn't take fs::path, sowe have to pass in std::wstring, which then gets converted to UTF-8 nicely but then relies on the codepage for the -A versions
    file.open(path, std::ios::in | std::ios::binary);
    if (!file.is_open())
        throw toml::parse_error("File could not be opened for reading", source_position{}, std::make_shared<const std::string>(path.string()));

    return toml::parse(file);
}
