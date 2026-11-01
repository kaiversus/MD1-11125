# 04. ProcessFinder.cpp — tìm PID hoặc tên

Source: [ProcessFinder.cpp](../Module_1/ProcessFinder.cpp) · Kiểu kết quả: [02-HEADERS.md](02-HEADERS.md)

## Hình dung trước

Task Manager hiển thị một danh sách các process. File này cũng xin Windows một danh sách tương tự, rồi nhìn từng dòng xem PID/tên có khớp yêu cầu không. Nó không mở process và không đọc command line.

## Code hiện tại

~~~cpp
#include "ProcessFinder.h"
#include <TlHelp32.h>

SearchResult Find(const std::wstring* name, DWORD pid) {
    SearchResult result;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        result.error = GetLastError();
        return result;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            bool match = false;
            if (name != nullptr) {
                match = (CompareStringOrdinal(entry.szExeFile, -1, name->c_str(), -1, TRUE) == CSTR_EQUAL);
            }
            else {
                match = (entry.th32ProcessID == pid);
            }

            if (match) {
                ProcessEntry process;
                process.name = entry.szExeFile;
                process.pid = entry.th32ProcessID;
                process.ppid = entry.th32ParentProcessID;
                result.processes.push_back(process);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    result.error = GetLastError() == ERROR_NO_MORE_FILES ? ERROR_SUCCESS : GetLastError();
    CloseHandle(snapshot);
    return result;
}

SearchResult FindByName(const std::wstring& name) {
    if (name.empty()) return { {}, ERROR_INVALID_PARAMETER };
    return Find(&name, 0);
}

SearchResult FindByPid(DWORD pid) {
    if (pid == 0) return { {}, ERROR_INVALID_PARAMETER };
    return Find(nullptr, pid);
}
~~~

## Giải thích từng đoạn

**`Find(const std::wstring* name, DWORD pid)`:** đây là hàm dùng chung cho cả hai kiểu tìm. Nếu `name != nullptr`, tìm theo tên; nếu `name == nullptr`, tìm theo PID. Con trỏ ở đây được dùng như một “công tắc” tùy chọn. `FindByName` và `FindByPid` ở cuối file chỉ kiểm tra đầu vào rồi gọi `Find`.

**Tạo snapshot:** `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)` hỏi Windows chụp danh sách process. `TH32CS_SNAPPROCESS` nói rõ muốn process; số `0` không chọn một PID riêng ở chế độ này. Thành công trả snapshot handle. Thất bại trả `INVALID_HANDLE_VALUE`, nên code lưu `GetLastError()` và thoát. Đây là **bản chụp**, không phải danh sách luôn cập nhật.

**Chuẩn bị `PROCESSENTRY32W`:** `entry{}` khởi tạo struct. `entry.dwSize = sizeof(entry)` bắt buộc để Windows biết phiên bản/kích thước struct người gọi đang cung cấp. `Process32FirstW` đổ process đầu vào `entry`; `Process32NextW` đổ lần lượt process sau. `do { ... } while (...)` được dùng vì đã có mục đầu từ `FirstW` trước khi lặp.

**So khớp:** `entry.th32ProcessID` là PID; `entry.szExeFile` là tên file EXE ngắn; `entry.th32ParentProcessID` là PPID. Khi tìm tên, `CompareStringOrdinal(..., TRUE)` so không phân biệt hoa/thường. Tham số `-1` nói hai chuỗi kết thúc bằng ký tự 0. Kết quả `CSTR_EQUAL` mới là trùng; hàm này không đơn giản trả `true/false`.

**Lưu kết quả:** mỗi mục khớp được chép vào `ProcessEntry`, rồi `push_back` vào `result.processes`. Code **không `break`** nên nếu ba process cùng tên, cả ba sẽ được trả về.

**Dọn dẹp:** khi duyệt xong, `CloseHandle(snapshot)` trả lại handle. `GetLastError() == ERROR_NO_MORE_FILES` có nghĩa đã hết danh sách bình thường; khi ấy `result.error = ERROR_SUCCESS`. Hãy nhớ `result.processes.empty()` và `result.error` là hai câu hỏi khác nhau: “có mục khớp không?” và “lần liệt kê có lỗi không?”.

## Ví dụ tự theo dõi

Snapshot có ba dòng: `notepad.exe / PID 10`, `pwsh.exe / PID 20`, `notepad.exe / PID 30`. Gọi `FindByName(L"notepad.exe")` sẽ thêm PID 10 và 30. Gọi `FindByPid(20)` chỉ thêm PID 20.

## Giới hạn cần ghi đúng

Process có thể thoát sau khi xuất hiện trong snapshot; vì vậy tìm thấy PID **không bảo đảm** `OpenProcess` sau đó thành công. `FindByName` từ chối chuỗi rỗng; `FindByPid` từ chối PID 0. Code dùng tên file EXE, không so đường dẫn đầy đủ.

## Nguồn và code mẫu

- [Microsoft: ví dụ C/C++ chụp và duyệt process](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes) — xem `CreateToolhelp32Snapshot`, `dwSize`, `Process32First/Next`.
- [CreateToolhelp32Snapshot](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot).
- [CompareStringOrdinal](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-comparestringordinal).

