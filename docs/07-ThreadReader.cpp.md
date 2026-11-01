# 07. ThreadReader.cpp — các luồng công việc trong process

Source: [ThreadReader.cpp](../Module_1/ThreadReader.cpp)

## Hình dung

Một process là một ứng dụng đang chạy; thread là các “người làm việc” bên trong nó. Mỗi thread có ID riêng. Trạng thái `Running`/`Waiting` là **ảnh chụp tức thời**: thread có thể đổi trạng thái ngay sau khi code đọc.

## Code hiện tại

~~~cpp
#include "ThreadReader.h"
#include <winternl.h>

using QuerySystem = NTSTATUS(NTAPI*)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

std::wstring GetState(ULONG state, ULONG waitReason) {
    if (state == 5) return (waitReason == 5 || waitReason == 12) ? L"Suspended" : L"Waiting";
    switch (state) {
    case 0: return L"Initialized"; case 1: return L"Ready";
    case 2: return L"Running"; case 3: return L"Standby";
    case 4: return L"Terminated"; case 6: return L"Transition";
    case 7: return L"DeferredReady"; default: return L"Unknown";
    }
}

bool ReadThreads(DWORD pid, std::vector<ThreadInfo>& out, std::wstring& error) {
    out.clear(); error.clear();
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    auto query = (QuerySystem)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!query) return false;

    std::vector<uint8_t> buffer(1024 * 1024);
    ULONG needed = 0;

    while (query(SystemProcessInformation, buffer.data(), (ULONG)buffer.size(), &needed) == (NTSTATUS)0xC0000004) {
        buffer.resize(buffer.size() * 2);
    }

    size_t pos = 0;
    while (pos < buffer.size()) {
        auto* process = (SYSTEM_PROCESS_INFORMATION*)(buffer.data() + pos);

        if ((DWORD)(uintptr_t)process->UniqueProcessId == pid) {
            size_t threadStart = pos + sizeof(SYSTEM_PROCESS_INFORMATION);
            for (ULONG i = 0; i < process->NumberOfThreads; i++) {
                SYSTEM_THREAD_INFORMATION tInfo;
                memcpy(&tInfo, buffer.data() + threadStart + (i * sizeof(tInfo)), sizeof(tInfo));

                ThreadInfo item;
                item.id = (DWORD)(uintptr_t)tInfo.ClientId.UniqueThread;
                item.state = GetState(tInfo.ThreadState, tInfo.WaitReason);
                out.push_back(item);
            }
            return true;
        }

        if (process->NextEntryOffset == 0) break;
        pos += process->NextEntryOffset;
    }

    error = L"Khong lay duoc Threads";
    return false;
}
~~~

## `GetState`: đổi số thành chữ

Windows trả `ThreadState` và `WaitReason` dưới dạng số. `GetState` dùng `switch` để đổi số thành nhãn dễ đọc. Ví dụ `state == 2` → `Running`. Khi `state == 5`, code xét thêm `waitReason`: giá trị 5 hoặc 12 được ghi `Suspended`, còn lại ghi `Waiting`. Đây là **cách diễn giải theo các số code hiện dùng**, không phải lời khẳng định mọi phiên bản Windows hoặc mọi lý do chờ đều được phân loại chính xác. Giá trị lạ → `Unknown`.

## `ReadThreads`: từng bước

1. `out.clear(); error.clear();` xóa kết quả/lỗi cũ.
2. Lấy `NtQuerySystemInformation` từ `ntdll.dll` bằng `GetProcAddress`. `QuerySystem` là tên kiểu con trỏ hàm. Nếu không tìm thấy, code trả `false`.
3. Tạo `buffer` 1 MiB. `SystemProcessInformation` yêu cầu Windows điền thông tin mọi process vào bộ đệm đó.
4. Nếu kết quả là `0xC0000004` (`STATUS_INFO_LENGTH_MISMATCH`: bộ đệm quá nhỏ), tăng gấp đôi rồi hỏi lại. `needed` là chỗ API có thể báo kích thước cần dùng, nhưng code hiện tại không dùng con số đó.
5. Buffer gồm nhiều khối process. Mỗi `SYSTEM_PROCESS_INFORMATION` có PID (`UniqueProcessId`), số thread, và `NextEntryOffset` dẫn tới khối process tiếp theo. `pos += NextEntryOffset` chính là bước nhảy qua khối hiện tại.
6. Khi PID khớp, code lấy các `SYSTEM_THREAD_INFORMATION` theo sau phần thông tin process, đọc `ClientId.UniqueThread` làm thread ID và gọi `GetState` để có nhãn. `memcpy` chép từng struct ra biến `tInfo` cục bộ.

## Chỗ cần đặc biệt cẩn thận

- `SYSTEM_PROCESS_INFORMATION` / `SYSTEM_THREAD_INFORMATION` và `NtQuerySystemInformation` là giao diện nội bộ; Microsoft nói cấu trúc có thể đổi. Code phụ thuộc bố cục hiện tại.
- Vòng tăng bộ đệm hiện tại chỉ nhận diện một mã lỗi cụ thể và **không kiểm tra đầy đủ các lỗi khác**, kích thước trả về hoặc giới hạn vùng nhớ trước mọi lần `memcpy`. Vì thế không nên mô tả nó là parser chống mọi lỗi.
- File này **không đọc TEB**. TEB là dữ liệu thuộc thread, nhưng nhãn trạng thái scheduler ở đây lấy từ `SystemProcessInformation`. Đừng ghi trong report “lấy thread state từ TEB”.
- `ReadThreads` có thể trả `false` nhưng `main.cpp` hiện không hiển thị lỗi đó.

## Bài tập

Giả sử buffer có ba process: PID 10 ở `pos=0`, PID 20 ở `pos=100`, PID 30 ở `pos=250`. `NextEntryOffset` của PID 10 là 100, của PID 20 là 150. Hãy tự tính `pos` sau hai vòng lặp. Vì sao không thể đơn giản `pos++`?

## Nguồn

- [NtQuerySystemInformation và bố cục SYSTEM_PROCESS_INFORMATION/SYSTEM_THREAD_INFORMATION](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation).
- [Ví dụ Microsoft duyệt thread bằng Tool Help](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes) — cách khác để liệt kê ID, không thay thế trực tiếp phần state của code này.

