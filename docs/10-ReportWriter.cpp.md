# 10. ReportWriter.cpp — biến dữ liệu thành báo cáo

Source: [ReportWriter.cpp](../Module_1/ReportWriter.cpp) · Chữ ký hàm nằm ở [ProjectPrinter.h](02-HEADERS.md#8-projectprinterh--định-dạng-và-lưu).

## Code hiện tại

~~~cpp
#include "ProjectPrinter.h"
#include <sstream>
#include <fstream>
#include <filesystem>

std::wstring WidenAscii(const std::string& text) {
    return std::wstring(text.begin(), text.end());
}

std::wstring FormatReport(const ProcessReport& r) {
    std::wostringstream out;
    out << L"Process Information:\n";
    out << L"- Name: " << r.process.name << L'\n';
    out << L"- PID: " << r.process.pid << L'\n';
    out << L"- PPID: " << r.process.ppid << L'\n';
    out << L"- Architecture: " << r.architecture << L'\n';
    out << L"- Path: " << r.path << L'\n';
    out << L"- Command Line: " << r.commandLine << L'\n';

    out << L"\nThread Information:\n";
    for (const auto& t : r.threads) {
        out << L"- Thread ID: " << t.id << L", State: " << t.state << L'\n';
    }

    out << L"\nDLL Information:\n";
    for (const auto& m : r.modules) {
        out << L"- DLL Name: " << m.name << L", Path: " << m.path << L'\n';
    }

    out << L"\nPE Import Information:\n";
    for (const auto& dll : r.imports) {
        out << L"- DLL Name: " << WidenAscii(dll.dll) << L", Imported Functions: ";
        for (size_t i = 0; i < dll.functions.size(); ++i) {
            if (i > 0) out << L", ";
            out << WidenAscii(dll.functions[i]);
        }
        out << L'\n';
    }
    return out.str();
}

bool SaveReport(const ProcessReport& report, const std::wstring& text, std::wstring& error) {
    error.clear();
    std::wstring filename = report.process.name + L"-" + std::to_wstring(report.process.pid) + L".txt";

    int bytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (bytes == 0) { error = L"Loi chuyen doi UTF-8"; return false; }

    std::string utf8(bytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), bytes, nullptr, nullptr);

    std::ofstream file(std::filesystem::path(filename), std::ios::binary);
    if (!file) { error = L"Khong the mo file output"; return false; }

    file.write(utf8.data(), utf8.size());
    return true;
}
~~~

## Tại sao tách thành file riêng?

Các reader chỉ đi **lấy dữ liệu**. File này quyết định dữ liệu được **trình bày** thế nào và **lưu** ra đâu. Nhờ vậy `main.cpp` không phải lặp lại hàng chục dòng `<<`, và console/file dùng cùng chuỗi `text`.

## `WidenAscii`

`ImportInfo` chứa `std::string` (byte) còn báo cáo dùng `std::wstring` (ký tự rộng). `std::wstring(text.begin(), text.end())` tạo một ký tự rộng từ mỗi byte. Với tên DLL và hàm PE thông thường dùng ký tự ASCII, cách này dễ hiểu và thường đủ dùng. **Nó không phải bộ giải mã UTF-8 tổng quát**; nếu byte chứa UTF-8 nhiều byte, kết quả không chắc đúng.

## `FormatReport`

`std::wostringstream out` giống một “máy viết vào chuỗi” thay vì in ngay ra màn hình. Mỗi `out << ...` nối thêm chữ. `L'\n'` tạo xuống dòng. Hàm tạo bốn phần:

1. Process Information: các trường trong `r.process`, cộng kiến trúc, path, command line.
2. Thread Information: vòng `for` đi qua từng `ThreadInfo`.
3. DLL Information: vòng `for` đi qua từng `ModuleInfo`.
4. PE Import Information: vòng ngoài đi qua DLL import; vòng trong đi qua các tên hàm. `if (i > 0) out << L", "` chỉ thêm dấu phẩy **giữa** các hàm, không thêm ở đầu/cuối.

`return out.str()` đưa toàn bộ chuỗi cho `main`. Nếu danh sách thread/module/import rỗng, code vẫn in tiêu đề phần đó nhưng không có dòng dữ liệu dưới nó. Điều này **không tự nói được** là “không có” hay “đọc thất bại”; phải xem reader và lỗi.

## `SaveReport`

Tên file được ghép như `notepad.exe-1234.txt`. Vì không ghép thư mục vào `filename`, file xuất hiện ở **thư mục làm việc hiện tại**, nơi bạn đứng khi chạy lệnh, không bắt buộc cạnh EXE.

`WideCharToMultiByte(CP_UTF8, ...)` được gọi hai lần. Lần 1 đưa `nullptr, 0` để hỏi cần bao nhiêu **byte**. Sau đó `std::string utf8(bytes, '\0')` cấp chỗ. Lần 2 ghi dữ liệu UTF-8 vào `utf8.data()`. `std::ofstream(..., std::ios::binary)` mở file và `file.write` ghi đúng số byte. Dùng binary ở đây để không có bước đổi xuống dòng ngầm của text mode.

## Điều cần trung thực trong report

Code kiểm tra `bytes == 0` và mở file thất bại. Nó **chưa kiểm tra kết quả lần chuyển mã thứ hai hoặc trạng thái sau `file.write`**. `main.cpp` cũng chưa kiểm tra `SaveReport` trả `false`. Vì vậy không nên ghi “mọi lỗi ghi file đều đã được thông báo cho người dùng”.

## Bài tập tự viết

Cho `ProcessReport` có tên `demo.exe`, PID 42, hai hàm import `OpenProcess` và `CloseHandle`. Không chạy code, hãy dự đoán tên file và một dòng `Imported Functions` sẽ trông thế nào. Sau đó dùng debugger hoặc chạy chương trình để đối chiếu.

## Nguồn

- [WideCharToMultiByte và CP_UTF8](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte).
- [std::basic_ostringstream trong Microsoft C++](https://learn.microsoft.com/en-us/cpp/standard-library/basic-ostringstream-class).
- [std::basic_ofstream trong Microsoft C++](https://learn.microsoft.com/en-us/cpp/standard-library/basic-ofstream-class).

