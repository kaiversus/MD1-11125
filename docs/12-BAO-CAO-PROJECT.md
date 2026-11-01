# Báo cáo toàn bộ project Module 1

**Đề tài:** Chương trình C++ khảo sát tiến trình Windows theo PID hoặc tên  
**Source được mô tả:** project `Module_1.vcxproj` ngày 01-11-2026  
**Cấu hình đã kiểm chứng:** Debug x64  
**Người thực hiện:** bổ sung họ tên và lớp trước khi nộp

> Đây là bản báo cáo về **toàn bộ project**, không phải dàn ý. Các trang [giải thích từng file](README.md) có nguyên code tương ứng. Những điều chưa kiểm thử được ghi rõ để không biến suy đoán thành kết quả.

## 1. Tóm tắt

Chương trình nhận đúng một đối số dòng lệnh: PID dạng số hoặc tên file EXE, ví dụ `Module_1.exe 30792` hay `Module_1.exe pwsh.exe`. Nó tìm tiến trình phù hợp; khi nhiều tiến trình cùng tên, nó xử lý từng PID. Với mỗi tiến trình, chương trình thu thập bốn nhóm: thông tin process, thread, DLL đang nạp và PE Import của file EXE. Báo cáo được in ra console và lưu trong file `<Tên-process>-<PID>.txt`.

Hai nguồn dữ liệu của chương trình khác nhau. Process, thread và DLL đang nạp mô tả **hệ thống ở thời điểm đọc**. PE Import mô tả **nội dung file EXE trên đĩa**. Vì thế hai danh sách DLL không bắt buộc giống nhau.

## 2. Mục tiêu, đầu vào, đầu ra và phạm vi

### 2.1. Mục tiêu

Mục tiêu là thực hành cách Windows quản lý process, thread và module; học cách xin quyền bằng handle, đọc bộ nhớ process khác qua PEB, phân tích cấu trúc PE và tổ chức chương trình thành các module C++ có trách nhiệm riêng.

### 2.2. Đầu vào

`wmain` nhận `argc` và `argv`. Nếu `argc != 2`, chương trình báo cần nhập PID hoặc tên tiến trình và trả mã 1. Đối số chỉ chứa chữ số được xem là PID; đối số còn lại được xem là tên EXE. Tên được so không phân biệt chữ hoa/thường. Code dùng `std::stoi` để đổi chuỗi số sang `int`, nên PID quá lớn hiện có thể ném exception mà chưa được bắt.

### 2.3. Đầu ra

Báo cáo gồm đúng bốn mục:

1. **Process Information:** name, PID, PPID, architecture, path, command line.
2. **Thread Information:** thread ID và state.
3. **DLL Information:** tên và đường dẫn DLL đang nạp.
4. **PE Import Information:** tên DLL và các hàm EXE khai báo import.

`FormatReport` tạo một chuỗi dùng chung cho console và file. `SaveReport` đổi chuỗi rộng sang UTF-8 trước khi ghi. Vì tên file là đường dẫn tương đối, file nằm trong **thư mục hiện tại khi chạy lệnh**, không nhất thiết cạnh file EXE nếu gọi từ thư mục khác.

### 2.4. Phạm vi

Project dùng C++20, Windows API và một số Native API trong `ntdll.dll`. Lần kiểm chứng hiện có là Debug x64 trên Windows báo phiên bản `Microsoft Windows NT 10.0.26300.0`. Báo cáo không khẳng định mọi phiên bản Windows, mọi kiến trúc hay mọi file PE đều được hỗ trợ.

## 3. Môi trường và cách chạy

Solution là `D:\CCD\Module_1\Module_1\Module_1.slnx`; project là `Module_1.vcxproj` cùng thư mục. Project đăng ký tám file `.cpp` và tám header đang dùng, chọn C++20, Unicode và Console Subsystem. Bản Debug x64 nằm ở `D:\CCD\Module_1\Module_1\x64\Debug\Module_1.exe`.

Ví dụ chạy bằng PowerShell:

~~~powershell
cd D:\CCD\Module_1\Module_1\x64\Debug
Get-Process pwsh
.\Module_1.exe pwsh.exe
$LASTEXITCODE
Get-ChildItem pwsh.exe-*.txt
~~~

PID phải đang tồn tại lúc chạy. Windows có thể cấp lại PID sau khi process kết thúc. Muốn dùng PID, lấy cột `Id` từ `Get-Process` rồi truyền số đó; không dùng cố định PID của ví dụ trong báo cáo.

## 4. Kiến thức nền cần để hiểu project

### 4.1. Process, thread, PID và PPID

Process là một chương trình đang hoạt động với không gian địa chỉ và tài nguyên riêng. PID là số định danh của process ở thời điểm đó. Thread là luồng thực thi bên trong process; một process có thể có nhiều thread. PPID là PID được ghi cho process cha lúc tạo, nhưng process cha có thể đã thoát về sau. `notepad.exe` trên đĩa không phải cùng một vật với một process Notepad đang chạy: hai cửa sổ Notepad có thể có hai PID.

### 4.2. Handle và quyền truy cập

PID giống số nhận dạng; `HANDLE` giống thẻ do Windows cấp để làm việc với một đối tượng. `OpenProcess` xin quyền `PROCESS_QUERY_INFORMATION | PROCESS_VM_READ` để hỏi thông tin và đọc bộ nhớ process đích. Windows có thể từ chối vì thiếu quyền hoặc process đã biến mất. Khi dùng xong handle, code gọi `CloseHandle`. Snapshot, file và file mapping cũng có handle riêng. Tham khảo [process handles](https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers) và [OpenProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess).

### 4.3. Snapshot

`CreateToolhelp32Snapshot` tạo một bản chụp tại một thời điểm, không phải danh sách tự cập nhật. `Process32FirstW/NextW` duyệt process; `Module32FirstW/NextW` duyệt module. Dữ liệu có thể đổi giữa lúc tìm PID và lúc mở process. Microsoft có [code mẫu Tool Help bằng C/C++](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes) rất gần với hai module của project.

### 4.4. PEB và đọc bộ nhớ process khác

PEB (Process Environment Block) là cấu trúc nội bộ của Windows. Đường đi để lấy command line là `PEB -> ProcessParameters -> CommandLine.Buffer`. Các địa chỉ đó thuộc **process đích**, nên chương trình không được dùng chúng như con trỏ tới bộ nhớ của mình. `ReadProcessMemory` sao chép từng khối từ process đích về biến cục bộ. `NtQueryInformationProcess` giúp tìm địa chỉ PEB. Xem [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory) và [NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess).

### 4.5. WOW64

WOW64 là môi trường chạy ứng dụng 32 bit trên Windows 64 bit. Cờ từ `IsWow64Process` giúp PebReader chọn cấu trúc PEB 32 hoặc 64 bit. Trong môi trường x64 đang dùng, code gắn nhãn process WOW64 là `32-bit` và process không WOW64 là `64-bit`. Cách này chưa nhận diện đầy đủ mọi môi trường, đặc biệt ARM64; Microsoft khuyên dùng `IsWow64Process2` khi cần phân biệt host/guest rõ hơn. [Tài liệu IsWow64Process](https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process).

### 4.6. PE và import

PE (Portable Executable) là định dạng EXE/DLL. PE có DOS header với dấu MZ, PE header, các data directory và section. Import directory liệt kê DLL và tên hoặc ordinal của hàm mà EXE khai báo dùng. RVA là vị trí tương đối so với đầu ảnh PE khi được ánh xạ. `SEC_IMAGE` nhờ Windows ánh xạ file theo layout ảnh PE, nên code truy cập bằng `image + rva`. [Đặc tả PE Format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) giải thích từng bảng; [CreateFileMappingW](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw) giải thích `SEC_IMAGE`.

## 5. Kiến trúc và luồng dữ liệu

~~~text
argv[1] -> FindByPid / FindByName -> nhiều ProcessEntry
                                  -> lặp mỗi PID
                                  -> OpenProcess
                                  -> ProcessReader + PebReader
                                  -> ThreadReader + ModuleReader
                                  -> PeImportReader(path EXE)
                                  -> ProcessReport
                                  -> FormatReport
                                  -> console + SaveReport(file .txt)
~~~

`main.cpp` điều phối toàn bộ. `ProjectData.h` định nghĩa dữ liệu chung: `ProcessEntry`, `ThreadInfo`, `ModuleInfo`, `ImportInfo`, `ProcessReport`. `ProcessFinder.h` thêm `SearchResult`; `ProcessReader.h` thêm `BasicProcessInfo`. Các reader lấy dữ liệu, còn `ReportWriter.cpp` trình bày và lưu. Tất cả header có code và giải thích từng trường ở [02-HEADERS.md](02-HEADERS.md).

### Vai trò từng file

- `main.cpp`: kiểm tham số, tìm process, mở/đóng handle, gọi các reader và ghi output.
- `ProcessFinder.cpp`: lấy name/PID/PPID từ process snapshot; trả mọi process khớp tên.
- `ProcessReader.cpp`: lấy architecture và đường dẫn file EXE.
- `PebReader.cpp`: lấy command line qua PEB của process đích.
- `ThreadReader.cpp`: lấy thread ID và state từ `SystemProcessInformation`.
- `ModuleReader.cpp`: chụp và lọc các DLL đang nạp.
- `PeImportReader.cpp`: đọc import directory của file EXE.
- `ReportWriter.cpp`: tạo bốn mục báo cáo và lưu UTF-8.

`ProjectPrinter.h` khai báo hai hàm của `ReportWriter.cpp`; không có `ReportWriter.h` trong project hiện tại. `ProcessInfo.h` cũ đã được xóa vì không còn thuộc luồng build.

## 6. Cách từng phần hoạt động

### 6.1. Tìm process theo PID hoặc tên

`wmain` kiểm `argc` và đọc `argv[1]`. Vòng `for` xét từng ký tự với `iswdigit`. Nếu tất cả là số, code gọi `FindByPid(std::stoi(input))`; nếu có chữ, gọi `FindByName(input)`. Hai hàm đó cùng dùng `Find` trong `ProcessFinder.cpp`. `Find` tạo snapshot `TH32CS_SNAPPROCESS`, đặt `PROCESSENTRY32W.dwSize` rồi duyệt bằng `Process32FirstW` và `Process32NextW`. Theo tên, `CompareStringOrdinal(..., TRUE)` so không phân biệt hoa/thường; theo PID, so `th32ProcessID`. Mỗi mục khớp được chép vào `ProcessEntry` gồm `szExeFile`, `th32ProcessID`, `th32ParentProcessID`. Vòng lặp không dừng ở mục khớp đầu tiên, nên có thể trả nhiều PID cho cùng tên. Sau cùng snapshot được đóng.

Nếu không có mục khớp, `main` in `Khong tim thay tien trinh.` và trả mã 1. Hiện `main` chỉ kiểm danh sách rỗng mà chưa phân biệt lỗi tạo snapshot với việc snapshot thành công nhưng không có mục khớp.

### 6.2. Đọc architecture và đường dẫn EXE

Với từng `ProcessEntry`, `main` tạo `ProcessReport` và gọi `OpenProcess`. Nếu mở thành công, `ReadBasicProcessInfo` gọi `IsWow64Process`; cờ này vừa tạo nhãn kiến trúc vừa được chuyển cho PebReader. Sau đó `QueryFullProcessImageNameW` điền đường dẫn EXE vào `std::vector<wchar_t>`. Biến `chars` lúc đầu là sức chứa buffer, khi API thành công được cập nhật thành số ký tự thực. Code dùng `out.path.assign(pathBuffer.data(), chars)` để chép đúng phần có dữ liệu. Nếu API lỗi, hàm ghi mã `GetLastError` và trả `false`. Đường dẫn EXE khác command line: đường dẫn chỉ file chạy, còn command line có thể chứa đối số lúc khởi động. [QueryFullProcessImageNameW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-queryfullprocessimagenamew).

### 6.3. Đọc command line qua PEB

`PebReader.cpp` lấy địa chỉ `NtQueryInformationProcess` từ `ntdll.dll` bằng `GetModuleHandleW` và `GetProcAddress`. Nếu process đích là WOW64, code hỏi `ProcessWow64Information` để tìm PEB 32 bit; nếu không, hỏi `ProcessBasicInformation` để tìm PEB của process 64 bit. Ở mỗi nhánh, `ReadProcessMemory` chép PEB, rồi khối `ProcessParameters`, rồi các ký tự ở `CommandLine.Buffer` về chương trình.

`UNICODE_STRING.Length` đếm **byte**, còn `std::wstring` đếm **ký tự**, vì vậy code cấp `Length / sizeof(wchar_t)` ký tự. Trên môi trường Windows x64 đang dùng, `wchar_t` có kích thước 2 byte. `Peb32` và `Parameters32` chỉ chứa các trường/byte đệm cần thiết, không phải định nghĩa đầy đủ của cấu trúc nội bộ Windows. Code kiểm kết quả query, nhưng chưa kiểm từng kết quả `ReadProcessMemory` và số byte thực đọc; một lần đọc thất bại giữa đường có thể để lại dữ liệu rỗng/sai.

### 6.4. Đọc thread ID và state

`ThreadReader.cpp` lấy `NtQuerySystemInformation` từ `ntdll.dll`, gọi với `SystemProcessInformation` và một buffer 1 MiB. Khi Windows trả mã `0xC0000004` báo buffer quá nhỏ, code tăng gấp đôi rồi hỏi lại. Buffer có nhiều khối `SYSTEM_PROCESS_INFORMATION`; `NextEntryOffset` dẫn tới khối kế tiếp. Khi `UniqueProcessId` trùng PID cần tìm, các `SYSTEM_THREAD_INFORMATION` đi kèm được đọc để lấy `ClientId.UniqueThread` và `ThreadState`. `GetState` đổi số state thành nhãn như `Running`, `Waiting`; một vài `WaitReason` được ghi `Suspended`.

**Nguồn state là SystemProcessInformation, không phải TEB.** TEB không được đọc trong source hiện tại. Trạng thái là ảnh chụp tại lúc gọi API; nó có thể đổi rất nhanh. Các struct và Native API này là chi tiết nội bộ Windows, có thể thay đổi. [NtQuerySystemInformation](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation).

### 6.5. Đọc DLL đang nạp

`ModuleReader.cpp` tạo snapshot `TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32` cho PID. Khi nhận `ERROR_BAD_LENGTH`, code thử lại tối đa bốn lần vì module list có thể thay đổi lúc chụp. `MODULEENTRY32W` được đặt `dwSize` rồi duyệt bằng `Module32FirstW/NextW`. Code chỉ giữ tên có đuôi `.dll`, so không phân biệt hoa/thường, và chép `szModule` cùng `szExePath` vào `ModuleInfo`. Sau đó snapshot được đóng. Vì lọc đuôi DLL, mục EXE trong snapshot không được đưa vào báo cáo. [Module Walking](https://learn.microsoft.com/en-us/windows/win32/toolhelp/module-walking).

### 6.6. Đọc bảng PE Import của EXE

Khi đã có path, `PeImportReader.cpp` dùng `CreateFileW` mở file EXE trên đĩa. `CreateFileMappingW(..., PAGE_READONLY | SEC_IMAGE, ...)` yêu cầu Windows tạo ánh xạ theo bố cục PE; `MapViewOfFile` trả con trỏ `image` trong không gian nhớ của **chương trình Module 1**. Parser kiểm dấu MZ/PE, phân biệt PE32 và PE32+, lấy import data directory, rồi đọc `IMAGE_IMPORT_DESCRIPTOR`. Một descriptor mô tả một DLL. `Name` trỏ tới tên DLL; `OriginalFirstThunk` hoặc `FirstThunk` dẫn tới danh sách hàm. Với mỗi thunk, nếu bit ordinal bật thì code ghi `#số`; nếu không, giá trị trỏ tới hint/name và code bỏ qua `sizeof(WORD)` byte hint để lấy tên hàm. `ValidAddress` kiểm tra phạm vi theo `SizeOfImage`, `memchr` kiểm dấu kết thúc chuỗi. Khi xong, code `UnmapViewOfFile`; file/mapping handle được `CloseHandle`.

Bảng này mô tả import **được khai báo trong file EXE**. Nó không bao gồm mọi DLL có thể được nạp động bằng `LoadLibrary` hoặc delay import. Do đó không được đồng nhất với danh sách ở mục DLL Information. [PE Format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [SEC_IMAGE](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw).

### 6.7. Định dạng và lưu báo cáo

`FormatReport` dùng `std::wostringstream` để ghép các trường từ `ProcessReport` thành bốn mục. Phần thread, module, import dùng các vòng lặp trên `vector`; vòng trong của import thêm dấu phẩy giữa các hàm. Cùng chuỗi `text` được `main` in ra console và đưa cho `SaveReport`, tránh hai nơi định dạng khác nhau.

`SaveReport` đặt tên file `<name>-<pid>.txt`. `WideCharToMultiByte(CP_UTF8, ...)` được gọi hai lần: lần đầu hỏi cần bao nhiêu byte; lần sau đổi chữ rộng thành UTF-8. `std::ofstream` ghi byte ra file. `WidenAscii` mở rộng từng byte tên import thành ký tự rộng; nó phù hợp tên ASCII thông thường, nhưng không phải bộ giải mã UTF-8 tổng quát. [WideCharToMultiByte](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte).

## 7. Kiểm thử thực tế ngày 01-11-2026

### 7.1. Build sau khi dọn file cũ

MSBuild Debug x64 biên dịch đủ tám file `.cpp`: `main.cpp`, `PebReader.cpp`, `ProcessFinder.cpp`, `ProcessReader.cpp`, `ThreadReader.cpp`, `ModuleReader.cpp`, `PeImportReader.cpp` và `ReportWriter.cpp`. Kết quả là `x64\Debug\Module_1.exe`. Lần build này diễn ra **sau khi xóa hai bản `ProcessInfo.h` cũ**, cho thấy project hiện không phụ thuộc header đó.

### 7.2. PID tồn tại

Chạy EXE với PID `30792` của `pwsh.exe` đang hoạt động: mã thoát `0`. Output có đủ bốn tiêu đề `Process Information:`, `Thread Information:`, `DLL Information:` và `PE Import Information:`. File `pwsh.exe-30792.txt` được tạo trong thư mục Debug. Đầu file ghi name `pwsh.exe`, PID `30792`, PPID `29660`, architecture `64-bit`; trong PE Import có ví dụ `SHELL32.dll` và hàm `ShellExecuteW`. Những số PID/PPID này chỉ thuộc lần chạy thử ấy.

### 7.3. Tên tồn tại

Chạy `Module_1.exe pwsh.exe` khi có một `pwsh.exe` phù hợp: mã thoát `0`, một phần `Process Information:` và các dòng PE Import. Ca này xác nhận tìm theo tên trong tình trạng máy lúc thử; **chưa xác nhận ca nhiều process trùng tên**.

### 7.4. PID và tên không tồn tại

Chạy `Module_1.exe 999999` và `Module_1.exe ___no_such_process___.exe`: mỗi lần đều trả mã `1` và thông báo `Khong tim thay tien trinh.`. PID có thể được Windows tái sử dụng về sau, nên kết quả chỉ có giá trị tại thời điểm thử.

### 7.5. Chưa có bằng chứng kiểm thử

Chưa ghi bằng chứng Release x64, process x86/WOW64, nhiều process cùng tên, process bị hạn chế quyền, đầu vào PID vượt phạm vi, file PE hỏng, tên Unicode đặc biệt hoặc đối chiếu `dumpbin /IMPORTS`. Những mục này là **ca cần làm tiếp**, không được viết thành kết quả đã đạt.

## 8. Xử lý lỗi và giới hạn của source hiện tại

1. **Đầu vào:** `std::stoi` chưa được bọc `try/catch`. Chuỗi số quá lớn có thể ném exception. `int` và `DWORD` không cùng miền giá trị; code chưa parse PID 32 bit một cách đầy đủ.
2. **Tìm process:** `SearchResult.error` được tạo, nhưng `main` chỉ xem danh sách rỗng; lỗi snapshot có thể bị trình bày như không tìm thấy process.
3. **Bộ nhớ PEB:** các lần `ReadProcessMemory` chưa kiểm tra giá trị trả về/số byte đọc. Cấu trúc PEB, offset WOW64 và Native API là chi tiết nội bộ có thể thay đổi.
4. **Thread:** code chưa kiểm tra đầy đủ mã lỗi `NtQuerySystemInformation`, giới hạn buffer trước mọi `memcpy` và cách diễn giải mọi `WaitReason`. `main` cũng bỏ qua `bool` của `ReadThreads`.
5. **DLL:** module snapshot có thể thay đổi; `ModuleReader` chỉ giữ tên kết thúc `.dll`. Khi `Module32FirstW` thất bại, hàm hiện có thể trả `true` với danh sách rỗng. `main` bỏ qua `bool` của `ReadModules`.
6. **PE Import:** parser đọc normal import directory, không đọc delay import/nạp động. Kiểm tra `ValidAddress` là kiểm tra cơ bản theo `SizeOfImage`, không chứng minh an toàn với mọi PE hỏng; việc ánh xạ vẫn phụ thuộc Windows chấp nhận EXE.
7. **Ghi file:** `SaveReport` chưa kiểm tra kết quả lần chuyển mã thứ hai và trạng thái sau `file.write`. `main` bỏ qua `bool` của `SaveReport`, nên có thể in console mà không báo rõ lỗi ghi file.

Các giới hạn này là kết luận từ code hiện tại, không phải lỗi giả định đã xảy ra trong lần chạy thử.

## 9. Hướng phát triển

Ưu tiên đầu tiên là xác thực PID trước khi chuyển số và hiển thị lỗi riêng cho từng reader/`SaveReport`. Sau đó kiểm kết quả mọi `ReadProcessMemory`, kích thước buffer của ThreadReader và cải thiện nhận diện kiến trúc bằng `IsWow64Process2`. Về kiểm thử, nên chạy hai process cùng tên, một process 32 bit, một ca thiếu quyền, một file PE bất thường; đối chiếu PE Import với `dumpbin /IMPORTS`. Trước khi nộp, build Release x64, giữ screenshot/output và video demo. Đây là hướng đề xuất, **chưa được triển khai trong source hiện tại**.

## 10. Kết luận

Module 1 đã có luồng hoàn chỉnh từ đối số PID/tên đến báo cáo bốn nhóm và file UTF-8. Điểm kiến thức quan trọng nhất là phân biệt dữ liệu runtime của process với import table trong file PE; tiếp theo là vòng đời handle và quy tắc đọc bộ nhớ process khác. Trong lần kiểm chứng Debug x64, project build thành công, chạy với PID/tên tồn tại, tạo file báo cáo, và báo không tìm thấy với đầu vào không khớp. Các giới hạn ở mục 8 cần được trình bày trung thực khi đánh giá chất lượng project.

## 11. Tài liệu tham khảo

- [Microsoft: Taking a Snapshot and Viewing Processes — có code mẫu C/C++](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes).
- [Microsoft: CreateToolhelp32Snapshot](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot).
- [Microsoft: ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory).
- [Microsoft: NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess).
- [Microsoft: NtQuerySystemInformation](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysysteminformation).
- [Microsoft: PE Format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).
- [Microsoft: CreateFileMappingW](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw).
- [Microsoft: WideCharToMultiByte](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte).
