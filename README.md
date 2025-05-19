The software part of my bachelor dissertation, a semi-automated aquaponics system

It uses an Arduino Mega as the microcontroller, and PHP, MySQL, JavaScript, and Chart.js for the web applications.

Has good file handling, and good arduino memory optimization.

Goal was to make use of technology to make agriculture more efficient.

Resulting in growing food being 10% less tideous

With its web applications, it can store data to an SD card and an SQL database, let users view and download data with their devices, send email notifications, and save/load machine settings from the SD card storage.

It has 2 separate web applications, online and local. The online one is only for remote monitoring, and the local one is for most of the functions, but has minimal frontend design due to the limited resources of the arduino.

For the emails, I used PHPMailer instead of SMTP.js to save arduino resources by using the online web host's resources instead.




the main arduino code can be improved, since it can sometimes skip functions
learned a bit about proper documentation
