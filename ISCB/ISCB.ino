
/**************************************************************
  This Program is use for control valve and pump
  Design by KoMoMi  22 JAN 2018 (LAST CHK)
  VERSION : 0.3.9
 **************************************************************/
//SCL as D1/GPIO5
//SDA as D2/GPIO4

// CONNECTIONS:
// DS3231 SDA --> SDA
// DS3231 SCL --> SCL
// DS3231 VCC --> 3.3v or 5v
// DS3231 GND --> GND

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Adafruit_MCP23017.h"
#include <RtcDS3231.h>
#include <WiFiManager.h>            //https://github.com/tzapu/WiFiManager
#include <ESP8266WebServer.h>
#include <EEPROM.h>

#define addrMode 0
#define insidePin 2 // Digital pin connected to DHTxx GPI02 (D4 of NodeMCU)
#define outsidePin 14 // Digital pin connected to DHTxx GPI14 (D5 of NodeMCU)
#define insideType DHT11 // DHT 11
#define outsideType DHT11 // DHT 11

DHT insideDHT(insidePin, insideType); // Initialize inside DHT sensor
DHT outsideDHT(outsidePin, outsideType); // Initialize outside DHT sensor



// Basic pin writing test for the MCP23017 I/O expander
// Connect pin #12 of the expander to D1(GPIO5) (i2c clock) of NodeMCU/ESP8266
// Connect pin #13 of the expander to D2(GPIO4) (i2c data) of NodeMCU/ESP8266
// Connect pins #15, 16 and 17 of the expander to ground (address selection)
// Connect pin #9 of the expander to 5V (power)
// Connect pin #10 of the expander to ground (common ground)
// Connect pin #18 through a ~10kohm resistor to 5V (reset pin, active low)

// Output #0 - #7 is on pin 21 - 28 (GPAx; x = 0 - 7)so connect an LED or whatever from that to ground
// Output #8 - #15 is on pin 0 - 8 (GPBx; x = 8 - 15)so connect an LED or whatever from that to ground
Adafruit_MCP23017 mcp;
int pin_z1a = 0; // IC pin 21 (GPA0)
int pin_z2a = 1; // IC pin 22 (GPA1)
int pin_z3a = 2; // IC pin 23 (GPA2)
int pin_z4a = 3; // IC pin 24 (GPA3)
int pin_z1b = 4; // IC pin 25 (GPA4)
int pin_z2b = 5; // IC pin 26 (GPA5)
int pin_z3b = 6; // IC pin 27 (GPA6)
int pin_z4b = 7; // IC pin 28 (GPA7)
int pin_pumpA = 8; // IC pin 1  (GPB0)
int pin_pumpB = 9; // IC pin 2 (GPB1)


//LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x3F, 16, 2);
RtcDS3231<TwoWire> Rtc(Wire);
ESP8266WebServer server(80); // establishing server at port 80 (HTTP protocol's default port)

int h_Time[] = {6, 9, 14, 15, 16, 20};
int k_time[] = {1, 1, 1, 1, 1, 1, 1, 1};

boolean  WiFiM = true; //Wifi Manager Status true/false
boolean autoMode = false ;
byte currValve = 0b11111111;
char xSSID[] = "KoMoMi_001";
char xPassword[] = "password";
//int _ip[] = {192,168,5,1};
IPAddress _ip = IPAddress(192, 168, 4, 1);
IPAddress _gw = IPAddress(10, 0, 1, 1);
IPAddress _sn = IPAddress(255, 255, 255, 0);

byte pumpA_status = 1;
byte pumpB_status = 1;
String xDate = "";
String xTime = "";

#define valveOff 0b11111111


struct MyObject {
  int intMode;
  int hTime[6] = {6, 9, 14, 15, 16, 20};
  int kTime[8] = {1, 1, 1, 1, 1, 1, 1, 1};
};

void setup()
{
  insideDHT.begin();
  outsideDHT.begin();
  Serial.begin(115200);
  Serial.println("===================");

  EEPROM.begin(512);
  //delay(10);
  //int intMode = EEPROM.read(addrMode);


  MyObject customVar;
  EEPROM.get( addrMode, customVar ); //****************Edited 26/1/2561****************
  //EEPROM.put( addrMode, customVar );
  
  //EEPROM.end(); //****************Edited 26/1/2561****************
  
  Serial.println("Mode : 0 = M, 1 = A -> ");
  Serial.println(customVar.intMode);

  if (customVar.intMode == 0)
    autoMode = false;
  else
    autoMode = true;

  for (int i = 0 ; i < sizeof(customVar.hTime) / sizeof(int) ; i++)
    h_Time[i] = customVar.hTime[i];

  for (int i = 0 ; i < sizeof(customVar.kTime) / sizeof(int) ; i++)
    k_time[i] = customVar.kTime[i];

  mcp.begin(); // use default address 0
  // Set mode GPIO
  mcp.pinMode(pin_z1a, OUTPUT);
  mcp.pinMode(pin_z2a, OUTPUT);
  mcp.pinMode(pin_z3a, OUTPUT);
  mcp.pinMode(pin_z4a, OUTPUT);
  mcp.pinMode(pin_z1b, OUTPUT);
  mcp.pinMode(pin_z2b, OUTPUT);
  mcp.pinMode(pin_z3b, OUTPUT);
  mcp.pinMode(pin_z4b, OUTPUT);
  mcp.pinMode(pin_pumpA, OUTPUT);
  mcp.pinMode(pin_pumpB, OUTPUT);

  mcp.digitalWrite(pin_z1a, HIGH);
  mcp.digitalWrite(pin_z2a, HIGH);
  mcp.digitalWrite(pin_z3a, HIGH);
  mcp.digitalWrite(pin_z4a, HIGH);
  mcp.digitalWrite(pin_z1b, HIGH);
  mcp.digitalWrite(pin_z2b, HIGH);
  mcp.digitalWrite(pin_z3b, HIGH);
  mcp.digitalWrite(pin_z4b, HIGH);
  mcp.digitalWrite(pin_pumpA, HIGH);
  mcp.digitalWrite(pin_pumpB, HIGH);


  Serial.println("\nWelcome to KoMoMi");
  Serial.println("Version 0.3.9b");
  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);



  //--------RTC SETUP ------------
  Rtc.Begin();


  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Serial.println(printDateTime(compiled));

  Serial.println();
  if (!Rtc.IsDateTimeValid())
  {
    // Common Cuases:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");

    // following line sets the RTC to the date & time this sketch was compiled
    // it will also reset the valid flag internally unless the Rtc device is
    // having an issue

    Rtc.SetDateTime(compiled);
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  lcd.init();                       // initialize the lcd
  lcd.backlight();                  // Enable LED backlight

  lcd.clear();
  lcd.print("=== KoMoMi ===");
  lcd.setCursor(0, 1);
  lcd.print("Version 0.3.9b");
  delay(2000);
  lcd.clear();
  lcd.print("Waiting...");

  if ( WiFiM ) {
    WiFiManagerF();
  } else
  {
    WiFi.begin("AirNet", "AirNetOldNet"); //Connect to the WiFi network
    while (WiFi.status() != WL_CONNECTED) { //Wait for connection
      delay(500);
      Serial.println("Waiting to connect…");
    }
  }

  server.on("/token", token);
  server.on("/getStatus", getStatus);
  server.on("/pump", pump);
  server.on("/automatic", autoService);
  server.on("/setDateTime", setDateTime);
  server.on("/setTable", setTable);
  server.on("/getTable", getTable);
  server.on("/getEnv", getEnv);

  server.begin();                                       //Start the server
  Serial.println("Server listening");
  lcd.clear();
  lcd.println("Server listening");
  delay(2000);
}

void autoService() {
  String message = "" ; //String( 100000000 + decimalToBinary(currValve)).substring(1, 10 );
  String st = server.arg(0);
  MyObject customVar;

  for (int i = 0 ; i < sizeof(h_Time) / sizeof(int) ; i++)
    customVar.hTime[i] = h_Time[i];

  for (int i = 0 ; i < sizeof(k_time) / sizeof(int) ; i++)
    customVar.kTime[i] = k_time[i];

  if (st == "1")  {
    autoMode = true;
    message = "1";
    customVar.intMode = 1;
  }
  else {
    autoMode = false;
    message = "0";
    customVar.intMode = 0;

  }

  EEPROM.put(addrMode, customVar);
  EEPROM.commit();
  message = (autoMode) ? "1" : "0";
  server.send(200, "text/plain", message );     //Response to the HTTP request

}

void setTable() {
  //http://192.168.4.1/setTable?time=(9,10,12,15,14,-1)&ktime=(1,1,0,1,0,1,1,1)&f
  //arg[0],Time = (6,9,14,15,16,22,)
  //arg[1],k_time = (3,3,3,3,2,3,4,5,)

  String str = server.arg(0);
  str.remove(0, 1);
  str.remove(str.length() - 1);
  Serial.println(str);
  String comma = ",";



  for (int i = 0; i < sizeof(h_Time) / sizeof(int); i++) {
    h_Time[i] = str.substring(0, str.indexOf(comma)).toInt();
    str.remove(0, str.indexOf(comma) + 1);
    Serial.print(String(h_Time[i]) + ", ");
    //t_htime[i] = str.toInt();

  }
  Serial.println();
  str = server.arg(1);
  str.remove(0, 1);
  str.remove(str.length() - 1);
  Serial.println(str);

  for (int i = 0; i < sizeof(k_time) / sizeof(int); i++) {
    k_time[i] = str.substring(0, str.indexOf(comma)).toInt();
    str.remove(0, str.indexOf(comma) + 1);
    Serial.print(String(k_time[i]) + ", ");
    //t_htime[i] = str.toInt();

  }
  Serial.println();
  String message = "set table ok";
  server.send(200, "text/plain", message );     //Response to the HTTP request

}

void getTable() {

  //  int h_Time[] ={6,9,14,15,16,22};
  //  int k_time[] = {3,3,3,3,2,3,4,5};

  String message = "h_Time = (";

  for (int i = 0; i < sizeof(h_Time) / sizeof(int); i++) {
    message += String(h_Time[i]) + ",";
  }

  message += ")";

  message += ":k_time = (";
  for (int i = 0; i < sizeof(k_time) / sizeof(int); i++) {
    message += String(k_time[i]) + ",";
  }
  message += ")\n";

  server.send(200, "text/plain", message );     //Response to the HTTP request

}


void getStatus() {
  String    message = String( 100000000 + decimalToBinary(currValve)).substring(1, 10 );
  pumpA_status = mcp.digitalRead(pin_pumpA);
  pumpB_status = mcp.digitalRead(pin_pumpB);
  message =   message + pumpA_status + pumpB_status ;
  server.send(200, "text/plain", message );     //Response to the HTTP request

}


void pump() {

  if (not autoMode) {

    String message = "";
    String str = server.arg(0);
    if (str == "1")  {
      pumpA_status = pumpA_status ^ 1;
      //message = "pump A on";
      if ( pumpA_status == 1 ) {
        mcp.digitalWrite(pin_pumpA, HIGH);
        lcd.setCursor(9, 1);
        lcd.print("#P" + str + " OFF");
      }
      else {
        mcp.digitalWrite(pin_pumpA, LOW);
        lcd.setCursor(9, 1);
        lcd.print("#P" + str + " ON ");
      }
    }
    if (str == "2")  {
      pumpB_status = pumpB_status ^ 1;
      //message = "pump B on";
      if ( pumpB_status == 1 ) {
        mcp.digitalWrite(pin_pumpB, HIGH);
        lcd.setCursor(9, 1);
        lcd.print("#P" + str + " OFF");
      }
      else {
        mcp.digitalWrite(pin_pumpB, LOW);
        lcd.setCursor(9, 1);
        lcd.print("#P" + str + " ON ");
      }
    }
    getStatus();
  } else {
    //Error Auto mode
    //return error
    String   message =   "Auto mode Enable Pls. Stop. "  ;
    server.send(201, "text/plain", message );     //Response to the HTTP request

  }
}


void token() { //Handler
  //String record[6] = {"z","h","m","s","i","dow"};
  if (not autoMode) {
    String record[6];
    String message = "Number of args received:";
    message += server.args();            //Get number of parameters
    message += "\n";                     //Add a new line

    for (int i = 0; i < server.args(); i++) {
      message += "Arg" + (String)i + " –> ";   //Include the current iteration value
      message += server.argName(i) + ": ";     //Get the name of the parameter
      message += server.arg(i) + "\n";              //Get the value of the parameter
    }

    Serial.println(server.arg(0));

    String str = server.arg(0);
    str.remove(0, 1);
    str.remove(str.length() - 1);
    Serial.println(str);
    String comma = ",";

    for (int i = 0; i < 6; i++) {
      record[i] = str.substring(0, str.indexOf(comma));
      str.remove(0, str.indexOf(comma) + 1);
      if (i == 5) {
        record[i] = str;
      }

    }
    check(record);
    getStatus();
  } else {

    //return error
    String   message =   "Auto mode Enable Pls. Stop. "  ;
    server.send(201, "text/plain", message );     //Response to the HTTP request

  }
}

void sendData(byte data) {
  String st = String(100000000 + decimalToBinary(data));
  int s = 8;
  int e = 9;
  for (int z = 0; z < 8; z++) {
    if (st.substring(s, e ) == "0") {
      mcp.digitalWrite(z, LOW);
    } else
      mcp.digitalWrite(z, HIGH);
    s--;
    e--;
  }
}



void setValve(int x, int stat) {
  sendData(currValve);
  String st = String(100000000 + decimalToBinary(currValve));
  Serial.println(st);
  Serial.println(st.substring(9 - x, 9 - x + 1));
  if (st.substring(9 - x, 9 - x + 1) == "0") {
    Serial.println("Zone " + String(x) + "A Selected");
    lcd.setCursor(9, 1);
    lcd.print("#Z" + String(x) + " ON ");
  } else {
    Serial.println("Zone " + String(x) + "A Unselected");
    lcd.setCursor(9, 1);
    lcd.print("#Z" + String(x) + " OFF ");
  }
}

void check(String zone[]) {
  lcd.setCursor(0, 1);
  int x = zone[0].toInt();
  switch (x)
  {
    case  1:
      currValve = currValve ^ 0b00000001;
      setValve(1, zone[3].toInt() );
      break;
    case 2:
      currValve = currValve ^ 0b00000010;
      setValve(2, zone[3].toInt() );
      break;
    case  3:
      currValve = currValve ^ 0b00000100;
      setValve(3, zone[3].toInt() );
      break;
    case  4:
      currValve = currValve ^ 0b00001000;
      setValve(4, zone[3].toInt() );
      break;
    case  5:
      currValve = currValve ^ 0b00010000;
      setValve(5, zone[3].toInt() );
      break;
    case 6:
      currValve = currValve ^ 0b00100000;
      setValve(6, zone[3].toInt() );
      break;
    case  7:
      currValve = currValve ^ 0b01000000;
      setValve(7, zone[3].toInt() );
      break;
    case  8:
      currValve = currValve ^ 0b10000000;
      setValve(8, zone[3].toInt() );
      break;
    default:
      Serial.println("Unknown item selected");
  }
  delay(100);

}
/*------------------------ Main Program ------------------------------*/
void loop()
{
  if ( not autoMode) {
    server.handleClient();    //Handling of incoming requests
    dispDateTime();
    lcd.setCursor(12, 0);
    lcd.print("Man");
    //lcd.print("-M-");
    //delay(2000);
    //readInsideDHT();

    //delay(100);
    //readOutsideDHT();

    delay(100); // one hundred milliseconds
  } else {
    automatic();
  }
  delay(10);  // ten milliseconds
}

/*------------------------ End Program ------------------------------*/

boolean chk_work() {
  RtcDateTime now = Rtc.GetDateTime();
  int HH = now.Hour();
  boolean state = false;
  //int hsize = sizeof(hh)/sizeof(int);
  for (int i = 0; i < sizeof(h_Time) / sizeof(int); i++) {
    if (HH == h_Time[i]) {
      Serial.print(String(h_Time[i]));
      state = true;
    }
  }
  if (state)
    return true;
  else return false;
  //return   (HH == h_Time[0] || HH == h_Time[1] || HH == h_Time[2] || HH == h_Time[3]) ? true : false;
}

void automatic() {
  Serial.println("***  Auto Starting  *****");
  clossAll();
  Serial.println("--auto--");
  lcd.setCursor(12, 0);
  lcd.print("Auto   ");
  String record[8];
  while (autoMode) //do if still auto mode
  {
    lcd.setCursor(12, 0);
    lcd.print("Auto   ");
    if (chk_work())  //if work hour is valid
    {
      bool stop = false;
      int k = 0;
      for (int i = 0; (i < 8) && !stop; i++) {
        Serial.print(" i is : ");
        Serial.print(i);
        Serial.print(" value is : ");
        Serial.print(k_time[i]);
        Serial.println("===========================");
      }
      for (int i = 0; (i < 8) && !stop; i++) 
      {        
        Serial.print("Zone No.");
        Serial.print(i);

        int k = k_time[i]; // K is time uses
        record[0] = i+1;
        Serial.print("\t Time operate is ");
        Serial.print(k);
        Serial.println(" minute");
        if (k > 0) //if time operate is set more than 0 do this
        {
          //record[0] = i; old program
          check(record);
          //control bump
          if (i < 4) 
          {
            mcp.digitalWrite(pin_pumpA, LOW);
            mcp.digitalWrite(pin_pumpB, HIGH);
          } else {
            mcp.digitalWrite(pin_pumpA, HIGH);
            mcp.digitalWrite(pin_pumpB, LOW);
          }
          for (int j = 0; j <= k * 60 ; j++) // k*60
          { 
            if (not autoMode) break; //break if manual is selected
            delay(100);
            server.handleClient();
            dispDateTime() ;
            Serial.println("--auto--");
            lcd.setCursor(12, 0);
            lcd.print("Auto   ");
            if (not autoMode ) break;
            if (not chk_work()) break;
          }
        } else {
          delay(200); 
          if ( i = 8 ) {
            Serial.println("End of 8 zone");
            stop = true; 
          }
        }
        
        Serial.print("The end of zone ");
        Serial.println(i); 
        if (not autoMode ) break;
        if (not chk_work()) break;
        check(record);
      }    
      //break; //break 8 loop
      server.handleClient();    //Handling of incoming requests
    } else
          {
            clossAll();
            if (not autoMode) break;
            delay(500);
            dispDateTime();
            lcd.setCursor(12, 0);
            lcd.print("Auto   ");
            server.handleClient();
          }
  }
  clossAll();
}

void clossAll() {
  Serial.println("Close All ...");
  mcp.digitalWrite(pin_pumpA, HIGH);
  mcp.digitalWrite(pin_pumpB, HIGH);
  currValve = valveOff;
  sendData(currValve);
}


void Nothing() {
}

void dispDateTime() {
  if (!Rtc.IsDateTimeValid())
  {
    // Common Cuases:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("RTC lost !!");
    delay(1000);
  }

  RtcDateTime now = Rtc.GetDateTime();
  // Serial.println(printDateTime(now));
  //Serial.print(now.Minute());
  char str[20];
  //lcd.clear();

  sprintf(str, "%02u/%02u/%04u", now.Day(), now.Month(), now.Year());
  xDate = str;
  lcd.setCursor(0, 0);
  lcd.print(xDate + "          ");

  sprintf(str, "%02u:%02u:%02u", now.Hour(), now.Minute(), now.Second());
  xTime = str;
  lcd.setCursor(0, 1);
  lcd.print(xTime);


};


void setDateTime() {
  // RtcDateTime currentTime = RtcDateTime(YEAR,MONTH, DAY, HOUR[0-23],MINUTE[0-59],SECON[0-59]);
  String _date = server.arg(0);
  String _time = server.arg(1);
  int iYear = _date.substring(6, 10).toInt();
  int iMM = _date.substring(3, 5).toInt();
  int iDD = _date.substring(0, 2).toInt();
  int _HH = _time.substring(0, 2).toInt();
  int _MM = _time.substring(3, 5).toInt();


  RtcDateTime currentTime = RtcDateTime(iYear, iMM, iDD, _HH, _MM, 00);
  Rtc.SetDateTime(currentTime);

  lcd.print("Upd Time OK!!");
  Serial.print("Upd Time OK!!");

  dispDateTime() ;
  String   message =   "time ok.. " + _date + "  " + _time ;
  server.send(200, "text/plain", message );     //Response to the HTTP request
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

String printDateTime(const RtcDateTime& dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  //Serial.print(datestring);
  return (datestring);
}


void WiFiManagerF() {
  //----------------------------------------------------------------------------------------------------------------------------------------
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //reset saved settings
  wifiManager.resetSettings();

  //set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));


  //wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  //wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
  wifiManager.setAPStaticIPConfig(_ip, _gw, _sn);

  lcd.setCursor(0, 0);
  lcd.print("Waiting for WiFi");
  lcd.setCursor(0, 1);
  //lcd.print("IP:192.168.4.1");
  lcd.print(_ip);
  Serial.println("IP: 192.168.4.1");
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds

  wifiManager.setTimeout(5);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  //wifiManager.autoConnect("AutoConnectAP");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  //tries to connect to last known settings
  //if it does not connect it starts an access point with the specified name
  //here  "ISCB" with password "password"
  //and goes into a blocking loop awaiting configuration
  if (wifiManager.autoConnect(xSSID, xPassword)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(WiFi.SSID());
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    Serial.println("WiFi Connected");
    Serial.println(WiFi.SSID());
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    delay(1000);
  }
}


/* Function to convert a decinal number to binary number */
long decimalToBinary(long n) {
  int remainder;
  long binary = 0, i = 1;

  while (n != 0) {
    remainder = n % 2;
    n = n / 2;
    binary = binary + (remainder * i);
    i = i * 10;
  }
  return binary;
}

/* Function to convert a binary number to decimal number */
long binaryToDecimal(long n) {
  int remainder;
  long decimal = 0, i = 0;
  while (n != 0) {
    remainder = n % 10;
    n = n / 10;
    decimal = decimal + (remainder * pow(2, i));
    ++i;
  }
  return decimal;
}

byte decToBcd(byte val) {   //Convert DEC to BCD
  return ((val / 10 * 16) + (val % 10));
}

byte bcdToDec(byte val) {   //Convert BCD to DEC
  return ((val / 16 * 10) + (val % 16));
}


/* Function to read data from DHT sensor */
float *readInsideDHT() {
  float hi = insideDHT.readHumidity(); // Read humidity
  float ti = insideDHT.readTemperature(); // Read temperature as Celsius (the default)
  if (isnan(hi) || isnan(ti)) { // Check if any reads failed and exit early (to try again).
    Serial.println("Failed to read from inside DHT sensor!");
  }
  Serial.print("Humidity inside: ");
  Serial.print(hi);
  Serial.print(" %\t");
  Serial.print("Temperature inside: ");
  Serial.print(ti);
  Serial.println(" *C ");

  /*    lcd.clear();
      lcd.print("Humi in-> ");
      lcd.print(hi);
      lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("Temp in-> ");
      lcd.print(ti);
      lcd.print("*C");
      delay(2000);
      lcd.clear();*/
}

void readOutsideDHT() {
  float ho = outsideDHT.readHumidity(); // Read humidity
  float to = outsideDHT.readTemperature(); // Read temperature as Celsius (the default)
  if (isnan(ho) || isnan(to)) { // Check if any reads failed and exit early (to try again).
    Serial.println("Failed to read from outside DHT sensor!");
    return;
  }
  Serial.print("Humidity outside: ");
  Serial.print(ho);
  Serial.print(" %\t");
  Serial.print("Temperature outside: ");
  Serial.print(to);
  Serial.println(" *C ");
  /*    lcd.clear();
      lcd.print("Humi out-> ");
      lcd.print(ho);
      lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("Temp out-> ");
      lcd.print(to);
      lcd.print("*C");
      delay(2000);
      lcd.clear();*/
}


void getEnv() {
  float hi = insideDHT.readHumidity(); // Read humidity
  float ti = insideDHT.readTemperature(); // Read temperature as Celsius (the default)
  if (isnan(hi) || isnan(ti)) { // Check if any reads failed and exit early (to try again).
    Serial.println("Failed to read from inside DHT sensor!");
  }

  
  float ho = outsideDHT.readHumidity(); // Read humidity
  float to = outsideDHT.readTemperature(); // Read temperature as Celsius (the default)
  if (isnan(ho) || isnan(to)) { // Check if any reads failed and exit early (to try again).
    Serial.println("Failed to read from outside DHT sensor!");
    return;
  }
  
  String   message =   "IS = ( " +String(ti)+"ํ C " + String(hi) + "% )";
  message += ":\ OS = ( " +String(to)+"ํ C " + String(ho) + "% )";
  

  server.send(200, "text/plain", message );     //Response to the HTTP request
}
