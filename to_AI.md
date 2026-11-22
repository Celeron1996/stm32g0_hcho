这个项目是一个用STM32G030C8T6作为主控MCU、读取甲醛传感器、并将甲醛数值显示在128*64的0.96寸OLED屏幕上的甲醛测量仪，以下是关于各模块的基本介绍：

1. OLED屏幕：分辨率为128*64的0.96寸单色白色OLED屏幕，屏幕的驱动芯片是SD1315，采用I2C接口，I2C接口连接，连接MCU的PB6和PB7脚，电源上常供电，I2C接口在硬件上已提供上拉电阻，参考资料：D:\project\STM32G0_HCHO\documents\096OLED

2. 甲醛模组：与MCU通过uart串口通信，连接MCU的PA2和PA3脚，电源上5V常供电，参考资料：D:\project\STM32G0_HCHO\documents\HCHO-module

3. STM32G030C8T6：无外部晶振，使用内部晶振，主频配置为64MHZ，使用的GPIO资源：
   
   1. SWDIO调试下载接口（PA13、PA14）
   
   2. 打印log专用的调试uart串口（PA9、PA10）
   
   3. 用于与甲醛模组通信的uart串口（PA2、PA3）
   
   4. 用于与OLED屏幕通信的I2C接口（PB6、PB7），电路上已接上拉电阻

4. 硬件原理图见：D:\project\STM32G0_HCHO\documents\schematic

5. 目录D:\project\STM32G0_HCHO\software\stm32g0_hcho 下的MDK5基本工程已经跑通编译下载调试，即通过J-LINK的SWDI接口下载与调试，请基于这个工程上实现功能

**需要实现的功能：接收甲醛模组的数值，并将其显示在OLED屏幕上，甲醛模组采用主动接收，不需要问答式，即模组每秒钟自动发送给MCU，MCU收到并处理，然后将数值以人类方便阅读的方式显示在OLED屏幕上**
