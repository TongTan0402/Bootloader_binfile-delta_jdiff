# **CÁCH TẠO FILE .BIN TRONG DỰ ÁN KEILC**
- **Bước 1:** Vào `Option for Target` -> `User`
- **Bước 2:** Tích vào ô `Run #1` trong phần `After Build/Rebuild`
- **Bước 3:** Nhập lệnh sau:
  ```fromelf --bin --output .\Objects\[file_name].bin .\Objects\[file_name].axf```
- **Bước 4:** Nhấn `OK` để hoàn tất

# **CÁCH TẠO FILE .PATCH TỪ OLD.BIN + NEW.BIN**
## Phương pháp 1: Sử dụng file `make_patch.bat`.
- Bước 1: Build file `.bin` từ dự án.
- Bước 2: Mở file `make_patch.bat`.
- Bước 3: Cập nhật `base_path` tới thư mục chứa file `.bin` vừa build.
- Bước 4: Đổi tên `file_name` thành tên file `.bin` vừa build.
- Bước 5: Chạy file `make_patch.bat`.

## Phương pháp 2: Sử dụng file make_patch.bat.
- Bước 1: Mở `cmd`.
- Bước 2: Chạy lệnh sau:
  ```
  ./jdiff.exe [old_file].bin [new_file].bin patch.patch
  ```