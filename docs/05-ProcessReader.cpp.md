# 05. ProcessReader.cpp — 32/64 bit và đường dẫn EXE

Source: [ProcessReader.cpp](../Module_1/ProcessReader.cpp) · Header: [02-HEADERS.md](02-HEADERS.md)

## Code hiện tại

~~~cpp
#include "ProcessReader.h"
#include <vector>

bool ReadBasicProcessInfo(HANDLE process, BasicProcessInfo& out, std::wstring& error) {
    out = {};
    error.clear();

    BOOL isWow64 = FALSE;
    if (!IsWow64Process(process, &isWow64)) {
        error = L"Loi IsWow64Process, ma loi: " + std::to_wstring(GetLastError());
        return false;
    }

    out.wow64 = (isWow64 != FALSE);
    out.architecture = out.wow64 ? L"32-bit" : L"64-bit";

    std::vector<wchar_t> pathBuffer(32768);
    DWORD chars = static_cast<DWORD>(pathBuffer.size());

    if (QueryFullProcessImageNameW(process, 0, pathBuffer.data(), &chars)) {
        out.path.assign(pathBuffer.data(), chars);
    }
    else {
        out.path = L"unknown";
        error = L"Loi QueryFullProcessImageNameW, ma loi: " + std::to_wstring(GetLastError());
        return false;
    }

    return true;
}
~~~

## Từng bước

1. `out = {};` đặt kết quả về trạng thái ban đầu; `error.clear()` xóa lỗi cũ. Điều này quan trọng vì `main` dùng lại biến `error` cho nhiều bước.
2. `BOOL isWow64 = FALSE` tạo biến kiểu boolean của Windows. `IsWow64Process(process, &isWow64)` hỏi Windows liệu process đích có chạy dưới WOW64 không. Dấu `&` cho API địa chỉ để nó ghi câu trả lời. Nếu API thất bại, hàm tạo lời lỗi với `GetLastError` và trả `false`.
3. `out.wow64 = (isWow64 != FALSE)` đổi kiểu `BOOL` sang `bool`. Nhánh `? :` gán chữ `32-bit` hoặc `64-bit`. **Cần hiểu phạm vi:** code project build x64 trên Windows x64; trong bối cảnh này process WOW64 thường là x86 và process không WOW64 thường là x64. Cách suy này không tự bao quát mọi kiến trúc Windows/ARM64; Microsoft khuyên dùng `IsWow64Process2` để phân biệt rõ hơn.
4. `std::vector<wchar_t> pathBuffer(32768)` cấp chỗ chứa đường dẫn. `DWORD chars` lúc gọi là sức chứa; sau khi thành công, API đổi nó thành số ký tự thực đã ghi.
5. `QueryFullProcessImageNameW(process, 0, pathBuffer.data(), &chars)` hỏi đường dẫn EXE đầy đủ. `.data()` đưa con trỏ tới vùng nhớ của vector. `0` chọn đường dẫn kiểu Win32. `out.path.assign(pathBuffer.data(), chars)` chép đúng `chars` ký tự, không cần tự đoán độ dài.
6. Nếu API lấy đường dẫn thất bại, code gán `unknown`, ghi lỗi, rồi trả `false`. Thành công thì trả `true`.

## Ví dụ

Giả sử PID thuộc `C:\Windows\System32\notepad.exe`. `out.path` là đường dẫn đó. Nó **không phải** command line. Nếu bạn mở `notepad.exe note.txt`, command line có thể chứa thêm `note.txt`, còn đường dẫn EXE vẫn là file Notepad.

## Tự kiểm tra

Tại sao `pathBuffer` là `wchar_t` mà không phải `char`? Vì API có hậu tố `W`. Tại sao `chars` truyền bằng `&`? Vì API cần viết lại số ký tự đã dùng. Tại sao hàm cần `HANDLE` chứ không chỉ PID? Vì hai API nhận một process handle có quyền phù hợp.

## Nguồn

- [IsWow64Process](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process) và [IsWow64Process2](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process2).
- [QueryFullProcessImageNameW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew).

