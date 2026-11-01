# 03. main.cpp — người điều phối

Source: [main.cpp](../Module_1/main.cpp) · Header cần đọc trước: [02-HEADERS.md](02-HEADERS.md)

## Trước khi xem code

`main` giống người trưởng nhóm: nó **không tự biết** cách đọc PEB hay phân tích PE, nhưng biết lúc nào cần gọi từng người làm việc đó. Đầu vào là một tham số của dòng lệnh. Đầu ra là báo cáo trên màn hình và file `.txt`.

## Code hiện tại

~~~cpp
#include "ProcessFinder.h"
#include "ProcessReader.h"
#include "PEBReader.h"
#include "ThreadReader.h"
#include "ModuleReader.h"
#include "PEImportReader.h"
#include "ProjectPrinter.h" 

#include <iostream>
#include <string>

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 2) {
        std::wcout << L"Vui long nhap PID hoac Ten tien trinh.\n";
        return 1;
    }

    std::wstring input = argv[1];
    bool isNumeric = true;
    for (wchar_t c : input) {
        if (!iswdigit(c)) isNumeric = false;
    }

    SearchResult found = isNumeric ? FindByPid(std::stoi(input)) : FindByName(input);

    if (found.processes.empty()) {
        std::wcout << L"Khong tim thay tien trinh.\n";
        return 1;
    }

    for (const auto& entry : found.processes) {
        ProcessReport report;
        report.process = entry;
        std::wstring error;

        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.pid);
        if (process != nullptr) {
            BasicProcessInfo basic;
            if (ReadBasicProcessInfo(process, basic, error)) {
                report.architecture = basic.architecture;
                report.path = basic.path;

                if (!ReadPebCommandLine(process, basic.wow64, report.commandLine, error)) {
                    std::wcout << L"[!] Loi doc Command Line PID " << entry.pid << L": " << error << L"\n";
                }
            }
            else {
                std::wcout << L"[!] Loi doc Architecture/Path PID " << entry.pid << L": " << error << L"\n";
            }
            CloseHandle(process);
        }
        else {
            std::wcout << L"[!] KHONG MO DUOC PID " << entry.pid << L", Ma loi Windows: " << GetLastError() << L"\n";
        }

        ReadThreads(entry.pid, report.threads, error);
        ReadModules(entry.pid, report.modules, error);

        if (report.path != L"unavailable" && report.path != L"unknown" && !report.path.empty()) {
            if (!ReadPeImports(report.path, report.imports, error)) {
                std::wcout << L"[!] Loi doc PE Import PID " << entry.pid << L": " << error << L"\n";
            }
        }

        std::wstring text = FormatReport(report);
        std::wcout << text << L"\n";
        SaveReport(report, text, error);
    }
    return 0;
}
~~~

## Giải thích theo thứ tự chạy

### 1. Các dòng `#include`

Mỗi header cho `main.cpp` biết tên hàm/kiểu dữ liệu của một module. Ví dụ `ProcessFinder.h` giới thiệu `FindByPid`; `ProjectPrinter.h` giới thiệu `FormatReport` và `SaveReport`. `<iostream>` cung cấp `std::wcout`; `<string>` cung cấp `std::wstring`.

### 2. `wmain(int argc, wchar_t* argv[])`

`wmain` là điểm bắt đầu của **code C++ do ta viết**: sau khi Windows nạp EXE, thư viện chạy C/C++ gọi hàm này. `argc` đếm số mục trong `argv`; `argv[0]` thường là tên chương trình và `argv[1]` là thứ bạn nhập. `argc != 2` yêu cầu đúng một tham số. Ví dụ `Module_1.exe notepad.exe` có `argc == 2`. Chữ `w` trong `wmain` và kiểu `wchar_t` giúp nhận tên Unicode.

### 3. Quyết định tham số là PID hay tên

Vòng `for (wchar_t c : input)` xem từng ký tự. Nếu mọi ký tự là chữ số theo `iswdigit`, code dùng `std::stoi(input)` để đổi chữ thành số, rồi `FindByPid`. Nếu có chữ, code dùng `FindByName`. Dấu `? :` ở đây có nghĩa “nếu đúng thì làm bên trái dấu `:`, nếu sai thì làm bên phải”.

Ví dụ `"21412"` → PID 21412; `"notepad.exe"` → tên. **Giới hạn hiện tại:** số quá lớn có thể làm `std::stoi` ném exception; code chưa `try/catch`. Chuỗi rỗng không xảy ra với lệnh bình thường có đúng một tham số, nhưng hàm tìm PID 0 vẫn từ chối.

### 4. `found.processes.empty()` và vòng lặp

`FindByName` có thể tìm nhiều process `notepad.exe`; `found.processes` là `vector` chứa tất cả mục khớp. `.empty()` hỏi danh sách có rỗng không. `for (const auto& entry : found.processes)` làm toàn bộ các bước sau **một lần cho mỗi PID**. `const auto&` nghĩa là lấy từng mục mà không tạo bản sao, và không sửa mục đó.

### 5. Tạo `ProcessReport` và mở process

`ProcessReport report;` tạo một báo cáo trống. `report.process = entry` chép tên/PID/PPID vừa tìm được. `OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, entry.pid)` xin handle có hai quyền: hỏi thông tin và đọc bộ nhớ. `|` là phép ghép cờ quyền, không phải `||`. Nếu không mở được, `process == nullptr`; code in mã `GetLastError()`, nhưng vẫn cố đọc thread/module bằng các phương pháp khác. Handle mở thành công được `CloseHandle(process)` sau khi đọc xong.

### 6. Đọc kiến trúc, đường dẫn, command line

`ReadBasicProcessInfo` đổ `architecture`, `path`, `wow64` vào `basic`. Chỉ khi thành công, code đưa đường dẫn/kiến trúc vào `report` và gọi `ReadPebCommandLine`. Cờ `basic.wow64` cho PebReader biết nên theo nhánh cấu trúc 32 hay 64 bit. Khi lỗi, một số trường vẫn giữ chữ `unavailable` ban đầu.

### 7. Đọc thread, DLL và PE Import

`ReadThreads` và `ReadModules` nhận PID và đổ danh sách vào báo cáo. Code hiện tại **không kiểm tra giá trị `bool` của hai lời gọi này**; nếu thất bại, danh sách có thể rỗng mà báo cáo không nói rõ nguyên nhân. Sau đó, nếu `report.path` có đường dẫn dùng được, `ReadPeImports` mở **file EXE trên đĩa**. Vì vậy phần PE Import phụ thuộc vào phần lấy đường dẫn.

### 8. In và ghi file

`FormatReport(report)` ghép dữ liệu thành một chuỗi `text`. `std::wcout << text` in chuỗi. `SaveReport(report, text, error)` ghi chính chuỗi đó ra file. Code hiện tại không kiểm tra kết quả `SaveReport`, nên nếu ổ đĩa/quyền ghi có vấn đề, bạn có thể thấy output trên màn hình nhưng thiếu file mà không được báo.

## Bạn thử tự kể lại

“Tôi nhập tên → chương trình tìm nhiều PID → với từng PID, nó xin handle → đọc process và PEB → đọc thread/DLL → mở file EXE để đọc import → ghép báo cáo → in và lưu.” Nếu bạn nói được câu đó mà không nhìn code, bạn đã hiểu vai trò của `main.cpp`.

## Nguồn và ví dụ gần với code

- [OpenProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess), [CloseHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle).
- [Process handles và identifiers](https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers).
- [Code mẫu Microsoft duyệt process và module](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes).

