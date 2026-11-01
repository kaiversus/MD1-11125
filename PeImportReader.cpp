#include "PeImportReader.h"
#include <windows.h>
#include <cstring>
#include <vector>

// Kiem tra dia chi truoc khi doc du lieu trong EXE.
bool ValidAddress(unsigned long long rva, size_t length, DWORD imageSize) {
    return rva <= imageSize && length <= imageSize - rva;
}

bool ReadImportsFromImage(BYTE* image, std::vector<ImportInfo>& out) {
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)image;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0) return false;
    IMAGE_NT_HEADERS64* pe = (IMAGE_NT_HEADERS64*)(image + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE) return false;

    bool is64Bit = pe->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    if (!is64Bit && pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) return false;
    DWORD imageSize = pe->OptionalHeader.SizeOfImage;
    IMAGE_DATA_DIRECTORY imports;
    if (is64Bit) {
        imports = pe->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    } else {
        IMAGE_NT_HEADERS32* pe32 = (IMAGE_NT_HEADERS32*)pe;
        imageSize = pe32->OptionalHeader.SizeOfImage;
        imports = pe32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    }
    if (imports.VirtualAddress == 0) return true;
    if (!ValidAddress(imports.VirtualAddress, imports.Size, imageSize)) return false;

    size_t count = imports.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    IMAGE_IMPORT_DESCRIPTOR* descriptors = (IMAGE_IMPORT_DESCRIPTOR*)(image + imports.VirtualAddress);
    for (size_t i = 0; i < count; i++) {
        IMAGE_IMPORT_DESCRIPTOR& desc = descriptors[i];
        if (desc.Name == 0 && desc.FirstThunk == 0) return true;
        if (!ValidAddress(desc.Name, 1, imageSize)) return false;
        char* dllName = (char*)(image + desc.Name);
        if (!memchr(dllName, 0, imageSize - desc.Name)) return false;

        ImportInfo item;
        item.dll = dllName;
        DWORD thunkRva = desc.OriginalFirstThunk;
        if (thunkRva == 0) thunkRva = desc.FirstThunk;
        size_t thunkSize = is64Bit ? sizeof(IMAGE_THUNK_DATA64) : sizeof(IMAGE_THUNK_DATA32);
        bool finished = false;
        for (size_t j = 0; j < imageSize / thunkSize; j++) {
            unsigned long long address = (unsigned long long)thunkRva + j * thunkSize;
            if (!ValidAddress(address, thunkSize, imageSize)) return false;
            unsigned long long value = 0;
            memcpy(&value, image + address, thunkSize);
            if (value == 0) { finished = true; break; }

            if (value & (is64Bit ? IMAGE_ORDINAL_FLAG64 : IMAGE_ORDINAL_FLAG32)) {
                item.functions.push_back("#" + std::to_string(value & 0xffff));
            } else {
                if (!ValidAddress(value, sizeof(WORD) + 1, imageSize)) return false;
                char* functionName = (char*)(image + value + sizeof(WORD));
                if (!memchr(functionName, 0, imageSize - value - sizeof(WORD))) return false;
                item.functions.push_back(functionName);
            }
        }
        if (!finished) return false;
        out.push_back(item);
    }
    return false;
}

bool ReadPeImports(const std::wstring& path, std::vector<ImportInfo>& out, std::wstring& error) {
    out.clear();
    error = L"Khong doc duoc PE Import";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
    CloseHandle(file);
    if (mapping == nullptr) return false;
    BYTE* image = (BYTE*)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if (image == nullptr) return false;

    std::vector<ImportInfo> result;
    bool success = ReadImportsFromImage(image, result);
    UnmapViewOfFile(image);
    if (success) {
        out = result;
        error.clear();
    }
    return success;
}
