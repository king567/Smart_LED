#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

//const char *ssid     = "swhh-lab";
//const char *password = "Zz-23317713";

const int ledPin = 5; //LED pin
const int analogInPin = 0; //Lux Senser

int brightness = 0; //init light

float lux = 0;
float pr = 0;
int Lumen = 0;
int LED_Brightness = 0;
unsigned int target_lux = 50; //default 50 lux

unsigned long LED_Change_Speed = 300; //LED偵測並改變亮度的速度 毫秒為單位

unsigned long target_hour = 0;
unsigned long target_minute = 0;
unsigned long target_Duration = 0;
unsigned long target_Standby = 0;
unsigned long target_loop = 0;

bool checkMessage = false; //檢查是否有修改過排程
bool ManualLED_mode = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.google.com", 28800, 60); //google ntp server time.google.com and Taiwan TimeZone = 28800

ESP8266WebServer server(80); //Server Port 80

void setup(){
  Serial.begin(115200);
  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  wifiManager.autoConnect("SmallLED", "Zz-23317713");

  analogWrite(ledPin, 50);
  delay(300);
  analogWrite(ledPin, 0);
  Serial.println("Connect Wifi Success");
  Serial.println("Singal of wifi Success");
  timeClient.begin();

  if (timeClient.update()) //Get time data from nt time.google.com
  {
    Serial.println("update time Success");
    analogWrite(ledPin, 50);
    delay(300);
    analogWrite(ledPin, 0);
    Serial.println("Singal of update time Success");
  }
  // Print the IP address
  Serial.println(WiFi.localIP());

  server.on("/", LightAPI);
  server.on("/API",LightAPI);
  server.on("/Status",Status);
  server.on("/TargetLUX",WebLux);
  server.on("/LEDMode",ManualLED);
  server.on("/Schedule", handleGenericArgs); //Associate the handler function to the path
  
  server.begin();
  Serial.println("Server started");
}

void loop() {
  
  Serial.println(timeClient.getFormattedTime());
  server.handleClient();
  
  pr = analogRead(analogInPin); //讀取光敏電阻訊號
  lux = CovertLux(pr); //轉換光敏電阻訊號為lux
  LED_Brightness = Brightness(pr,lux); //設定電燈亮度值
  Lumen = map(LED_Brightness, 0, 255, 0, 100); //轉換單位為%

  if (ManualLED_mode == false)
  {
    checkMessage = false; //reset message
    checkMessage = Check6AMto7PM(); //check is 6am or 9pm if true set message to true
    schedule();
  }
  else
  {
    analogWrite(ledPin,LED_Brightness); //開啟電燈並偵測
    Serial.print("pr:");
    Serial.println(pr);
    Serial.print("LUX:");
    Serial.println(lux);
    Serial.print("Lumen:");
    Serial.print(Lumen);
    Serial.println("%");
  }

  delay(1000);
}

void LightAPI(){

  String Data_Packet = F("{\"LUX\":");
  Data_Packet += String(lux);
  Data_Packet += F(",");
  Data_Packet += F("\"Lightness\":");
  Data_Packet += String(Lumen);
  Data_Packet += F("}");

  server.send(200,"application/json",Data_Packet);
}

void WebLux(){
  String message = "";
  for (int i = 0; i < server.args(); i++)
  {
    target_lux = server.arg(i).toInt();
    message += "modify target lux to ";
    message += server.arg(i);
  }
  server.send(200,"text/plain",message);
}

void handleGenericArgs() { //Handler

  String message = "Number of args received:";
  message += server.args();            //Get number of parameters
  message += "\n";                            //Add a new line

  for (int i = 0; i < server.args(); i++)
  {
    message += "GET Arg" + (String)i + " –> "; //Include the current iteration value
    message += server.argName(i) + ": ";       //Get the name of the parameter
    message += server.arg(i) + "\n";           //Get the value of the parameter
  }

  for (int i = 0; i < server.args(); i++)
  {
    if (server.argName(i) == "hour")
    {
      target_hour = server.arg(i).toInt();
    }
    else if (server.argName(i) == "minute")
    {
      target_minute = server.arg(i).toInt();
    }
    else if (server.argName(i) == "duration")
    {
      target_Duration = server.arg(i).toInt();
    }
    else if (server.argName(i) == "standby")
    {
      target_Standby = server.arg(i).toInt();
    }
    else if (server.argName(i) == "loop")
    {
      target_loop = server.arg(i).toInt();
    }
    checkMessage = true;
    Serial.println("update Schedule Success");
  }

  server.send(200, "text/plain", message);       //Response to the HTTP request
  checkMessage = true;
  Serial.println(target_hour);
  Serial.println(target_minute);
  Serial.println(target_Duration);
  Serial.println(target_Standby);
  Serial.println(target_loop);

}


float CovertLux(float pr){
  float lux;
  float rate;
  if(pr < 20){rate = 1;}
  else if(pr >= 20 && pr < 30){rate = 2.5;}
  else if(pr >= 30 && pr < 40){rate = 1.6;}  
  else if(pr >= 40 && pr < 50){rate = 1.42;}
  else if(pr >= 50 && pr < 60){rate = 1.47;}
  else if(pr >= 60 && pr < 70){rate = 1.395;}
  else if(pr >= 70 && pr < 100){rate = 1.25;}
  else if(pr >= 100 && pr < 150){rate = 1.351;}
  else if(pr >= 150 && pr < 200){rate = 1.53;}
  else if(pr >= 200 && pr < 250){rate = 1.78;}
  else if(pr >= 250 && pr < 300){rate = 1.96;}
  else if(pr >= 300 && pr < 350){rate = 2.14;}
  else if(pr >= 350 && pr < 400){rate = 2.33;}
  else if(pr >= 400 && pr < 450){rate = 1.403;}
  else if(pr >= 450 && pr < 500){rate = 2.393;}
  else if(pr >= 500){rate = 2.439;}
  else{rate = 1.7;}
  lux = pr*rate;
  return lux;
}

int Brightness(float pr,float lux){
    if (lux>= target_lux - 10 && lux<=target_lux + 10){}
    else{
      pr = analogRead(analogInPin); //讀取光敏電阻訊號
      lux = CovertLux(pr); //轉換光敏電阻訊號為lux
        if(lux < target_lux){
            /* for big led
              if(brightness >= 170){brightness = 170}
              else{brightness = brightness+10;}
            */
            if(brightness >= 255){brightness = 255;} //避免數值溢出255
            else{brightness = brightness+10;}
         }
        else{
          if(brightness > 0){brightness = brightness-10;}
          else if (brightness < 0){brightness = 0;} //避免數值溢出0
         }
    }
    return brightness;
}

bool Check6AMto7PM(){
  bool status = true;
  //6 AM ~ 7 PM
  if (timeClient.getHours() >= 6 && timeClient.getHours() <= 19)
  {
    status = false;
  }
  return status;
}

time_t local_hour(){
  return timeClient.getHours();
}
time_t local_minute(){
  return timeClient.getMinutes();
}
time_t local_second(){
  return timeClient.getSeconds();
}


void schedule(){
  if (timeClient.getHours() == target_hour && timeClient.getMinutes() == target_minute && checkMessage == false)
  {
    int Timer = 0; //計算時長
    unsigned long target_time = target_hour*3600+target_minute*60;
    unsigned long local_time = local_hour()*3600+local_minute()*60+local_second();
    Serial.println("start schedule");
    for (size_t loop_time = 0; loop_time < target_loop && checkMessage == false; loop_time++)
    {
      Serial.print("start loop = ");
      Serial.println(loop_time);
      Serial.println(local_time);
      Serial.println(target_time + target_Duration + Timer);

      while (local_time < target_time + target_Duration + Timer && checkMessage == false){
        local_time = local_hour() * 3600 + local_minute() * 60 + local_second();
        Serial.println("open led");
        Serial.print("Local time (s) = ");
        Serial.println(local_time);
        Serial.print("target_time + Duration=");
        Serial.println(target_time + target_Duration);
        Serial.print("Time left = ");
        Serial.println(target_time + target_Duration - local_time + Timer);
        Serial.print("Timer = ");
        Serial.println(Timer);

        pr = analogRead(analogInPin); //讀取光敏電阻訊號
        lux = CovertLux(pr); //轉換光敏電阻訊號為lux
        LED_Brightness = Brightness(pr,lux); //設定電燈亮度值
        Lumen = map(LED_Brightness, 0, 255, 0, 100); //轉換單位為%
        analogWrite(ledPin,LED_Brightness); //開啟電燈並偵測

        Serial.print("pr:");
        Serial.println(pr);
        Serial.print("LUX:");
        Serial.println(lux);
        Serial.print("Lumen:");
        Serial.print(Lumen);
        Serial.println("%");

        server.handleClient(); //proccess network request
        delay(LED_Change_Speed); //改變電燈亮度的速度
      }
      Timer = Timer + target_Duration; //important
      while (local_time < target_time + target_Standby + Timer && checkMessage == false){
      local_time = local_hour() * 3600 + local_minute() * 60 + local_second();
      Serial.println("close led");
      Serial.print("Local time (s) = ");
      Serial.println(local_time);
      Serial.print("target_time + target_Standby = ");
      Serial.println(target_time + target_Standby + Timer);
      Serial.print("Time left = ");
      Serial.println(target_time + target_Standby - local_time + Timer);

      Serial.print("Timer = ");
      Serial.println(Timer);
      pr = analogRead(analogInPin); //讀取光敏電阻訊號
      lux = CovertLux(pr); //轉換光敏電阻訊號為lux
      Lumen = 0;
      analogWrite(ledPin, 0); //關閉電燈

      Serial.print("pr:");
      Serial.println(pr);
      Serial.print("LUX:");
      Serial.println(lux);
      Serial.print("Lumen:");
      Serial.print(Lumen);
      Serial.println("%");

      server.handleClient(); //proccess network request
      delay(LED_Change_Speed); //改變電燈亮度的速度
    }
      Timer = Timer + target_Standby; //important
    }

  }
  
}

void ManualLED(){
  String message = "";
  for (int i = 0; i < server.args(); i++)
  {
    if (server.arg(i).toInt() > 0)
    {
      message += "Manual Open led Success ";
      server.send(200,"text/plain",message);
      ManualLED_mode = true;
    }
    else
    {
     message += "close led and starting plan";
     server.send(200,"text/plain",message);
     ManualLED_mode = false;
    }
    
/*     if (server.arg(i).toInt() <= 0 && server.arg(i).toInt() >=255)
    {
      int Manual_brightness = 0;
      Manual_brightness = server.arg(i).toInt();
      message += "modify LED Brightness to ";
      message += server.arg(i);
      server.send(200,"text/plain",message);
      analogWrite(ledPin,Manual_brightness);
      ManualLED_mode = true;
      checkMessage = true;
    }
    else
    {
      message += "please input number range 0-255";
      server.send(200,"text/plain",message);
    } */
  }
}

void Status(){
  //Get all parameter status
    String message = F("{\"TargetLUX\":");
    message += String(target_lux);
    message += F(",");
    message += F("\"LEDMode\":");
    message += String(ManualLED_mode);
    message += F(",");
    message += F("\"hour\":");
    message += String(target_hour);
    message += F(",");
    message += F("\"minute\":");
    message += String(target_minute);
    message += F(",");
    message += F("\"Duration\":");
    message += String(target_Duration);
    message += F(",");
    message += F("\"Standby\":");
    message += String(target_Standby);
    message += F(",");
    message += F("\"loop\":");
    message += String(target_loop);
    message += F("}");
    server.send(200,"application/json",message);
}