# 11. Tự thực hành và viết report bằng chứng

Tài liệu trong thư mục này giúp bạn hiểu source; report của bạn nên diễn đạt **bằng lời của chính bạn** và ghi bằng chứng chạy thật. Có thể dùng dàn ý sau, nhưng đừng chép nhận xét “đã kiểm thử” nếu chưa tự làm.

## 1. Cách chạy và quan sát

Mở PowerShell tại thư mục chứa bản Debug EXE, rồi chạy với PID **đang tồn tại**:

```powershell
cd D:\CCD\Module_1\Module_1\x64\Debug
Get-Process notepad
.\Module_1.exe <PID-trong-cot-Id>
$LASTEXITCODE
Get-ChildItem *.txt
```

`<PID-trong-cot-Id>` là chỗ bạn thay bằng số thật, ví dụ `1234`; đừng gõ nguyên dấu `< >`. Hoặc chạy `.\Module_1.exe notepad.exe` để tìm theo tên. Nếu không có Notepad, mở nó trước hoặc chọn process khác bạn có quyền xem. `$LASTEXITCODE` cho mã thoát của chương trình vừa chạy. File text được tạo trong thư mục hiện tại. Nếu PID không còn, chương trình báo không tìm thấy; điều đó không nhất thiết là lỗi build.

## 2. Dàn ý report có thể dùng

### Mục tiêu

“Viết chương trình dòng lệnh nhận PID hoặc tên process, tìm process phù hợp, thu thập thông tin process/thread/DLL/PE Import, rồi in và lưu báo cáo.”

### Kiến trúc và luồng dữ liệu

Vẽ sơ đồ từ [README](README.md), sau đó viết 1–2 câu cho **mỗi module**: đầu vào, nguồn dữ liệu, đầu ra. Nói rõ `main.cpp` điều phối; `ProjectData.h` giữ cấu trúc kết quả; `ReportWriter.cpp` định dạng và ghi.

### Kiến thức nền

Giải thích PID khác HANDLE; process khác thread; `ReadProcessMemory` cần thiết vì địa chỉ PEB thuộc process đích; loaded DLL khác PE Import. Dùng ví dụ đơn giản của chính bạn. Nếu viết về WOW64, nêu code đang build x64 và nhánh WOW64 phục vụ process 32 bit.

### Cách lấy từng trường

- Tên, PID, PPID: Tool Help process snapshot, `PROCESSENTRY32W`.
- Architecture/path: `IsWow64Process`, `QueryFullProcessImageNameW`.
- Command line: `NtQueryInformationProcess` → PEB → ProcessParameters → `ReadProcessMemory`.
- Thread ID/state: `NtQuerySystemInformation(SystemProcessInformation)`, không phải TEB.
- DLL đang nạp: Tool Help module snapshot.
- PE imports: mở file EXE, ánh xạ `SEC_IMAGE`, đi qua import descriptors và thunks.
- Output: `FormatReport` dùng chung cho console và file; `SaveReport` đổi sang UTF-8.

### Kiểm thử có bằng chứng

Ghi chính xác ngày, Windows, cấu hình `Debug|x64` hoặc `Release|x64`, lệnh đã chạy, PID lúc đó, vài dòng output và tên file kết quả. Kiểm thử tối thiểu: PID có thật; tên có thật; PID không có; tên không có; hai process cùng tên nếu tạo được; process thiếu quyền; đường dẫn/command line có Unicode; so PE Import với `dumpbin /IMPORTS` nếu Visual Studio có công cụ này. Với mỗi ca, phân biệt “kỳ vọng” và “quan sát thực tế”.

### Giới hạn thật của phiên bản hiện tại

Nêu `std::stoi` chưa bắt exception cho PID quá lớn; `PebReader` chưa kiểm tra từng `ReadProcessMemory`; `ThreadReader` dùng giao diện nội bộ và chưa kiểm tra đầy đủ buffer/trạng thái lỗi; `main` bỏ qua kết quả `ReadThreads`, `ReadModules`, `SaveReport`; nhãn kiến trúc dựa vào `IsWow64Process` trong phạm vi x64 đang dùng; PE parser không phải công cụ chống mọi file PE hỏng. Viết giới hạn như vậy thể hiện bạn hiểu code, không làm bài yếu đi.

### Nguồn tham khảo

Đưa link đúng nơi bạn dùng: [code mẫu Tool Help](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes), [PE Format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess), [NtQuerySystemInformation](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation). Các file giải thích riêng còn có link cụ thể cho từng API.

## 3. Bài tập để thật sự tự viết lại

### Bài A — viết giả mã, không nhìn C++

Viết 10–15 dòng tiếng Việt cho luồng `main`. Ví dụ “nếu thiếu tham số thì báo lỗi”, “nếu nhập số thì tìm PID”, “với từng process thì mở handle”. Sau đó tự chuyển mỗi dòng sang một lệnh C++.

### Bài B — viết lại ProcessFinder trên giấy

Tạo ba `ProcessEntry` tưởng tượng. Viết vòng lặp lọc theo tên, dùng `vector.push_back`, **không dùng Windows API**. Khi làm được, thay danh sách tưởng tượng bằng `Process32FirstW/NextW`. Như vậy bạn chỉ học một ý mới mỗi lần.

### Bài C — luyện output parameter

Viết hàm `bool GetDouble(int input, int& out)` sao cho `out = input * 2`. Gọi nó từ `main`. Khi hiểu vì sao `out` thay đổi, bạn sẽ hiểu `ReadModules(pid, report.modules, error)`.

### Bài D — phân biệt hai danh sách DLL

Lấy một kết quả chạy thật. Ghi vài DLL ở mục “DLL Information” và vài DLL ở “PE Import Information”. Dùng một câu giải thích tại sao chúng có thể khác nhau. Không kết luận một danh sách “sai” chỉ vì khác.

### Bài E — đọc PEB bằng mũi tên

Vẽ bốn ô: `process HANDLE`, `PEB address`, `ProcessParameters address`, `CommandLine.Buffer address`. Nối chúng bằng API/trường tương ứng. Khoanh những địa chỉ thuộc process đích; chúng không được dereference trực tiếp trong process của bạn.

## 4. Câu hỏi tự kiểm tra

1. Tại sao `Process32FirstW` cần `dwSize`?
2. Tại sao `FindByName` trả `vector`, không trả một PID?
3. `OpenProcess` có thể thất bại dù snapshot vừa thấy PID không?
4. `ReadProcessMemory` sao chép từ đâu tới đâu?
5. Tên EXE, đường dẫn EXE và command line khác nhau thế nào?
6. Tại sao `ThreadReader` không thể nói đã đọc TEB?
7. `SEC_IMAGE` giúp gì cho việc dùng `image + RVA`?
8. Vì sao cần `CloseHandle` và `UnmapViewOfFile`?
9. `FormatReport` có đọc Windows API không?
10. Nếu màn hình có báo cáo nhưng file không xuất hiện, bạn sẽ xem hàm nào trước?

Bạn nên trả lời bằng lời của mình trước khi xem lại các trang tài liệu. Nếu một câu khó, quay về file tương ứng và theo dõi biến đầu vào/đầu ra.

