# **CÁCH ĐỌC THẺ NHỚ**
- Gọi hàm `SD_Init()`. Nếu giá trị trả về `=0` -> Thẻ nhớ khởi tạo thành công
- Nếu muốn ghi file vào thẻ nhớ thì gọi hàm `SD_WriteFile([file_name], [&data], [size], [offset])`.
- Nếu muốn đọc file trên thẻ nhớ thì gọi hàm `SD_ReadFile([file_name], [&data], [size], [offset])`. Dữ liệu của file được lưu trong mảng `data`.