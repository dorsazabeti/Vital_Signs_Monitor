# سند زنده پروژه Vital Signs Monitor

> این فایل مرجع مشترک تیم برای وضعیت فنی پروژه، تاریخچه کارها، ساختار فایل‌ها، روش اجرا و برنامه مراحل بعدی است. پس از هر تغییر مهم در کد، سخت‌افزار، نتیجه تست یا تقسیم کار باید همین فایل نیز به‌روزرسانی شود.

## 1. معرفی کوتاه پروژه

هدف پروژه ساخت یک پایشگر علائم حیاتی روی برد `STM32F746G-DISCOVERY` است. داده‌های بیمار فعلاً در کامپیوتر و با Python شبیه‌سازی می‌شوند، از طریق USB Virtual COM Port و `USART1` به برد می‌رسند و روی LCD داخلی برد با LVGL نمایش داده می‌شوند. در ادامه باید تشخیص وضعیت غیرعادی، نمودار ECG و ارسال هشدار شبکه نیز به سیستم اضافه شوند.

پارامترهای اصلی پروژه:

- ضربان قلب یا Heart Rate بر حسب BPM
- درصد اشباع اکسیژن خون یا SpO2
- دمای بدن بر حسب درجه سلسیوس
- سیگنال ECG
- نام سناریوی فعلی بیمار

اعضای تیم:

- درسا ضابطی: شبیه‌ساز Python، سناریوها، تولید ECG و قرارداد داده
- آراد ایزدی‌دوست: راه‌اندازی برد، LCD، حافظه، UART و یکپارچه‌سازی Embedded
-  فرخی: طراحی رابط گرافیکی و Assetها؛ همکاری در بخش شبکه و هشدار

## 2. وضعیت فعلی در یک نگاه

| بخش | وضعیت فعلی | توضیح |
| --- | --- | --- |
| پروگرام و اجرای برد | انجام شده | فایل `BIN` از آدرس `0x08000000` پروگرام می‌شود. |
| LCD و Backlight | انجام شده و تست‌شده | رزولوشن `480x272` و مسیر LTDC سالم است. |
| FMC و SDRAM | انجام شده و تست‌شده | SDRAM در آدرس `0xC0000000` راه‌اندازی و تست می‌شود. |
| LVGL | انجام شده | LVGL 9، رندر RGB565 و دو بافر Partial فعال هستند. |
| داشبورد | انجام شده | سه کارت HR، Temperature و SpO2 به همراه انیمیشن قلب نمایش داده می‌شوند. |
| Python simulator | انجام شده | پنج سناریو، ECG، Mock mode و خروجی JSON روی UART آماده است. |
| دریافت UART در Firmware | در کد پیاده‌سازی شده | USART1، وقفه، بافر خط، parser و انتقال داده به UI وجود دارند؛ تست End-to-End نهایی روی برد باید ثبت شود. |
| ECG روی LCD | در کد پیاده‌سازی شده | آخرین تغییرات نمودار ECG را اضافه کرده‌اند؛ تست نهایی روی برد باید ثبت شود. |
| وضعیت غیرعادی و هشدار UI | انجام نشده | Threshold، رنگ هشدار و stale-data indicator باید اضافه شوند. |
| Ethernet و Alert | پیاده‌سازی نرم‌افزاری کامل؛ نیازمند تست برد | ETH با IP ثابت، ARP، Ping، UDP، Threshold، rate limit، گیرنده Python، اعلان macOS و webhook آماده است. |
| حسگر واقعی | خارج از فاز فعلی | پس از کامل شدن مسیر شبیه‌ساز تا نمایشگر قابل بررسی است. |

نکته مهم: پیاده‌سازی فعلی از نظر کد، مسیر UART تا `Vitals_UpdateUI()` را کامل کرده است؛ اما تا زمانی که روی برد واقعی تغییر زنده اعداد مشاهده و ثبت نشود، وضعیت آن «پیاده‌سازی‌شده ولی نیازمند تأیید سخت‌افزاری» محسوب می‌شود.

## 3. معماری فعلی سیستم

```text
Python Patient Model
        |
        |  newline-delimited compact JSON
        v
USB VCP / USART1 @ 115200 8N1
        |
        |  RX interrupt, one byte at a time
        v
UART line buffers (128 bytes)
        |
        |  UART_ReadLine() in FreeRTOS task
        v
Lightweight JSON parser
        |
        v
Vitals_UpdateUI()
        |
        v
LVGL objects and timers
        |
        |  partial RGB565 rendering
        v
SDRAM framebuffer @ 0xC0000000
        |
        v
LTDC -> 480x272 LCD
```

اصل معماری مهم پروژه این است که ISR فقط دریافت بایت و مدیریت بافر را انجام دهد. Parse کردن JSON و فراخوانی LVGL باید فقط در Context مربوط به Task انجام شوند.

## 4. ساختار پوشه‌ها و فایل‌ها

### 4.1 فایل‌های ریشه‌ی Firmware

- `CMakeLists.txt`: تعریف Build، سورس‌ها، Includeها، Linker script، فلگ‌های MCU و تولید خودکار ELF/HEX/BIN.
- `arm-none-eabi.cmake`: معرفی GNU Arm Embedded Toolchain به CMake.
- `STM32F746NGHX_FLASH.ld`: Linker script نسخه‌ای که از Flash اجرا می‌شود.
- `STM32F746NGHX_RAM.ld`: Linker script جایگزین برای اجرای RAM.
- `VitalSignsMonitor.ioc`: تنظیمات CubeMX و منبع تولید کد Peripheralها.
- `lv_conf.h`: تنظیمات LVGL برای این Firmware.
- فایل‌های `.project`، `.cproject`، `.mxproject` و `.settings/`: تنظیمات STM32CubeIDE/Eclipse؛ منبع منطق برنامه نیستند.

### 4.2 پوشه `Core/Inc`

Headerهای اصلی Firmware در این پوشه قرار دارند:

- `main.h`: Pinها، GPIOها و Prototypeهای عمومی از جمله `Vitals_UpdateUI()`.
- `fmc.h`: Handle و API مربوط به SDRAM/FMC.
- `ltdc.h`: Handle و API مربوط به LTDC.
- `lv_port_disp.h`: رابط Display port برای LVGL.
- `usart.h`: Handleهای USART، اندازه بافر `128` بایت و `UART_ReadLine()`.
- `stm32f7xx_it.h`: Prototype وقفه‌ها، از جمله `USART1_IRQHandler()`.
- `images.h`: اعلان تمام Image descriptorهای استفاده‌شده در LVGL.
- `FreeRTOSConfig.h`: تنظیمات FreeRTOS.
- `network_alert.h`: API عمومی راه‌اندازی، پردازش، ورود Vitalها و مشاهده وضعیت شبکه.
- `network_alert_config.h`: IPها، پورت‌ها، Thresholdها و زمان‌بندی هشدار.
- `network_alert_logic.h`: قرارداد منطق مستقل هشدار و Payload.
- Headerهای سایر Peripheralها مانند `eth.h`، `dma2d.h`، `gpio.h`، `tim.h`، `i2c.h`، `spi.h` و غیره که عمدتاً توسط CubeMX تولید شده‌اند.
- `Backup/`: نسخه‌های پشتیبان قدیمی؛ منبع Build فعلی نیست.

### 4.3 پوشه `Core/Src`

فایل‌های کلیدی:

- `main.c`: ترتیب Boot، تنظیم MPU، تست SDRAM، نمایش مراحل تشخیصی، Initialize کردن LVGL، ساخت داشبورد و تابع `Vitals_UpdateUI()`.
- `fmc.c`: تنظیم FMC، Timing حافظه، Command sequence کامل SDRAM و Refresh rate.
- `ltdc.c`: Timing نمایشگر، GPIOهای RGB، Pixel clock، Layer اصلی و Interrupt مربوط به LTDC.
- `lv_port_disp.c`: اتصال LVGL به Framebuffer، دو بافر 40 خطی و تابع Flush برای کپی خروجی به SDRAM.
- `freertos.c`: ساخت `defaultTask`، اجرای `lv_timer_handler()`، خواندن خط UART، Parse کردن JSON و به‌روزرسانی UI.
- `network_alert.c`: درایور شبکهٔ سطح برنامه شامل Link، ARP، Ping و ارسال IPv4/UDP روی HAL ETH.
- `network_alert_logic.c`: تشخیص وضعیت غیرعادی، debounce، rate limit، recovery و ساخت JSON هشدار.
- `usart.c`: تنظیم USART1 و USART6، فعال‌سازی NVIC برای USART1، دریافت Interrupt-based و مدیریت دو بافر خط.
- `stm32f7xx_it.c`: Handler وقفه‌های سیستم و `USART1_IRQHandler()` که HAL UART handler را صدا می‌زند.
- `gpio.c`: GPIOهای عمومی از جمله LCD display enable و backlight.
- `stm32f7xx_hal_timebase_tim.c`: Timebase یک میلی‌ثانیه‌ای HAL با TIM6.
- فایل‌های سایر Peripheralها از جمله `eth.c`، `dma2d.c`، `adc.c`، `i2c.c` و `spi.c`: کد تولیدشده توسط CubeMX؛ در `LCD_BRINGUP_MODE=1` بیشتر آن‌ها Initialize نمی‌شوند.
- `Backup/`: نسخه‌های قدیمی و خارج از Build فعلی.

### 4.4 پوشه `Core/Images`

Assetها به صورت آرایه‌های C و `lv_image_dsc_t` ذخیره شده‌اند:

- `Heart1.c` تا `Heart8.c`: هشت فریم انیمیشن ضربان قلب.
- `Thermometer.c`: تصویر دماسنج.
- `Bubble.c`: حباب روی دماسنج که مقدار دما داخل آن نمایش داده می‌شود.
- `Gauge.c`: صفحه گیج SpO2.
- `GaugeArrow.c`: عقربه گیج SpO2.

این فایل‌ها باید با Image Converter سازگار با LVGL 9 تولید شوند. تغییر دستی Width/Height در descriptor بدون Resize واقعی داده‌ی تصویر صحیح نیست.

### 4.5 پوشه `Core/Startup`

- `startup_stm32f746nghx.s`: Vector table، Reset handler و اعلان Weak وقفه‌ها. ورودی `USART1_IRQHandler` در Vector table موجود است.

### 4.6 پوشه‌های کتابخانه‌ای و Middleware

- `Drivers/CMSIS`: Headerها و Startup support مربوط به Cortex-M7 و STM32F7.
- `Drivers/STM32F7xx_HAL_Driver`: درایورهای رسمی HAL شرکت ST.
- `Middlewares/Third_Party/FreeRTOS`: Kernel و CMSIS-RTOS wrapper.
- `Middlewares/Third_Party/FatFs`: فایل‌سیستم FatFs.
- `Middlewares/ST/STM32_USB_Host_Library`: USB Host middleware.
- `FATFS`: Glue code تولیدشده برای FatFs.
- `USB_HOST`: Application و Target glue code مربوط به USB Host.
- `lvgl`: سورس کامل کتابخانه LVGL؛ فایل‌های نمونه، تست و مستندات داخل آن منطق اختصاصی پروژه نیستند.

### 4.7 پوشه `simulation`

- `__main__.py`: امکان اجرا با `python -m simulation`.
- `main.py`: CLI، زمان‌بندی نمونه‌ها و هماهنگ‌سازی مدل بیمار با UART.
- `patient.py`: تولید پیوسته علائم حیاتی و موج ECG شامل P/Q/R/S/T، drift و noise.
- `scenarios.py`: تعریف بازه‌های پنج سناریوی بیمار.
- `uart.py`: ساخت JSON فشرده، افزودن `\n`، اتصال PySerial و Mock fallback.
- `requirements.txt`: وابستگی `pyserial>=3.5`.

فایل‌های تست شبیه‌ساز در ریشه مخزن و در `../tests/test_simulation.py` قرار دارند.

### 4.8 پوشه `docs`

- `IoT_Project1_Proposal_FA.pdf`: پروپوزال، معماری، Roadmap و تقسیم اولیه کار.
- `Vital_Signs_Monitor_Week1_Report_FA.pdf`: گزارش Bring-up، UI مستقیم با LTDC، Assetها و شبیه‌ساز هفته اول.
- `IOT_lab_project1_week2_report.pdf`: گزارش انتقال به LVGL، عیب‌یابی LCD/SDRAM، داشبورد و توسعه شبیه‌ساز در هفته دوم.
- `PROJECT_CONTEXT.md`: همین سند زنده و مرجع وضعیت فعلی.

### 4.9 فایل‌های بیرون از `CubeIDEProject`

- `../README.md`: راهنمای فعلی شبیه‌ساز و اجرای تست‌ها.
- `../UART_PROTOCOL.md`: قرارداد رسمی فریم UART و فیلدهای JSON.
- `../tests/`: تست‌های خودکار شبیه‌ساز.

### 4.10 فایل‌های تولیدی یا محلی

- `build/`: خروجی CMake شامل ELF، HEX، BIN و MAP؛ نباید منبع اصلی یا Commit شود.
- `output/`: خروجی گزارش‌های تولیدشده؛ در صورت نیاز فقط Deliverable نهایی Commit شود.
- `tmp/`: فایل‌های موقت.
- `.metadata/`: وضعیت محلی CubeIDE.
- `.venv/` و `__pycache__/`: محیط و Cache پایتون.
- `.DS_Store`: فایل محلی macOS و نباید Commit شود.

## 5. تنظیمات فنی مهم Firmware

### 5.1 پردازنده و Clock

- MCU: `STM32F746NGH6`، هسته Cortex-M7.
- System clock: `200 MHz`.
- HSE به عنوان منبع PLL استفاده می‌شود.
- LTDC pixel clock از PLLSAI تأمین می‌شود.

### 5.2 LCD و LTDC

- رزولوشن: `480x272`.
- فرمت Framebuffer: `RGB565`، یعنی 16 بیت برای هر Pixel.
- آدرس Framebuffer: `0xC0000000`.
- حجم Framebuffer اصلی: `480 * 272 * 2 = 261120 bytes`.
- Timing فعلی LTDC:
  - Horizontal Sync: `40`
  - Vertical Sync: `9`
  - Accumulated HBP: `53`
  - Accumulated VBP: `11`
  - Accumulated Active Width: `533`
  - Accumulated Active Height: `283`
  - Total Width: `565`
  - Total Height: `285`
- GPIOهای LTDC با سرعت `VERY_HIGH` و Alternate Function مناسب تنظیم شده‌اند.

### 5.3 SDRAM و FMC

- Bank: `FMC_SDRAM_BANK1`.
- Bus width: 16-bit.
- Row bits: 12.
- Column bits: 8.
- Internal banks: 4.
- CAS latency: 2.
- SDRAM clock period: 2.
- Refresh count: `0x0603`.
- Command sequence اجراشده:
  1. Clock enable
  2. Delay
  3. Precharge all
  4. Eight auto-refresh cycles
  5. Load mode register
  6. Program refresh rate

تنظیم MPU دو ناحیه مهم دارد:

- Region 1: حافظه 8MB از `0xC0000000`، قابل خواندن/نوشتن و Non-cacheable برای Framebuffer.
- Region 2: رجیسترهای FMC از `0xA0000000` با اندازه 64KB.

Non-cacheable بودن Framebuffer باعث می‌شود LTDC همیشه داده‌ی واقعی نوشته‌شده توسط CPU/LVGL را ببیند و نیاز به Cache clean دستی نباشد.

### 5.4 LVGL

- API مورد استفاده متعلق به LVGL 9 است (`lv_image_create` و `lv_display_create`).
- فرمت خروجی: `LV_COLOR_FORMAT_RGB565`.
- Render mode: `LV_DISPLAY_RENDER_MODE_PARTIAL`.
- دو Draw buffer مستقل، هر کدام به اندازه 40 خط (`480 * 40` پیکسل).
- `flush_cb` خروجی LVGL را به Framebuffer واقع در SDRAM کپی می‌کند.
- Tick یک میلی‌ثانیه‌ای LVGL از Callback مربوط به TIM6 تأمین می‌شود.
- `lv_timer_handler()` در `defaultTask` اجرا می‌شود و Delay بین 1 تا 20 میلی‌ثانیه محدود می‌شود.
- Stack فعلی `defaultTask` برابر 4096 است.

### 5.5 UART و JSON

- Peripheral: `USART1`.
- VCP RX: `PB7`.
- VCP TX: `PA9`.
- تنظیم: `115200 baud`, `8 data bits`, `no parity`, `1 stop bit`, بدون Flow control.
- اولویت NVIC: `5`.
- روش دریافت فعلی: یک بایت در هر وقفه با `HAL_UART_Receive_IT()`.
- بافر خط: 128 بایت.
- `\r` نادیده گرفته می‌شود و `\n` پایان فریم است.
- بافر Build و Ready جدا هستند تا ISR و Task روی یک رشته واحد هم‌زمان کار نکنند.
- فریم Overlong دور ریخته می‌شود.
- Parser فعلی سبک و بدون کتابخانه JSON است.
- فیلدهای `scenario`, `hr`, `spo2`, `temp` Parse می‌شوند.
- فیلد `ecg` فعلاً عمداً نادیده گرفته می‌شود.
- بازه اعتبار Firmware:
  - HR: از 30 تا 220
  - SpO2: از 50 تا 100
  - Temperature: از 32.0 تا 43.0 درجه

نمونه فریم:

```json
{"scenario":"Normal","hr":74,"spo2":98,"temp":36.8,"ecg":0.021}
```

## 6. روند Boot و حالت‌های تشخیصی فعلی

Firmware فعلی برای عیب‌یابی LCD و حافظه چند مرحله دیداری دارد:

1. راه‌اندازی GPIO و LTDC.
2. نمایش مستقیم تصویر قلب از Flash روی پس‌زمینه سرمه‌ای برای حدود 3 ثانیه؛ این مرحله مسیر Flash -> LTDC -> LCD را مستقل از SDRAM و LVGL تأیید می‌کند.
3. راه‌اندازی FMC و SDRAM.
4. اجرای تست Write/Read روی چند Offset و Pattern حافظه.
5. در صورت شکست تست، نمایش رنگ بنفش و توقف دائمی.
6. در صورت موفقیت، پر کردن Framebuffer با رنگ قرمز و نمایش آن برای حدود 2 ثانیه.
7. راه‌اندازی LVGL، ساخت داشبورد و شروع FreeRTOS.

این مراحل برای Bring-up بسیار مفید بودند، ولی در نسخه نمایشی نهایی باید پشت یک Compile-time debug flag قرار بگیرند تا Boot پنج ثانیه‌ای و صفحات تشخیصی حذف شوند.

## 7. آنچه در هفته اول انجام شد

- اتصال ST-LINK و تأیید پروگرام و اجرای MCU.
- آماده‌سازی CMake و GNU Arm Embedded Toolchain.
- استفاده از مثال رسمی ST برای راه‌اندازی اولیه LCD و LTDC.
- تأیید مسیر Flash -> LTDC -> LCD.
- ساخت Prototype اولیه UI با Layerهای مستقیم LTDC.
- نمایش Splash screen، مقدار ثابت `HR: 76` و انیمیشن هشت‌فریمی قلب.
- استفاده از Alpha format برای حذف کادر رنگی اطراف قلب.
- آماده‌سازی Assetهای Heart، Gauge، GaugeArrow، Thermometer و Bubble.
- ساخت نسخه اولیه Python simulator و سناریوهای بیمار.
- تعریف خروجی JSON و Baud rate هدف.

## 8. آنچه در هفته دوم انجام شد

- بررسی TouchGFX و انتخاب LVGL به عنوان Framework نهایی رابط کاربری.
- افزودن LVGL به Build و هماهنگ کردن APIها با LVGL 9.
- ایجاد Display port اختصاصی در `lv_port_disp.c`.
- پیاده‌سازی Partial rendering با دو بافر 40 خطی.
- تبدیل، Resize و بهینه‌سازی Assetها برای کاهش مصرف Flash.
- انتقال انیمیشن قلب از تغییر مستقیم LTDC Layer به LVGL image object و timer.
- پیاده‌سازی داشبورد سه‌کارت برای HR، Temperature و SpO2.
- ساخت API عمومی `Vitals_UpdateUI()`.
- هماهنگی LVGL tick با TIM6 و اجرای LVGL در FreeRTOS task.
- عیب‌یابی مرحله‌ای صفحه سفید/سیاه با جداسازی مسیرهای Flash، SDRAM و LVGL.
- پیدا کردن علت اصلی Fault در دسترسی FMC: تنظیم عمومی MPU دسترسی به رجیسترهای FMC را مسدود کرده بود.
- افزودن MPU regionهای SDRAM و FMC.
- افزودن SDRAM initialization command sequence، refresh rate و تست حافظه.
- تنظیم سرعت GPIOهای LTDC/FMC روی `VERY_HIGH`.
- تکمیل شبیه‌ساز ماژولار Python، پنج سناریو، ECG، CLI، Mock mode و UART framing.
- تعریف قرارداد `UART_PROTOCOL.md`.
- تولید موفق ELF/HEX/BIN با CMake.

## 9. تغییرات پس از گزارش هفته دوم

موارد زیر اکنون در Working Tree وجود دارند و از وضعیت قدیمی گزارش هفته دوم جلوتر هستند:

- `USART1_IRQHandler()` در Header، Startup و فایل وقفه به درستی متصل است.
- NVIC مربوط به USART1 با Priority 5 فعال شده است.
- دریافت بایت با Interrupt از داخل `MX_USART1_UART_Init()` شروع می‌شود.
- Debug قبلی `HAL_UART_Transmit("RX")` حذف شده است.
- ISR فقط دریافت، Frame detection و انتقال رشته به Ready buffer را انجام می‌دهد.
- `UART_ReadLine()` دسترسی Task به خط کامل را با Critical section کوتاه انجام می‌دهد.
- JSON parser سبک برای HR، SpO2، Temperature و Scenario در `freertos.c` اضافه شده است.
- `StartDefaultTask()` داده معتبر را به `Vitals_UpdateUI()` می‌دهد.
- متن وضعیت UI پس از دریافت، به صورت `<scenario> | LIVE UART` نمایش داده می‌شود.

## 10. مشکلات قبلی LCD و علت آن‌ها

نشانه‌های مشاهده‌شده:

- هنگام نگه‌داشتن Reset صفحه سفید بود؛ چون MCU و LTDC فعال نبودند و Backlight فقط پنل خام را نشان می‌داد.
- پس از رها کردن Reset صفحه سیاه می‌شد؛ یعنی LTDC و Backlight فعال بودند ولی داده قابل مشاهده در Layer/Framebuffer وجود نداشت.
- نمایش موفق تصویر سرمه‌ای و قلب از Flash ثابت کرد LCD، Clock، Timing، GPIO و مسیر LTDC سالم هستند.
- توقف CPU هنگام ورود به FMC و بررسی Fault registerها نشان داد مشکل اصلی در SDRAM data یا LVGL نبود؛ MPU دسترسی رجیسترهای FMC را مسدود کرده بود.

راه‌حل اعمال‌شده:

- تعریف Region قابل دسترس برای رجیسترهای FMC.
- تعریف Region کامل و Non-cacheable برای SDRAM.
- افزودن Sequence رسمی راه‌اندازی SDRAM و Refresh rate.
- افزودن تست Write/Read حافظه قبل از استفاده LVGL.
- استفاده از Framebuffer واقعی SDRAM با RGB565.

نتیجه: صفحه سرمه‌ای با قلب، صفحه تست SDRAM و سپس داشبورد LVGL با موفقیت نمایش داده شدند.

## 11. شبیه‌ساز و سناریوها

سناریوهای فعلی:

| سناریو | HR | SpO2 | Temperature |
| --- | --- | --- | --- |
| `Normal` | 68-82 | 96-99 | 36.5-37.2 |
| `Tachycardia` | 110-140 | 95-99 | 36.5-37.3 |
| `Bradycardia` | 42-55 | 95-99 | 36.4-37.1 |
| `Low_SpO2` | 85-110 | 84-90 | 36.5-37.2 |
| `Abnormal_Temp` | 90-115 | 95-99 | 38.3-39.5 |

ویژگی‌ها:

- حرکت نرم مقدار فعلی به Target جدید به جای پرش ناگهانی.
- Noise کم و محدودیت‌های فیزیولوژیک کلی.
- تولید ECG وابسته به Heart Rate.
- Sample rate پیش‌فرض 50Hz و قابل تنظیم.
- خروجی قابل تکرار با `--seed`.
- Mock mode در صورت نبود پورت یا PySerial.
- فهرست پورت‌ها با `--list-ports`.

## 12. روش Build و تولید خروجی

از داخل `CubeIDEProject`:

```bash
cd build
rm -rf -- ./*
cmake .. -DCMAKE_TOOLCHAIN_FILE=../arm-none-eabi.cmake
make -j4
cd ..
```

`CMakeLists.txt` فعلی پس از Build موفق، `VitalSignsMonitor.hex` و `VitalSignsMonitor.bin` را به صورت خودکار می‌سازد. اگر تولید دستی لازم بود:

```bash
"/Users/arad/Library/Application Support/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-objcopy" \
  -O ihex \
  build/VitalSignsMonitor.elf \
  build/VitalSignsMonitor.hex

"/Users/arad/Library/Application Support/stm32cube/bundles/gnu-tools-for-stm32/14.3.1+st.2/bin/arm-none-eabi-objcopy" \
  -O binary \
  build/VitalSignsMonitor.elf \
  build/VitalSignsMonitor.bin
```

فایل مناسب برای پروگرام فعلی:

```text
build/VitalSignsMonitor.bin
```

آدرس شروع پروگرام BIN:

```text
0x08000000
```

Build موجود در زمان تهیه این سند تقریباً این مشخصات را دارد:

- BIN: حدود 980 KiB
- ELF: حدود 1.1 MiB
- `text + data`: حدود 1,003,604 bytes
- BSS: حدود 181,132 bytes

## 13. روش اجرای Python simulator

از ریشه مخزن، یک بار محیط را آماده کنید:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

نمایش پورت‌ها:

```bash
python -m simulation --list-ports
```

اجرای Mock:

```bash
python -m simulation --scenario Normal --duration 5
```

اجرای واقعی روی macOS با پورت مشاهده‌شده در تست‌های قبلی:

```bash
python -m simulation \
  --port /dev/cu.usbmodem2103 \
  --scenario Normal \
  --sample-rate 10
```

برای تست سناریوهای دیگر، مقدار `--scenario` را با یکی از پنج نام معتبر جایگزین کنید.

اجرای تست‌های Python:

```bash
python3 -m unittest discover -s tests -v
```

## 14. برنامه پیشنهادی هفته سوم برای کار هم‌زمان سه نفر

هدف هفته سوم باید یک Demo کامل و قابل اندازه‌گیری باشد: داده Python روی USB ارسال شود، UI به شکل زنده و زیبا تغییر کند، وضعیت غیرعادی واضح باشد و یک Proof of Concept شبکه نیز وجود داشته باشد.

### مسیر A - آراد: Embedded integration و پایداری دریافت

فایل‌های اصلی تحت مالکیت این مسیر:

- `Core/Src/usart.c`
- `Core/Inc/usart.h`
- `Core/Src/freertos.c`
- `Core/Src/stm32f7xx_it.c`
- بخش Boot و Integration در `Core/Src/main.c`

کارها:

1. تست واقعی End-to-End با هر پنج سناریو و ثبت نتیجه.
2. افزودن شمارنده فریم سالم، فریم خراب و Overflow برای Debug.
3. افزودن تشخیص قطع داده پس از دو ثانیه و نمایش `NO DATA` یا `STALE`.
4. هماهنگ کردن سرعت انیمیشن قلب با HR واقعی.
5. تبدیل Ready-line تکی به Queue/Ring buffer کوچک در صورت مشاهده Drop در 50Hz.
6. قرار دادن صفحات تشخیصی 3 و 2 ثانیه‌ای پشت Debug flag.
7. تثبیت یک BIN پایه که هر سه عضو بتوانند روی همان Commit تست کنند.

معیار تحویل:

- هر پنج سناریو حداقل دو دقیقه بدون هنگ اجرا شوند.
- HR، SpO2، Temperature و Scenario روی LCD تغییر کنند.
- قطع کابل یا توقف Python حداکثر بعد از دو ثانیه در UI دیده شود.

### مسیر B - یاسمن فرخی: UI/UX و هشدار دیداری

برای جلوگیری از Conflict، ابتدا کد داشبورد از `main.c` به دو فایل مستقل منتقل شود:

- `Core/Inc/ui_dashboard.h`
- `Core/Src/ui_dashboard.c`

سپس کارهای UI فقط در این فایل‌ها و `Core/Images` انجام شوند:

1. یکدست کردن فاصله‌ها، فونت‌ها، رنگ‌ها و Alignment سه کارت.
2. طراحی حالت‌های `NORMAL`, `WARNING`, `CRITICAL` و `NO DATA`.
3. تغییر رنگ Card یا Border بر اساس وضعیت هر Vital.
4. متحرک کردن عقربه SpO2 بر اساس مقدار واقعی.
5. بهتر کردن نمایش دما و واحدها.
6. آماده کردن یک فضای مشخص برای نمودار ECG هفته بعد.
7. تهیه Screenshot/عکس واقعی برای گزارش هفته سوم.

معیار تحویل:

- UI در همه سناریوها خوانا باشد.
- وضعیت خطر بدون خواندن عدد نیز از رنگ و Label مشخص شود.
- تغییرات UI با فایل‌های UART آراد Conflict نداشته باشد.

### مسیر C - درسا: شبکه، Simulator و تست خودکار

فایل‌های اصلی تحت مالکیت این مسیر:

- `simulation/`
- `../tests/`
- بخش‌های مرتبط با شبکه در یک ماژول جدید و مستقل

کارها:

1. اجرای تست ماتریسی پنج سناریو روی پورت واقعی و ذخیره Log خلاصه.
2. افزودن reconnect یا پیام خطای واضح برای قطع پورت.
3. ~~طراحی Proof of Concept هشدار شبکه در سمت PC.~~ انجام شد و مسیر نهایی از Ethernet خود STM32 نیز پیاده‌سازی شد.
4. ~~تعریف Payload هشدار شامل scenario، HR، SpO2، Temp، timestamp و severity.~~ انجام شد.
5. ~~افزودن تست برای Thresholdها، Payload شبکه و عدم ارسال تکراری هشدار در هر Sample.~~ انجام شد.
6. ~~مستندسازی مسیر شبکه.~~ در `docs/NETWORK_ALERT.md` انجام شد.

معیار تحویل:

- حداقل یک هشدار واقعی برای `Tachycardia` یا `Low_SpO2` ارسال و ثبت شود.
- Normal هشدار اشتباه تولید نکند.
- با قطع شبکه یا پورت، شبیه‌ساز Crash نکند.

### ترتیب Merge پیشنهادی هفته سوم

1. آراد ابتدا Refactor فایل‌های UI و Commit پایه را انجام دهد.
2. هر نفر Branch جدا بسازد و فقط فایل‌های مسیر خودش را تغییر دهد.
3. تغییر قرارداد JSON بدون هماهنگی انجام نشود.
4. ابتدا Simulator/Network و UI جداگانه Merge شوند.
5. Embedded integration آخر Merge شود و سپس Build و تست روی برد انجام شود.
6. نتیجه نهایی، عکس LCD، Log Python و Commit hash در همین فایل ثبت شود.

## 15. کارهای باقی‌مانده پس از هفته سوم

اولویت بالا:

- رسم نمودار زنده ECG با `lv_chart` یا Canvas.
- طراحی Buffer مناسب ECG جدا از Update نرخ پایین‌تر اعداد.
- تشخیص Threshold روی Firmware یا تعریف روشن محل تصمیم‌گیری.
- فعال‌سازی تدریجی Peripheralهای لازم با خارج شدن از `LCD_BRINGUP_MODE=1`.
- تست فیزیکی Link/Ping/UDP پیاده‌سازی Ethernet روی برد و ثبت Log نتیجه.

اولویت متوسط:

- استفاده از DMA2D برای Flush سریع‌تر در صورت نیاز.
- Queue یا DMA circular reception برای UART نرخ بالاتر.
- Timestamp/sequence number برای تشخیص Drop و داده قدیمی.
- Watchdog و Error reporting قابل مشاهده.
- تست طولانی‌مدت و اندازه‌گیری مصرف Stack/Heap.
- بررسی ورودی Touch برای تغییر سناریو یا صفحه‌ها.

مرحله Bonus:

- جایگزینی شبیه‌ساز با سنسور واقعی.
- ذخیره داده روی SD/FatFs.
- صفحه History و Trend.
- در صورت ترجیح استاد، جایگزینی اعلان macOS/webhook فعلی با سرویس Email مشخص.

## 16. بدهی‌های فنی و ریسک‌های فعلی

- `LCD_BRINGUP_MODE=1` در `CMakeLists.txt` فعال است؛ ماژول شبکه برای استقلال از `main.c`، ETH را در Task خود راه‌اندازی می‌کند.
- `main.c` در حال حاضر `usart.h` را دو بار Include کرده است؛ بی‌ضرر ولی باید تمیز شود.
- sequence مربوط به invalidate/reload و Backlight در انتهای راه‌اندازی LVGL دو بار تکرار شده است.
- Boot تشخیصی فعلی حدود پنج ثانیه Delay اجباری دارد.
- Ready buffer UART تنها یک خط کامل را نگه می‌دارد؛ اگر Task دیر برسد، Frame جدید ممکن است Drop شود.
- Parser فعلی JSON عمومی نیست و دقیقاً برای قالب فشرده‌ی قرارداد نوشته شده است.
- خطای برگشتی `HAL_UART_Receive_IT()` در Callback فعلی فقط cast به void شده و شمارش/Recovery ندارد.
- هنوز Timeout داده، Frame counter و نشانه خطای ارتباط در UI وجود ندارد.
- UI فعلی با هر فریم معتبر Labelها را Update می‌کند؛ برای 50Hz بهتر است نرخ UI عددی از نرخ ECG جدا شود.
- Network stack موردنیاز Demo پیاده‌سازی و تست نرم‌افزاری شده، اما Link/Ping/UDP هنوز باید روی برد واقعی تأیید شود.
- Build با `GLOB_RECURSE` بخش بزرگی از سورس‌های Library را وارد می‌کند؛ فعلاً کار می‌کند ولی زمان Build و حجم پروژه بالاست.
- پوشه‌های `Backup` و سورس کامل LVGL حجم Zip و مخزن را زیاد می‌کنند؛ حذف یا تغییر آن‌ها فقط پس از هماهنگی تیم انجام شود.

## 17. چک‌لیست تست تحویل

### Build

- [ ] Configure تمیز CMake موفق است.
- [ ] Build بدون Error تمام می‌شود.
- [ ] ELF، HEX و BIN تولید می‌شوند.
- [ ] اندازه Firmware از ظرفیت Flash بیشتر نیست.

### Board bring-up

- [ ] تصویر Flash reference دیده می‌شود.
- [ ] صفحه تست SDRAM دیده می‌شود و صفحه بنفش خطا ظاهر نمی‌شود.
- [ ] داشبورد LVGL بدون Artifact نمایش داده می‌شود.
- [ ] انیمیشن قلب ادامه پیدا می‌کند و سیستم هنگ نمی‌کند.

### UART

- [ ] پورت `/dev/cu.usbmodem...` شناسایی می‌شود.
- [ ] سناریوی Normal روی LCD مقادیر زنده نشان می‌دهد.
- [ ] چهار سناریوی غیرعادی نیز صحیح نمایش داده می‌شوند.
- [ ] Frame خراب، خط بلند و قطع ارتباط باعث Crash نمی‌شوند.
- [ ] قطع داده در UI مشخص می‌شود.

### Network

- [x] تست خودکار: Normal هشدار تولید نمی‌کند.
- [x] تست خودکار: وضعیت غیرعادی Payload معتبر تولید می‌کند.
- [x] تست خودکار: هشدارها Debounced و Rate-limited هستند.
- [x] گیرنده خطای پیام و webhook را بدون توقف مدیریت می‌کند.
- [ ] تست برد: Link و `ping 192.168.7.2` موفق است.
- [ ] تست برد: UDP هشدار در گیرنده و `alerts.jsonl` ثبت می‌شود.

### Documentation

- [ ] عکس LCD نهایی گرفته شده است.
- [ ] فرمان اجرا، نام پورت و سناریوی Demo ثبت شده‌اند.
- [ ] Commit hash نسخه Demo ثبت شده است.
- [ ] این فایل با نتیجه واقعی تست به‌روز شده است.

## 18. قوانین همکاری و به‌روزرسانی این سند

- قبل از تغییر، فایل واقعی خوانده شود تا کد تکراری ساخته نشود.
- کد اختصاصی فقط در بخش‌های `USER CODE` فایل‌های CubeMX قرار گیرد یا به فایل مستقل منتقل شود.
- هر نفر مالک فایل‌های مسیر خودش باشد تا Conflict کاهش یابد.
- تغییر پروتکل UART باید هم‌زمان در Firmware، Simulator و `UART_PROTOCOL.md` ثبت شود.
- هر ادعای «انجام شد» باید با حداقل یکی از Build موفق، تست خودکار، Log یا تست روی برد پشتیبانی شود.
- فایل‌های `build/`, `.venv/`, `.metadata/`, `__pycache__/` و `.DS_Store` Commit نشوند.
- پس از هر پیام یا تغییر مهم، حداقل بخش‌های «وضعیت فعلی»، «کارهای انجام‌شده»، «کارهای باقی‌مانده» و «چک‌لیست تست» بررسی و در صورت نیاز به‌روزرسانی شوند.

## 19. وضعیت Git هنگام ایجاد این سند

Commit پایه هنگام شروع کار شبکه:

```text
f69568c Ecg start
```

وضعیت مخزن پیش از شروع این مرحله تمیز و با `origin/main` همگام بود. در کار شبکه، `main.c` عمداً تغییر نکرده است تا با کار هم‌زمان عضو دیگر تیم Conflict ایجاد نشود. فایل‌های تغییرکرده و جدید این مرحله با `git status` قابل مشاهده‌اند و هنوز Commit نشده‌اند.

---

آخرین جمع‌بندی: زیرساخت نمایشگر، UART، ECG و داشبورد در کد موجود است. مسیر شبکه و هشدار شامل IP ثابت، ARP، Ping، UDP، منطق Threshold، گیرنده Python، اعلان و webhook نیز بدون تغییر `main.c` پیاده‌سازی و با تست خودکار بررسی شده است. Milestone بعدی، Flash و تأیید Link/Ping/UDP روی برد واقعی، تکمیل حالت‌های هشدار UI و ثبت عکس و Log دموی نهایی است.
