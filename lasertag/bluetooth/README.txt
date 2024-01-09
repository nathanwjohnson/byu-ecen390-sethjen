To compile the main.c example into an executable that allows you to communicate 
between a smartphone and a ZYBO board, uncomment #add_subdirectory(bluetooth) 
in the lasertag CMakeLists.txt file. To connect with this program via 
Bluetooth, download the Bluefruit Connect app onto either an IOS or Android 
phone, pair with the Adafruit Bluefruit LE device, and then select the UART 
panel. Type a single letter in the box at the bottom, hit send, and its 
uppercase version should appear in the upper window. The blue LED on the 
Bluetooth modem will glow when paired with the app.

Note: bluetooth.c is provided for documentation purpose only so you can see 
what the provided functions do when you call them from your own code. Do not 
compile and link bluetooth.c into your project. It is already compiled into the 
lasertag library. Otherwise, you will get multiple symbol definitions when you 
build your project.
