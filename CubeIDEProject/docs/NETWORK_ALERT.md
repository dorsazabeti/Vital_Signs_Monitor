# راهنمای کامل شبکه و هشدار پروژه

## نتیجهٔ پیاده‌سازی

مسیر شبکه بدون وابستگی به LwIP و بدون تغییر `main.c` تکمیل شده است. برد با IP ثابت از طریق Ethernet هشدار را به‌صورت UDP/JSON به رایانه می‌فرستد. برنامهٔ Python رایانه پیام را اعتبارسنجی می‌کند، در ترمینال نشان می‌دهد، در فایل ثبت می‌کند و روی macOS اعلان دسکتاپ می‌سازد. در صورت نیاز همان پیام می‌تواند به یک HTTP webhook نیز Forward شود.

این راه‌حل با امکانات فعلی مخزن سازگار است: CubeMX فقط درایور ETH را تولید کرده و LwIP در پروژه وجود ندارد، بنابراین یک مسیر کوچک ARP + IPv4 + UDP مستقیماً روی HAL نوشته شده است. IP ثابت نیز یکی از دو روش مجاز DHCP/static IP در صورت پروژه است.

## معماری

```text
Python simulator -> UART -> STM32 parser -> NetworkAlert_UpdateVitals()
                                             |
                                      threshold/debounce
                                             |
STM32 LAN8742A <- RMII <- ARP + IPv4 + UDP <-+
        |
        | Ethernet cable, UDP port 5055
        v
Python network_alert receiver
        |-- terminal output
        |-- JSONL log
        |-- macOS desktop notification
        `-- optional HTTP webhook
```

## تنظیمات پیش‌فرض

برای دموی مستقیم برد به لپ‌تاپ:

| وسیله | IP | Netmask |
| --- | --- | --- |
| لپ‌تاپ | `192.168.7.1` | `255.255.255.0` |
| برد | `192.168.7.2` | `255.255.255.0` |

- UDP مقصد: `5055`
- UDP مبدأ برد: `5056`
- MAC برد: `00:80:E1:00:00:00`
- PHY: مدل LAN8742A با آدرس صفر

همهٔ این مقادیر فقط در `Core/Inc/network_alert_config.h` قرار دارند. برای اتصال از طریق مودم یا سوییچ، IP برد، IP گیرنده، netmask و gateway را در همان فایل متناسب با شبکه عوض کنید. IP انتخاب‌شده برای برد نباید توسط وسیلهٔ دیگری استفاده شود.

## منطق هشدار

Thresholdهای فعلی:

- HR کمتر از `60` یا بیشتر از `100`
- SpO2 کمتر از `92%`
- دما کمتر از `35.0 C` یا حداقل `38.0 C`

برای جلوگیری از هشدار لحظه‌ای، سه نمونهٔ غیرعادی متوالی لازم است. بعد از فعال شدن، حداکثر هر ۳۰ ثانیه یک Reminder ارسال می‌شود. با دریافت سه نمونهٔ عادی متوالی، پیام `recovered` فرستاده می‌شود. مقادیر بحرانی‌تر مانند `SpO2 < 88` با severity برابر `critical` و بقیه با `warning` ارسال می‌شوند.

## قالب پیام

```json
{"version":1,"type":"vital_alert","event":"triggered","sequence":1,"uptime_ms":12500,"severity":"critical","scenario":"Low_SpO2","heart_rate":78,"spo2":87,"temperature_c":36.9,"reasons":["low_spo2"]}
```

فیلد `sequence` برای تشخیص پیام تکراری و `uptime_ms` برای تشخیص Reset شدن برد استفاده می‌شود. Event یکی از `triggered`، `reminder` یا `recovered` است.

## اجرای گیرنده روی macOS

1. در تنظیمات Ethernet لپ‌تاپ، IPv4 را روی حالت Manual بگذارید و `192.168.7.1` با netmask برابر `255.255.255.0` تنظیم کنید.
2. کابل Ethernet را وصل و Firmware را روی برد Flash کنید.
3. از ریشهٔ مخزن گیرنده را اجرا کنید:

```bash
python3 -m network_alert --log alerts.jsonl
```

اعلان دسکتاپ در macOS به‌صورت پیش‌فرض فعال است. برای خاموش کردن آن:

```bash
python3 -m network_alert --no-notify --log alerts.jsonl
```

برای ارسال هم‌زمان پیام معتبر به سرویس Push/Automation دارای webhook:

```bash
python3 -m network_alert \
  --log alerts.jsonl \
  --webhook-url https://example.invalid/your-webhook
```

خرابی webhook فقط در ترمینال گزارش می‌شود و گیرنده را متوقف نمی‌کند.

## تست تحویل

1. قبل از اجرای Simulator، دستور `ping 192.168.7.2` باید پاسخ بگیرد؛ پاسخ ICMP در ماژول شبکه پیاده‌سازی شده است.
2. گیرندهٔ Python را اجرا کنید.
3. سناریوی `Normal` را حداقل ۳۰ ثانیه بفرستید؛ نباید Alert ثبت شود.
4. سناریوی `Low_SpO2` یا `Tachycardia` را بفرستید؛ باید پیام `triggered` در ترمینال، اعلان macOS و یک خط در `alerts.jsonl` دیده شود.
5. دوباره `Normal` را بفرستید؛ باید پیام `recovered` ثبت شود.
6. کابل Ethernet را جدا کنید و UART/UI را ادامه دهید؛ نمایشگر نباید هنگ کند. بعد از اتصال دوباره، ARP به‌طور خودکار تکرار و ارسال هشدار از سر گرفته می‌شود.

تست‌های بدون برد:

```bash
python3 -m unittest discover -s tests -v

cc -std=c11 -Wall -Wextra -Werror \
  -I CubeIDEProject/Core/Inc \
  tests/network_alert_logic_test.c \
  CubeIDEProject/Core/Src/network_alert_logic.c \
  -o /tmp/network_alert_logic_test
/tmp/network_alert_logic_test
```

## فایل‌های این بخش

- `Core/Src/network_alert.c`: راه‌اندازی ETH، بررسی Link، ARP، پاسخ Ping، ساخت IPv4/UDP و ارسال
- `Core/Src/network_alert_logic.c`: Threshold، debounce، rate limit، recovery و تولید JSON
- `Core/Inc/network_alert_config.h`: تمام تنظیمات قابل تغییر شبکه و Thresholdها
- `Core/Inc/network_alert.h`: API و وضعیت قابل مشاهدهٔ ماژول
- `network_alert/receiver.py`: گیرنده، اعتبارسنجی، Log، اعلان و webhook
- `tests/network_alert_logic_test.c`: تست مستقل منطق Firmware روی رایانه
- `tests/test_network_alert_receiver.py`: تست گیرنده و قرارداد پیام

## نکات فنی و محدودیت تأیید

- کد شبکه از `freertos.c` راه‌اندازی و سرویس می‌شود؛ `main.c` عمداً تغییر نکرده است.
- DMA descriptorها و Bufferهای Ethernet در ۱۶ کیلوبایت SRAM2 رزرو شده‌اند تا وارد DTCM غیرقابل‌دسترسی برای DMA نشوند.
- این پیاده‌سازی عمداً کوچک است و فقط ARP، UDP خروجی و ICMP Echo مورد نیاز Demo را دارد؛ یک TCP/IP stack عمومی نیست.
- تست‌های خودکار نرم‌افزار موفق‌اند. نتیجهٔ Link، Ping و دریافت UDP باید یک‌بار با برد و کابل واقعی ثبت شود، چون محیط توسعه به سخت‌افزار فیزیکی دسترسی ندارد.
