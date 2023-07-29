#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <DS1302.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

#define wLevel A0
#define DHTPIN 7
#define wTempInit 8 
#define pumpRelay 25
#define feederRelay 24
#define feederMotor 3
#define phSensor A1
#define phRelay 23
#define phMotorRelay 22
#define phMotor 6

#define DHTTYPE DHT11

OneWire oneWire2ndInit(wTempInit);
DallasTemperature wTemp(&oneWire2ndInit);

Servo feederServo;
Servo phServo;
int feederPos = 0;
int phPos = 0;

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

Time t;
int Hour;
int Min;
int Sec;
int Year;
int Month;
int Day;
DS1302 rtc(28, 29, 30);


bool getFlag = false;
//*****************pH*******************
float phCalibration = 22.69;
unsigned long int phAvgValue;
int phBuf[10], phTemp;
bool phDipped = false;
float dipStart = 0;

float phValue = 0;
float tempWater = 0;
float tempAir = 0;
float humidity = 0;
int waterLevel = 0;
int displayState = 0; 
int numDisplayStates = 3; 


const byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};
IPAddress shieldIP(192,168,68,123);

EthernetServer server = EthernetServer(80);
const int listSize = 20;

float airTempList[listSize];
float waterTempList[listSize];
float phList[listSize];
String secList[listSize];
float cache[3];

const int HTTP_PORT   = 80;
const char HOST_NAME[] = "lormatechnoponics.tech"; 

String email = "";
String feedingTime = "";

EthernetClient client;
EthernetClient client2;

unsigned long lastPumpOnTime = 0;
bool pumpOnCooldown = false;

File dataFile;
String filename = "";
void setup()
{
  Serial.begin(9600);
  Ethernet.begin(mac);
  server.begin();
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());

  lcd.init();
  lcd.backlight();

  dht.begin();

  pinMode(wLevel, INPUT);

  wTemp.begin();

  pinMode(pumpRelay, OUTPUT);
  feederServo.attach(feederMotor);

  pinMode(feederRelay, OUTPUT);
  pinMode(phRelay, OUTPUT);

  phServo.attach(phMotor);
  pinMode(phMotorRelay, OUTPUT);

  digitalWrite(pumpRelay, HIGH);
  digitalWrite(feederRelay, HIGH);
  digitalWrite(phRelay, HIGH);
  digitalWrite(phMotorRelay, HIGH);

  for (int i = 0; i < listSize; i++)
  {
    airTempList[i] = 0;
    waterTempList[i] = 0;
    phList[i] = 0;
    secList[i] = "x";
  }

  

  
  myGetTime();
  getSensors();
  queueWaterAndAirTemp();
  dipPh();

  printData();
  
  getLatestEntries();
  Serial.print("CACHE Water temperature: ");
  Serial.println(cache[0]);
  Serial.print("CACHE Air temperature: ");
  Serial.println(cache[1]);
  Serial.print("CACHE pH level: ");
  Serial.println(cache[2]);
  
  displayLCD();

  openSD();
  if(SD.begin(4))
      Serial.println("FOUND SD CARD");

  
  loadSettings();

  
  Serial.println("email: " + email);
  Serial.println("feedingTime: " + feedingTime);

  
  SD.end();
}

void loop() {
  // realLoop();
 testLoop2();
}

//short intervals
void testLoop2(){
 maintainWaterLvl(0);
 myGetTime();
 printTime();
 Serial.println("");
 if(Sec % 30 == 0){
   getSensors();
   queueWaterAndAirTemp();
   printData();
    if(Min % 3 == 0){
      feedFish();
   }
  if(!phDipped){
    dipPh();
  }
 }
 if(phDipped && millis() - dipStart >= 30000){
   getPh();
   queuePhLevel();
   Serial.println("UNDIPPING PH");
   phDipped = false;
 }
 else if(phDipped && millis() - dipStart < 150000){
  Serial.println("PH NOT READY YET, " + String(millis() - dipStart));
 }
  if(Sec % 10 == 0){
      Serial.println("tempWater = " + String(tempWater));
      Serial.println("tempAir = " + String(tempAir));
      Serial.println("phValue = " + String(phValue));
      Serial.println("cache[0] = " + String(cache[0]));
      Serial.println("cache[1] = " + String(cache[1]));
      Serial.println("cache[2] = " + String(cache[2]));

    if(abs(tempWater - cache[0]) > 0.5 || abs(tempAir - cache[1]) > 0.5 || (abs(phValue - cache[2]) > 0.5 && phValue != 0)){
      Serial.println("CHANGED");
      writeSensorsIntoCSV();
      insertToDb(6);
      cache[0] = tempWater;
      cache[1] = tempAir;
      cache[2] = phValue;
    }
    else{
      Serial.println("!!!!!!!!!!!NOT SAVING, DATA NOT CHANGED.");
    }
  }
 if(Sec % 2 == 0){
   displayLCD();
   printWebApp();
 }
 delay(1000); 
}

//testing hardware functionality
void testLoop(){
    maintainWaterLvl(30);
    myGetTime();
    printTime();
    if (Sec % 4 == 0)
    {
      getSensors();
      queueWaterAndAirTemp();
      feedFish();
      printWebApp();
      writeSensorsIntoCSV();
      insertToDb(6);
    }
     if(Sec % 2 == 0){
       displayLCD();
     }
    delay(1000);
}

void realLoop(){
 maintainWaterLvl(30);

 myGetTime();
 printTime();// print to serial for testing
 Serial.println("");

  //every 1 hour, get water temperature, air temperature, humidity values and display to lcd, also queueing
  //water and air temperature to real time values array
  //if there was significant change to data, insert data to DB and SD
  if(Min >= 25 && Sec == 0 && getFlag == false){
    Serial.println("GETFLAG = true");
    getFlag = true;
    //dip the pH probe into the water if not dipped, wait for 2.5 minutes for the pH probe to stabilize, then get the pH.
    if(!phDipped)
      dipPh();
  
    getSensors();
    queueWaterAndAirTemp();
    printData();// print to serial for testing
    
    //every 8 AM and 8 PM, feed the fish

    if((Hour == feedingTime.toInt()  || Hour == (12 + feedingTime.toInt())) && Min == 0 && Sec == 0){
        feedFish();
    }
  }
  else if(Min < 25 && Sec == 0){
    Serial.println("GETFLAG = false");
    getFlag = false;
  }


  //if ph is dipped and has already been 2.5 minutes, get ph
  if(phDipped && millis() - dipStart >= 150000){
    Serial.println("UNDIPPED PH, WILL SAVE IF NEEDEDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD");
    getPh();
    queuePhLevel();
    phDipped = false;

    //set the safety thresholds, if went past thresholds, send notifs. (water lower limit, air lower limit, ph lower limit, water upper limit, air upper limit, ph upper limit)
    danger(25, 25, 4, 40, 40, 10);

    Serial.println("tempWater = " + String(tempWater));
    Serial.println("tempAir = " + String(tempAir));
    Serial.println("phValue = " + String(phValue));
    Serial.println("cache[0] = " + String(cache[0]));
    Serial.println("cache[1] = " + String(cache[1]));
    Serial.println("cache[2] = " + String(cache[2]));

    if((abs(tempWater - cache[0]) > 0.5 || abs(tempAir - cache[1]) > 0.5 || (abs(phValue - cache[2]) > 0.5)) && phValue != 0){
      Serial.println("CHANGED");
      writeSensorsIntoCSV();
      insertToDb(6);
      cache[0] = tempWater;
      cache[1] = tempAir;
      cache[2] = phValue;
    }
    else{
      Serial.println("!!!!!!!!!!!NOT SAVING, DATA NOT CHANGED.");
    }
  }
  else if(phDipped && millis() - dipStart < 150000){
    Serial.println("PH NOT READY YET, " + String(millis() - dipStart));
  }






  //change lcd content
 //every 2s,   webapp if there is client
 if(Sec % 2 == 0){
   displayLCD();
   printWebApp();
 }


 delay(1000); 
}

//get latest from sd card, to prevent duplicates
void getLatestEntries(){    
  Serial.println("GETTING LATEST ENTRIES----------------------------------------------------------");
  
  openSD();
  if(SD.begin(4))
      Serial.println("FOUND SD CARD");
  dataFile = SD.open("ATD.csv", FILE_READ);

  if (!dataFile) {
    sendNotifs(6, "sdCardATV");
    Serial.println("Failed to open " + filename + " data file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    cache[0] = 0;
    cache[1] = 0;
    cache[2] = 0;
  }
  else{
    Serial.println("FOUND " + filename + " FILE");
    dataFile.seek(dataFile.size()-6);

    bool found = false;
    while (dataFile.position() > 0 && !found) {
      char currentChar = dataFile.read();
      if (currentChar == '\n') {
        found = true;
      }
      else {
        dataFile.seek(dataFile.position() - 2);
      }
    }
      Serial.println();

    if (found) {
      String dataString = dataFile.readStringUntil('\n');
      Serial.println("dataString: ");
      Serial.println(dataString);

      int firstCommaIndex = dataString.indexOf(",") + 1; 
      int secondCommaIndex = dataString.indexOf(",", firstCommaIndex);  
      int thirdCommaIndex = dataString.indexOf(",", secondCommaIndex + 1); 
      int fourthCommaIndex = dataString.indexOf(",", thirdCommaIndex + 1); 

      cache[0] = (dataString.substring(secondCommaIndex + 1, thirdCommaIndex)).toFloat();  
      cache[1] = (dataString.substring(thirdCommaIndex + 1, fourthCommaIndex)).toFloat();  
      cache[2] = (dataString.substring(fourthCommaIndex + 1, dataString.length() - 1)).toFloat(); 

      Serial.println("HERE ARE  THE VALUES");
      Serial.println(cache[0]);
      Serial.println(cache[1]); 
      Serial.println(cache[2]); 
    }
    else {
      Serial.println("Last row not found!");
    }
  }




  Serial.print("CLOSING ");     
  Serial.println(dataFile.name());
  dataFile.close();
  if (dataFile.available()) {
      Serial.println("Data file is still open.");
  } else {
      Serial.println("Data file is closed.");
  }

  SD.end();
  Serial.println("SAFE TO EJECT");
}


//download today's data
void printTodayDataStorage(EthernetClient client)
{
    openSD();
    if(SD.begin(4))
        Serial.println("FOUND SD CARD");

    Serial.println("GETTING DATA, DO NOT EJECT SD CARD");

    char filenamef[20];

    sprintf(filenamef, "%04d%02d%02d.csv", Year, Month, Day);
    filename = String(filenamef);
    dataFile = SD.open(filename, FILE_READ);

    
    if (dataFile) {
        Serial.println("FOUND " + filename);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/csv");
        client.println("Content-Disposition: attachment; filename="+filename+".csv");
        client.println("Connection: close");
        client.println();
        while (dataFile.available()) {
          client.write(dataFile.read());
        }
        Serial.println("SUCCESSFULLY DOWNLOAD "+filename);
    } else {
        Serial.println("DID NOT FIND " + filename);
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        Serial.println("FAILED TO DOWNLOAD " + filename);
    }
    dataFile.close();
    if (dataFile.available()) {
        Serial.println("WARNING Data file is still open.");
    } else {
        Serial.println("Data file is closed.");
    }
    SD.end();
}

//download ALL data
void printAllTimeDataStorage(EthernetClient client)
{
    openSD();
    if(SD.begin(4))
        Serial.println("FOUND SD CARD");

    Serial.println("GETTING DATA, DO NOT EJECT SD CARD");
    File dataFile = SD.open("ATD.csv", FILE_READ);
    if (dataFile) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/csv");
        client.println("Content-Disposition: attachment; filename=ATD.csv");
        client.println("Connection: close");
        client.println();
        while (dataFile.available()) {
          client.write(dataFile.read());
        }
        Serial.println("SUCCESSFULLY DOWNLOAD ATD");
    } else {
        client.println("HTTP/1.1 404 Not Found");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        Serial.println("FAILED TO DOWNLOAD ATD");
    }
    dataFile.close();
    if (dataFile.available()) {
        Serial.println("WARNING Data file is still open.");
    } else {
        Serial.println("Data file is closed.");
    }
    SD.end();
}

//save data into sd card
void writeSensorsIntoCSV(){
    Serial.println("--------------------------------------------------------------------------");
    openSD();
    if(SD.begin(4))
        Serial.println("FOUND SD CARD");

    char filenamef[20];

    sprintf(filenamef, "%04d%02d%02d.csv", Year, Month, Day);
    filename = String(filenamef);
        dataFile = SD.open(filename, FILE_WRITE);
        Serial.println("SAVING DATA, DO NOT EJECT SD CARD");
        if (!dataFile) {
            sendNotifs(6, "sdCardToday");
            Serial.println("Failed to open " + filename + " data file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        }
        else{
            Serial.println("FOUND " + filename + " FILE");
            if (dataFile.size() == 0) {
                    String header = "Time,WaterTemp,AirTemp,pHLevel";
                    writeCSV(header, filename);
                    Serial.println(filename + " EMPTY, WRITING HEADER");
                }
            String dataString = String(Hour) + ":" + String(Min) + ":" + String(Sec) + ",";
            dataString += String(tempWater) + "," + String(tempAir) + "," + String(phValue);
            writeCSV(dataString, filename);
        }
        Serial.print("CLOSING ");     Serial.println(dataFile.name());
        dataFile.close();
        if (dataFile.available()) {
            Serial.println("Data file is still open.");
        } else {
            Serial.println("Data file is closed.");
        }


    filename = "ATD.csv";
    dataFile = SD.open(filename, FILE_WRITE);
    Serial.println("SAVING DATA, DO NOT EJECT SD CARD");
    if (!dataFile) {
        sendNotifs(6, "sdCardATV");
        Serial.println("Failed to open " + filename + " data file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    else{
        Serial.println("FOUND " + filename + " FILE");
        if (dataFile.size() == 0) {
            String header = "Date,Time,WaterTemp,AirTemp,pHLevel";
                writeCSV(header, filename);
                Serial.println(filename + " EMPTY, WRITING HEADER");
            }
          String dataString = String(Year) + "-" + String(Month) + "-" + String(Day) + ",";
          dataString += String(Hour) + ":" + String(Min) + ":" + String(Sec) + ",";
          dataString += String(tempWater) + "," + String(tempAir) + "," + String(phValue);
          writeCSV(dataString, filename);
    }
    Serial.print("CLOSING ");     Serial.println(dataFile.name());
    dataFile.close();
    if (dataFile.available()) {
        Serial.println("Data file is still open.");
    } else {
        Serial.println("Data file is closed.");
    }

    SD.end();
    Serial.println("SAFE TO EJECT");
}
//helper function for saving data into sd card
void writeCSV(String dataString, String _filename)
{
    Serial.println("");
    Serial.println("WRITING CSV");
    Serial.println("{");
    Serial.println("    " + _filename);
    Serial.println("    " + dataString);
    Serial.println("}");
    const int MAX_COLS = 5;
    String rowData[MAX_COLS];

    int start = 0;
    int index = 0;
    for (int i = 0; i < dataString.length(); i++) {
        if (dataString.charAt(i) == ',') {
            rowData[index++] = dataString.substring(start, i);
            start = i + 1;
        }
    }
    rowData[index] = dataString.substring(start);

    dataFile.seek(dataFile.size());
    for (int i = 0; i < MAX_COLS; i++) {
        dataFile.print(rowData[i]);
        if (i < MAX_COLS - 1) {
            dataFile.print(",");
        }
    }
    dataFile.println();
}

void openSD() {
  unsigned long startTime = millis(); 
  while (!SD.begin(4)) {
    if (millis() - startTime > 5000) { 
      Serial.println("SD CARD NOT FOUND!");
      return;
    }
  }
  Serial.println("SD CARD FOUND!");
}

//set thresholds for danger, will send notifications
void danger(float wLow, float aLow, float pLow, float wHigh, float aHigh, float pHigh){
  if(
      tempWater > wHigh
  || tempAir > aHigh
  || phValue > pHigh
  || tempWater < wLow
  || tempAir < aLow
  || phValue < pLow
  ){
    if(tempWater > wHigh){
      sendNotifs(6, "wHigh");
      Serial.println("wHigh: ");
    }
    else if(tempAir > aHigh){
      sendNotifs(6, "aHigh");
      Serial.println("aHigh: ");
    }
    else if(phValue > pHigh){
      sendNotifs(6, "pHigh");
      Serial.println("pHigh: ");
    }
    else if(tempWater < wLow){
      sendNotifs(6, "wLow");
      Serial.println("wLow: ");
    }
    else if(tempAir < aLow){
      sendNotifs(6, "aLow");
      Serial.println("aLow: ");
    }
    else if(phValue < pLow && phValue != 0){
      sendNotifs(6, "pLow");
      Serial.println("pLow: ");
    }
  }
}

//this is for starting the web server
void printWebApp()
{
  client = server.available();
  if (client) {
    Serial.println("New client connected!");
    boolean currentLineIsBlank = true;
    String route = ""; 
    while(client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        //which page to display?
        if (c == '\n' && currentLineIsBlank) {
          if(route == "/sdCardATD"){
            printAllTimeDataStorage(client);
          }
          else if(route == "/sdCardToday"){
            printTodayDataStorage(client);
          }
          else if (route.indexOf("/settings") != -1) {
            String params = route.substring(route.indexOf("/settings?"));
            Serial.println("PARAMS = " + params);
            saveSettings(params);
            printMainPage(client);
          }
          else {
            printMainPage(client);
          }
          break;
        } 
        else if (c == ' ' && route == "") {
          route = client.readStringUntil(' ');
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
  else{
    Serial.println("NO CLIENT");
  }
}

void printTestPage(EthernetClient client){
  Serial.println("PRINTING TEST PAGE");
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html"));
    client.println(F("<html><head><title>Arduino Web Server</title></head>"));

    client.println(F("<body>TESTINGGGG</body></html>"));
}


//displaying latest data from sd card
void printMainPage(EthernetClient client)
{

    client.println(F(""));
    client.println(F("HTTP/1.1 200 OK\r"));
    client.println(F("Content-Type: text/html\r"));


    client.println(F(""));
    client.println(F("<html><head><title>Arduino Web Server</title></head>"));
    client.println(F("<body>"));
    client.println(F("<center>"));
    client.println(F("<h1>REAL TIME</h1>"));
    client.println(F("<div>"));
    client.println(F("<h1>Air Temperature</h1>"));
    client.println(F("<canvas id='graph'></canvas>"));
    client.println(F("</div>"));
    client.println(F("<br>"));
    client.println(F("<br>"));
    client.println(F("<br>"));
    client.println(F("<div>"));
    client.println(F("<h1>Water Temperature</h1>"));
    client.println(F("<canvas id='graph2'></canvas>"));
    client.println(F("</div>"));

    client.println(F("<br>"));
    client.println(F("<br>"));
    client.println(F("<br>"));
    client.println(F("<div>"));
    client.println(F("<h1>pH Level</h1>"));
    client.println(F("<canvas id='graph3'></canvas>"));
    client.println(F("</div>"));

    client.println(F("</center>"));
    client.println(F("<script>"));
    client.println(F("// Sample data"));

    printATempJavascript(client);
    printWTempJavascript(client);
    printPhJavascript(client);
    printTimeJavascript(client);
    

    client.println(F(""));
    client.println(F("const canvasWidth = 1200;"));
    client.println(F("const canvasHeight = 400;"));
    client.println(F(""));
    client.println(F("// Bar graph dimensions"));
    client.println(F("const barMargin = 10;"));
    client.println(F("const barWidth = (canvasWidth - (data.length - 1) * barMargin) / data.length;"));
    client.println(F("const maxBarHeight = canvasHeight - 100;"));
    client.println(F(""));

    client.println(F("// Create canvas"));
    client.println(F("const canvas = document.getElementById('graph');"));
    client.println(F("canvas.width = canvasWidth;"));
    client.println(F("canvas.height = canvasHeight;"));
    client.println(F("canvas.style.border = '1px solid black';"));
    client.println(F("canvas.style.padding = '10px';"));
    client.println(F(""));
    client.println(F("// Get context"));
    client.println(F("const context = canvas.getContext('2d');"));
    client.println(F(""));
    client.println(F("// Draw bars"));
    
    client.println(F("for (let i = 0; i < data.length; i++) {"));

    client.println(F("const minValue = Math.min(...data);"));
    client.println(F("const maxValue = Math.max(...data);"));
    client.println(F("let barHeight;\n"));
    client.println(F("barHeight = maxBarHeight * ((data[i] - minValue + 0.1) / (maxValue - minValue + 0.1));\n"));

    client.println(F("const x = i * (barWidth + barMargin);"));
    client.println(F("const y = canvasHeight - barHeight - 20;"));

    client.println(F("let color;"));
    client.println(F("if (data[i] < 10) {color = 'blue';"));
    client.println(F("} else if (data[i] >= 10 && data[i] <= 15) {color = 'DodgerBlue';"));
    client.println(F("} else if (data[i] >= 15 && data[i] <= 33) {color = 'green';"));
    client.println(F("} else if (data[i] > 33 && data[i] <= 35) {color = 'orange';"));
    client.println(F("} else {color = 'red';}"));

    client.println(F("let symbol;"));
    client.println(F("if (color === 'green') {symbol = '\u25CF';"));
    client.println(F("} else if (color === 'orange' || color === 'DodgerBlue') {symbol = '\u25C6';"));
    client.println(F("} else {symbol = '\u25A0';}"));

    client.println(F("context.fillStyle = color;"));
    client.println(F("context.fillRect(x, y, barWidth, barHeight);"));

    client.println(F("context.font = '12px sans-serif';"));
    client.println(F("context.textAlign = 'center';"));
    client.println(F("context.fillText(data[i] + ' ' + symbol, x + barWidth / 2, y - 5);"));
    client.println(F("context.fillText(time[i], x + barWidth / 2, canvasHeight - 5);"));
    client.println(F("}"));
    client.println(F(""));
    client.println(F("//GRAPH 2"));
    client.println(F(""));
    client.println(F("// Create canvas"));
    client.println(F("const canvas2 = document.getElementById('graph2');"));
    client.println(F("canvas2.width = canvasWidth;"));
    client.println(F("canvas2.height = canvasHeight;"));
    client.println(F("canvas2.style.border = '1px solid black';"));
    client.println(F("canvas2.style.padding = '10px';"));
    client.println(F("const context2 = canvas2.getContext('2d');\n"));
    
    client.println(F("for (let i = 0; i < data2.length; i++) {\n"));

    client.println(F("const minValue = Math.min(...data2);"));
    client.println(F("const maxValue = Math.max(...data2);"));
    client.println(F("let barHeight;\n"));
    client.println(F("barHeight = maxBarHeight * ((data2[i] - minValue + 0.1) / (maxValue - minValue + 0.1));\n"));

    client.println(F("const x = i * (barWidth + barMargin);\n"));
    client.println(F("const y = canvasHeight - barHeight - 20;\n"));



    client.println(F("let color;"));
    client.println(F("if (data2[i] < 10) {color = 'blue';"));
    client.println(F("} else if (data2[i] >= 10 && data2[i] <= 15) {color = 'DodgerBlue';"));
    client.println(F("} else if (data2[i] >= 15 && data2[i] <= 33) {color = 'green';"));
    client.println(F("} else if (data2[i] > 33 && data2[i] <= 35) {color = 'orange';"));
    client.println(F("} else {color = 'red';}"));

    client.println(F("let symbol;"));
    client.println(F("if (color === 'green') {symbol = '\u25CF';"));
    client.println(F("} else if (color === 'orange' || color === 'DodgerBlue') {symbol = '\u25C6';"));
    client.println(F("} else {symbol = '\u25A0';}"));

    client.println(F("context2.fillStyle = color;"));
    client.println(F("context2.fillRect(x, y, barWidth, barHeight);"));

    client.println(F("context2.font = '12px sans-serif';"));
    client.println(F("context2.textAlign = 'center';"));
    client.println(F("context2.fillText(data2[i] + ' ' + symbol, x + barWidth / 2, y - 5);"));
    client.println(F("context2.fillText(time[i], x + barWidth / 2, canvasHeight - 5);"));
    client.println(F("}\n"));

    client.println(F(""));
    client.println(F(""));
    client.println(F("//GRAPH 3"));
    client.println(F(""));
    client.println(F("// Create canvas"));
    client.println(F("const canvas3 = document.getElementById('graph3');"));
    client.println(F("canvas3.width = canvasWidth;"));
    client.println(F("canvas3.height = canvasHeight;"));
    client.println(F("canvas3.style.border = '1px solid black';"));
    client.println(F("canvas3.style.padding = '10px';"));
    client.println(F("const context3 = canvas3.getContext('2d');\n"));
    
    client.println(F("for (let i = 0; i < data3.length; i++) {\n"));

    client.println(F("const minValue = Math.min(...data3);"));
    client.println(F("const maxValue = Math.max(...data3);"));
    client.println(F("let barHeight;\n"));
    client.println(F("barHeight = maxBarHeight * ((data3[i] - minValue + 0.1) / (maxValue - minValue + 0.1));\n"));

    client.println(F("const x = i * (barWidth + barMargin);\n"));
    client.println(F("const y = canvasHeight - barHeight - 20;\n"));



    client.println(F("let color;"));
    client.println(F("if (data3[i] < 5.0) {color = 'red';"));
    client.println(F("} else if (data3[i] >= 5.0 && data3[i] <= 6.5) {color = 'orange';"));
    client.println(F("} else if (data3[i] >= 6.5 && data3[i] <= 9.0) {color = 'green';"));
    client.println(F("} else if (data3[i] > 9.0 && data3[i] <= 10.0) {color = 'DodgerBlue';"));
    client.println(F("} else {color = 'SlateBlue';}"));

    client.println(F("let symbol;"));
    client.println(F("if (color === 'green') {symbol = '\u25CF';"));
    client.println(F("} else if (color === 'orange' || color === 'DodgerBlue') {symbol = '\u25C6';"));
    client.println(F("} else {symbol = '\u25A0';}"));

    client.println(F("context3.fillStyle = color;"));
    client.println(F("context3.fillRect(x, y, barWidth, barHeight);"));

    client.println(F("context3.font = '12px sans-serif';"));
    client.println(F("context3.textAlign = 'center';"));
    client.println(F("context3.fillText(data3[i] + ' ' + symbol, x + barWidth / 2, y - 5);"));
    client.println(F("context3.fillText(time[i], x + barWidth / 2, canvasHeight - 5);"));
    client.println(F("}\n"));

    client.println(F("</script>\n"));

    client.println(F("<center>"));


    client.println(F("<a href='/sdCardATD'>DOWNLOAD ALL DATA FROM SD CARD</a>"));
    client.println(F("<br><a href='/sdCardToday'>DOWNLOAD TODAY'S DATA FROM SD CARD</a>"));
    

    if (client2.connect(HOST_NAME, HTTP_PORT)){
      client.println(F("<br><a href='http://lormatechnoponics.tech/24hours.php'>View 24 Hours</a>"));
      client.println(F("<br><a href='https://lormatechnoponics.tech/graphs.html'>View ALL DATA FROM ONLINE DATABASE (GRAPHS)</a>"));
      client.println(F("<br><a href='http://lormatechnoponics.tech/table.html'>View ALL DATA FROM ONLINE DATABASE (TABLE)</a>"));
      
      client.println(F("<br><a href='http://lormatechnoponics.tech/download.php'>DOWNLOAD ALL DATA FROM LORMATECHNOPONICS.TECH</a>"));
      client.println(F("<br><br>DOWNLOAD DATA FROM SPECIFIC DATES: "));
      client.println(F("<form action='http://lormatechnoponics.tech/download.php'>"));
      client.println(F("<label for='sDate'>Start Date:</label><br>"));
      client.println(F("<input type='date' id='sDate' name='sDate' required>"));
      client.println(F("<input type='time' id='sTime' name='sTime' required><br>"));
      client.println(F("<label for='eDate'>End Date:</label><br>"));
      client.println(F("<input type='date' id='eDate' name='eDate' required>"));
      client.println(F("<input type='time' id='eTime' name='eTime' required><br><br>"));
      client.println(F("<input type='submit' value='Submit'>"));
      client.println(F("</form>"));
    }
    else{
      client.println(F("<br>Can't view hourly data. Can't Download data. Reason: Can't connect to database."));
    }

    client.print(F("Receiving email for danger notifications, current email = "));
    client.println(email);
    client.println(F("<form action='/settings' method='GET'>"));
    client.println(F("  <label for='email'>Email:</label>"));
    client.println("  <input type='email' id='email' name='email' value='" + email + "' required>");
    client.println(F("  <br>"));
    client.println(F("  <label for='feedingTime'>Feeding HOUR(e.g 8 = 8 AM and PM):</label>"));
    client.println("  <input type='number' id='feedingTime' name='feedingTime' value='" + feedingTime + "' min='1' max='12' required>");
    client.println(F("  <br>"));
    client.println(F("  <input type='submit' value='Submit'>"));
    client.println(F("</form>"));


  client.println(F("<div>"));
  client.println(F("  <h2>Legends</h2>"));
  client.println(F("  <h3>Temperature</h3>"));
  client.println(F("  <span style='background-color: blue; width: 20px; height: 20px; display: inline-block; border-radius: 0;'></span> Very Bad, Extremely Cold - Temperature below 10°C <br>"));
  client.println(F("  <span style='background-color: DodgerBlue; width: 20px; height: 20px; display: inline-block; transform: rotate(45deg);'></span> Bad, Too Cold - Temperature between 10°C and 15°C <br>"));
  client.println(F("  <span style='background-color: green; width: 20px; height: 20px; display: inline-block; border-radius: 50%;'></span> Good - Temperature between 15°C and 33°C <br>"));
  client.println(F("  <span style='background-color: orange; width: 20px; height: 20px; display: inline-block; transform: rotate(45deg);'></span> Bad, Too Hot - Temperature between 33°C and 35°C <br>"));
  client.println(F("  <span style='background-color: red; width: 20px; height: 20px; display: inline-block; border-radius: 0;'></span> Very Bad, Extremely Hot - Temperature above 35°C <br>"));
  client.println(F("  <br>"));
  client.println(F("  <h3>pH Level</h3>"));
  client.println(F("  <span style='background-color: red; width: 20px; height: 20px; display: inline-block; border-radius: 0;'></span> Very Bad, Extremely Acidic - pH Level below 5.0 <br>"));
  client.println(F("  <span style='background-color: orange; width: 20px; height: 20px; display: inline-block; transform: rotate(45deg);'></span> Bad, Too Acidic - pH Level between 5.0 and 6.5 <br>"));
  client.println(F("  <span style='background-color: green; width: 20px; height: 20px; display: inline-block; border-radius: 50%;'></span> Good - pH Level between 6.5 and 9.0 <br>"));
  client.println(F("  <span style='background-color: DodgerBlue; width: 20px; height: 20px; display: inline-block; transform: rotate(45deg);'></span> Bad, Too Dirty - pH Level between 9.0 and 10.0 <br>"));
  client.println(F("  <span style='background-color: SlateBlue; width: 20px; height: 20px; display: inline-block; border-radius: 0;'></span> Very Bad, Extremely Dirty - pH Level above 10.0 <br>"));
  client.println(F("</div>"));









    client.println(F("</center>"));


    client.println(F("</body>\n"));
    client.println(F("</html>\n"));
}


void displayLCD()
{
  if (displayState == 0)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("pH Level: ");
    lcd.print(phValue);
    lcd.setCursor(0, 1);
    lcd.print(Ethernet.localIP());
  }
  else if (displayState == 1)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Water Temp: ");
    lcd.print(tempWater);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("Level: ");
    lcd.print(waterLevel);
    lcd.print(" %     ");
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Air Temp: ");
    lcd.print(tempAir);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print(" %");
  }

  displayState = (displayState + 1) % numDisplayStates;
}

//dip ph sensor
void dipPh()
{
  Serial.println("DIPPING PH");
  digitalWrite(phMotorRelay, LOW);
  phServo.write(180);
  delay(500);
  digitalWrite(phMotorRelay, HIGH);

  dipStart = millis();
  phDipped = true;
}

//activate ph sensor
void getPh(){
  digitalWrite(phRelay, LOW);
  delay(3000);
  for (int i = 0; i < 10; i++)
    {
      phBuf[i] = analogRead(phSensor);
      delay(30);
    }
    for (int i = 0; i < 9; i++)
    {
      for (int j = i + 1; j < 10; j++)
      {
        if (phBuf[i] > phBuf[j])
        {
          phTemp = phBuf[i];
          phBuf[i] = phBuf[j];
          phBuf[j] = phTemp;
        }
      }
    }
    phAvgValue = 0;
    for (int i = 2; i < 8; i++)
      phAvgValue += phBuf[i];
    float pHVol = (float)phAvgValue * 5.0 / 1024 / 6;
    phValue = -5.70 * pHVol + phCalibration;
    digitalWrite(phRelay, HIGH);

    digitalWrite(phMotorRelay, LOW);
    phServo.write(0);
    delay(1000);
    digitalWrite(phMotorRelay, HIGH);
}

void feedFish()
{
  Serial.println("FEEDING FISH");
  digitalWrite(feederRelay, LOW);
  feederServo.write(0);
  delay(1000);
  feederServo.write(180);
  delay(1000);
  feederServo.write(0);
  delay(1000);
  feederServo.write(180);
  delay(1000);
  feederServo.write(0);
  delay(1000);
  feederServo.write(180);
  delay(1000);
  digitalWrite(feederRelay, HIGH);
}

void myGetTime()
{
  t = rtc.getTime();
  Hour = t.hour;
  Min = t.min;
  Sec = t.sec;
  Year = t.year;
  Month = t.mon;
  Day = t.date;
}

String timeToString(){
  return(Hour + String(":") + Min + String(":") + Sec);
}

void printTime()
{
  Serial.print(Year);
  Serial.print("-");
  Serial.print(Month);
  Serial.print("-");
  Serial.println(Day);

  Serial.println(timeToString());
}

void queuePhLevel()
{
  for (int x = 0; x < listSize - 1; x++)
  {
    phList[x] = phList[x + 1];
  }
  phList[listSize - 1] = phValue;
}

void queueWaterAndAirTemp()
{
  for (int x = 0; x < listSize - 1; x++)
  {
    airTempList[x] = airTempList[x + 1];
    waterTempList[x] = waterTempList[x + 1];
    secList[x] = secList[x + 1];
  }
  airTempList[listSize - 1] = tempWater;
  waterTempList[listSize - 1] = tempAir;
  secList[listSize - 1] = timeToString();
}

//inside the function has code to prevent getting stuck
void maintainWaterLvl(int lvl)
{ 
  waterLevel = analogRead(wLevel);
  waterLevel = map(waterLevel, 0, 1023, 0, 100); 
  Serial.println("WATER LEVEL = " + String(waterLevel)); 
  if(!pumpOnCooldown && waterLevel < lvl){
    Serial.println("STARTING COOLDOWNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN");
    pumpOnCooldown = true;
    lastPumpOnTime = millis();
      int timer = 0;
      while (waterLevel < lvl && timer < 12) {
        digitalWrite(pumpRelay, LOW);
        waterLevel = analogRead(wLevel); 
        waterLevel = map(waterLevel, 0, 1023, 0, 100); 
    
        Serial.println("WLEVEL = " + String(waterLevel));
        Serial.println("TIMER = " + String(timer));
        timer++;
        delay(10000);
      }
      digitalWrite(pumpRelay, HIGH);
  }
  else{
      //60000 one minute
      //3600000 one hour
      //600000 10 mins
      if(pumpOnCooldown && waterLevel < lvl){
        if (millis() - lastPumpOnTime >= 3600000) {
          pumpOnCooldown = false;
          Serial.println("COOLDOWN DONEEEEEEEEEEEEEEEEEEEEE");
          maintainWaterLvl(lvl);
        }
        else{
          Serial.println("STILL ON COOLDOWN: " + String(millis() - lastPumpOnTime));
        }
      }
      else if (pumpOnCooldown && waterLevel > lvl){
        Serial.println("pump on cooldown and no need to go off");
      }
      else{
        Serial.println("pump NOT on cooldown and no need to go off");
      }
    }
}

void getSensors()
{
  wTemp.requestTemperatures();
  tempWater = (String(wTemp.getTempCByIndex(0)) == " NaN") ? -1 : wTemp.getTempCByIndex(0);
  tempAir = dht.readTemperature();
  humidity = dht.readHumidity();
  waterLevel = analogRead(wLevel);
  waterLevel = map(waterLevel, 0, 1023, 0, 100);
}

void insertToDb(int retries){
    Serial.println("INSERTING TO DB");
    String date = String(Year) + "-" + String(Month) + "-" + String(Day);
    String time = timeToString();

    String queryString = "GET /insert_temp.php?date="+date+"&time="+time+"&wTemp="+String(tempWater, 2)+"&aTemp="+String(tempAir, 2)+"&phLvl="+String(phValue, 2)+" HTTP/1.1";
    Serial.println(queryString);

    if (client.connect(HOST_NAME, HTTP_PORT)) {
        Serial.println("CONNECTED TO DB, INSERTING");
        client.println(queryString + "\r\nHost: " + String(HOST_NAME) + "\r\nUser-Agent: Arduino/1.0\r\nConnection: close\r\n");
        delay(2000); 
        Serial.println("");
        
        if(client.available()){
            String line = client.readStringUntil('\n');
            Serial.println("FIRST LINE = " + line);
            if (!line.startsWith("HTTP/1.1 200 OK")) {
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
                retries--;
                if(retries == 0){
                    Serial.println("BAD REQUEST AFTER 3 TRIES, EXITING LOOP.");
                }
                else{
                    client.stop();
                    insertToDb(retries);
                    return;
                }
            }
        }
        else{
          Serial.println("NO CLIENT AT INSERTING TO DB.");
        }

        Serial.println();
        Serial.println();
        Serial.println("################################################################################");
        while (client.available()) { 
            char c = client.read();
            Serial.print(c);
            client.flush();
        }
        Serial.println("");
        Serial.println("################################################################################");
        Serial.println();
        Serial.println();
        client.stop(); 
    } else {
        Serial.println("Connection failed"); 
        Serial.print("My IP address: ");
        Serial.println(Ethernet.localIP());
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
        Serial.println("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    }
    Serial.println("---------------------------------------------------------");
}

void sendNotifs(int retries, String danger) {
  Serial.println("SENDING NOTIFS");

  String date = String(Year) + "-" + String(Month) + "-" + String(Day);
  String time = timeToString();

  String queryString = "GET /emailScript.php?date=" + date + "&time=" + time + "&wTemp=" + tempWater + "&aTemp=" + tempAir + "&phLvl=" + phValue + "&receiver=" + email + "&danger=" + danger + " HTTP/1.1";
  Serial.println(queryString);


  if (client.connect(HOST_NAME, HTTP_PORT)){
    Serial.println("CONNECTED TO MYSQL- NOTIFS");
    client.println(queryString + "\r\nHost: " + String(HOST_NAME) + "\r\nConnection: close\r\n");
    if(client.available()){
        String line = client.readStringUntil('\n');
        Serial.println("FIRST LINE = " + line);
        if (!line.startsWith("HTTP/1.1 200 OK")) {
            Serial.println("Received 'HTTP/1.1 400 Bad Request' response, retrying request...");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
            retries--;
            if(retries == 0){
                Serial.println("BAD REQUEST AFTER 3 TRIES, EXITING LOOP.");
            }
            else{
                client.stop();
                sendNotifs(retries, danger);
                return;
            }
        }
      }
      else{
        Serial.println("NO CLIENT AT NOTIFS");
      }

    delay(3000); 

    while (client.available()) { 
      char c = client.read();
      Serial.print(c);
      client.flush();
    }
    Serial.println();
    client.stop(); 
  } else {
    Serial.println("Connection failed"); 
  }
}


void printDownloadAllData(EthernetClient client, int retries, String values){
  if (client2.connect(HOST_NAME, HTTP_PORT)) {
    Serial.println("DOWNLOADING");
    client2.println("GET /download.php" + values + " HTTP/1.1\r\nHost: " + String(HOST_NAME) + "\r\nConnection: close\r\n");
    Serial.println("VALUES = " + values);
    client2.println();

    if(client2.available()){
      String line = client2.readStringUntil('\n');
      Serial.println("FIRST LINE = " + line);
      if (!line.startsWith("HTTP/1.1 200 OK")) {
        Serial.println("Received 'HTTP/1.1 400 Bad Request' response, retrying request...");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
        retries--;
        if(retries == 0){
            Serial.println("BAD REQUEST AFTER 3 TRIES, EXITING LOOP.");
        }
        else{
            client2.stop();
            printDownloadAllData(client, retries, values);
            return;
        }
      }
      client.write(line.c_str());
    }
    else{
      Serial.println("NO CLIENT AT DOWNLOAD DATA");
    }


    // Read and write 64 bytes at a time
    byte buffer[64];
    int bytesRead;
    while (client2.connected() && (bytesRead = client2.readBytes(buffer, sizeof(buffer))) > 0) {
      client.write(buffer, bytesRead);
    }

    client2.stop();
    Serial.println("Connection closed");
    
    client.println(F("HTTP/1.1 302 Found"));
    client.println(F("Location: /"));
    client.println();
  }
  else {
    Serial.println("Connection failed");
  }
}









//functions below are helper functions for the web server
void printWTempJavascript(EthernetClient client){
  client.print(F("const data = ["));
    for (int i = 0; i < listSize; i++)
    {
      if (waterTempList[i] != 0)
      {
        client.print(String(waterTempList[i]) + ",");
      }
    }
  client.println(F("];"));
}
void printATempJavascript(EthernetClient client){
  client.print(F("const data2 = ["));
    for (int i = 0; i < listSize; i++)
    {
      if (airTempList[i] != 0)
      {
        client.print(String(airTempList[i]) + ",");
      }
    }
  client.println(F("];"));
}
void printPhJavascript(EthernetClient client){
  client.print(F("const data3 = ["));
    for (int i = 0; i < listSize; i++)
    {
      if (phList[i] != 0)
      {
        client.print(String(phList[i]) + ",");
      }
    }
  client.println(F("];"));
}
void printTimeJavascript(EthernetClient client){
  client.print(F("const time = ["));
    for (int i = 0; i < listSize; i++)
    {
      if (secList[i] != "x")
      {
        client.print("'" + secList[i] + "',");
      }
    }
  client.println(F("];"));
}


void printData()
{
  Serial.print("Air Temp: ");
  for (int x = 0; x < listSize; x++)
  {
    Serial.print(airTempList[x]);
    Serial.print(", ");
  }
  Serial.println("");
  Serial.print("Water Temp: ");
  for (int x = 0; x < listSize; x++)
  {
    Serial.print(waterTempList[x]);
    Serial.print(", ");
  }
  Serial.println("");
  Serial.print("pH LVL: ");
  for (int x = 0; x < listSize; x++)
  {
    Serial.print(phList[x]);
    Serial.print(", ");
  }
  Serial.println("");
  Serial.print("Time: ");
  for (int x = 0; x < listSize; x++)
  {
    Serial.print(secList[x]);
    Serial.print(", ");
  }
  Serial.println("");
} 



void saveSettings(String settings) {
  settings.replace("email=", "");
  settings.replace("%40", "@");
  settings.replace("&feedingTime=", ",");

  int commaIndex = settings.indexOf(',');
  email = settings.substring(0, commaIndex);
  feedingTime = settings.substring(commaIndex + 1);

  File file = SD.open("settings.txt", O_RDWR);
  if (file) {
    // Write the new settings
    file.println(settings);
    
    // Calculate the size of the old data
    int oldSize = file.size() - settings.length();
    
    // Fill the remaining space with whitespace characters
    for (int i = 0; i < oldSize; i++) {
      file.write(' ');
    }
    
    Serial.println("Settings saved successfully.");
  } else {
    Serial.println("Error opening file for writing.");
  }
  file.close();
}

void loadSettings() {
  File file = SD.open("settings.txt", FILE_READ);

  String settings = "";
  if (file) {
    while (file.available()) {
      settings += char(file.read());
    }
    Serial.println("Settings loaded successfully.");
  } else {
    Serial.println("Error opening file for reading.");
  }

  int commaIndex = settings.indexOf(',');
  email = settings.substring(0, commaIndex);
  feedingTime = settings.substring(commaIndex + 1);
  file.close();
}