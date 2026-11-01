# 02. Tất cả header (.h): bản đồ dữ liệu và lời hứa của các hàm

## Header là gì?

`.h` là nơi **giới thiệu** kiểu dữ liệu và chữ ký hàm cho các file khác. `.cpp` là nơi viết **cách làm** của hàm. Ví dụ header nói `bool ReadModules(DWORD pid, std::vector<ModuleInfo>& out, std::wstring& error);`; `ModuleReader.cpp` mới có phần tìm DLL thực sự. `#include "ModuleReader.h"` giống như cho file hiện tại xem bản giới thiệu đó. `#pragma once` giúp một header chỉ được đưa vào một lần trong mỗi lượt biên dịch.

Ở các chữ ký dưới đây, `out` là biến nhận dữ liệu, `error` là biến nhận lời báo lỗi. Chúng có `&` nên hàm có thể sửa biến do người gọi đưa vào. `const` trước `&` nghĩa là chỉ đọc.

## 1. ProjectData.h — các “hộp” dữ liệu chung

Source: [ProjectData.h](../Module_1/ProjectData.h)

~~~cpp
#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct ProcessEntry {
    std::wstring name;
    DWORD pid = 0;
    DWORD ppid = 0;
};

struct ThreadInfo {
    DWORD id = 0;
    std::wstring state;
};

struct ModuleInfo {
    std::wstring name;
    std::wstring path;
};

struct ImportInfo {
    std::string dll;
    std::vector<std::string> functions;
};

struct ProcessReport {
    ProcessEntry process;
    std::wstring architecture = L"unavailable";
    std::wstring path = L"unavailable";
    std::wstring commandLine = L"unavailable";
    std::vector<ThreadInfo> threads;
    std::vector<ModuleInfo> modules;
    std::vector<ImportInfo> imports;
};
~~~

### Giải thích

- `ProcessEntry`: một process tìm được. `name` là tên như `notepad.exe`, `pid` là ID của nó, `ppid` là ID process cha. `DWORD` là số nguyên không âm 32 bit của Windows; `= 0` tạo giá trị ban đầu.
- `ThreadInfo`: `id` là thread ID; `state` là nhãn chữ như `Running` hoặc `Waiting`.
- `ModuleInfo`: một module/DLL **đang nạp**: tên ngắn và đường dẫn đầy đủ. Kiểu `std::wstring` hợp với các API hậu tố `W`.
- `ImportInfo`: một DLL được ghi trong **bảng import của file EXE**. `functions` là danh sách tên hàm của DLL đó. PE lưu tên này như chuỗi byte, nên code dùng `std::string` rồi chuyển khi in.
- `ProcessReport`: gom mọi kết quả của **một PID**. `process` chứa thông tin cơ bản; ba `vector` chứa danh sách thread, module và import. `unavailable` là giá trị ban đầu để báo “chưa lấy được”, tránh để ô trống không rõ nguyên nhân.

Ví dụ tự tạo một báo cáo: `ProcessReport r; r.process.pid = 1234; r.threads.push_back(ThreadInfo{5678, L"Running"});`. Bây giờ `r.threads` có một phần tử. Đây là mô hình dữ liệu, chưa gọi Windows API.

## 2. ProcessFinder.h — kết quả tìm process và hai cách tìm

Source: [ProcessFinder.h](../Module_1/ProcessFinder.h)

~~~cpp
#pragma once
#include "ProjectData.h"

struct SearchResult {
    std::vector<ProcessEntry> processes;
    DWORD error = ERROR_SUCCESS;
};

SearchResult FindByName(const std::wstring& name);
SearchResult FindByPid(DWORD pid);
~~~

`SearchResult` có `processes` (có thể 0, 1 hoặc nhiều mục) và `error` là mã lỗi Windows. `FindByName` nhận tên; `FindByPid` nhận PID. Chỉ hàm theo tên mới có khả năng trả nhiều process cùng tên. `ERROR_SUCCESS` là mã 0 của Windows, không có nghĩa là chắc chắn tìm được process: danh sách có thể rỗng mà lần chụp vẫn thành công.

## 3. ProcessReader.h — kiến trúc và đường dẫn

Source: [ProcessReader.h](../Module_1/ProcessReader.h)

~~~cpp
#pragma once
#include <windows.h>
#include <string>

struct BasicProcessInfo {
    std::wstring architecture;
    std::wstring path;
    bool wow64 = false;
};

bool ReadBasicProcessInfo(HANDLE process, BasicProcessInfo& out, std::wstring& error);
~~~

`BasicProcessInfo` chứa nhãn kiến trúc, đường dẫn EXE và cờ `wow64`. `HANDLE process` là handle đã được `main` mở; hàm này **không tự mở PID**. Nếu trả `true`, hãy đọc `out`; nếu trả `false`, xem `error`. Cờ WOW64 được truyền tiếp cho PebReader để chọn cách đọc con trỏ 32 bit hay 64 bit.

## 4. PebReader.h — đọc command line

Source: [PebReader.h](../Module_1/PebReader.h)

~~~cpp
#pragma once
#include <windows.h>
#include <string>

bool ReadPebCommandLine(HANDLE process, bool wow64, std::wstring& commandLine, std::wstring& error);
~~~

`ReadPebCommandLine` dùng `process` để đọc bộ nhớ process đích. `wow64` chọn nhánh 32 bit hay 64 bit. Chuỗi đọc được đi vào `commandLine`; lời giải thích lỗi đi vào `error`. Tên PEB ở đây là **nơi lấy dữ liệu**, không phải thứ chương trình in ra.

## 5. ThreadReader.h — danh sách thread

Source: [ThreadReader.h](../Module_1/ThreadReader.h)

~~~cpp
#pragma once
#include "ProjectData.h"

bool ReadThreads(DWORD pid, std::vector<ThreadInfo>& out, std::wstring& error);
~~~

`pid` chỉ process muốn xem. `out` nhận nhiều `ThreadInfo`. Code hiện tại dùng `NtQuerySystemInformation`, không dùng handle process và không đọc TEB. Chữ “state” là trạng thái tức thời; nó có thể đổi ngay sau khi lấy.

## 6. ModuleReader.h — DLL đang nạp

Source: [ModuleReader.h](../Module_1/ModuleReader.h)

~~~cpp
#pragma once
#include "ProjectData.h"

bool ReadModules(DWORD pid, std::vector<ModuleInfo>& out, std::wstring& error);
~~~

`out` nhận nhiều `ModuleInfo`. Đây là module của **process đang chạy**, khác với `ImportInfo`. Code lọc tên có đuôi `.dll`, vì snapshot còn có thể chứa chính file EXE.

## 7. PeImportReader.h — import của file EXE

Source: [PeImportReader.h](../Module_1/PeImportReader.h)

~~~cpp
#pragma once
#include "ProjectData.h"

bool ReadPeImports(const std::wstring& path, std::vector<ImportInfo>& out, std::wstring& error);
~~~

`path` là đường dẫn **file EXE trên đĩa**, không phải PID. `out` nhận danh sách DLL và tên hàm import. Nó mở file và ánh xạ PE, không đọc bảng DLL đang nạp từ process đích. Nếu đường dẫn không có hoặc file bị từ chối mở, hàm trả `false`.

## 8. ProjectPrinter.h — định dạng và lưu

Source: [ProjectPrinter.h](../Module_1/ProjectPrinter.h)

~~~cpp
#pragma once
#include "ProjectData.h"

std::wstring FormatReport(const ProcessReport& report);
bool SaveReport(const ProcessReport& report, const std::wstring& text, std::wstring& error);
~~~

`FormatReport` nhận một `ProcessReport` và **trả về chuỗi** đã trình bày thành các dòng. `SaveReport` nhận cùng dữ liệu và chuỗi đó, ghi file `.txt`, rồi trả `true/false`. Hàm lưu cần `report` để ghép tên file từ tên process và PID. Trong project hiện tại phần cài đặt nằm ở `ReportWriter.cpp`; không có `ReportWriter.h`.

## Bài tập nhỏ với header

1. Gạch dưới tên của mọi `struct` trong `ProjectData.h`. Với mỗi struct, nói nó mô tả **một thứ** hay **một danh sách**.
2. Tìm `&` trong các chữ ký. Thử bỏ `&` ở bản nháp riêng và tự giải thích vì sao kết quả ở `main` sẽ không được cập nhật như mong muốn.
3. Tìm `#include "ProjectData.h"`. Nếu `ModuleReader.h` không include nó, trình biên dịch có biết `ModuleInfo` là gì không?

Nguồn đọc thêm: [C++ references](https://learn.microsoft.com/en-us/cpp/cpp/references-cpp), [C++ structs](https://learn.microsoft.com/en-us/cpp/cpp/struct-cpp), [C++ vector](https://learn.microsoft.com/en-us/cpp/standard-library/vector-class).

