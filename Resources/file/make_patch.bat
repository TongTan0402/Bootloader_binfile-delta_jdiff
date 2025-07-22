@echo off
setlocal enabledelayedexpansion
:: Đường dẫn đầy đủ đến file .bin được build từ Keil MDK
set base_path=D:\Documents\Programs\STM32\SPL\Blink_Led\Objects\

set file_name=Blink_Led.bin

set BIN_PATH=%base_path%%file_name%

:: Kiểm tra xem file .bin có tồn tại không
if exist %BIN_PATH% (
    echo Found %file_name%, processing...

    :: Di chuyển bản build mới vào thư mục hiện tại
    copy /Y %BIN_PATH% %file_name%

    :: Hiển thị danh sách file (tuỳ chọn)
    dir /B /O:D

    :: Xoá old.bin nếu có
    if exist old.bin del /f old.bin

    :: Đổi tên new.bin thành old.bin nếu tồn tại
    if exist new.bin rename new.bin old.bin

    :: Xoá patch.patch nếu có
    if exist patch.patch del /f patch.patch

    :: Đổi tên bản build mới thành new.bin
    rename %file_name% new.bin

    :: Tạo patch bằng jdiff.exe
    if exist jdiff.exe (
        jdiff.exe old.bin new.bin patch.patch
        if %errorlevel%==0 (
            echo Patch created successfully: patch.patch
        ) else (
            echo Failed to create patch.
        )
    ) else (
        echo jdiff.exe not found! Please place jdiff.exe in this folder.
    )

) else (
    echo Không tìm thấy file %file_name% trong đường dẫn %BIN_PATH%. Vui lòng kiểm tra lại.
)

endlocal