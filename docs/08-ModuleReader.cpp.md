# 08. ModuleReader.cpp — DLL đang được nạp

Source: [ModuleReader.cpp](../Module_1/ModuleReader.cpp)

## Code hiện tại

~~~cpp
#include "ModuleReader.h"
#include <TlHelp32.h>

bool ReadModules(DWORD pid, std::vector<ModuleInfo>& out, std::wstring& error) {
    out.clear(); error.clear();

    HANDLE snapshot = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 4; i++) {
        snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (snapshot != INVALID_HANDLE_VALUE || GetLastError() != ERROR_BAD_LENGTH) break;
    }

    if (snapshot == INVALID_HANDLE_VALUE) {
        error = L"Loi chup anh DLL"; return false;
    }

    MODULEENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Module32FirstW(snapshot, &entry)) {
        do {
            std::wstring name = entry.szModule;
            if (name.length() >= 4 && _wcsicmp(name.substr(name.length() - 4).c_str(), L".dll") == 0) {
                ModuleInfo m;
                m.name = entry.szModule;
                m.path = entry.szExePath;
                out.push_back(m);
            }
        } while (Module32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return true;
}
~~~

## Giải thích như đang quan sát Task Manager

`CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid)` chụp danh sách module của PID. Hai cờ ghép bằng `|`: lấy module thông thường và thêm module 32 bit khi phù hợp với chương trình x64. Snapshot có thể gồm EXE và DLL. Windows đôi khi trả `ERROR_BAD_LENGTH` khi danh sách đang đổi; code thử tối đa bốn lần.

`MODULEENTRY32W entry{}` là hộp Windows sẽ điền. Phải gán `entry.dwSize = sizeof(entry)`. `Module32FirstW` lấy mục đầu; `Module32NextW` đi qua các mục tiếp theo. `entry.szModule` là tên ngắn; `entry.szExePath` là đường dẫn.

Đoạn `name.length() >= 4` kiểm tra đủ chỗ để lấy bốn ký tự cuối. `name.substr(name.length() - 4)` lấy phần cuối; `_wcsicmp(..., L".dll") == 0` so không phân biệt hoa/thường. Nếu đúng, code chép tên/path vào `ModuleInfo` và thêm vào `out`. Nó lọc như vậy để không đưa `notepad.exe` vào danh sách DLL.

Cuối cùng phải `CloseHandle(snapshot)`. Nếu tạo snapshot thất bại, code đặt lỗi chung `Loi chup anh DLL` và trả `false`; nếu `Module32FirstW` thất bại, code hiện vẫn trả `true` với danh sách rỗng — đây là một giới hạn khi diễn giải kết quả.

## DLL loaded và PE Import

Nếu `notepad.exe` khai báo import DLL A, nhưng lúc chạy gọi `LoadLibrary` để nạp DLL B, snapshot có thể thấy A và B; bảng import trong file có thể chỉ thấy A. Ngược lại, module có thể biến mất hay thay đổi giữa các lần chụp. `ModuleReader` trả lời “**đang nạp gì lúc chụp?**”; `PeImportReader` trả lời “**file EXE khai báo import gì?**”.

## Nguồn và code mẫu

- [Microsoft: ví dụ C/C++ duyệt danh sách module](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes).
- [CreateToolhelp32Snapshot và các cờ module](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot).
- [Module32FirstW](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-module32firstw), [Module Walking](https://learn.microsoft.com/en-us/windows/win32/toolhelp/module-walking).

