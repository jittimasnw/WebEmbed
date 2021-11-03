#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <WiFiAP.h>
#include <EEPROM.h>

#define DEFAULT_AP_SSID "IPFAIR"
#define DEFAULT_AP_PASS "fAiRxRyX2907"

#define SPI_FLASH_SEC_SIZE 1024

String apSsid = DEFAULT_AP_SSID;
String apPass = DEFAULT_AP_PASS;
String myapSsid = "fair-esp32-ap";
String myapPass = "12345678";

String myId = "";

WebServer server(80);

#ifndef LED_BUILTIN
#define LED_BUILTIN 16
#else
#undef LED_BUILTIN
#define LED_BUILTIN 16
#endif

void handleRoot()
{
  digitalWrite(LED_BUILTIN, LOW);
  server.send(200, "text/plain", "hello from esp32!");
  digitalWrite(LED_BUILTIN, HIGH);
}

void handleNotFound()
{
  digitalWrite(LED_BUILTIN, LOW);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(LED_BUILTIN, HIGH);
}
void handleApSetup()
{
  digitalWrite(LED_BUILTIN, LOW);
  server.send(200, "text/html", "<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <title>Access Point Configuration </title> <style>body{font-family: 'Courier New', Courier, monospace;}.input-group{padding: 5px;}.button-group{padding: 5px;}button{font-family: inherit; padding: 5px; width: 100px; border: solid 1px gray; border-radius: 10px;}input{font-family: inherit; border: solid 1px gray; border-radius: 10px; width: 300px; padding: 5px;}</style></head><body align='center'> <div class='input-group'><label> SSID: </label><input id='ssid'></div><div class='input-group'><label> PASS: </label><input id='pass'></div><div class='button-group'> <button id='reloadButton'> Reload </button> <button id='submitButton'> Submit </button> </div><script>document.getElementById('reloadButton').onclick=function (){console.log('reload button is clicked...'); var xmlHttp=new XMLHttpRequest(); xmlHttp.onreadystatechange=function (){if (xmlHttp.readyState==XMLHttpRequest.DONE){if (xmlHttp.status==200){/* success */ var res=JSON.parse(xmlHttp.responseText); console.log(res); document.getElementById('ssid').value=res.ssid; document.getElementById('pass').value=res.pass;}else{/* fail */ alert('loading fail');}}}; xmlHttp.open('GET', '/ap'); xmlHttp.send();}; document.getElementById('submitButton').onclick=function (){console.log('submit button is clinked...'); var xmlHttp=new XMLHttpRequest(); xmlHttp.onreadystatechange=function (){if (xmlHttp.readyState==XMLHttpRequest.DONE){if (xmlHttp.status==200){/* success */ alert('success')}else{/* fail */ alert('fail');}}}; var data=JSON.stringify({ssid: document.getElementById('ssid').value, pass: document.getElementById('pass').value}); xmlHttp.open('POST', '/ap'); xmlHttp.send(data);}; document.getElementById('reloadButton').click(); </script></body></html>");
  digitalWrite(LED_BUILTIN, HIGH);
}

void handleApGet()
{
  digitalWrite(LED_BUILTIN, LOW);

  // {"ssid": "<ssid>", "pass": "<pass>"}
  String str = "";
  str += "{";
  str += "\"ssid\":\"" + apSsid + "\","; // pack ssid to json string
  str += "\"pass\":\"" + apPass + "\"";  // pack pass to json string
  str += "}";

  server.send(200, "text/json", str);

  digitalWrite(LED_BUILTIN, HIGH);
}

void handleApPost()
{
  // {ssid: <ssid>, pass: <pass>}, args is 1 at arg(0)

  digitalWrite(LED_BUILTIN, LOW);

  if (server.args() != 1)
  {
    server.send(400, "text/plain", "argument error");
  }
  else
  {
    String str = server.arg(0);
    StaticJsonDocument<100> doc;
    DeserializationError err = deserializeJson(doc, str);
    if (err)
    {
      server.send(500, "text/plain", "server error");
    }
    else
    {
      apSsid = doc["ssid"].as<String>();
      apPass = doc["pass"].as<String>();
      server.send(200, "text/plain", "Ok Success");

      if (_apSsid != apSsid || _apPass != apPass)
      {
        apSsid = _apSsid;
        apPass = _apPass;
        eepromWrite();
      }
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
}

void eepromWrite()
{
  char c;
  int addr = 0;
  unsigned char s, i;

  EEPROM.begin(SPI_FLASH_SEC_SIZE);

  c = '@';
  EEPROM.put(addr, c);
  addr++;
  c = '$';
  EEPROM.put(addr, c);
  addr++;

  s = (unsigned char)apSsid.length();
  EEPROM.put(addr, s);
  addr++;
  for (i = 0; i < s; i++)
  {
    c = apSsid[i];
    EEPROM.put(addr, c);
    addr++;
  }

  s = (unsigned char)apPass.length();
  EEPROM.put(addr, s);
  addr++;
  for (i = 0; i < s; i++)
  {
    c = apPass[i];
    EEPROM.put(addr, c);
    addr++;
  }

  EEPROM.end();
}

void eepromRead()
{

  char c;
  int addr = 0;
  unsigned char s, i;

  EEPROM.begin(SPI_FLASH_SEC_SIZE);

  char header[3] = {' ', ' ', '\0'};
  EEPROM.get(addr, header[0]);
  addr++;
  EEPROM.get(addr, header[1]);
  addr++;

  if (strcmp(header, "@$") != 0)
  {
    eepromWrite();
    return;
  }
  else
  {

    EEPROM.get(addr, s);
    addr++;
    apSsid = "";
    for (i = 0; i < s; i++)
    {
      EEPROM.get(addr, c);
      apSsid = apSsid + c;
      addr++;
    }

    EEPROM.get(addr, s);
    addr++;
    apPass = "";
    for (i = 0; i < s; i++)
    {
      EEPROM.get(addr, c);
      apPass = apPass + c;
      addr++;
    }
  }
}

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  eepromRead();

  WiFi.begin(apSsid.c_str(), apPass.c_str());
  Serial.println("Connect to " + apSsid + "");

  int counter = 0;
  do
  {
    delay(500);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(500);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.print(".");
    counter += 1;
  } while (WiFi.status() != WL_CONNECTED && counter < 10);
  Serial.println("");
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.disconnect();
    Serial.println("Fail");
  }
  else
  {
    Serial.println("Success..");
    Serial.println("IP address(STA mode): " + (WiFi.localIP()).toString());
    if (MDNS.begin("esp32"))
    {
      Serial.println("MDNS responder started");
    }
  }

  char temp[10];
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(temp, "%04X", (uint16_t)(chipid >> 32));
  myapSsid = myapSsid + "-[" + String(temp);
  sprintf(temp, "%08X", (uint32_t)chipid);
  myapSsid = myapSsid + String(temp) + "]";
  Serial.println("Ap ssid: " + myapSsid);

  WiFi.softAP(myapSsid.c_str(), myapPass.c_str());
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP address(AP Mode): ");
  Serial.println(myIP);

  server.on("/", handleRoot);

  server.on("/inline", []()
            { server.send(200, "text/plain", "this works as well"); });

  // front-end ---------------------------------------------------
  // display ap configuratio page
  server.on("/apSetup", handleApSetup);

  // webservices (back-end) --------------------------------------
  // return ssid, pass in json format
  server.on("/ap", HTTP_GET, handleApGet);

  // update ssid, pass from web browser
  server.on("/ap", HTTP_POST, handleApPost);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started");

  digitalWrite(LED_BUILTIN, HIGH);
}

void loop(void)
{
  server.handleClient();
  delay(2);
}
