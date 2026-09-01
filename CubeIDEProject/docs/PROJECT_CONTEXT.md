# سند زنده پروژه Vital Signs Monitor

آخرین به‌روزرسانی: ۱۰ شهریور ۱۴۰۵ / ۱ سپتامبر ۲۰۲۶

این سند وضعیت واقعی پروژه اول آزمایشگاه اینترنت اشیا را ثبت می‌کند. منظور از «تکمیل نرم‌افزاری» این است که مسیر شبیه‌ساز، Firmware، UI و هشدار شبکه پیاده‌سازی و با تست‌های قابل اجرا بدون برد بررسی شده است. مواردی که ذاتاً به برد، کابل و شبکه فیزیکی نیاز دارند جداگانه با عنوان «نیازمند تأیید سخت‌افزاری» مشخص شده‌اند.

## ۱. هدف پروژه

کامپیوتر علائم حیاتی بیمار شامل ضربان قلب، SpO2، دمای بدن، ECG و نام سناریو را شبیه‌سازی می‌کند. هر نمونه به‌صورت JSON خطی روی UART به برد `STM32F746G-DISCO` می‌رسد. برد داده را اعتبارسنجی و روی LCD با LVGL نمایش می‌دهد. در صورت غیرعادی بودن علائم، برد از Ethernet یک هشدار UDP/JSON می‌فرستد و برنامه گیرنده روی کامپیوتر آن را ثبت و به‌صورت اعلان دسکتاپ یا webhook تحویل می‌دهد.

اعضای تیم مطابق پروپوزال:

- درسا ضابطی: شبیه‌ساز Python، سناریوها، ECG، قرارداد UART و همکاری در شبکه و تست
- آراد ایزدی‌دوست: راه‌اندازی برد، LCD/SDRAM، UART و یکپارچه‌سازی Firmware
- فرخی: طراحی UI و Assetها و همکاری در بخش هشدار
- تحویل نهایی، تست End-to-End و گزارش: مشترک

## ۲. وضعیت فعلی

| بخش | وضعیت | مدرک یا توضیح |
| --- | --- | --- |
| شبیه‌ساز Python | تکمیل نرم‌افزاری | پنج سناریو، تغییر نرم، ECG معنی‌دار، Mock، CLI و Seed |
| ارتباط UART سمت PC | تکمیل نرم‌افزاری | JSON فشرده، `115200 8N1`، reconnect خودکار پس از قطع پورت |
| دریافت UART روی STM32 | تکمیل نرم‌افزاری | وقفه تک‌بایتی، صف ۸ فریم، کنترل overflow/error و re-arm خودکار |
| Parser Firmware | تکمیل و تست‌شده | HR، SpO2، Temp، ECG و Scenario؛ ECG منفی و Fixed-point صحیح |
| LCD، SDRAM و LVGL | راه‌اندازی پایه قبلاً تأیید شده | نسخه یکپارچه جدید باید دوباره روی برد Flash و مشاهده شود |
| داشبورد نهایی | تکمیل نرم‌افزاری | سه کارت، قلب متحرک، دماسنج، گیج، ECG زنده و وضعیت ارتباط |
| هشدار دیداری | تکمیل نرم‌افزاری | Normal/Warning/Critical و `NO DATA` بعد از دو ثانیه |
| تشخیص وضعیت غیرعادی | تکمیل و تست‌شده | Threshold مشترک، سه نمونه تأیید، reminder و recovery |
| Ethernet | تکمیل نرم‌افزاری | Static IP، ARP، ICMP Echo و UDP بدون LwIP |
| گیرنده هشدار | تکمیل و تست‌شده | اعتبارسنجی، حذف تکراری، JSONL، اعلان macOS و webhook |
| Build کامل ARM | موفق | نسخه Pull‌شده در حالت `MinSizeRel` روی مک آراد Build و Link شد و ELF/HEX/BIN تولید شدند. مصرف Flash حدود ۷۴٫۵٪ است. |
| تست End-to-End برد | نیازمند تأیید سخت‌افزاری | Flash، LCD، UART، Link، Ping و UDP باید روی برد ثبت شوند |

نتیجه فعلی: بخش نرم‌افزاری موردنیاز پروژه تمام شده است. حسگر واقعی، ذخیره روی SD و صفحات History جزو Bonus هستند و مانع تحویل پروژه اصلی نیستند.

## ۳. معماری نهایی

```text
Python PatientModel (50 Hz)
       |
       | newline-delimited compact JSON / USB VCP / USART1
       v
UART ISR -> 128-byte builder -> 8-frame queue
       |
       v
VitalsParser_Parse() in FreeRTOS task
       |                         |
       |                         +--> NetworkAlert_UpdateVitals()
       v                                      |
UI_Dashboard / live ECG                       v
       |                           debounce + rate limit
       v                                      |
LVGL -> RGB565 SDRAM -> LTDC -> LCD            v
                                    ARP + IPv4 + UDP / Ethernet
                                               |
                                               v
                                    Python alert receiver
                                    log + desktop push + webhook
```

ISR فقط بایت و صف را مدیریت می‌کند. Parse، LVGL و منطق شبکه همگی در Task اجرا می‌شوند؛ بنابراین فراخوانی سنگین از Context وقفه انجام نمی‌شود.

## ۴. فایل‌های اصلی

### شبیه‌ساز و قرارداد

- `simulation/patient.py`: مدل پیوسته علائم و موج P/Q/R/S/T
- `simulation/scenarios.py`: بازه‌های پنج سناریو
- `simulation/uart.py`: فریم JSON، PySerial، Mock و reconnect
- `simulation/main.py`: CLI و زمان‌بندی ارسال
- `UART_PROTOCOL.md`: قرارداد رسمی UART و اعتبارسنجی

نسخه همسان پوشه `CubeIDEProject/simulation` برای بسته کامل CubeIDE نگه داشته شده و در وضعیت فعلی با نسخه ریشه یکسان است.

### Firmware

- `Core/Src/main.c`: Boot، MPU، SDRAM، LTDC، LVGL و ساخت داشبورد
- `Core/Src/usart.c` و `Core/Inc/usart.h`: ISR، صف RX، آمار خطا و recovery
- `Core/Src/vitals_parser.c`: Parser مستقل و قابل تست
- `Core/Src/freertos.c`: اتصال UART به Parser، UI، ECG و شبکه؛ stale timeout
- `Core/Src/ui_dashboard.c`: تمام UI مستقل از `main.c`
- `Core/Inc/vitals_thresholds.h`: Threshold مشترک UI و شبکه
- `Core/Src/network_alert_logic.c`: debounce، severity، reminder، recovery و JSON
- `Core/Src/network_alert.c`: ETH، ARP، Ping و UDP
- `Core/Inc/network_alert_config.h`: IP، Port و زمان‌بندی شبکه
- `STM32F746NGHX_FLASH.ld`: رزرو SRAM2 قابل دسترسی DMA برای Ethernet
- `CMakeLists.txt` و `arm-none-eabi.cmake`: Build قابل‌حمل Firmware

### تست و گیرنده شبکه

- `tests/test_simulation.py`: سناریو، ECG، framing، loopback و reconnect
- `tests/test_network_alert_receiver.py`: اعتبارسنجی و dedup پیام شبکه
- `tests/vitals_parser_test.c`: Parser و ECG مثبت/منفی/خراب
- `tests/network_alert_logic_test.c`: Threshold، debounce، reminder و recovery
- `scripts/run_tests.sh`: اجرای یکجای تمام تست‌های Python و C؛ در صورت وجود `.venv` به‌طور خودکار Python همان محیط را انتخاب می‌کند و در غیر این صورت از متغیر `PYTHON` یا `python3` استفاده می‌کند.
- `network_alert/receiver.py`: UDP receiver، log، اعلان و webhook

## ۵. رفتار UART و Parser

نمونه فریم:

```json
{"scenario":"Normal","hr":74,"spo2":98,"temp":36.8,"ecg":-0.025}
```

- Baud: `115200`، هشت بیت، بدون parity، یک stop bit، بدون flow control
- پایان هر فریم: `\n`
- اندازه هر بافر: ۱۲۸ بایت؛ عمق صف: ۸ فریم
- فریم بلند یا ناقص کنار گذاشته می‌شود و دریافت فریم بعدی ادامه پیدا می‌کند.
- ECG با دقت یک‌هزارم به عدد صحیح تبدیل می‌شود؛ مثال بالا روی برد مقدار `-25` می‌شود.
- دما با دقت یک‌دهم نگهداری می‌شود؛ `36.8` به `368` تبدیل می‌شود.
- بازه اعتبار: HR از ۳۰ تا ۲۲۰، SpO2 از ۵۰ تا ۱۰۰، دما از ۳۲٫۰ تا ۴۳٫۰ و ECG از منفی ۲ تا مثبت ۳.
- اگر دو ثانیه فریم معتبر نرسد، UI پیام `NO DATA | CHECK UART` نشان می‌دهد.
- اگر پورت سریال قطع شود، شبیه‌ساز Crash نمی‌کند و هر دو ثانیه برای اتصال مجدد تلاش می‌کند.

عیب‌یابی ثبت‌شده در ۱ سپتامبر ۲۰۲۶: Python سراسری مک آراد پکیج اشتباه `serial==0.0.97` را از `site-packages` بارگذاری می‌کرد. این پکیج PySerial نیست و APIهای `Serial` و `serial_for_url` را ندارد؛ در نتیجه شبیه‌ساز با وجود `/dev/cu.usbmodem2103` وارد Mock mode می‌شد. راه‌حل توصیه‌شده استفاده از `.venv` پروژه و نصب `requirements.txt` شامل `pyserial>=3.5` است. برای تست UART/LCD فقط کابل Mini-USB متصل به درگاه ST-LINK/VCP لازم است و کابل LAN فقط برای تست Ethernet/UDP کاربرد دارد.

## ۶. UI و Thresholdها

Threshold مشترک UI و Ethernet:

| Vital | Warning | Critical |
| --- | --- | --- |
| HR | کمتر از ۶۰ یا بیشتر از ۱۰۰ | کمتر از ۴۵ یا بیشتر از ۱۳۰ |
| SpO2 | کمتر از ۹۲٪ | کمتر از ۸۸٪ |
| Temperature | کمتر از ۳۵٫۰ یا حداقل ۳۸٫۰ | کمتر از ۳۴٫۰ یا حداقل ۳۹٫۵ |

Border هر کارت به‌صورت مستقل Normal، Warning یا Critical می‌شود. متن پایین صفحه سناریو و مهم‌ترین هشدار را نشان می‌دهد. سرعت انیمیشن قلب با HR تنظیم می‌شود. عقربه SpO2 با مقدار زنده حرکت می‌کند و دما داخل Bubble تصویری کنار دماسنج نمایش داده می‌شود. نمودار ECG صد مقدار رسم‌شده آخر را بدون Point marker نشان می‌دهد؛ Firmware از هر سه نمونه UART یک نمونه را وارد نمودار می‌کند تا سرعت پیشروی یک‌سوم شود، در حالی که Parse و شبکه همچنان همه نمونه‌ها را دریافت می‌کنند. نمودار در حالت `CIRCULAR` از چپ به راست نوشته می‌شود، پس از رسیدن به انتها از ابتدای صفحه ادامه می‌دهد و سه خانه خالی جلوی مکان نوشتن یک Sweep gap متحرک ایجاد می‌کنند. متن اعداد با نرخ ۱۰ هرتز به‌روزرسانی می‌شود تا بار رندر بی‌دلیل ایجاد نشود.

PNG اصلی Bubble دارای Canvas بزرگ `734x793` بود و تبدیل مستقیم آن بیش از ۲٫۳ مگابایت Flash نیاز داشت. نسخه فعلی `Bubble.c` از تاریخچه سالم پروژه، واقعاً به `105x61` Resize شده و فقط `25620` بایت داده دارد؛ بنابراین دوباره وارد CMake و UI شد. پیکسل‌های هر سطر Bubble مستقیماً و دقیقاً به‌صورت افقی Flip شده‌اند؛ هیچ Rotation یا Flip عمودی در زمان اجرا وجود ندارد و نوک Message box به سمت چپ و دماسنج است. تصویر قلب نیز با توجه به عرض ۱۲۰ پیکسلی asset در کارت ۱۴۸ پیکسلی، روی مختصات افقی `14` قرار گرفته و دقیقاً Center شده است.

## ۷. شبکه و هشدار

تنظیم پیش‌فرض کابل مستقیم:

- لپ‌تاپ: `192.168.7.1/24`
- برد: `192.168.7.2/24`
- UDP مقصد: `5055`
- UDP مبدأ: `5056`

بعد از سه نمونه غیرعادی پیام `triggered` ساخته می‌شود. اگر وضعیت ادامه یابد حداکثر هر ۳۰ ثانیه یک `reminder` ارسال می‌شود. بعد از سه نمونه عادی پیام `recovered` فرستاده می‌شود. پیام شامل version، sequence، uptime، severity، scenario، مقادیر و reasonها است.

پیاده‌سازی برای نیاز Demo عمداً کوچک است و یک TCP/IP stack عمومی نیست: ARP، IPv4/UDP خروجی و ICMP Echo را پوشش می‌دهد. راهنمای کابل، receiver و webhook در `docs/NETWORK_ALERT.md` است.

در Build فعلی `LCD_BRINGUP_MODE=1` باقی مانده است، اما شبکه غیرفعال نیست: `StartDefaultTask()` تابع `NetworkAlert_Init()` را بدون شرط اجرا می‌کند و این تابع در صورت Reset بودن Handle اترنت، خودش `MX_ETH_Init()` را فراخوانی می‌کند. بنابراین BIN موفق تولیدشده برای تست هم‌زمان LCD، UART و Ethernet مناسب است و برای فعال‌کردن شبکه نیازی به Build مجدد یا تغییر این Macro نیست.

## ۸. اجرا و تست

نصب وابستگی Python:

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

Mock:

```bash
python3 -m simulation --scenario Normal --duration 5
```

UART واقعی:

```bash
python3 -m simulation --list-ports
python3 -m simulation --port /dev/cu.usbmodemXXXX --scenario Low_SpO2
```

گیرنده Ethernet:

```bash
python3 -m network_alert --log alerts.jsonl
```

تمام تست‌های بدون برد:

```bash
./scripts/run_tests.sh
```

نتیجه ثبت‌شده در آخرین اجرا:

- ۱۶ تست Python: موفق
- تست C منطق هشدار با Address/Undefined Sanitizer: موفق
- تست C Parser با Address/Undefined Sanitizer: موفق
- Syntax check فایل‌های تغییرکرده Firmware با هشدارهای سخت‌گیرانه: موفق
- Runner عادی `./scripts/run_tests.sh` بدون Activate دستی نیز موفق است، چون `.venv` را خودکار انتخاب می‌کند.
- Build نهایی ARM در محیط یکپارچه‌سازی هم‌تیمی به علت نبود Toolchain اجرا نشد.
- روی مک آراد Toolchain موجود است. بیلد پیش از دریافت نسخه یکپارچه تا مرحله Link رفت و به علت `Bubble.c` حدود ۲٫۳ مگابایتی با Flash overflow متوقف شد.
- پس از Pull و خارج شدن نسخه بزرگ Bubble از CMake، Build کامل `MinSizeRel` در ۱ سپتامبر ۲۰۲۶ موفق شد. سپس نسخه Resize‌شده Bubble دوباره وارد UI شد و Build نهایی پس از Flip افقی واقعی Bubble و افزودن ECG sweep نیز موفق بود: `text=805256`، `data=1808` و `bss=198304`. مصرف واقعی Flash از جمع `text+data` برابر `807064` بایت، حدود ۷۷٪ از Flash یک مگابایتی، است.
- خروجی‌های جدید `build/VitalSignsMonitor.elf`، `build/VitalSignsMonitor.hex` و `build/VitalSignsMonitor.bin` پس از تغییر Bubble و ECG دوباره تولید شدند.

Build Firmware از ریشه مخزن روی مک آراد:

```bash
cmake -S CubeIDEProject -B CubeIDEProject/build \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/CubeIDEProject/arm-none-eabi.cmake" \
  -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build CubeIDEProject/build -j4
```

اگر کامپایلر در PATH نیست، گزینه زیر نیز داده شود:

```text
-DSTM32_GNU_TOOLS_PATH=/absolute/path/to/toolchain/bin
```

## ۹. کارهای انجام‌شده در یکپارچه‌سازی نهایی

- دریافت آخرین سه Commit هم‌تیمی و همگام شدن `main` با `origin/main`
- جدا کردن UI از `main.c` و حذف تعریف‌های تکراری/خراب Gauge، ECG و Bubble
- رفع خطاهای کامپایل نسخه هم‌تیمی، Scope متغیر abnormal و شرط اشتباه دما
- ساخت UI نهایی با ECG بدون پوشاندن کارت‌ها
- ساخت Parser مستقل و اصلاح ECG منفی و دقت سه رقم اعشار
- تبدیل Ready-line تکی UART به صف هشت‌تایی و افزودن recovery و آمار خطا
- افزودن stale-data indicator و هماهنگی سرعت قلب با HR
- مشترک کردن Thresholdهای UI و شبکه و افزودن Warning/Critical
- افزودن reconnect شبیه‌ساز و تست قطع/اتصال مجدد
- حذف مسیر شخصی عضو تیم از Toolchain CMake
- جایگزینی Asset بسیار بزرگ Bubble با نسخه Resize‌شده ۲۵ کیلوبایتی و استفاده مجدد از آن در UI
- حذف Point markerهای ECG و کاهش سرعت پیشروی نمودار به یک‌سوم
- Center کردن تصویر قلب و Flip افقی پیکسل‌های Message bubble دما بدون Rotation
- افزودن ECG sweep حلقوی چپ‌به‌راست با gap سه‌نقطه‌ای
- اصلاح Runner تست برای انتخاب خودکار `.venv`
- افزودن Runner یکپارچه تست و به‌روزرسانی مستندات

## ۱۰. تنها کارهای باقی‌مانده برای تحویل واقعی

این موارد را نمی‌توان بدون برد و کابل از داخل محیط توسعه انجام داد:

1. Flash کردن BIN یا ELF جدید روی برد و تأیید Center بودن قلب، Flip افقی صحیح Bubble، خوانایی دما، حذف نقاط ECG، سرعت یک‌سوم و ظاهر Sweep حلقوی.
2. اجرای پنج سناریو روی UART و ثبت عکس یا ویدئو از Dashboard و ECG.
3. توقف شبیه‌ساز و تأیید ظاهر شدن `NO DATA` حداکثر بعد از دو ثانیه.
4. تنظیم Ethernet مک روی `192.168.7.1/24` و تأیید Link و `ping 192.168.7.2`.
5. اجرای `Low_SpO2`، مشاهده `triggered` و ثبت خط در `alerts.jsonl`؛ سپس Normal و مشاهده `recovered`.
6. ثبت Commit hash نسخه Demo و نتایج بالا در گزارش نهایی.

## ۱۱. چک‌لیست تحویل

### تست نرم‌افزاری

- [x] تست سناریوها، ECG و schema
- [x] تست UART framing و PySerial loopback
- [x] تست reconnect پورت
- [x] تست Parser C و ECG منفی
- [x] تست Threshold/debounce/reminder/recovery
- [x] تست اعتبارسنجی و dedup گیرنده شبکه
- [x] بررسی نحو فایل‌های تغییرکرده Firmware

### Build و برد

- [x] Build کامل با `arm-none-eabi-gcc`
- [x] ELF/HEX/BIN و اندازه مجاز Flash
- [ ] نمایش سالم UI و ECG روی LCD
- [ ] پنج سناریو روی UART بدون هنگ
- [ ] حالت `NO DATA` پس از قطع UART
- [ ] Ethernet Link و Ping
- [ ] دریافت UDP triggered و recovered
- [ ] عکس LCD، Log شبکه و Commit hash نسخه Demo

## ۱۲. وضعیت Git

در ۱ سپتامبر ۲۰۲۶، Pull بین Commit محلی زیر:

```text
c64c66c nothing
```

و Commit جدید `origin/main`:

```text
d483696 fix
```

فقط روی نام‌ها و محتوای مستندات کانتکست Conflict ایجاد کرد. نتیجه با یک فایل canonical به نام `docs/PROJECT_CONTEXT.md` حل شد و نسخه‌های تکراری `PROJECT_CONTEXT.txt` و `PROJECT_CONTEXT2.md` از نتیجه Merge کنار گذاشته شدند. Merge در Commit `e932f83` تکمیل و روی `origin/main` Push شد.

سه فایل cache تولیدی CMake که در Commit محلی `nothing` اشتباهی Track شده بودند (`CubeIDEProject/CMakeCache.txt` و دو فایل داخل `CubeIDEProject/CMakeFiles`) نیز پیش از Merge commit از مخزن حذف شدند. خروجی صحیح Build فقط در پوشه نادیده‌گرفته‌شده `CubeIDEProject/build/` قرار دارد.

پس از آن، مجموعه اصلاحات UI شامل Bubble کوچک، ECG بدون نقطه و با سرعت یک‌سوم، Center شدن قلب، جهت اولیه Bubble و اصلاح Runner تست در Commit `51568e1` ثبت شد. در اصلاح بعدی Rotation اولیه Bubble با Flip افقی واقعی داده‌های پیکسلی جایگزین و ECG sweep حلقوی چپ‌به‌راست اضافه شد. Build ARM و تمام تست‌های خودکار پیش از Commit اصلاحی موفق بوده‌اند.
