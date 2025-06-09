# 🌈 Trình diễn ánh sáng với NeoPixel

## 1. 🔍 Tổng quan

Tạo các hiệu ứng ánh sáng đầy màu sắc sử dụng vi điều khiển và LED RGB WS2812B (NeoPixel). Dự án này trình diễn nhiều hiệu ứng động đồng bộ hóa với âm nhạc.

### ✨ Tính năng

* Điều khiến LEDs theo cường độ & tần số âm thanh của bài nhạc

## 2. 💡 Tại sao chọn NeoPixel?

* Hiển thị hàng triệu màu nhờ trộn RGB
* Tạo hiệu ứng ánh sáng sôi động như tiệc tùng
* Dễ gây ấn tượng và tùy chỉnh

## 3. 🧰 Phần cứng đề xuất

| Thành phần                      | Mô tả                                                                      |
| ------------------------------- | -------------------------------------------------------------------------- |
| 🧠 Vi điều khiển                | STM32F407VET6                                                              |
| 💡 Dải LED WS2812B              | W2812B 54LEDs                                                              |
| 🔋 Nguồn cấp                    | 5V                                                                         |
| 🎤 Module âm thanh              | MAX9814 - mạch khuếch đại âm thanh                                         |
| 🔘 Nút bấm/Encoder              | Module nút bấm 12x12 (2 cái) - Chuyển chế độ & Điều chỉnh độ sáng          |

## 4. ⚙️ Chức năng chính

### 🗂 A. Điều khiển LED cơ bản

* Giao tiếp chuẩn 1-Wire tốc độ \~800kHz
* Gửi 24 bit dữ liệu GRB cho mỗi LED
* Giữ mức thấp ≥ 50µs để cập nhật dữ liệu (latch)

### 🌟 B. Các hiệu ứng

| **Hiệu ứng**         | **Mô tả**                                                                                        | **Dựa vào**                  |
| -------------------- | ------------------------------------------------------------------------------------------------ | ---------------------------- |
| **Sound Color**      | LED đổi màu và độ sáng theo âm lượng, tạo hiệu ứng “nhiệt độ âm thanh”                           | Âm lượng                     |
| **Ripple**           | Hiệu ứng gợn sóng lan từ giữa dải LED, độ sáng giảm dần theo khoảng cách                         | Âm lượng                     |
| **Sound Bar**        | Hiển thị thanh VU meter với gradient màu từ tím đến đỏ                                           | Âm lượng                     |
| **Random 1/6 LEDs**  | Mỗi nhóm 6 LED sẽ ngẫu nhiên sáng 1 LED, nhấp nháy theo nhạc                                     | Âm lượng                     |
| **Flash Fade**       | Mỗi khi phát hiện beat mạnh, toàn dải đổi màu ngẫu nhiên rồi mờ dần                              | Tần số đỉnh (Beat)           |
| **Dynamic VU Meter** | Tăng dần số lượng LED từ trung tâm ra 2 bên; màu theo tần số                                     | Âm lượng & Tần số            |
| **Spectrum Bands**   | Dải LED chia 3 vùng: bass (trái), mid (giữa), treble (phải); sáng vùng tương ứng với tần số đỉnh | Tần số                       |
| **Frequency Chase**  | Mỗi beat tạo một dải sáng di chuyển từ đầu dải LED, màu sắc theo tần số đỉnh                     | Tần số đỉnh & Beat           |
| **Rainbow Roll**     | Dải màu cầu vồng cuộn đều, tốc độ phụ thuộc tần số đỉnh                                          | Tần số đỉnh                  |
| **Bass Pulse Glow**  | Khi phát hiện bass mạnh, cả dải LED sáng lên theo màu cầu vồng rồi mờ dần                        | Bass (tần số thấp & biên độ) |

### 🔘 C. Chuyển đổi hiệu ứng 

* Nút "Mode": chuyển đổi hiệu ứng
* Nút "BrightNess": Điều chỉnh độ sáng 

## 5. 🎶 Hiển thị theo nhạc 

### 📥 A. Nhận tín hiệu âm thanh

* Micro thu âm thanh → khuếch đại → ADC của vi điều khiển
* Đo độ lớn trung bình hoặc tức thời của tín hiệu

### 🎨 B. Phản ứng ánh sáng

* Âm lớn → LED sáng mạnh, màu nóng
* Âm nhỏ → LED dịu, màu lạnh

### 🧠 C. Phân tích tần số (FFT) 

* Chia dải LED theo tần số: Bass / Mid / Treble
* Mỗi phần LED hiển thị một dải tần riêng
* Sử dụng Timer2 trigger ADC1

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

