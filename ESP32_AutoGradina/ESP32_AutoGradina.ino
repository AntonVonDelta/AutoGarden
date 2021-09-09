//#define SMALL_FACTOR
#define DEBUG
#define MAX_SCHEDULES_COUNT 50 // Change in js file as well

#include "DynamicDebug.h"
#include "ESP32_Timer.h"
#include "FileData.h"
#include "JSON.h"
#include "RTClib.h"

#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_sntp.h>
#include <esp_task_wdt.h>
#include <math.h>
#include <time.h>



void processGardenEvents();
void loadGardenSchedules();
void updateTimeVar();
void handleGardenUpdate();
void handleGardenRefresh();
void handleTimerUpdate();
void handleTimerStart();
void handleTimerRefresh();
void handleNotFound();
void handleDebug();
void handleGradina();
void handleScriptGarden();
void handleTimer();
void handleScript();
void handleStyle();
void operateValve(int, bool);
void operateRelay(int pin, bool start);

WebServer server(80);
Preferences prefs;
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional


/*     General Settings and Constants */
const char *ssid = "YOUR SSID";
const char *password = "YOUR PASSWORD";
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 10800;
const int daylightOffset_sec = 3600;
char formatted_time[32] = "Unknown";
int wifi_checks_count=-1;       // counts used for checking if wifi connects


/*     Reconnection timeouts and constant */
int reconnect_tries = 0;
int max_reconnect_tries = 5;
uint8_t last_try = 0;
esp_reset_reason_t reset_code = ESP_RST_UNKNOWN;


/*     Timer Settings and Constants   */
RTC_DS3231 rtc;
const int led_pin = 2;
const int timer_pin = 32;
volatile bool time_initialized = false;
volatile bool time_changed = false;
ESP32_Timer Timer_Device;


/*     Garden Settings and Constants  */
struct __attribute__((packed)) GARDEN_SCHEDULE_ENTRY {
    uint16_t start_time;
    uint16_t end_time;
    uint8_t valve_components;
    uint8_t relay_components;
    uint8_t active_days;
    bool enabled;
};
GARDEN_SCHEDULE_ENTRY garden_schedules[MAX_SCHEDULES_COUNT];

bool garden_running = false;
int garden_schedules_count = 0;
const int pwm_count = 5;
const int pwm_frequency = 500;
const int pwm_resolution = 8;
const int pwm_pins[] = {13, 12, 14, 27, 26};
bool pwm_states[] = {false, false, false, false, false};
const int relay_count = 2;
const int relay_pins[] = {33, 23};
bool relay_states[] = {false, false};




void debug_execute(int data) {
    if (data == 'a') {
        DynamicDebug::print("Garden running ", garden_running);
        DynamicDebug::print("Reconnect tries ", (unsigned int)reconnect_tries);
        DynamicDebug::print("Reset reason ", reset_code);
    }
    if (data == 'b') {
        DynamicDebug::print("The time is ", formatted_time);
    }
    if (data == 'c') {
        DynamicDebug::print("############################");
        DynamicDebug::print("time(NULL) : ", time(NULL));
        DynamicDebug::print("rtc.now().unixtime() : ", rtc.now().unixtime());
        char buff[100] = "hh:mm:ss DD-MM-YYYY";
        DateTime temp = rtc.now();
        DynamicDebug::print("rtc.now().toString(buff) : ", temp.toString(buff));
        DynamicDebug::print(temp.second());
        DynamicDebug::print(temp.minute());
        DynamicDebug::print(temp.hour());
        DynamicDebug::print("############################");
    }
    if (data == 'd') {
        // sntp_request(NULL);
    }
    if (data == 'e') {
        DynamicDebug::print("Heap Size: ", ESP.getHeapSize());
        DynamicDebug::print("Free heap: ", ESP.getFreeHeap());
        DynamicDebug::print("Min heap (): ", ESP.getMinFreeHeap());
        DynamicDebug::print("Max heap (): ", ESP.getMaxAllocHeap());
        DynamicDebug::print("ESP32 CPU FREQ: ",getCpuFrequencyMhz());
        DynamicDebug::print("ESP32 APB FREQ: ",getApbFrequency() / 1000000);
        DynamicDebug::print("ESP32 FLASH SIZE(MB): ",ESP.getFlashChipSize() / (1024 * 1024));
        DynamicDebug::print("ESP32 FREE PSRAM: ",ESP.getFreePsram() / 1024);
    }
    if (data == 'f') {
        DynamicDebug::print("RSSI: ", WiFi.RSSI());
    }
    if (data == 'g') {
        DynamicDebug::print("XTAL: ", getXtalFrequencyMhz());
        DynamicDebug::print("CPU: ", getCpuFrequencyMhz());
        DynamicDebug::print("APB: ", getApbFrequency());
    }
}

void sntp_callback(timeval *tv) {
    DynamicDebug::print("SNTP notification was called");
    time_initialized = true;
    time_changed = true;
}

bool WIFI_Connect() {
    for (int i = 0; i < 1; i++) {
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        WiFi.setSleep(false);
        delay(1000);
        if (WiFi.status() == WL_CONNECTED) {
            DynamicDebug::print("Connected to ", ssid);
            DynamicDebug::print("IP address: ", WiFi.localIP());
            return true;
        }
    }
    return false;
}


void setup(void) {
    // Lower speed for power savings - too much heat
    //https://deepbluembedded.com/esp32-change-cpu-speed-clock-frequency/
    setCpuFrequencyMhz(80);
	
    // Configure auto reset watchdog
    esp_task_wdt_init(30, true); // enable panic so ESP32 restarts. Do not lower this number: OTA will fail(takes over 10secs) and Wifi reconnect(probably)
    esp_task_wdt_add(NULL);               // add current thread to WDT watch
	
    // Enable OTA authentification
    ArduinoOTA.setPassword("YOUR ARDUINO OTA PASSWORD");
    reset_code = esp_reset_reason();
   
    prefs.begin("FatherProj", false);
    DynamicDebug::begin();
    Serial.begin(9600);
	
    // Init timer and relay pins as OUTPUT
    pinMode(led_pin, OUTPUT);
    pinMode(timer_pin, OUTPUT);
    pinMode(relay_pins[0], OUTPUT);
    pinMode(relay_pins[1], OUTPUT);
	
	
    // Init PWM
    for (int i = 0; i < pwm_count; i++) {
        ledcSetup(i, pwm_frequency, pwm_resolution);
        ledcAttachPin(pwm_pins[i], i);
    }
	
    // Init WIFI
    WiFi.persistent(false);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WIFI_Connect();
	
    // Init and get the time
    // Look here : https://github.com/esp8266/Arduino/issues/4637#issuecomment-435611842 for DST issue
    while (!rtc.begin()) {
        DynamicDebug::print("Couldn't find RTC");
        delay(1000);
    }
	
    time_t rtc_time_t;
    if (rtc.lostPower()) {
        DynamicDebug::print("RTC lost power!");
        rtc.adjust(
            DateTime(__DATE__, __TIME__)); // If rtc was closed then at least update after a very recent date and time
        rtc_time_t = DateTime(__DATE__, __TIME__).unixtime(); // rtc unix time
    } else {
        delay(2000);
        rtc_time_t = rtc.now().unixtime();
        time_initialized = true;
    }
	
    timezone tz = {0, 0};
    timeval tv = {rtc_time_t, 0};
    settimeofday(&tv, NULL);
    sntp_set_time_sync_notification_cb(&sntp_callback);
    configTime(0, 0, ntpServer);
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();
    updateTimeVar();
    DynamicDebug::print("The time is ", formatted_time);
	
	
    // Init Timer library
    unsigned int timer_opened_time = prefs.getUInt("opened_time", 60);
    unsigned int timer_closed_time = prefs.getUInt("closed_time", 60);
    bool timer_running = prefs.getBool("timer_running", true);
	
    Timer_Device.setTimeouts(timer_opened_time, timer_closed_time);
    Timer_Device.Init();
    Timer_Device.Start();
    if (!timer_running)  Timer_Device.Pause();
	
    // Init garden data
    loadGardenSchedules();
    garden_running = prefs.getBool("garden_running", true);
	
	
    // Init and configure the Http Server
#ifndef SMALL_FACTOR
    server.on("/", []() {
        String header = "HTTP/1.1 302 Found\r\nLocation:/timer\r\n\r\n";
        server.client().write(header.c_str(), header.length());
    });
    server.on("/debug", handleDebug);
    server.on("/debug_script.js", handleScriptDebug);
    server.on("/debug_update", handleDebugUpdate);
    server.on("/timer", handleTimer);
    server.on("/gradina", handleGradina);
    server.on("/style.css", handleStyle);
    server.on("/script.js", handleScript);
    server.on("/script_garden.js", handleScriptGarden);
    server.on("/timer_update", handleTimerUpdate);
    server.on("/timer_start", handleTimerStart);
    server.on("/timer_refresh", handleTimerRefresh);
    server.on("/garden_refresh", handleGardenRefresh);
    server.on("/garden_update", handleGardenUpdate);
    server.on("/relay", []() {
        if (server.arg("i") != "") {
            int i = server.arg("i").toInt();
            relay_states[i] = !relay_states[i];
            server.send(200, "text/plain", String(" Relay ") + String(i) + String(" : ") + String(relay_states[i]));
            operateRelay(i, relay_states[i]);
        }
    });
    server.on("/valve", []() {
        if (server.arg("i") != "") {
            int i = server.arg("i").toInt();
            pwm_states[i] = !pwm_states[i];
            server.send(200, "text/plain",
                        String(" Valve channel ") + String(i) + String(" : ") + String(pwm_states[i]));
            operateValve(i, pwm_states[i]);
        }
    });
    server.onNotFound(handleNotFound);
    server.begin();
    DynamicDebug::print("HTTP server started");
#endif


    // Autoupdating OTA
    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            DynamicDebug::print("Start updating ", type);
        })
        .onEnd([]() { DynamicDebug::print("End"); })
        .onProgress([](unsigned int progress, unsigned int total) {
            DynamicDebug::print("Progress: ", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            DynamicDebug::print("Error[]: ", error);
            if (error == OTA_AUTH_ERROR)
                DynamicDebug::print("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                DynamicDebug::print("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                DynamicDebug::print("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                DynamicDebug::print("Receive Failed");
            else if (error == OTA_END_ERROR)
                DynamicDebug::print("End Failed");
        });
    ArduinoOTA.begin();
}


void loop(void) {
    esp_task_wdt_reset();
    ArduinoOTA.handle();
	
	
    if (time_changed) {
        time_t tm;
        time(&tm);
        rtc.adjust(DateTime(tm));
        time_changed = false;
    }
	
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(led_pin, LOW);
        if (reconnect_tries == 0) {
            DynamicDebug::print("Lost connection");
        }
        if(wifi_checks_count==-1){
            if (millis() - last_try > 5000) {
                last_try = millis();
                WIFI_Connect();
                reconnect_tries++;
                wifi_checks_count++;
            }
        }else{
            delay(100);
            if (WiFi.status() != WL_CONNECTED && wifi_checks_count<200) {
                wifi_checks_count++;
            }else{
                wifi_checks_count=-1;
                last_try = millis();
            }
        }
    } else {
        if (reconnect_tries != 0) {
            DynamicDebug::print("Reconnect tries: ", reconnect_tries);
        }
        reconnect_tries = 0;
        digitalWrite(led_pin, HIGH);
    }
	
#ifdef DEBUG
    if (Serial.available()) {
        int data = Serial.read();
        debug_execute(data);
    }
#endif

    server.handleClient();
    updateTimeVar();
    Timer_Device.Process();
	
    if (Timer_Device.getSocketState() == ESP32_Timer::OPENED) {
        digitalWrite(timer_pin, HIGH);
    } else digitalWrite(timer_pin, LOW);
	
	
    processGardenEvents();
}




void processGardenEvents() {
    if (!time_initialized)
        return;
	
    if (!garden_running) {
        // First close relays as this controls the mini pump. We don't want a pipe shock to get to a closed valve
        bool found = false;
        for (int i = 0; i < relay_count; i++) {
            if (relay_states[i])
                found = true;
        }
        for (int i = 0; i < pwm_count; i++) {
            if (pwm_states[i])
                found = true;
        }
		
		
        // Necessary because otherwise we block the itnerface with the delays
        if (!found)
            return;
        for (int i = 0; i < relay_count; i++) {
            operateRelay(i, false);
            relay_states[i] = false;
        }
        delay(2000); // Pipe shock protection
        for (int i = 0; i < pwm_count; i++) {
            operateValve(i, false);
            pwm_states[i] = false;
        }
		
        return;
    }
	
	
    time_t unix_time;
    time(&unix_time);
    tm *time_info = localtime(&unix_time);
	
	
    unsigned int absolute_minutes = (time_info->tm_hour) * 60 + (time_info->tm_min); // Minutes from midnight
    unsigned int week_day = pow(2, (time_info->tm_wday + 6) % 7); // Convert from Sunday to Monday as first week day
    bool new_pwm_states[pwm_count];
    bool new_relay_states[relay_count];
	
    for (int i = 0; i < pwm_count; i++) {
        new_pwm_states[i] = false;
    }
    for (int i = 0; i < relay_count; i++) {
        new_relay_states[i] = false;
    }
	
    for (int i = 0; i < garden_schedules_count; i++) {
        GARDEN_SCHEDULE_ENTRY *temp_entry = &(garden_schedules[i]);
        if (!temp_entry->enabled)
            continue;
        if (temp_entry->start_time > absolute_minutes || temp_entry->end_time <= absolute_minutes)
            continue;
        if (!((temp_entry->active_days) & week_day))
            continue;
		
		
        // Process valve component
        for (int i = 0; i < pwm_count; i++) {
            int mask = 1 << i;
            if (!((temp_entry->valve_components) & mask))
                continue;
            new_pwm_states[i] = true;
        }
		
		
        // Process relay component
        for (int i = 0; i < relay_count; i++) {
            int mask = 1 << i;
            if (!((temp_entry->relay_components) & mask))
                continue;
            new_relay_states[i] = true;
        }
    }
	
	
    for (int i = 0; i < pwm_count; i++) {
        if (pwm_states[i] == new_pwm_states[i])
            continue;
        operateValve(i, new_pwm_states[i]);
        pwm_states[i] = new_pwm_states[i];
    }
	
	
    // Do this after valve component
    // Open first a valve and then the pump with the relay
    for (int i = 0; i < relay_count; i++) {
        if (relay_states[i] == new_relay_states[i])
            continue;
        operateRelay(i, new_relay_states[i]);
        relay_states[i] = new_relay_states[i];
    }
}

void loadGardenSchedules() {
    size_t entries_size = prefs.getBytesLength("garden_entries");
    int entries_count = entries_size / sizeof(GARDEN_SCHEDULE_ENTRY);
	
    if (entries_count == 0 || entries_count > MAX_SCHEDULES_COUNT) {
        entries_count = 0;
        return;
    }
    size_t res = prefs.getBytes("garden_entries", (void *)garden_schedules, entries_count * sizeof(GARDEN_SCHEDULE_ENTRY));
    if (res == 0)
        return;
	
	
    garden_schedules_count = entries_count;
}
void handleGardenUpdate() {
    if (server.arg("mode") == "toggle_state") {
        garden_running = !garden_running;
        prefs.putBool("garden_running", garden_running);
    } else {
        // set_schedule
        int total = server.arg("total").toInt();
		
        if (total > MAX_SCHEDULES_COUNT) {
            server.send(204);
            return;
        }
		
        garden_schedules_count = 0;
        for (int i = 1; i <= total; i++) {
            GARDEN_SCHEDULE_ENTRY entry;
            entry.start_time = server.arg(String("start[") + String(i) + String("]")).toInt();
            entry.end_time = server.arg(String("end[") + String(i) + String("]")).toInt();
            entry.valve_components = server.arg(String("valve_components[") + String(i) + String("]")).toInt();
            entry.relay_components = server.arg(String("relay_components[") + String(i) + String("]")).toInt();
            entry.active_days = server.arg(String("days[") + String(i) + String("]")).toInt();
            entry.enabled = (entry.active_days != 0);
            garden_schedules[garden_schedules_count] = entry;
            garden_schedules_count++;
        }
		
		
        prefs.putBytes("garden_entries", (void *)&garden_schedules, total * sizeof(GARDEN_SCHEDULE_ENTRY));
    }
	
    server.send(204);
}

void handleGardenRefresh() {
    JSON json;
    json.createObject();
    json.set("current_time", formatted_time);
    json.set("garden_state", garden_running);
	
    json.createArray("schedule_list");
    for (int i = 0; i < garden_schedules_count; i++) {
        GARDEN_SCHEDULE_ENTRY *temp_entry = &(garden_schedules[i]);
        json.createObject();
        json.set("start_time", temp_entry->start_time);
        json.set("end_time", temp_entry->end_time);
        json.set("valve_components", temp_entry->valve_components);
        json.set("relay_components", temp_entry->relay_components);
        json.set("active_days", temp_entry->active_days);
        json.closeObject();
    }
    json.closeArray();
	
    int opened_valves = 0;
    for (int i = 0; i < pwm_count; i++) {
        if (pwm_states[i]) {
            opened_valves += pow(2, i);
        }
    }
	
    json.set("opened_valves", opened_valves);
    json.closeObject();
    json.finish();
	
	
    server.send(200, "application/json", json.c_str());
}
void handleTimerUpdate() {
    if (server.arg("mode") == "time") {
        // Update the timeouts
        unsigned int opened_time = server.arg("opened_time").toInt();
        unsigned int closed_time = server.arg("closed_time").toInt();
		
        prefs.putUInt("opened_time", opened_time);
        prefs.putUInt("closed_time", closed_time);
		
        Timer_Device.setTimeouts(opened_time, closed_time);
    } else {
		
        // Update	the state of the timer: if it is paused or counting
        if (Timer_Device.getState() == 0) {
            Timer_Device.Pause();
            prefs.putBool("timer_running", false);
        } else if (Timer_Device.getState() == 1) {
            Timer_Device.Continue();
            prefs.putBool("timer_running", true);
        } else {
            Timer_Device.Continue();
            prefs.putBool("timer_running", true);
        }
    }
	
    server.send(204);
}
void handleTimerStart() {
    unsigned int time = server.arg("time").toInt();
	
    Timer_Device.StartTempChron(time);
    server.send(204);
}
void handleTimerRefresh() {
    sendTimerRefreshedData();
}
void sendTimerRefreshedData() {
    JSON json;
    unsigned int opened_time = prefs.getUInt("opened_time", 60);
    unsigned int closed_time = prefs.getUInt("closed_time", 60);
	
	
    json.createObject();
    json.set("output_state", Timer_Device.getSocketState());
    json.set("primary_output_state", Timer_Device.getPrimaryClockState());
    json.set("timer_state", Timer_Device.getState());
    json.set("remaining", Timer_Device.getRemainingTime(false));
    json.set("secondary_remaining", Timer_Device.getRemainingTime(true));
    json.set("current_time", formatted_time);
    json.set("opened_time", opened_time);
    json.set("closed_time", closed_time);
    json.closeObject();
    json.finish();
	
    server.send(200, "application/json", json.c_str());
}
void handleNotFound() {
    server.send(404, "text/plain", "test");
    return;
}
void handleDebug() {
    server.send(200, "text/html", debug_html_content);
}
void handleScriptDebug() {
    server.send(200, "text/javascript", debug_script_content);
}
void handleDebugUpdate() {
    if (server.hasArg("c")) {
        int data = server.arg("c")[0];
        debug_execute(data);
    }
    server.send(200, "text/plain", DynamicDebug::c_str());
}
void handleGradina() {
    server.send(200, "text/html", garden_html_content);
}
void handleScriptGarden() {
    server.send(200, "text/javascript", garden_script_content);
}
void handleTimer() {
    server.send(200, "text/html", timer_htlm_content);
}
void handleScript() {
    server.send(200, "text/javascript", timer_script_content);
}
void handleStyle() {
    server.send(200, "text/css", style_content);
}
void updateTimeVar() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        DynamicDebug::print("Failed to obtain time");
        return;
    }
	
    char time_buffer[32];
    strftime(time_buffer, 32, "%d-%m-%Y %H:%M:%S", &timeinfo);
    strcpy(formatted_time, time_buffer);
}
void operateValve(int channel, bool start) {
    if (start) {
        // digitalWrite(pwm_pin1,HIGH);
        ledcWrite(channel, 256);
        delay(1500);
        ledcWrite(channel, 150);
    } else {
        ledcWrite(channel, 0);
    }
}
void operateRelay(int channel, bool start) {
    int pin = relay_pins[channel];
    if (start) {
        digitalWrite(pin, HIGH);
    } else {
        digitalWrite(pin, LOW);
    }
}
