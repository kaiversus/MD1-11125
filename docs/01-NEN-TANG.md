# 01. Kiến thức nền: học từ ví dụ trước khi đọc Windows API

## 1. Chương trình dòng lệnh nhận đầu vào thế nào?

Ví dụ bạn gõ `Module_1.exe notepad.exe`. Windows khởi chạy chương trình và đưa cho `wmain` một danh sách:

```text
argv[0] = đường dẫn/tên chương trình Module_1.exe
argv[1] = "notepad.exe"
argc    = 2 (có 2 phần tử)
```

`wmain` là biến thể của `main` dùng ký tự rộng (`wchar_t`) để nhận tên có Unicode. `argc != 2` nghĩa là người dùng chưa đưa **đúng một** tham số. `return 0` thường báo thành công; `return 1` báo một tình huống không thành công theo quy ước của chương trình này.

## 2. Những ký hiệu C++ bạn sẽ gặp liên tục

```cpp
std::wstring name = L"notepad.exe";
std::vector<ProcessEntry> processes;
processes.push_back(entry);
bool ok = ReadModules(pid, modules, error);
```

- `std::` chỉ “thứ này nằm trong thư viện chuẩn C++”.
- `std::wstring` là chuỗi ký tự rộng. Chữ `L` trong `L"abc"` tạo chuỗi ký tự rộng tương ứng. `std::string` thường chứa các byte `char`.
- `std::vector<T>` là danh sách có thể lớn lên; `T` là kiểu của từng phần tử. `push_back` thêm một phần tử.
- `bool` nhận `true` hoặc `false`. Hàm trả `bool` thường trả `true` nếu lấy dữ liệu thành công.
- `struct` gom nhiều thông tin có liên quan: một `ProcessEntry` có tên, PID, PPID.
- `const T& x` nghĩa là hàm xem `x` có sẵn mà không sao chép, và hứa không sửa nó. `T& out` cho phép hàm đổ kết quả vào biến của người gọi.
- `T* p` là **con trỏ**, giữ địa chỉ của `T`. `nullptr` nghĩa là không trỏ vào đâu. `p->field` là đọc trường `field` của vật mà `p` trỏ tới.
- `&variable` (trong lời gọi hàm) đưa **địa chỉ** của biến để hàm khác ghi kết quả. Nó khác dấu `&` trong khai báo reference.
- `sizeof(T)` hỏi kích thước của kiểu `T` tính bằng byte. `{ }` sau khai báo như `entry{}` thường khởi tạo các trường về giá trị mặc định/0.
- `?` và `:` là cách viết ngắn của `if/else`: `condition ? A : B`.
- `|` ghép các cờ bit (ví dụ hai quyền truy cập). `||` là “hoặc” logic; hai dấu này **khác nhau**.

Ví dụ về tham chiếu: `ReadModules(pid, report.modules, error)` có thể sửa `report.modules` và `error` của `main` vì các tham số trong header là `&`. Xem [pointer](https://learn.microsoft.com/en-us/cpp/cpp/pointers-cpp), [reference](https://learn.microsoft.com/en-us/cpp/cpp/references-cpp), [struct](https://learn.microsoft.com/en-us/cpp/cpp/struct-cpp), [vector](https://learn.microsoft.com/en-us/cpp/standard-library/vector-class).

## 3. Process, thread, DLL

Hãy hình dung **process** như một phòng làm việc đang mở cho một chương trình. Nó có không gian bộ nhớ riêng, tài nguyên riêng và một PID. **Thread** là một luồng công việc chạy trong phòng đó; một process có thể có nhiều thread. **DLL** là thư viện mã mà process có thể dùng. `notepad.exe` là file trên đĩa; “tiến trình Notepad” là bản đang chạy trong bộ nhớ. Có thể mở hai Notepad và được hai PID khác nhau.

**PPID** là PID của process cha tại lúc Windows ghi nhận. PID cha có thể đã thoát; đừng mặc định PPID luôn trỏ tới một process còn sống.

## 4. PID, HANDLE, quyền truy cập

PID giống **số phòng**. `OpenProcess` xin một **thẻ ra vào** (`HANDLE`) với quyền cụ thể. Ở đây code xin `PROCESS_QUERY_INFORMATION` để hỏi thông tin và `PROCESS_VM_READ` để đọc bộ nhớ. Windows có thể từ chối vì quyền, bảo vệ hệ thống, hoặc process đã thoát. Thẻ dùng xong phải `CloseHandle`. Một snapshot và file mapping cũng có handle riêng. [Microsoft: process handles và PID](https://learn.microsoft.com/en-us/windows/win32/procthread/process-handles-and-identifiers), [OpenProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess).

## 5. Snapshot là gì?

Snapshot là bản chụp **tại một thời điểm**. `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)` tạo bản chụp process; `Process32FirstW` đọc mục đầu; `Process32NextW` đọc các mục sau. `TH32CS_SNAPMODULE` chụp module của một PID. Vì hệ thống vẫn chạy trong lúc chương trình đọc, dữ liệu có thể thay đổi giữa hai bước. Xem [Tool Help snapshot](https://learn.microsoft.com/en-us/windows/win32/api/tlhelp32/nf-tlhelp32-createtoolhelp32snapshot) và [code mẫu Microsoft](https://learn.microsoft.com/en-us/windows/desktop/ToolHelp/taking-a-snapshot-and-viewing-processes).

## 6. Bộ nhớ của process khác và PEB

Mỗi process có “bản đồ địa chỉ” riêng. Số địa chỉ `0x1234` trong process A không tự động chỉ cùng vật trong process B. Vì vậy con trỏ trong PEB của process B **không được dereference trực tiếp** từ chương trình này; phải gọi `ReadProcessMemory(handleB, diaChiTrongB, bienCuaTa, soByte, ...)`. Hàm này sao chép byte từ B sang biến của ta. [ReadProcessMemory](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-readprocessmemory).

PEB (Process Environment Block) là cấu trúc nội bộ của Windows chứa các thông tin về process. Trong code, đường đi là:

```text
HANDLE process đích
  -> NtQueryInformationProcess -> địa chỉ PEB của đích
  -> ReadProcessMemory -> bản sao PEB ở chương trình ta
  -> ProcessParameters -> địa chỉ khối tham số của đích
  -> ReadProcessMemory -> bản sao khối tham số
  -> CommandLine.Buffer -> địa chỉ chữ của command line
  -> ReadProcessMemory -> chuỗi command line
```

`UNICODE_STRING.Length` là **số byte**, còn `std::wstring` đếm **số ký tự wchar_t**. Trên Windows một `wchar_t` thường là 2 byte, nên code chia `Length / sizeof(wchar_t)`. WOW64 là môi trường chạy chương trình 32 bit trên Windows 64 bit; vì con trỏ 32 bit và 64 bit khác kích thước, `PebReader.cpp` có hai nhánh. [NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess).

## 7. PE Import khác DLL đang nạp

PE (Portable Executable) là định dạng của `.exe` và `.dll` trên Windows. File PE có DOS header (dấu `MZ`), PE header, nhiều section, và các “data directory”. Import directory liệt kê DLL và hàm mà chương trình khai báo sẽ dùng. Ví dụ import `KERNEL32.dll` và `CreateFileW`. Đó là thông tin **trong file**, không phải ảnh chụp bộ nhớ process đang chạy.

**RVA** là vị trí tương đối so với đầu ảnh PE khi Windows nạp vào bộ nhớ. Với `SEC_IMAGE`, Windows sắp xếp file thành ảnh trong bộ nhớ; code dùng `image + rva`. Thường đọc file thô phải tự đổi RVA sang file offset theo section, nhưng code hiện tại nhờ Windows làm phần ánh xạ. [Microsoft PE Format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format), [CreateFileMappingW và SEC_IMAGE](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-createfilemappingw).

## 8. Unicode, `W`, UTF-8

Tên hàm Windows có hậu tố `W` như `CreateFileW`, `Process32FirstW` nhận chuỗi Unicode dạng ký tự rộng. `std::wcout` in chuỗi rộng. Khi ghi file text, chương trình đổi chuỗi rộng sang byte UTF-8 bằng `WideCharToMultiByte(CP_UTF8, ...)`. Hai bước thường là: hỏi cần bao nhiêu byte, cấp chuỗi byte, rồi chuyển đổi. [WideCharToMultiByte](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte).

## 9. Cách học chủ động

Trước khi nhìn từng lời giải, tự trả lời: “Đầu vào của hàm là gì? Nó lấy dữ liệu ở đâu? Nó đổ vào biến nào? Nó trả `true/false` trong trường hợp nào? Tài nguyên nào cần đóng?” Nếu trả lời được năm câu đó cho từng `.cpp`, bạn đã hiểu phần lớn chương trình.

