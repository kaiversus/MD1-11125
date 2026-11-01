# 06. PebReader.cpp — lấy command line từ bộ nhớ process khác

Source: [PebReader.cpp](../Module_1/PebReader.cpp) · Đọc trước: [01-NEN-TANG.md](01-NEN-TANG.md), phần 4–6.

## Câu chuyện một câu

Windows giữ thông tin khi khởi chạy process trong một cấu trúc gọi là **PEB**. Code tìm PEB của process đích, đi tới `ProcessParameters`, rồi tới `CommandLine`, mỗi lần dùng `ReadProcessMemory` để **sao chép** dữ liệu từ process đích sang chương trình của mình.

## Code hiện tại

~~~cpp
#include "PEBReader.h"
#include <winternl.h>
#include <cstdint>

struct Unicode32 { USHORT length; USHORT capacity; uint32_t buffer; };
struct Peb32 { uint8_t reserved[0x10]; uint32_t parameters; };
struct Parameters32 { uint8_t reserved[0x40]; Unicode32 command; };
using QueryFn = NTSTATUS(NTAPI*)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

bool ReadPebCommandLine(HANDLE process, bool wow64, std::wstring& commandLine, std::wstring& error) {
    commandLine.clear();
    error.clear();
    SIZE_T bytesRead = 0;

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto query = (QueryFn)GetProcAddress(ntdll, "NtQueryInformationProcess");
    if (!query) { error = L"Khong the tai NtQueryInformationProcess"; return false; }

    if (wow64) {
        ULONG_PTR peb32Address = 0;
        if (query(process, ProcessWow64Information, &peb32Address, sizeof(peb32Address), nullptr) < 0 || peb32Address == 0) {
            error = L"Loi ProcessWow64Information"; return false;
        }

        Peb32 peb32{};
        ReadProcessMemory(process, (LPCVOID)peb32Address, &peb32, sizeof(peb32), &bytesRead);

        Parameters32 params32{};
        ReadProcessMemory(process, (LPCVOID)(uintptr_t)peb32.parameters, &params32, sizeof(params32), &bytesRead);

        if (params32.command.length > 0 && params32.command.buffer != 0) {
            commandLine.assign(params32.command.length / sizeof(wchar_t), L'\0');
            ReadProcessMemory(process, (LPCVOID)(uintptr_t)params32.command.buffer, commandLine.data(), params32.command.length, &bytesRead);
        }
    }
    else {
        PROCESS_BASIC_INFORMATION basic{};
        if (query(process, ProcessBasicInformation, &basic, sizeof(basic), nullptr) < 0) {
            error = L"Loi ProcessBasicInformation"; return false;
        }

        PEB peb{};
        ReadProcessMemory(process, basic.PebBaseAddress, &peb, sizeof(peb), &bytesRead);

        RTL_USER_PROCESS_PARAMETERS params{};
        ReadProcessMemory(process, peb.ProcessParameters, &params, sizeof(params), &bytesRead);

        if (params.CommandLine.Length > 0 && params.CommandLine.Buffer != nullptr) {
            commandLine.assign(params.CommandLine.Length / sizeof(wchar_t), L'\0');
            ReadProcessMemory(process, params.CommandLine.Buffer, commandLine.data(), params.CommandLine.Length, &bytesRead);
        }
    }
    return true;
}
~~~

## Những kiểu lạ ở đầu file

- `Unicode32` mô tả một chuỗi kiểu 32 bit: `length` là số byte đang dùng, `capacity` là sức chứa, `buffer` là địa chỉ 32 bit của ký tự. Nó giống ý tưởng `UNICODE_STRING` nhưng dùng `uint32_t` cho con trỏ.
- `Peb32` và `Parameters32` chỉ chứa các byte `reserved` để đi tới đúng vị trí trường cần đọc. `reserved[0x10]` nghĩa là bỏ qua 16 byte đầu; `reserved[0x40]` là 64 byte. Đây **không phải** định nghĩa đầy đủ của PEB và ProcessParameters. Các vị trí này phụ thuộc cách Windows bố trí struct, nên dễ vỡ nếu giả định không còn đúng.
- `using QueryFn = ...` đặt tên ngắn cho **kiểu con trỏ hàm** của `NtQueryInformationProcess`. `NTSTATUS` là kiểu mã kết quả; `NTAPI` là quy ước gọi hàm. Bạn không cần thuộc cú pháp con trỏ hàm ngay: chỉ cần hiểu `query(...)` sau đó gọi API tìm được.
- `SIZE_T bytesRead` là biến Windows điền số byte đã sao chép.

## Vì sao `GetModuleHandleW` rồi `GetProcAddress`?

`ntdll.dll` có `NtQueryInformationProcess`. `GetModuleHandleW(L"ntdll.dll")` lấy handle của DLL đã nạp vào **chính chương trình này**; `GetProcAddress(...)` tìm địa chỉ hàm trong DLL đó. Kết quả được ép sang `QueryFn`, rồi gọi như hàm bình thường. Microsoft cảnh báo API này là giao diện nội bộ có thể thay đổi; tra động như vậy cho phép phát hiện trường hợp không tìm thấy hàm.

## Nhánh WOW64: process đích 32 bit

1. `query(process, ProcessWow64Information, &peb32Address, ...)` lấy địa chỉ PEB 32 bit của process đích. `peb32Address` là **địa chỉ trong process đích**, không phải một `Peb32` đã nằm trong chương trình ta.
2. `ReadProcessMemory(...peb32Address, &peb32, sizeof(peb32), ...)` sao chép các byte PEB cần dùng sang biến `peb32`.
3. `peb32.parameters` là địa chỉ vùng tham số của process đích. Lần `ReadProcessMemory` thứ hai sao chép nó vào `params32`.
4. `params32.command.buffer` là địa chỉ ký tự command line trong process đích. Code tạo `std::wstring` dài `length / sizeof(wchar_t)` rồi lần đọc thứ ba chép ký tự vào `.data()`.

Ví dụ đơn giản: PEB ở “ô” 100, trong đó ghi địa chỉ parameters là 500; ô 500 ghi địa chỉ chữ là 900. Chương trình phải đọc ô 100, rồi 500, rồi 900. Đọc thẳng ô 900 trong bộ nhớ **của mình** sẽ sai.

## Nhánh không WOW64: process đích 64 bit

`ProcessBasicInformation` trả `PROCESS_BASIC_INFORMATION`, trong đó có `PebBaseAddress`. Sau đó code đọc `PEB`, đọc `RTL_USER_PROCESS_PARAMETERS`, rồi đọc `CommandLine.Buffer`. Ý tưởng hoàn toàn giống nhánh 32 bit; điểm khác là kiểu con trỏ và cấu trúc có kích thước 64 bit.

## Các điều kiện và giới hạn quan trọng

- `Length` của `UNICODE_STRING` là **byte**, `std::wstring::assign` cần **số ký tự**. Vì vậy chia cho `sizeof(wchar_t)`.
- Code kiểm tra kết quả của `NtQueryInformationProcess`, nhưng **không kiểm tra kết quả của từng `ReadProcessMemory`**. Nếu process thoát hoặc vùng nhớ không đọc được ở giữa, hàm vẫn có thể trả `true` với chuỗi rỗng/sai. Trong report, mô tả đây là giới hạn của phiên bản hiện tại, đừng viết “đã xử lý mọi lỗi đọc PEB”.
- `ProcessParameters`, PEB và offset ở nhánh 32 bit là cấu trúc nội bộ của Windows. Đây là cách học/khảo sát cấu trúc hệ thống, không nên xem như API ổn định tuyệt đối cho mọi phiên bản.
- Command line là dữ liệu của process đích; nó khác `argv[1]` của chương trình Module 1.

## Tự tập viết lại

Không nhìn code, hãy vẽ ba mũi tên: `PEB -> ProcessParameters -> CommandLine.Buffer`. Sau đó viết giả mã cho ba lần `ReadProcessMemory`. Khi nhìn lại code, xác định biến nào là **địa chỉ ở process đích**, biến nào là **bản sao cục bộ**.

## Nguồn

- [NtQueryInformationProcess, ProcessBasicInformation và ProcessWow64Information](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess).
- [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory).
- [GetProcAddress](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress).

