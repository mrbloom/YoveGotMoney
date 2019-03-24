#include <M5Stack.h>
#include <Avatar.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include "WebServer.h"
#include <HTTPClient.h>
#include <Preferences.h>
#include <JsonArray.h>
#include <JsonHashTable.h>
#include <JsonObjectBase.h>
#include <JsonParser.h>

using namespace m5avatar;

Avatar avatar;

const bool DBGR=false;
char* balance;

const IPAddress apIP(192, 168, 1, 196);
const char* apSSID = "Magnitik";
boolean settingMode;
String ssidList;
String wifi_ssid;
String wifi_password;

// DNSServer dnsServer;
WebServer webServer(80);

// wifi config store
Preferences preferences;


void setup() {
  m5.begin();
  if(!DBGR){
    avatar.init();  
  }
  
  preferences.begin("wifi-config");

  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      settingMode = false;
      startWebServer();
      return;
    }
  }
  settingMode = true;
  setupMode();  
}

void loop() {
  if(!DBGR){
    if (M5.BtnA.wasPressed())
    {
      avatar.setSpeechText(balance);
      avatar.setMouthOpenRatio(0.7);
      delay(200);
      avatar.setMouthOpenRatio(0);
    }  
  }
  
  if (settingMode) {
  }
  webServer.handleClient();
  M5.update();
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}


boolean restoreConfig() {
  wifi_ssid = preferences.getString("WIFI_SSID");
  wifi_password = preferences.getString("WIFI_PASSWD");
  print("WIFI-SSID: ");
  println(wifi_ssid);
  print("WIFI-PASSWD: ");
  println(wifi_password);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

  if (wifi_ssid.length() > 0) {
    return true;
  } else {
    return false;
  }
}

boolean checkConnection() {
  int count = 0;
  print("Waiting for Wi-Fi connection");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      
      println("Connected!");
      return (true);
    }
    delay(500);
    print(".");
    count++;
  }
  println("Timed out.");
  return false;
}

String payload(String api_url){
  try
    {
      HTTPClient http;
      http.begin(api_url);
      int httpCode = http.GET();                                        //Make the request

      if (httpCode > 0) { //Check for the returning code
        println("PAYLOAD");
        String payload = http.getString();
        return payload;
      }
      else {
        print("Error on HTTP request");
      }

      http.end(); //Free the resources
    }
    catch (...)
    {
      print("Get Data Error");
    }
}

void print(String str){
  if(DBGR){
    M5.Lcd.print(str);
  }
}

void println(String str){
  if(DBGR){
    M5.Lcd.println(str);
  }
}

char* jsonParse(String& json,char* field){
    JsonParser<32> parser;
    char str[2000];
    json.toCharArray(str, 2000);    
    JsonHashTable hashTable = parser.parseHashTable(str);    
    if (!hashTable.success())    {
        return NULL;
    }
    else
    {
      return hashTable.getString(field);
    }
}

void startWebServer() {
  if (settingMode) {
    print("Starting Web Server at ");
    println(IpAddress2String(WiFi.softAPIP()));
    webServer.on("/settings", []() {
      String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += ssidList;
      s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    webServer.on("/setap", []() {
      String ssid = urlDecode(webServer.arg("ssid"));
      print("SSID: ");
      println(ssid);
      String pass = urlDecode(webServer.arg("pass"));
      print("Password: ");
      println(pass);
      println("Writing SSID to EEPROM...");

      // Store wifi config
      println("Writing Password to nvr...");
      preferences.putString("WIFI_SSID", ssid);
      preferences.putString("WIFI_PASSWD", pass);

      println("Write nvr done!");
      String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
      s += ssid;
      s += "\" after the restart.";
      webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
    webServer.onNotFound([]() {
      String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      webServer.send(200, "text/html", makePage("AP mode", s));
    });
  }
  else {
    print("Starting Web Server at ");
    println(IpAddress2String(WiFi.localIP()));

    String api_url = "https://api.etherscan.io/api?module=account&action=balance&address=0xddbd2b932c763ba5b1b7ae3b362eac3e8d40121a&tag=latest&apikey=NQCE1PBYQ4G3BVUENV2A1H8KVQ1AC81Z5U";
    String json = payload(api_url);    
    balance = jsonParse(json,"result");
    //String ether_str = String(balance);
    //String bal = ether_str.substring(0,6)+"."+ether_str.substring(6,8);
    //println("bal");
    
    //char *str="1234567890";
    //String(bal.toFloat()).toCharArray(str, 10);
    //balance = str;

    println(json);
    

    String api_url_btc = "https://api.blockcypher.com/v1/btc/main/addrs/1DEP8i3QJCsomS4BSMY2RpU1upv62aGvhD/balance";
    String payload_str_btc = payload(api_url_btc); 
    println(payload_str_btc);

    webServer.on("/", []() {
      String s = "<h1>WIFI Settings</h1><p><a href=\"/reset\">Reset Wifi</a></p>";
      webServer.send(200, "text/html", makePage("Wifii Server", s));
    });
    webServer.on("/reset", []() {
      // reset the wifi config
      preferences.remove("WIFI_SSID");
      preferences.remove("WIFI_PASSWD");
      String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
      webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      delay(3000);
      ESP.restart();
    });
  }
  webServer.begin();
}

void setupMode() {
  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  println("");
  for (int i = 0; i < n; ++i) {
    ssidList += "<option value=\"";
    ssidList += WiFi.SSID(i);
    ssidList += "\">";
    ssidList += WiFi.SSID(i);
    ssidList += "</option>";
  }
  delay(100);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  WiFi.mode(WIFI_MODE_AP);
  // WiFi.softAPConfig(IPAddress local_ip, IPAddress gateway, IPAddress subnet);
  // WiFi.softAP(const char* ssid, const char* passphrase = NULL, int channel = 1, int ssid_hidden = 0);
  // dnsServer.start(53, "*", apIP);
  startWebServer();
  print("Starting Access Point at \"");
  print(apSSID);
  println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<meta charset=\"UTF-8\">";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
