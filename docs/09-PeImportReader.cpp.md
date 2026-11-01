# 09. PeImportReader.cpp — đọc DLL và hàm trong file EXE

Source: [PeImportReader.cpp](../Module_1/PeImportReader.cpp) · Nền tảng: [01-NEN-TANG.md](01-NEN-TANG.md), phần 7.

## Mục tiêu: đừng nhầm với ModuleReader

File này xem **nội dung file EXE trên đĩa**, không hỏi process đích đang nạp DLL gì. PE Import là một danh sách do trình liên kết ghi vào EXE: “Khi chạy, tôi cần DLL này và hàm này”. Danh sách module đang nạp lúc chạy được lấy ở [ModuleReader.cpp](08-ModuleReader.cpp.md).

## Code hiện tại

~~~cpp
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
~~~

## Sơ đồ một file PE

```text
EXE (được Windows ánh xạ như ảnh trong bộ nhớ)
  DOS header: dấu MZ và vị trí PE header
  PE header: 32/64 bit, kích thước ảnh, thư mục dữ liệu
  Import directory: nhiều dòng; mỗi dòng nói về một DLL
    Name -> tên DLL
    OriginalFirstThunk / FirstThunk -> danh sách hàm hoặc ordinal
      Name -> tên hàm, ví dụ CreateFileW
      Ordinal -> số định danh hàm, ví dụ #12
```

PE dùng từ **RVA** (relative virtual address): số byte tính từ đầu ảnh khi file được ánh xạ như EXE. Nếu `image` trỏ tới đầu ảnh và RVA là 500, `image + 500` trỏ tới vị trí đó trong ảnh. Đây là lý do code dùng `SEC_IMAGE`: Windows làm việc ánh xạ theo layout của PE, giúp không cần tự tìm raw offset của section như cách đọc file byte thô. [PE Format của Microsoft](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [SEC_IMAGE](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw).

## Hàm `ReadPeImports`: mở, ánh xạ, dọn dẹp

1. `out.clear()` xóa dữ liệu cũ. `error` được đặt thông báo chung để nếu bất kỳ bước nào hỏng, người gọi có lời báo.
2. `CreateFileW(path.c_str(), GENERIC_READ, ...)` mở file EXE chỉ để đọc. `path.c_str()` đưa chuỗi C kết thúc bằng `0` cho Windows. Nếu trả `INVALID_HANDLE_VALUE`, file không mở được.
3. `CreateFileMappingW(file, ..., PAGE_READONLY | SEC_IMAGE, ...)` xin Windows tạo một “cách nhìn” file như ảnh PE. `PAGE_READONLY` ở đây là cờ kết hợp theo yêu cầu API; với `SEC_IMAGE`, quyền từng trang ảnh do file EXE quyết định. Sau khi có mapping, code đóng file handle.
4. `MapViewOfFile(mapping, FILE_MAP_READ, ...)` đưa ảnh vào không gian địa chỉ của **chương trình Module 1** và trả con trỏ `image`. Điều này **không có nghĩa** code đang chạy EXE đó như một process mới.
5. `ReadImportsFromImage(image, result)` đọc cấu trúc PE. Sau đó `UnmapViewOfFile(image)` trả lại vùng ánh xạ. Nếu thành công, copy `result` sang `out` và xóa lỗi.

Hãy để ý ba tài nguyên khác nhau: file handle, mapping handle, mapped view. Hai cái đầu đóng bằng `CloseHandle`; view giải phóng bằng `UnmapViewOfFile`. [CreateFileMappingW](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw), [MapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile).

## Hàm `ReadImportsFromImage`: đi từ ngoài vào trong

**1. Kiểm tra MZ và PE.** `IMAGE_DOS_HEADER` ở đầu ảnh có `e_magic`; `IMAGE_DOS_SIGNATURE` là dấu MZ. `e_lfanew` nói PE header bắt đầu ở đâu. `pe->Signature` được so với `IMAGE_NT_SIGNATURE`. Ký hiệu ép kiểu `(IMAGE_DOS_HEADER*)image` nghĩa là “hãy xem các byte này theo hình dạng một DOS header”. Đây là phần khó của C++ hệ thống: bytes không tự biến thành struct; ta phải biết layout của định dạng.

**2. Chọn PE32 hoặc PE32+.** `OptionalHeader.Magic` cho biết định dạng 32 bit hoặc 64 bit. `is64Bit` quyết định dùng `IMAGE_NT_HEADERS64` hay `IMAGE_NT_HEADERS32` để lấy `SizeOfImage` và import data directory. “Optional header” là tên lịch sử; file EXE cần phần này để chạy.

**3. Tìm bảng import.** `imports.VirtualAddress` là RVA bắt đầu bảng; `imports.Size` là kích thước. Nếu RVA bằng 0 thì file không có bảng import theo thông tin code đọc được, hàm trả thành công với danh sách rỗng. `ValidAddress` kiểm tra địa chỉ và số byte cần đọc không vượt `SizeOfImage`.

**4. Mỗi import descriptor là một DLL.** `IMAGE_IMPORT_DESCRIPTOR` có `Name` là RVA tên DLL, và `OriginalFirstThunk` hoặc `FirstThunk` dẫn tới danh sách hàm. Một descriptor rỗng đánh dấu hết danh sách. `memchr(..., 0, ...)` tìm ký tự kết thúc chuỗi; nếu không có ký tự 0 trước giới hạn ảnh, code không đọc chuỗi như tên DLL.

**5. Mỗi thunk là một hàm.** Nếu bit `IMAGE_ORDINAL_FLAG64`/`IMAGE_ORDINAL_FLAG32` bật, giá trị chứa **ordinal**: code viết `#` rồi số. Nếu không, giá trị là RVA tới `IMAGE_IMPORT_BY_NAME`: hai byte đầu là hint, sau đó là tên hàm. Vì vậy code dùng `value + sizeof(WORD)`. `memcpy` chép 4 hoặc 8 byte thunk vào `value`; `thunkSize` phụ thuộc PE 32/64 bit. Giá trị 0 đánh dấu hết danh sách hàm của DLL ấy.

**6. Thêm kết quả.** Mỗi `ImportInfo item` có tên DLL và `vector` tên hàm. `out.push_back(item)` đưa nó vào kết quả. Sau này `ReportWriter.cpp` mới đổi `std::string` thành chuỗi để in.

## Ví dụ giấy bút

Giả sử import directory chứa một descriptor: `Name RVA = 1000` → `KERNEL32.dll`; `OriginalFirstThunk RVA = 2000` → hai mục: mục thứ nhất trỏ tới tên `CreateFileW`, mục thứ hai bằng 0. Kết quả là một `ImportInfo`: `dll = "KERNEL32.dll"`, `functions = {"CreateFileW"}`. Số 1000/2000 chỉ là ví dụ để hiểu mũi tên, không phải địa chỉ thật của Notepad.

## Giới hạn code hiện tại

`ValidAddress` chỉ so với `SizeOfImage`; code còn dựa vào việc Windows chấp nhận và ánh xạ file dưới dạng ảnh PE. Nó không phải bộ phân tích mọi file PE hỏng/độc hại một cách đầy đủ. Code chỉ đọc **import directory thông thường**; PE còn có delay import và các cơ chế nạp DLL lúc chạy. Không nên viết “danh sách này là tất cả DLL mà process sẽ dùng”. Với file không thể ánh xạ bằng `SEC_IMAGE`, hàm trả lỗi chung.

## Tự học bằng cách hỏi “địa chỉ ở đâu?”

Khi thấy `image + desc.Name`, `image + address`, `image + value + sizeof(WORD)`, hãy ghi cạnh chúng: “đây là địa chỉ trong **ảnh PE đã ánh xạ ở process của mình**”. Nó khác `PebReader.cpp`, nơi địa chỉ thuộc **process đích**, phải dùng `ReadProcessMemory`.

## Nguồn và code/tài liệu liên quan

- [Microsoft PE Format: Import Directory, Lookup Table, Hint/Name Table](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) — đặc tả chính thức có bảng cấu trúc.
- [CreateFileMappingW, gồm giải thích SEC_IMAGE](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw).
- [MapViewOfFile](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-mapviewoffile).

