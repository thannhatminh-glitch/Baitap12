

## 📘 Giới thiệu

Chương trình này được viết cho vi điều khiển **STM32F103**, sử dụng **FreeRTOS** để quản lý đa nhiệm.
Mục tiêu là điều khiển **đèn LED nhấp nháy** theo các **tần số (Hz)** và **độ rộng xung (Duty Cycle)** khác nhau, đồng thời cho phép **thay đổi tham số động** trong thời gian thực thông qua **Queue**.

---

## ⚙️ Thành phần chính

### 🧩 Cấu trúc dữ liệu `Led_config_t`

```c
typedef struct
{
    uint32_t frequency; // Tần số nhấp nháy (Hz)
    uint32_t duty;      // Độ rộng xung (%)
} Led_config_t;
```

Cấu trúc này được dùng để chứa **tần số** và **độ rộng xung** cho LED, và được truyền qua **Queue** giữa hai task.

---

## 🪜 Sơ đồ hoạt động

```
+-----------------------+
|   TASK_UPDATE_PARAMS  |
|-----------------------|
| - Chu kỳ 5s           |
| - Gửi cấu trúc chứa   |
|   frequency, duty     |
|   qua Queue           |
+-----------+-----------+
            |
            v
+------------------------+
|    TASK_LED_CONTROL    |
|------------------------|
| - Nhận cấu trúc từ     |
|   Queue (xQueueReceive)|
| - Tính toán thời gian  |
|   T_on và T_off        |
| - Bật/Tắt LED tương    |
|   ứng với duty cycle   |
+------------------------+
```

---

## 🧠 Giải thích hoạt động

1. **Task `TASK_UPDATE_PARAMETERS`**

   * Chứa một mảng các cặp giá trị `{tần số, duty}`:

     ```c
     {2, 10}, {4, 30}, {6, 50}, {8, 70}, {10, 90}
     ```
   * Cứ mỗi **5 giây**, task này sẽ gửi một cấu trúc `Led_config_t` mới vào Queue.
   * Dùng hàm `xQueueOverwrite()` để **ghi đè** giá trị cũ nếu Queue đầy (chỉ chứa 1 phần tử).

2. **Task `TASK_LED_CONTROL`**

   * Ban đầu có giá trị mặc định: `2Hz, 50% duty`.
   * Liên tục đọc giá trị mới từ Queue (nếu có).
   * Tính chu kỳ LED theo:

     ```c
     T_period = configTICK_RATE_HZ / frequency;
     T_on  = T_period * duty / 100;
     T_off = T_period - T_on;
     ```
   * Dùng `GPIO_ResetBits()` để bật LED (mức thấp), và `GPIO_SetBits()` để tắt LED (mức cao).
   * Trong khi LED đang bật hoặc tắt, task vẫn chờ xem có giá trị mới được gửi qua Queue hay không — nếu có, sẽ cập nhật ngay mà **không chờ hết chu kỳ**.

3. **Sử dụng Queue**

   * Queue được tạo bằng:

     ```c
     xQueue_led_config = xQueueCreate(1, sizeof(Led_config_t));
     ```
   * Chứa **duy nhất một cấu trúc tham số**.
   * Hai task sử dụng chung Queue này để **truyền dữ liệu đồng bộ**.

---

## 🔩 Cấu hình phần cứng

| Chân STM32 | Chức năng | Ghi chú        |
| ---------- | --------- | -------------- |
| PB12       | Output    | Điều khiển LED |
| GND        |           | Cực âm LED     |
| VCC        |           | Cực dương LED  |

LED được điều khiển bằng **mức logic thấp (0)** → **LED sáng**,
và **mức cao (1)** → **LED tắt**.

---

## 🧰 Các thành phần sử dụng

* **STM32F10x Standard Peripheral Library**
* **FreeRTOS**

  * Task Management (`task.h`)
  * Queue Management (`queue.h`)

---

## 🧾 Tóm tắt chức năng

| Tên Task                 | Chức năng chính                                    | Ưu tiên |
| ------------------------ | -------------------------------------------------- | ------- |
| `TASK_LED_CONTROL`       | Điều khiển bật/tắt LED theo tần số & duty hiện tại | 1       |
| `TASK_UPDATE_PARAMETERS` | Cập nhật tần số & duty mới gửi qua Queue mỗi 5s    | 2       |

---

## 🧮 Chu kỳ ví dụ

| Tần số (Hz) | Chu kỳ (ms) | Duty (%) | LED sáng (ms) | LED tắt (ms) |
| ----------- | ----------- | -------- | ------------- | ------------ |
| 2           | 500         | 10       | 50            | 450          |
| 4           | 250         | 30       | 75            | 175          |
| 6           | 167         | 50       | 83            | 83           |
| 8           | 125         | 70       | 88            | 37           |
| 10          | 100         | 90       | 90            | 10           |

---

## 🧩 Ưu điểm của thiết kế

✅ Dễ mở rộng: chỉ cần thêm phần tử vào mảng `parameters`.
✅ Thay đổi tham số động: nhờ cơ chế Queue, LED thay đổi tức thì.
✅ Đảm bảo tính thời gian thực: sử dụng **tick** của FreeRTOS thay vì delay blocking.
✅ Độc lập phần cứng: chỉ phụ thuộc vào FreeRTOS và GPIO.

