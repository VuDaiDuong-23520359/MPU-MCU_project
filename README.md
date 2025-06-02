# 🌈 Trình diễn ánh sáng với NeoPixel

## 1. 🔍 Tổng quan

Tạo các hiệu ứng ánh sáng đầy màu sắc sử dụng vi điều khiển và LED RGB WS2812B (NeoPixel). Dự án này trình diễn nhiều hiệu ứng động như chuyển màu gradient, sóng cầu vồng, đốm sáng chạy, và đồng bộ hóa với âm nhạc (tùy chọn).

### ✨ Tính năng

* Điều khiến LEDs theo cường độ âm thanh của bài nhạc

## 2. 💡 Tại sao chọn NeoPixel?

* Hiển thị hàng triệu màu nhờ trộn RGB
* Tạo hiệu ứng ánh sáng sôi động như tiệc tùng
* Dễ gây ấn tượng và tùy chỉnh

## 3. 🧰 Phần cứng đề xuất

| Thành phần                      | Mô tả                                                                      |
| ------------------------------- | -------------------------------------------------------------------------- |
| 🧠 Vi điều khiển                | STM32F407VET6                                                              |
| 💡 Dải LED WS2812B              | W2812B 54-LEDs                                                             |
| 🔋 Nguồn cấp                    | 5V                                                                         |
| 🎤 Module âm thanh *(tùy chọn)* | MAX9814 - mạch khuếch đại âm thanh                                         |
| 🔘 Nút bấm/Encoder              | Chuyển chế độ, chỉnh tốc độ, độ sáng,... (đang phát triển)                 |
| 💻 Màn hình OLED *(tùy chọn)*   | OLED I2C 0.96" để hiển thị menu, tên hiệu ứng (đang phát triển)            |

## 4. ⚙️ Chức năng chính

### 🗂 A. Điều khiển LED cơ bản

* Giao tiếp chuẩn 1-Wire tốc độ \~800kHz
* Gửi 24 bit dữ liệu GRB cho mỗi LED
* Giữ mức thấp ≥ 50µs để cập nhật dữ liệu (latch)

### 🌟 B. Các hiệu ứng

| Hiệu ứng      | Mô tả                                   |
| ------------- | --------------------------------------- |
| 🔁 Soundbar    | Số LED sáng dựa trên cường độ âm thanh  |
| 💨 Pixel chạy  | Một hoặc nhiều đốm sáng di chuyển       |
| ⚡ Nhấp nháy   | LED chớp nhanh theo chu kỳ              |

### 🔘 C. Chuyển đổi hiệu ứng (Đang phát triển)

* Nút "Mode": chuyển đổi hiệu ứng
* Nút "Next": thay đổi màu chủ đạo, tốc độ, v.v.

## 5. 🎶 Hiển thị theo nhạc *(tùy chọn nâng cao)*

### 📥 A. Nhận tín hiệu âm thanh

* Micro thu âm thanh → khuếch đại → ADC của vi điều khiển
* Đo độ lớn trung bình hoặc tức thời của tín hiệu

### 🎨 B. Phản ứng ánh sáng

* Âm lớn → LED sáng mạnh hoặc màu nóng
* Âm nhỏ → LED dịu hoặc chuyển màu lạnh

### 🧠 C. Phân tích tần số (FFT) (Đang phát triển)

* Chia dải LED theo tần số: Bass / Mid / Treble
* Mỗi phần LED hiển thị một dải tần riêng
* Yêu cầu vi điều khiển mạnh (STM32F4, ESP32,...)

## 6. 📢 Giao thức điều khiển WS2812B

### A. Cấu trúc dữ liệu

* Mỗi LED: 24 bit (GRB)
* LED đầu tiên lấy 24 bit đầu, phần còn lại truyền tiếp

### B. Thời gian của từng bit

| Bit | Mức Cao       | Mức Thấp      |
| --- | ------------- | ------------- |
| 0   | \~0.35–0.45µs | \~0.80–0.90µs |
| 1   | \~0.70–0.80µs | \~0.45–0.55µs |

* Tổng thời gian mỗi bit \~1.25µs
* Sau khi truyền xong: giữ mức thấp ≥ 50µs để reset/latch

### C. Phương pháp điều khiển

| Phương pháp    | Mô tả                                                                     |
| -------------- | ------------------------------------------------------------------------- |
| 🛠 Bit-banging  | Điều khiển GPIO thủ công bằng delay (tốn CPU)                             |
| 🔄 DMA + PWM   | Sử dụng Timer và DMA để tạo tín hiệu chính xác                            |

## 7. 🔹 Ví dụ truyền dữ liệu

* Màu xanh dương thuần = `G=0, R=0, B=255` → `00000000 00000000 11111111`
* Gửi lần lượt 24 bit cho mỗi LED

---

