# Longan_GD32VF_examples
Tiny example project for Longan Nano (GD32VF) in the platformio environment. 

Adapted my small OLED library which is capable of displaying the two fonts
(an 8x8px and 8x16px font) in three sizes each, by upscaling them with the
simple-but-fast [scale2x and scale3x](https://scale2x.it) algorithm. 

Very alpha, just to get the hang of it. 

Just start PlatformIO, create an empty Longan Nano project with the GD32... framework, and put the 
contents of the GD32V_LCD folder into the project. It should compile fine then. 

Have a look at the [Longan "Getting started..." demo project page](https://longan.sipeed.com/en/get_started/blink.html).

This project is compiled as a native GD32 framework:
```
framework = gd32vf103-sdk
```
This command in the `platformio.ini`file makes the compiler look for the GD32 environment. 
The wonderful thing about PlatformIO is that you can also use the Arduino framework, opening
the Longan to a plethora of libraries for all purposes. 

Stay tuned. 

On compiling, the main file produces numerous warnings for symbols from the header files the compiler pretends it does not know. 
(But it does. Trust me.)

## Steps I had to overcome: 
- Get Visual Studio Code to run (by disabling IE in Win10)
- Get platformio to run (by disabling some weird hidden autostart)
- Get PlatformIO to compile my private library (by shifting the library and header files till it worked)
- Get my Win10 machine to accept the Longan as USB device (by installing [a generic driver](https://longan.sipeed.com/en/get_started/blink.html#install-drivers-using-zadig))
- Get the Longan Nano to go into DFU mode for uploading (by pressing and holding the BOOT button next to the LED, and then pressing RESET)

Find the whole process on www.untergeek.de (ASAP) 
