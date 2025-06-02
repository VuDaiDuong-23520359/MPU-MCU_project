# Bảng LED RGB (NeoPixel) với hiệu ứng “Light Show”
## 1. Mô tả tổng quan
•	Dùng vi điều khiển để điều khiển một ma trận LED RGB hoặc dải LED RGB (chẳng hạn WS2812B, SK6812 - thường gọi là “NeoPixel”).
•	Hiển thị các hiệu ứng màu sắc khác nhau: chuyển màu gradient, chạy theo mẫu (pattern), sóng cầu vồng (rainbow wave), chữ chạy (text scroller), v.v.
•	(Tuỳ chọn nâng cao) Đồng bộ âm nhạc: đọc tín hiệu âm thanh từ micro hoặc cổng audio, rồi đổi màu/nhấp nháy theo nhạc (music visualization).
## 2. Tại sao
•	LED RGB có thể hiển thị hàng triệu màu.
•	Hiệu ứng chuyển động nhiều màu, nháy theo nhạc, rất thu hút và có cảm giác “tiệc tùng” (party lighting).
•	Dễ chia sẻ, trình diễn, và gây ấn tượng.
________________________________________
## 3. Thành phần phần cứng đề xuất
### 1.	Vi điều khiển
o	Có thể là bất kỳ dòng ARM Cortex-M (ví dụ STM32F0, STM32F1, STM32F4, v.v.)
o	Yêu cầu: có đủ tốc độ để điều khiển được dải LED (thường giao tiếp 1-wire đặc biệt của WS2812B) và xử lý hiệu ứng.
### 2.	Dải LED RGB (WS2812B hoặc SK6812)
o	Chọn độ dài/mật độ tuỳ nhu cầu. Chẳng hạn, 1 dải 30 LED, 60 LED, hay ghép thành ma trận 8x8, 8x16, 16x16, etc.
o	Nguồn cấp: cần chú ý cấp đủ dòng (mỗi LED RGB có thể tiêu thụ tới 60mA ở trắng sáng tối đa).
### 3.	Micro/Module audio (nếu muốn đồng bộ nhạc)
o	Đơn giản nhất: module mic electret + mạch khuếch đại (VD: LM386) để đo cường độ âm.
o	Nâng cao: module tích hợp FFT hoặc tự viết thuật toán FFT (cần MCU mạnh hơn).
### 4.	Các nút bấm (hoặc rotary encoder)
o	Để chuyển đổi hiệu ứng, thay đổi độ sáng, tốc độ, v.v.
### 5.	Màn hình hiển thị nhỏ (tùy chọn)
o	Ví dụ OLED I2C 0.96 inch để hiển thị menu chọn hiệu ứng.
o	Không bắt buộc, nhưng tăng tính trực quan.
________________________________________
## 4. Chức năng và các bước triển khai
### Chức năng cơ bản
#### 1.	Điều khiển LED
o	Lập trình gửi tín hiệu điều khiển 1-Wire đặc thù của WS2812B (chuỗi xung 800kHz).
o	Xác định mảng lưu giá trị màu (RGB) cho từng LED, sau đó xuất liên tục để duy trì màu mong muốn.
#### 2.	Các hiệu ứng chuyển màu
o	Chuyển màu đơn: tất cả LED cùng đổi màu dần dần (fade in/out).
o	Cầu vồng (rainbow): màu trải theo dải LED từ đầu đến cuối, chuyển động liên tục.
o	Chạy pixel: 1 hoặc nhiều “đốm sáng” chạy qua dải LED.
o	Nhấp nháy (strobe): LED chớp tắt theo tần số định sẵn.
#### 3.	Chuyển đổi hiệu ứng
o	Sử dụng nút bấm “Mode” để chọn hiệu ứng.
o	Sử dụng nút “Next” để thay đổi màu chủ đạo, tốc độ…
### Chức năng nâng cao (đồng bộ âm nhạc - Music Visualization)
#### 1.	Đọc cường độ âm thanh qua ADC
o	Cảm biến micro -> mạch khuếch đại -> ADC MCU.
o	Lấy giá trị tức thời hoặc giá trị trung bình sau một khoảng thời gian ngắn.
#### 2.	Thay đổi màu/độ sáng theo cường độ
o	Ví dụ: khi âm thanh lớn, LED sáng mạnh hoặc chuyển sang màu nóng. Khi âm thanh thấp, LED dịu dần.
#### 3.	Phân tích tần số (FFT) (nếu đủ tài nguyên & muốn sâu hơn)
o	Chia LED thành các dải màu tương ứng với các dải tần (bass, mid, treble).
o	Yêu cầu MCU có khả năng xử lý nhanh (VD: STM32F4).



## ⚙️ Nguyên lý điều khiển WS2812B
### 1. Cấu trúc bên trong WS2812B
#### 1.	LED RGB: Một con WS2812B chứa 3 diode LED (Red, Green, Blue).
#### 2.	Driver tích hợp: Bên trong WS2812B có mạch nhận dữ liệu (Data In) và giải mã để điều khiển độ sáng (PWM) cho từng màu (R, G, B).
Mỗi WS2812B có Data In và Data Out. Khi bạn gửi dữ liệu vào con LED đầu tiên, nó sẽ “lấy” phần dữ liệu dành cho nó, sau đó truyền tiếp phần còn lại sang Data Out để đưa tới LED kế tiếp.
________________________________________
### 2. Giao thức 1-wire ở tốc độ cao
WS2812B sử dụng một kiểu giao thức 1-wire có timing rất chặt chẽ ở tần số khoảng 800 kHz. Dữ liệu được truyền theo bit (24 bit cho mỗi LED, tương ứng 8 bit cho màu Xanh Lục – Green, 8 bit cho Đỏ – Red, 8 bit cho Xanh Dương – Blue).
Lưu ý thứ tự bit màu: Thông thường WS2812B sử dụng thứ tự GRB (Green - Red - Blue), chứ không phải RGB.
#### 2.1. Cấu trúc gói dữ liệu
•	Mỗi LED cần 24 bit (GRB).
o	Ví dụ: màu xanh dương (Blue) thuần = G=0, R=0, B=255 => 24 bit = 00000000 00000000 11111111.
•	Tổng dải LED: Nếu có N LED, ta gửi lần lượt 24*N bit. LED đầu tiên sẽ nhận 24 bit đầu tiên, LED thứ 2 nhận 24 bit kế tiếp, v.v.
#### 2.2. Cách mã hóa bit “0” và bit “1”
Mỗi bit được truyền trong khoảng 1.25 microseconds (µs), được chia thành 2 pha: pha “High” (mức 1) và pha “Low” (mức 0). Thời lượng pha High/Low sẽ quyết định bit đó là 0 hay 1.
•	Bit “0”:
o	High ~ 0.35 - 0.45 µs
o	Low ~ 0.80 - 0.90 µs (sao cho tổng ~1.25 µs)
•	Bit “1”:
o	High ~ 0.70 - 0.80 µs
o	Low ~ 0.45 - 0.55 µs (tổng vẫn ~1.25 µs)
Các timing cụ thể có thể hơi khác nhau một chút tùy datasheet, nhưng phải nằm trong ngưỡng mà WS2812B chấp nhận.
#### 2.3. Khoảng “Reset” (latch)
Sau khi gửi xong tất cả bit (24*N bit cho N LED), cần giữ đường Data ở mức thấp tối thiểu 50 µs (thường người ta gọi là “Reset code” hay “Latch”). Khoảng nghỉ này cho phép WS2812B “chốt” (latch) dữ liệu và hiển thị màu lên LED.
________________________________________
### 3. Quá trình điều khiển
Để điều khiển dải LED WS2812B:
#### 1.	Chuẩn bị mảng dữ liệu:
o	Tạo một mảng (array) có chiều dài = N LED * 3 byte, tương ứng GRB. Mỗi LED có thể được gán giá trị (G, R, B) [0..255].
#### 2.	Mã hóa bit để gửi theo đúng chuẩn timing**:
o	Mỗi byte (8 bit) của G, R, B sẽ được chuyển thành chuỗi xung trên Data line.
o	Thông thường, người ta dùng thuật toán “bit-banging” hoặc sử dụng DMA + SPI (nếu xung chuẩn) để sinh xung với đúng timing.
#### 3.	Gửi dữ liệu:
o	Lần lượt phát 24 bit cho LED1, 24 bit cho LED2, …, cho đến LED N.
o	Cuối cùng, giữ Data = 0 ít nhất 50 µs để reset.
#### 4.	Lặp lại khi muốn thay đổi màu. Nếu không gửi lại, WS2812B vẫn lưu màu cũ và tiếp tục hiển thị.
________________________________________
### 4. Một ví dụ timing đơn giản (minh họa)
Giả sử một bit “0” muốn truyền, ta cần:
•	High ~ 0.4 µs, Low ~ 0.85 µs.
Giả sử một bit “1” muốn truyền, ta cần:
•	High ~ 0.8 µs, Low ~ 0.45 µs.
Nếu MCU chạy đủ nhanh (ví dụ 48 MHz, 72 MHz, 80 MHz, 180 MHz, v.v.), ta có thể tạo các delay chính xác (bằng lệnh asm hoặc timer) để tạo ra xung, hoặc sử dụng phần cứng (như SPI hoặc PWM “hack” timing) để giảm tải CPU.

