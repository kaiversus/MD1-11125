# Hướng dẫn đọc hiểu Module 1

Tài liệu này viết cho người mới chỉ biết những phần C++ rất cơ bản. Mục tiêu là bạn có thể **đọc code thật, giải thích được nó chạy theo thứ tự nào, rồi tự viết lại từng phần**. Mọi khối `Code hiện tại` được chép tự động từ source trong `Module_1/Module_1` ngày 01-11-2026. Nếu bạn sửa source sau ngày này, hãy xem source mới là bản đúng nhất.

## Chương trình làm gì?

Bạn chạy `Module_1.exe 21412` hoặc `Module_1.exe notepad.exe`. Chương trình tìm tiến trình có PID/tên đó, rồi lấy bốn nhóm dữ liệu:

1. Tên, PID, PID cha, 32/64 bit, đường dẫn EXE, command line.
2. ID và trạng thái các thread.
3. Các DLL đang được tiến trình nạp.
4. Các DLL/hàm mà **file EXE trên đĩa** khai báo trong bảng PE Import.

Nó in báo cáo ra console và ghi file `<Tên tiến trình>-<PID>.txt` trong **thư mục hiện tại nơi bạn chạy lệnh**. PID chỉ có ý nghĩa khi tiến trình đó còn chạy; Windows có thể cấp lại PID sau này.

## Đọc theo thứ tự này

1. [01-NEN-TANG.md](01-NEN-TANG.md): từ C++ cơ bản đến process, handle, PEB, PE.
2. [02-HEADERS.md](02-HEADERS.md): tất cả các `.h`, có code thật và giải thích từng kiểu dữ liệu.
3. [03-main.cpp.md](03-main.cpp.md): luồng chạy tổng thể.
4. [04-ProcessFinder.cpp.md](04-ProcessFinder.cpp.md): tìm tiến trình.
5. [05-ProcessReader.cpp.md](05-ProcessReader.cpp.md): kiến trúc và đường dẫn.
6. [06-PebReader.cpp.md](06-PebReader.cpp.md): command line trong bộ nhớ tiến trình khác.
7. [07-ThreadReader.cpp.md](07-ThreadReader.cpp.md): thread và trạng thái.
8. [08-ModuleReader.cpp.md](08-ModuleReader.cpp.md): DLL đang nạp.
9. [09-PeImportReader.cpp.md](09-PeImportReader.cpp.md): DLL/hàm trong file EXE.
10. [10-ReportWriter.cpp.md](10-ReportWriter.cpp.md): ghép và lưu báo cáo.
11. [12-BAO-CAO-PROJECT.md](12-BAO-CAO-PROJECT.md): bản báo cáo đầy đủ về project.
12. [11-BAO-CAO-VA-THUC-HANH.md](11-BAO-CAO-VA-THUC-HANH.md): bài tập tự kiểm chứng sau khi đọc báo cáo.

## Bản đồ luồng chạy

```text
argv[1] (PID hoặc tên)
       |
       v
ProcessFinder --> danh sách ProcessEntry
       |
       v
với mỗi PID: OpenProcess
       |--> ProcessReader: 32/64 bit + đường dẫn EXE
       |--> PebReader: command line từ PEB
       |--> ThreadReader: thread ID + state
       |--> ModuleReader: DLL đang nạp
       |--> PeImportReader: import table của file EXE
       v
ProcessReport --> FormatReport --> màn hình
                            \--> SaveReport --> file .txt
```

## Phân biệt ba thứ rất dễ nhầm

- **PID** là số định danh của một tiến trình đang chạy. **HANDLE** là “vé” do Windows cấp để thao tác với đối tượng đó. PID 21412 không phải handle 21412.
- **DLL đang nạp** là trạng thái ở thời điểm chụp của tiến trình. **PE Import** là thông tin trong file EXE trên đĩa. Hai danh sách có thể khác nhau, vì DLL có thể được nạp lúc chạy hoặc một import có thể chưa hiện như bạn dự đoán.
- **Đường dẫn EXE** chỉ tới file chạy, ví dụ `C:\Windows\System32\notepad.exe`. **Command line** gồm chương trình và các tham số khi khởi chạy, ví dụ `notepad.exe note.txt`.

## Phạm vi code hiện tại

`ProcessInfo.h` cũ đã được xóa khỏi thư mục source và bản sao lưu; project hiện dùng `ProjectData.h`, `ProcessFinder.h` và `ProcessReader.h` cho những dữ liệu tương ứng.

Code hiện tại chạy được trong Debug x64 với PID thử nghiệm. Tài liệu giải thích **code đang làm gì**, đồng thời chỉ rõ các giới hạn/chỗ chưa kiểm tra lỗi. Đừng ghi trong report rằng đã kiểm thử mọi PID, mọi Windows hay mọi file PE nếu bạn chưa tự kiểm chứng.

## Nguồn tra cứu chính

- [Ví dụ chính thức của Microsoft: duyệt process, module, thread bằng Tool Help](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes) — có **code mẫu** gần với ProcessFinder và ModuleReader.
- [Đặc tả PE/COFF của Microsoft](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) — nền tảng để hiểu PE Import.
- [NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess) và [NtQuerySystemInformation](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation) — các Native API code đang dùng; Microsoft nêu rõ giao diện/struct nội bộ có thể thay đổi.
- [Pointer trong C++](https://learn.microsoft.com/en-us/cpp/cpp/pointers-cpp) và [reference trong C++](https://learn.microsoft.com/en-us/cpp/cpp/references-cpp) — nên đọc cùng phần header.

Các link trên là tài liệu/code mẫu để **đối chiếu ý tưởng**, không phải nơi copy nguyên code của project. Source trong thư mục project là phần bạn cần giải thích trong report.

