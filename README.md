The software part of my bachelor dissertation, a semi-automated aquaponics system. It uses an arduino MEGA to function. It can store data in an SD card and a MySQL database through its web application, view and download data through the webapp, send notifications through e-mail, and save/load machine settings from the SD card storage.

It has 2 separate web applications, online and local. The online one is only for remote monitoring, and the local one is for most of the functions, but has limited css due to the limited resources of the arduino.

I used PHPMailer for emails to save arduino resources, it uses the web host's resources instead
