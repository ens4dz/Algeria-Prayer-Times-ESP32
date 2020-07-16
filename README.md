# Salat-Time-esp32
.الهدف من المشروع تصميم ساعة عصرية اقتصادية وقابلة للتخصيص، مثل عرض أحاديث أو أذكار أو مواقيت المحاضرات

مميزات المشروع:
تكلفة منخفضة
قابلية التغيير الشكل في أي وقت
سهل النقل والتركيب
![salat](https://raw.githubusercontent.com/ens4dz/Salat-Time-esp32/master/salat_art.jpg)
![vga_pins](https://raw.githubusercontent.com/ens4dz/Salat-Time-esp32/master/vga_pins.jpg)

## الخصائص الحالية:
- [x] دقة الوقت، تجلب الوقت تلقائيا عند أول تشغيل من الانترنت، 
- [x] طريقة حساب الوقت حسب الوزارة حرفيا.
- [x] تدعم 60 مدينة جزائرية حسب التطبيق الرسمي للجزائر
- [x] تقليص قاعدة البيانات (تحسن ملحوظ من7 ميغا أصبحت 1 ميغا)
- [x] اضافة مؤقتة تعمل ببطارية صغيرة، لتفادي ضياع التوقيت عند انقطاع الكهرباء 
- [x] امكانية تغيير المدينة باستخدام الهاتف والوايفاي

![wifi-config](https://raw.githubusercontent.com/ens4dz/Salat-Time-esp32/master/Screenshot_2020-07-16-12-28-24-966_com.android.chrome.jpg)



## مزايا ناقصة:
- [ ] عرض التاريخ الهجري والميلادي (تحتاج لبرمجة مكتبة للكتابة بالعربية)
- [ ] اضافة اعلانات مثل #أغلق_هاتفك بين الأذان والإقامة الصلاة
- [ ] تحديث قاعدة البيانات تلقائيا بحلول عام هجري جديد
- [ ] اضافة طريقة عرض حديثة، بحيث يعرض صلاة واحدة كل ثانتين لتظهر بشكل أفضل في الشاشات الصغيرة

## المكتبات المستخدمة:

### VGA:
1-https://github.com/bitluni/ESP32Lib 
(GfxWrapper.h needs a patch for rotation )

2-https://github.com/adafruit/Adafruit-GFX-Library


### DB:

3-https://github.com/siara-cc/esp32_arduino_sqlite3_lib


### Rtc time:

4-https://github.com/Makuna/Rtc//clock 


### WEB server and OTA:

5-https://github.com/ayushsharma82/AsyncElegantOTA

6-https://github.com/me-no-dev/ESPAsyncWebServer

7-https://github.com/me-no-dev/AsyncTCP

أرحب بشدة بأي مساهمة لتحسين المشروع
