#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>

Preferences preferences;

// WiFi config
struct WiFiConfig {
  const char* ssid;
  const char* password;
  IPAddress local_ip;
  IPAddress gateway;
  IPAddress subnet;
  const char* osc_passcode;
};

WiFiConfig knownNetworks[] = {
  //          SSID             PASSWORD         LOCAL IP            GATEWAY            SUBNET                       OSC PASSCODE
  {"SSID 1", "Password SSID 1", IPAddress(fixed IP for device), IPAddress(x, x, x, x), IPAddress(255, 255, 255, 0), "xxxx"},
  {"SSID 2", "Password SSID 2", IPAddress(fixed IP for device), IPAddress(x, x, x, x), IPAddress(255, 255, 255, 0), "xxxx"}
};

String currentSsid = "";
String currentPasscode = "";

String targetIP;
int targetPort;
String oscAddress1;
String oscAddress2;

const int SWITCH1_PIN = 0;
const int SWITCH2_PIN = 1;
bool lastSwitch1State = true;
bool lastSwitch2State = true;

WiFiUDP Udp;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void setup() {
  Serial.begin(115200);
  pinMode(SWITCH1_PIN, INPUT_PULLUP);
  pinMode(SWITCH2_PIN, INPUT_PULLUP);

  preferences.begin("osc", false);
  loadPreferences();

  connectToWiFi();
  setupWebServer();
  webSocket.begin();
  Udp.begin(8888);

  webSocket.broadcastTXT("{\"sw1\":true}");
  webSocket.broadcastTXT("{\"sw2\":true}");
  delay(50);
  webSocket.broadcastTXT("{\"sw1\":false}");
  webSocket.broadcastTXT("{\"sw2\":false}");

  sendQLabAuth();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  checkSwitches();
  delay(10);
}

void connectToWiFi() {
  for (WiFiConfig config : knownNetworks) {
    WiFi.config(config.local_ip, config.gateway, config.subnet);
    WiFi.begin(config.ssid, config.password);

    for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      currentSsid = config.ssid;
      currentPasscode = config.osc_passcode;
      return;
    }
  }
  currentSsid = "";
  currentPasscode = "";
}

void checkSwitches() {
  bool currentSwitch1 = digitalRead(SWITCH1_PIN);
  bool currentSwitch2 = digitalRead(SWITCH2_PIN);

  if (currentSwitch1 != lastSwitch1State && currentSwitch1 == HIGH) {
    webSocket.broadcastTXT("{\"sw1\":true}");
    sendOSC(oscAddress1.c_str(), 1);
    delay(50);
    webSocket.broadcastTXT("{\"sw1\":false}");
  }
  lastSwitch1State = currentSwitch1;

  if (currentSwitch2 != lastSwitch2State && currentSwitch2 == HIGH) {
    webSocket.broadcastTXT("{\"sw2\":true}");
    sendOSC(oscAddress2.c_str(), 1);
    delay(50);
    webSocket.broadcastTXT("{\"sw2\":false}");
  }
  lastSwitch2State = currentSwitch2;
}

void sendQLabAuth() {
  if (currentPasscode.length() == 0) return;
  OSCMessage msg("/connect");
  msg.add(currentPasscode.c_str());
  Udp.beginPacket(targetIP.c_str(), targetPort);
  msg.send(Udp);
  Udp.endPacket();

  delay(50);
  OSCMessage forget("/forgetMeNot");
  forget.add(true); // boolean true
  Udp.beginPacket(targetIP.c_str(), targetPort);
  forget.send(Udp);
  Udp.endPacket();
}

void sendOSC(const char* address, int value) {
  String addr(address);
  if (!addr.startsWith("/")) {
    addr = "/" + addr;
  }
  if (addr.length() == 0) {
    return;
  }
  OSCMessage msg(addr.c_str());
  msg.add((int32_t)value);
  Udp.beginPacket(targetIP.c_str(), targetPort);
  msg.send(Udp);
  Udp.endPacket();
}

void loadPreferences() {
  targetIP = preferences.getString("ip", "2.0.0.50");
  targetPort = preferences.getInt("port", 53000);
  oscAddress1 = preferences.getString("osc1", "/go");
  oscAddress2 = preferences.getString("osc2", "/panic");
}

void savePreferences() {
  preferences.putString("ip", targetIP);
  preferences.putInt("port", targetPort);
  preferences.putString("osc1", oscAddress1);
  preferences.putString("osc2", oscAddress2);
}

void setupWebServer() {
  server.on("/", []() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>OSC Remote</title>"
      "<style>"
      "body{background:#181a1b;color:#eee;font-family:sans-serif;margin:0;padding:0;}"
      "h1{margin:24px 0 0 0;font-size:1.35em;text-align:center;letter-spacing:0.05em;}"
      "#main{max-width:430px;margin:0 auto;padding:10px 0 0 0;}"
      "form{background:#232425;border-radius:10px;padding:18px 10px 10px 10px;margin-top:8px;}"
      "label{display:block;color:#bbb;font-size:1em;margin-bottom:5px;}"
      ".inputwrap{display:flex;align-items:center;margin-bottom:12px;}"
      ".inputwrap label{flex:0 0 110px;margin-bottom:0;}"
      ".inputwrap input{flex:1 1 0;border:1px solid #444;border-radius:7px;background:#161718;color:#eee;padding:6px 8px;font-size:1em;text-align:left;}"
      "input:focus{outline:2px solid #40a7ff;}"
      "button.big{font-size:1.2em;padding:16px 0;width:120px;height:64px;border:2px solid #333;background:#232425;transition:0.08s;box-shadow:none;border-radius:16px;}"
      "button.on{border:3px solid #fff !important;}"
      "#buttons{display:flex;justify-content:center;gap:18px;margin:15px 0 10px 0;}"
      ".btntxt{color:#fff;font-weight:bold;font-size:1.18em;letter-spacing:0.03em;}"
      ".sign{margin:26px 0 0 0; text-align:right; font-size:1.02em; color:orange; font-family:monospace;}"
      "</style></head><body>"
      "<div id='main'>"
      "<h1>OSC Remote</h1>"
      "<div id='buttons'>"
      "<button class='big' id='go' onclick=\"buttonClick('go')\"><span class='btntxt'>GO</span></button>"
      "<button class='big' id='panic' onclick=\"buttonClick('panic')\"><span class='btntxt'>PANIC</span></button>"
      "</div>"
      "<form autocomplete='off'>"
      "<h3 style='margin:0 0 10px 0;font-size:1.1em;color:#999;'>Configuración OSC</h3>"
      "<div class='inputwrap'><label>IP Destino:</label>"
      "<input type='text' id='ip' value='" + targetIP + "'></div>"
      "<div class='inputwrap'><label>Puerto:</label>"
      "<input type='number' id='port' value='" + String(targetPort) + "'></div>"
      "<div class='inputwrap'><label>Mensaje GO:</label>"
      "<input type='text' id='osc1' value='" + oscAddress1 + "'></div>"
      "<div class='inputwrap'><label>Mensaje PANIC:</label>"
      "<input type='text' id='osc2' value='" + oscAddress2 + "'></div>"
      "<button type='button' onclick='saveConfig()' style='margin-top:12px;'>Guardar Configuración</button>"
      "</form>"
      "<div class='sign'>Federico G&auml;rtner</div>"
      "</div>"
      "<script>"
      "var ws;"
      "function setupWS(){"
      "ws=new WebSocket('ws://'+location.hostname+':81/');"
      "ws.onmessage=function(evt){"
      "let data={};try{data=JSON.parse(evt.data);}catch(e){}"
      "if('sw1' in data){animateButton('go',data.sw1);}"
      "if('sw2' in data){animateButton('panic',data.sw2);}"
      "};"
      "}"
      "function animateButton(btn,on){"
      "let b=document.getElementById(btn);"
      "if(on){b.classList.add('on');setTimeout(()=>b.classList.remove('on'),50);}"
      "}"
      "function buttonClick(btn){"
      "fetch(`/triggerOSC?addr=${encodeURIComponent(document.getElementById('osc'+(btn=='go'?1:2)).value)}`);"
      "animateButton(btn,true);"
      "}"
      "function saveConfig(){"
      "fetch(`/setConfig?ip=${document.getElementById('ip').value}&port=${document.getElementById('port').value}&osc1=${document.getElementById('osc1').value}&osc2=${document.getElementById('osc2').value}`).then(r=>{if(r.ok){alert('Guardado!');setTimeout(()=>location.reload(),500);}})"
      "}"
      "window.onload=setupWS;"
      "</script></body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/setConfig", []() {
    if (server.hasArg("ip") && server.hasArg("port") && server.hasArg("osc1") && server.hasArg("osc2")) {
      targetIP = server.arg("ip");
      targetPort = server.arg("port").toInt();
      oscAddress1 = server.arg("osc1");
      oscAddress2 = server.arg("osc2");
      savePreferences();
      server.send(200, "text/plain", "UPDATED");
      sendQLabAuth();
    } else {
      server.send(400, "text/plain", "SOMETHING'S WRONG");
    }
  });

  server.on("/triggerOSC", []() {
    if (server.hasArg("addr")) {
      String addr = server.arg("addr");
      sendOSC(addr.c_str(), 1);
      server.send(200, "text/plain", "OSC sent");
    } else {
      server.send(400, "text/plain", "Missing OSC Address");
    }
  });

  server.begin();
}