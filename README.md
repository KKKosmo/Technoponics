Technoponics- Software part of my bachelor dissertation, a semi-automated aquaponics system. It uses an Arduino Mega as the microcontroller, and PHP with MySQL for the web applications. With its web applications, it can store data to an SD card and a database, let a user view and download data, send email notifications, and save/load machine settings from the SD card storage. 

It has 2 separate web applications, online and local. The online one is only for remote monitoring, and the local one is for most of the functions, but has minimal front end design due to the limited resources of the arduino. For the email system, I used PHPMailer to save arduino resources by using the online web host's resources instead.


the main arduino code can be improved, since it can sometimes skip functions

learned a bit about proper documentation
