
    #include <ESP8266WiFi.h>
    #include <WiFiClient.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    #include <EEPROM.h>
    // DHT
    //#include <DHT.h>
    // DS18D20
    #include <OneWire.h>
    #include <DallasTemperature.h>
    // Servo
    #include <Servo.h>
    //////

    Servo myservo;
    int myServo,myServoHistory;
    //int napetost_baterije;
    float napetost_baterije, napajanje;
    float soil_hum, soil_ph, soil_light;
    float zunaj_soil_hum, zunaj_soil_ph, zunaj_light;
    float raw_napetost_baterije, raw_napajanje;
    float raw_soil_hum, raw_soil_ph, raw_soil_light;
    float raw_zunaj_soil_hum, raw_zunaj_soil_ph, raw_zunaj_light;
    float avg_napetost_baterije, avg_napajanje;
    float avg_soil_hum, avg_soil_ph, avg_soil_light;
    float avg_zunaj_soil_hum, avg_zunaj_soil_ph, avg_zunaj_light;
    boolean TurnOff60SoftAP;
    unsigned long wifi_connect_previous_millis = 0;
    
    ESP8266WebServer server(80);
    const char* host = "esp8266";
    const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
     
    // Initialize DHT sensor 
    // NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
    // you need to increase the threshold for cycle counts considered a 1 or 0.
    // You can do this by passing a 3rd parameter for this threshold.  It's a bit
    // of fiddling to find the right value, but in general the faster the CPU the
    // higher the value.  The default for a 16mhz AVR is a value of 6.  For an
    // Arduino Due that runs at 84mhz a value of 30 works.
    // This is for the ESP8266 processor on ESP-01 
    //#define DHTTYPE DHT22
    //#define DHTPIN  2  // DHT DATA PIN = GPIO2
    //DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266

    #define ONE_WIRE_BUS 4  // DS18B20 pin = GPIO4
    OneWire oneWire(ONE_WIRE_BUS);
    DallasTemperature DS18B20(&oneWire);
    float ds_temp_zunaj, ds_temp_znotraj, ds_temp_tla, ds_temp_tla_zunaj;                //ds temp variables

    // Tell it where to store your config data in EEPROM
    #define EEPROM_START 0    
    
    typedef struct {
      int ID_ds_temp_zunaj;
      int ID_ds_temp_znotraj;
      int ID_ds_temp_tla;
      int ID_ds_temp_tla_zunaj;
      // zracenje 
      boolean zracenje_state;
      int zracenje_temp_value;
      int zracenje_open_time;
      int zracenje_temp_hist;
      int zracenje_auto_open_time;
      // namakanje rastlinjak
      boolean namakanje_rastlinjak_state;
      int namakanje_rastlinjak_value;
      int namakanje_rastlinjak_open_time;
      int namakanje_rastlinjak_hist;
      int namakanje_rastlinjak_auto_open_time;
      // namakanje zunaj
      boolean namakanje_zunaj_state;
      int namakanje_zunaj_value;
      int namakanje_zunaj_open_time;
      int namakanje_zunaj_hist;
      int namakanje_zunaj_auto_open_time;
      // wifi
      char ssid[32];
      char password[64];
      char ap_ssid[32];
      char ap_password[64];
      // korektorji
      int soil_light_k;
      int soil_light_n;
      int soil_ph_k;
      int soil_ph_n;
      int soil_hum_k;
      int soil_hum_n;
      int voltage_k;
      int voltage_n;
    } ObjectSettings;
    
    ObjectSettings SETTINGS;
    
    // zracenje
    boolean zracenje_timer = false, zracenje_auto_timer = false;
    unsigned long zracenje_auto_previous_millis = 0, zracenje_previous_millis = 0;
     
    // namakanje_rastlinjak
    boolean namakanje_rastlinjak_timer = false, namakanje_rastlinjak_auto_timer = false;
    unsigned long namakanje_rastlinjak_auto_previous_millis = 0, namakanje_rastlinjak_previous_millis = 0;
    
    // namakanje_zunaj
    boolean namakanje_zunaj_timer = false, namakanje_zunaj_auto_timer = false;
    unsigned long namakanje_zunaj_auto_previous_millis = 0, namakanje_zunaj_previous_millis = 0;
       
    String webString="";
    String webMetaRefreshShowdata="<meta http-equiv=\"refresh\" content=\"0;url=/\">";
    String webMetaRefreshSettings="<meta http-equiv=\"refresh\" content=\"0;url=/settings\">";
    
    // Generally, you should use "unsigned long" for variables that hold time
    unsigned long previousMillis = 0;        // will store last temp was read
    const long interval = 2000;              // interval at which to read sensor
    //  0 = D3 *
    //  1 = D10
    //  2 = D4 *
    //  3 = D9 *
    //  4 = D2 * DS18D20
    //  5 = D1 *
    //  6 = (SPI) - flash
    //  7 = (SPI) - flash
    //  8 = (SPI) - flash
    //  9 = S2    - flash
    // 10 = S3    - flash
    // 11 = (SPI) - flash
    // 12 = D6 *
    // 13 = D7 *
    // 14 = D5 *
    // 15 = D8 * SERVO
    // 16 = D0 *
    int pin_namakanje_rastlinjak = 16;      //D0
    int pin_namakanje_zunaj = 5;            //D1
    //DS18D20                               //D2
    int pin_beeper = 0;                     //D3
    int pin_led = 3;                        //D4
    int pin_select_s0 = 14;   //D5          // pin za izbiro AD0
    int pin_select_s1 = 12;   //D6          // pin za izbiro AD0
    int pin_select_s2 = 13;   //D7          // pin za izbiro AD0
    int pin_servo = 15;       //D8

    const int Average_index_total = 8;      // stevilo vrednosti, ki jih povprecim
    //const int Average_numReadings = 120 * Average_index_total;  // stevilo vzorce, ki jih vzamem za avg
    const int Average_numReadings = 300 * Average_index_total;  // stevilo vzorce, ki jih vzamem za avg
    
    int Average_readings[Average_numReadings];      // the readings from the analog input
    int Average_readIndex = 0;              // the index of the current reading
    int Average_total[Average_index_total];                  // the running total
    int Average = 0;                        // the average

    void setup(void)
    {
      // You can open the Arduino IDE Serial Monitor window to see what the code is doing
      Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
      //dht.begin();           // initialize temperature sensor
      EEPROM.begin(512);
      loadConfig();
      DS18B20.begin();

      //myservo.attach(5,771, 1798); // popravek za MG996R https://projectgus.com/2009/07/servo-pulse-width-range-with-arduino/
      myservo.attach(pin_servo);

      ConnectToWifi();
      
      MDNS.begin(host);
      
      server.on("/espupdate", HTTP_GET, [](){
        //server.sendHeader("Connection", "close");
        //server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/html", serverIndex);
      });
      server.on("/update", HTTP_POST, [](){
        server.sendHeader("Connection", "close");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
        ESP.restart();
      },[](){
        HTTPUpload& upload = server.upload();
        if(upload.status == UPLOAD_FILE_START){
          Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if(!Update.begin(maxSketchSpace)){//start with max available size
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_WRITE){
          if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_END){
          if(Update.end(true)){ //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
      });
      server.begin();
      MDNS.addService("http", "tcp", 80);
      Serial.printf("Ready! Open http://%s.local in your browser\n", host);

      
      server.on("/", handle_root);
      server.on("/info",esp_info);
      server.on("/json",handle_json);
      server.on("/tempsensors",handle_temp_sensors);
      server.on("/rawanalog",handle_raw_analog);
      server.on("/settings",handle_Settings);
      server.on("/winclosestep",handle_window_close_step);
      server.on("/winopenstep",handle_window_open_step);
      server.on("/winclosefull",handle_window_close_full);
      server.on("/winopenfull",handle_window_open_full);     
      server.on("/waterraston",handle_rastlinjak_water_on);
      server.on("/waterrastoff",handle_rastlinjak_water_off);
      server.on("/waterzuon",handle_zunaj_water_on);
      server.on("/waterzuoff",handle_zunaj_water_off);
      server.on("/beeperon",handle_beeper_on);
      server.on("/beeperoff",handle_beeper_off);
      server.on("/ledon",handle_led_on);
      server.on("/ledoff",handle_led_off);
      server.on("/df",handle_load_defaults);
      server.on("/tszum",handle_temp_senzor_zunaj_id_dec);
      server.on("/tszup",handle_temp_senzor_zunaj_id_inc);
      server.on("/tsznm",handle_temp_senzor_znotraj_id_dec);
      server.on("/tsznp",handle_temp_senzor_znotraj_id_inc);
      server.on("/tstlm",handle_temp_senzor_tla_id_dec);
      server.on("/tstlp",handle_temp_senzor_tla_id_inc);
      server.on("/tstlzum",handle_temp_senzor_tla_zunaj_id_dec);
      server.on("/tstlzup",handle_temp_senzor_tla_zunaj_id_inc);
      server.on("/zracm",handle_zracenje_temp_dec);
      server.on("/zracp",handle_zracenje_temp_inc);
      server.on("/zracam",handle_zracenje_manual_auto);
      server.on("/zractm",handle_zracenje_open_time_dec);
      server.on("/zractp",handle_zracenje_open_time_inc);
      server.on("/zracatm",handle_zracenje_auto_open_time_dec);
      server.on("/zracatp",handle_zracenje_auto_open_time_inc);
      server.on("/zrachim",handle_zracenje_histereza_dec);
      server.on("/zrachip",handle_zracenje_histereza_inc);  
      server.on("/namm",handle_namakanje_rastlinjak_value_dec);
      server.on("/namp",handle_namakanje_rastlinjak_value_inc);  
      server.on("/namam",handle_namakanje_rastlinjak_manual_auto);  
      server.on("/namtm",handle_namakanje_rastlinjak_open_time_dec);
      server.on("/namtp",handle_namakanje_rastlinjak_open_time_inc);
      server.on("/namatm",handle_namakanje_rastlinjak_auto_open_time_dec);
      server.on("/namatp",handle_namakanje_rastlinjak_auto_open_time_inc);
      server.on("/namhim",handle_namakanje_rastlinjak_histereza_dec);
      server.on("/namhip",handle_namakanje_rastlinjak_histereza_inc);
      server.on("/namzm",handle_namakanje_zunaj_value_dec);
      server.on("/namzp",handle_namakanje_zunaj_value_inc);  
      server.on("/namzam",handle_namakanje_zunaj_manual_auto);  
      server.on("/namztm",handle_namakanje_zunaj_open_time_dec);
      server.on("/namztp",handle_namakanje_zunaj_open_time_inc);
      server.on("/namzhim",handle_namakanje_zunaj_histereza_dec);
      server.on("/namzhip",handle_namakanje_zunaj_histereza_inc);
      server.on("/namzatm",handle_namakanje_zunaj_auto_open_time_dec);
      server.on("/namzatp",handle_namakanje_zunaj_auto_open_time_inc);
      server.on("/reboot",handle_reboot);
      //server.on("/savee",saveConfig);
      server.on("/save", []() {
        //ssid = server.arg("ssid");
        strcpy (SETTINGS.ssid, server.arg("ssid").c_str());        
        //password = server.arg("pwd");
        strcpy (SETTINGS.password, server.arg("pass").c_str());
        strcpy (SETTINGS.ap_ssid, server.arg("ap_ssid").c_str());        
        strcpy (SETTINGS.ap_password, server.arg("ap_pass").c_str());
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
        saveConfig();
//        Serial.print("ssid: ");
//        Serial.println(SETTINGS.ssid);
//        Serial.print("pass: ");
//        Serial.println(SETTINGS.password);
//        Serial.print("ap_ssid: ");
//        Serial.println(SETTINGS.ap_ssid);
//        Serial.print("ap_pass: ");
//        Serial.println(SETTINGS.ap_password); 
        });
      server.on("/light_km",handle_light_km);
      server.on("/light_kp",handle_light_kp);
      server.on("/light_nm",handle_light_nm);
      server.on("/light_np",handle_light_np);
      server.on("/ph_km",handle_ph_km);
      server.on("/ph_kp",handle_ph_kp);
      server.on("/ph_nm",handle_ph_nm);
      server.on("/ph_np",handle_ph_np);
      server.on("/hum_km",handle_hum_km);
      server.on("/hum_kp",handle_hum_kp);
      server.on("/hum_nm",handle_hum_nm);
      server.on("/hum_np",handle_hum_np);
      server.on("/voltage_km",handle_voltage_km);
      server.on("/voltage_kp",handle_voltage_kp);
      server.on("/voltage_nm",handle_voltage_nm);
      server.on("/voltage_np",handle_voltage_np);
      server.on("/enapsta",handle_enable_ap_sta);
      server.on("/disap",handle_disable_ap);
      server.on("/recon",ConnectToWifi);    
      server.begin();
      //Serial.println("HTTP server started");
     
      pinMode(pin_namakanje_rastlinjak, OUTPUT);
      digitalWrite(pin_namakanje_rastlinjak, LOW);
      pinMode(pin_namakanje_zunaj, OUTPUT);
      digitalWrite(pin_namakanje_zunaj, LOW);
      pinMode(pin_beeper, OUTPUT);
      digitalWrite(pin_beeper, LOW);
      pinMode(pin_led, OUTPUT);
      digitalWrite(pin_led, LOW);
      pinMode(pin_select_s0, OUTPUT);
      pinMode(pin_select_s1, OUTPUT);
      pinMode(pin_select_s2, OUTPUT);

      for (int thisReading = 0; thisReading < Average_numReadings; thisReading++) {
        Average_readings[thisReading] = 0;
      }
      Average_readIndex = 0;
    }
     
    void loop(void) {
      server.handleClient();  
//      if (millis() > wifi_connect_previous_millis + 60000 and TurnOff60SoftAP) {
//        WiFi.mode(WIFI_STA); // po 60sec ko se poveže na AP izklopim SoftAP
//        Serial.println("60sec up, turning off SoftAP!");
//        TurnOff60SoftAP = false;
//      }
      //if (millis() - previousMillis > 5000) {
      if (millis() - previousMillis > 1000) {
        previousMillis = millis();
        // tole se izvaja na 5sek
        Read_DS18B20();
        ReadADC();
        ZracenjeFunc();
        NamakanjeRastlinjakFunc();
        NamakanjeZunajFunc();
      }
    }

    void loadConfig() {
      EEPROM.get(EEPROM_START, SETTINGS);
      Serial.println("Settings loaded!");
    }
    
    void saveConfig() {
      EEPROM.put(EEPROM_START, SETTINGS);
      EEPROM.commit();
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
      Serial.println("Settings saved!");
    }
      
    void handle_load_defaults() {
      // temp senzorji
      SETTINGS.ID_ds_temp_zunaj = 0;
      SETTINGS.ID_ds_temp_znotraj = 0;
      SETTINGS.ID_ds_temp_tla = 0;
      // zracenje
      SETTINGS.zracenje_state = true;
      SETTINGS.zracenje_temp_value = 25;
      SETTINGS.zracenje_open_time = 5;
      SETTINGS.zracenje_temp_hist = 1;
      SETTINGS.zracenje_auto_open_time = 5;
      // namakanje rastlinjak
      SETTINGS.namakanje_rastlinjak_state = true;
      SETTINGS.namakanje_rastlinjak_value = 30;
      SETTINGS.namakanje_rastlinjak_open_time = 5;
      SETTINGS.namakanje_rastlinjak_hist = 2;
      SETTINGS.namakanje_rastlinjak_auto_open_time = 5;
      // namakanje zunaj
      SETTINGS.namakanje_zunaj_state = true;
      SETTINGS.namakanje_zunaj_value = 30;
      SETTINGS.namakanje_zunaj_open_time = 5;
      SETTINGS.namakanje_zunaj_hist = 2;
      SETTINGS.namakanje_zunaj_auto_open_time = 5;
      // wifi
      strcpy (SETTINGS.ssid, "ssid-to-connect-to");
      strcpy (SETTINGS.password, "password");
      strcpy (SETTINGS.ap_ssid, "ESP8266-VRT");
      strcpy (SETTINGS.ap_password, "password");
      // korektorji
      SETTINGS.soil_light_k = 44;
      SETTINGS.soil_light_n = 0;
      SETTINGS.soil_ph_k = 3228;
      SETTINGS.soil_ph_n = 454;
      SETTINGS.soil_hum_k = 13;
      SETTINGS.soil_hum_n = 0;
      SETTINGS.voltage_k = 43;
      SETTINGS.voltage_n = 0;
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
      Serial.println("Defaults loaded!");    
    }

    void handle_enable_ap_sta() {
      TurnOff60SoftAP = false;
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(SETTINGS.ap_ssid, SETTINGS.ap_password);
      Serial.print("Soft AP enabled! ");
      Serial.println(WiFi.softAPIP());
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    
    void handle_disable_ap() {
      WiFi.mode(WIFI_STA);
      Serial.print("SoftAP disabled! ");
      Serial.println(WiFi.localIP());
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }   
    
    void ConnectToWifi() {
      TurnOff60SoftAP = true;
      WiFi.mode(WIFI_AP_STA);
      /* You can remove the password parameter if you want the AP to be open. */
      WiFi.softAP(SETTINGS.ap_ssid, SETTINGS.ap_password);
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());
      // Connect to WiFi network
      WiFi.begin(SETTINGS.ssid, SETTINGS.password);
      Serial.print("\n\r \n\rConnecting to WiFi");
      // Wait for connection
      wifi_connect_previous_millis = millis();
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (millis() > wifi_connect_previous_millis + 30000) {
          break; //prekinem povezovanje in nadaljujem z AP_STA nacinom
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        //wifi_connect_previous_millis = millis();
        Serial.println("");
        //Serial.println("DHT Weather Reading Server");
        Serial.print("Connected to SSID: ");
        Serial.println(SETTINGS.ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
      }
    }
    
    void handle_root() {
        long days = 0;
        long hours = 0;
        long mins = 0;
        unsigned long secs = 0;
        secs = millis()/1000; //convert milliseconds to seconds
        mins=secs/60; //convert seconds to minutes
        hours=mins/60; //convert minutes to hours
        days=hours/24; //convert hours to days
        secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max
        mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
        hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
        webString="<!DOCTYPE html><HTML><meta charset=\"UTF-8\">\n"
        //"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"
        "<title>   { Rastlinjak & Vrt }   </title>"
        "<style>\n"
        "table, th, td {\n"
        "font-family: DroidSans;\n"
        "font-size: 160%;\n"
        "text-align: center;\n"
        "}</style>\n"
        "<script>setInterval(function(){location.reload();},5000);</script>\n"
        "<table style=\"width:100%\">\n"
        "<tr><th>ZUNAJ</th><th>ZNOTRAJ</th><th>PRI TLEH</th></tr>\n"
        "<tr><td>" + String((float)ds_temp_zunaj) + "°C</td><td>" + String((float)ds_temp_znotraj) + "°C</td><td>" + String((float)ds_temp_tla) + "°C</td></tr>\n" 
        "<tr><td>&nbsp;</td></tr>"
        "<tr><th>VLAGA ZEMLJE</th><th>pH ZEMLJE</th><th>SEVANJE</th></tr>\n"
        "<tr><td>" + String((float)soil_hum) + "%</td><td>" + String((float)soil_ph) + "</td><td>" + String((float)soil_light) + "</td></tr>\n"
        "<tr><td>&nbsp;</td></tr>"
        "<tr><th>VLAGA ZEMLJE (VRT)</th><th>pH ZEMLJE (VRT)</th><th>SEVANJE (VRT)</th></tr>\n"
        "<tr><td>" + String((float)zunaj_soil_hum) + "%</td><td>" + String((float)zunaj_soil_ph) + "</td><td>" + String((float)zunaj_light) + "</td></tr>\n"
        "<tr><td>&nbsp;</td></tr>"
        "<tr><th>WiFi SIGNAL</th><th>NAPAJANJE</th><th>UPTIME</th></tr>\n"
        "<tr><td>" + WiFi.RSSI() + "dBm</td><td>" + String((float)napetost_baterije) + "V " + String((float)napajanje) + "V</td><td>" + String((long)days) + "d " + String((long)hours) + "h " + String((long)mins) + "m " + String((long)secs) + "s</td></tr>\n"
        "<div>"
        "</table><br>\n"
        "<table align=\"center\">\n"
        "<style scoped>.g{width:100px;height:100%}</style>"
        "<tr><td>OKNA:</td><td><a href=\"/winclosestep\"><button class=\"g\"><h1>-</h1></button></a></td><td>" + String((int)myServo) + "</td><td><a href=\"/winopenstep\"><button class=\"g\"><h1>+</h1></button></a></td>"
        "<td><a href=\"/winclosefull\"><button class=\"g\"><h1>ZAPRI</h1></button></a></td>"
        "<td><a href=\"/winopenfull\"><button class=\"g\"><h1>ODPRI</h1></button></a></td></tr>\n"
        "<tr><td>NAMAKANJE RASTL.:</td><td></td><td>" + String((boolean)digitalRead(pin_namakanje_rastlinjak)) + "</td><td></td>"
        "<td><a href=\"/waterrastoff\"><button class=\"g\"><h1>OFF</h1></button></a></td>"
        "<td><a href=\"/waterraston\"><button class=\"g\"><h1>ON</h1></button></a></td></tr>\n"
        "<tr><td>NAMAKANJE ZUNAJ:</td><td></td><td>" + String((boolean)digitalRead(pin_namakanje_zunaj)) + "</td><td></td>"
        "<td><a href=\"/waterzuoff\"><button class=\"g\"><h1>OFF</h1></button></a></td>"
        "<td><a href=\"/waterzuon\"><button class=\"g\"><h1>ON</h1></button></a></td></tr>\n"
        "<tr><td>BEEPER:</td><td></td><td>" + String((boolean)digitalRead(pin_beeper)) + "</td><td></td>"
        "<td><a href=\"/beeperoff\"><button class=\"g\"><h1>OFF</h1></button></a></td>"
        "<td><a href=\"/beeperon\"><button class=\"g\"><h1>ON</h1></button></a></td></tr>\n"
        "<tr><td>STIKALO:</td><td></td><td>" + String((boolean)digitalRead(pin_led)) + "</td><td></td>"
        "<td><a href=\"/ledoff\"><button class=\"g\"><h1>OFF</h1></button></a></td>"
        "<td><a href=\"/ledon\"><button class=\"g\"><h1>ON</h1></button></a></td></tr>\n"
        "</table><br>\n"
        "<a href=\"/settings\"><button class=\"gb\" style=\"width:100%\"><h1>NASTAVITVE</h1></button></a>\n"
        "</div>\n";
        "</BODY></HTML>\n";
        server.send(200, "text/html", webString);
        delay(100);
    }

    void handle_Settings() {   
        webString="<!DOCTYPE html><HTML><meta charset=\"UTF-8\">\n"
        "<title>   { Nastavitve }   </title>\n"
        "<div>"
        "<style>table,td{font-size:160%;text-align:center;}\n"
        "input[type=text], input[type=password] {  display: block; width: 100%; height: 100%;  line-height: 160px; font-size: 80px;  border: 1px solid #bbb;}\n"
        "</style><table align=\"center\" border=\"0\">\n"
        "<style scoped>.g{width:100px;height:100%}</style>\n"
        "<tr><th colspan=\"4\">NASTAVITVE ZRAČENJA</th></tr>\n"
        "<tr><td>zračenje nad:</td><td><a href=\"/zracm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.zracenje_temp_value) + "°C</td><td><a href=\"/zracp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>histereza</td><td><a href=\"/zrachim\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.zracenje_temp_hist) + "°C</td><td><a href=\"/zrachip\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čas ročnega zrač.:</td><td><a href=\"/zractm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.zracenje_open_time) + "min</td><td><a href=\"/zractp\"><button  style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čs avto. zrač.:</td><td><a href=\"/zracatm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.zracenje_auto_open_time) + "min</td><td><a href=\"/zracatp\"><button  style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n";
        server.sendContent(webString); // poslje paket
        webString="<tr><td>avto. zračenje:</td><td colspan=\"2\">"+ String((boolean)SETTINGS.zracenje_state) + "</td><td><a href=\"/zracam\"><button  style=\"width:100%;height:100%\"><h1>ON/OFF</h1></button></a></td></tr>\n"
        "<tr><th colspan=\"4\">NASTAVITVE NAMAKANJA - RASTLINJAK</th></tr>"
        "<tr><td>namakanje pod:</td><td><a href=\"/namm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_rastlinjak_value) + "%</td><td><a href=\"/namp\"><button  style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>histereza:</td><td><a href=\"/namhim\"><button  style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_rastlinjak_hist) + "%</td><td><a href=\"/namhip\"><button  style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čas ročnega nam.:</td><td><a href=\"/namtm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_rastlinjak_open_time) + "min</td><td><a href=\"/namtp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čas avto. nam.:</td><td><a href=\"/namatm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_rastlinjak_auto_open_time) + "min</td><td><a href=\"/namatp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n";
        server.sendContent(webString); // poslje paket
        webString="<tr><td>avto. namakanje:</td><td colspan=\"2\">"+ String((boolean)SETTINGS.namakanje_rastlinjak_state) + "</td><td><a href=\"/zracam\"><button  style=\"width:100%;height:100%\"><h1>ON/OFF</h1></button></a></td></tr>\n"
        "<tr><th colspan=\"4\">NASTAVITVE NAMAKANJA - VRT</th></tr>"
        "<tr><td>namakanje pod:</td><td><a href=\"/namzm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_zunaj_value) + "%</td><td><a href=\"/namzp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>histereza:</td><td><a href=\"/namzhim\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_zunaj_hist) + "%</td><td><a href=\"/namzhip\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čas ročnega nam.:</td><td><a href=\"/namztm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_zunaj_open_time) + "min</td><td><a href=\"/namztp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>čas avto. nam.:</td><td><a href=\"/namzatm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.namakanje_zunaj_auto_open_time) + "min</td><td><a href=\"/namzatp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n";
        server.sendContent(webString); // poslje paket
        webString = "<tr><td>avto. namakanje:</td><td colspan=\"2\">"+ String((boolean)SETTINGS.namakanje_zunaj_state) + "</td><td><a href=\"/zracam\"><button  style=\"width:100%;height:100%\"><h1>ON/OFF</h1></button></a></td></tr>\n"
        "<tr><th colspan=\"4\">NASTAVITVE TEMP. SENZORJEV</th></tr>"
        "<tr><td>id zunaj:</td><td><a href=\"/tszum\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.ID_ds_temp_zunaj) +"</td><td><a href=\"/tszup\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>id znotraj:</td><td><a href=\"/tsznm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.ID_ds_temp_znotraj) +"</td><td><a href=\"/tsznp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>id tla:</td><td><a href=\"/tstlm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.ID_ds_temp_tla) +"</td><td><a href=\"/tstlp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><th colspan=\"4\">NASTAVITVE KOREKCIJ</th></tr>"
        "<tr><td>light_k:</td><td><a href=\"/light_km\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_light_k) +"</td><td><a href=\"/light_kp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>light_n:</td><td><a href=\"/light_nm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_light_n) +"</td><td><a href=\"/light_np\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>pH_k:</td><td><a href=\"/ph_km\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_ph_k) +"</td><td><a href=\"/ph_kp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>pH_n:</td><td><a href=\"/ph_nm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_ph_n) +"</td><td><a href=\"/ph_np\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>hum_k:</td><td><a href=\"/hum_km\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_hum_k) +"</td><td><a href=\"/hum_kp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>hum_n:</td><td><a href=\"/hum_nm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.soil_hum_n) +"</td><td><a href=\"/hum_np\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>voltage_k:</td><td><a href=\"/voltage_km\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.voltage_k) +"</td><td><a href=\"/voltage_kp\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><td>voltage_n:</td><td><a href=\"/voltage_nm\"><button style=\"width:100px;height:100%\"><h1>-</h1></button></a></td><td>" + String((int)SETTINGS.voltage_n) +"</td><td><a href=\"/voltage_np\"><button style=\"width:100px;height:100%\"><h1>+</h1></button></a></td></tr>\n"
        "<tr><th colspan=\"4\">NASTAVITVE WIFI</th></tr>"
        "<tr><td colspan=\"4\">"
        "</table>";
        server.sendContent(webString); // poslje paket
        webString = "<tr><td><form method='get' action='save'>SSID:</td><td><input name='ssid' value='" + String(SETTINGS.ssid) + "' maxlength='32' type='text'></td></tr>"
        "<tr><td>PSK:</td><td><input name='pass' value='" + String(SETTINGS.password) + "' maxlength='32' pattern='[0-9A-Za-z]{8,30}' type='password' ></td></tr>"
        "<tr><td><label>AP SSID:</label></td><td><input name='ap_ssid' value='" + String(SETTINGS.ap_ssid) + "' maxlength='32' type='text'></td></tr>"
        "<tr><td><label>AP PSK:</label></td><td><input name='ap_pass' value='" + String(SETTINGS.ap_password) + "' maxlength='32' type='text' pattern='[0-9A-Za-z]{8,30}'></td></tr>"
        "<tr><td colspan=\"2\"><button style=\"width:100%;height:150px\" type='submit' style='width:100%'><h1>SHRANI NASTAVITVE</h1></button></form></td></tr>"
        "<tr><td colspan=\"2\"><a href=\"/df\"><button  style=\"width:100%;height:150px\"><h1>NALOŽI PRIVZETO</h1></button></a></td></tr>"
        "</table></td></tr></table>"
        "</div><div>" //potrebno zaradi gumbov!!!
        "<style scoped>.g{width:100%;height:300px}h1{font-size:400%;color:blue;}</style>"
        "<a href=\"/\"><button class=\"g\"><h1>NAZAJ</h1></button></a>"
        "<a href=\"/enapsta\"><button class=\"g\" type='submit'><h1>VKLOP AP+STA</h1></button></a>"
        "<a href=\"/disap\"><button class=\"g\"><h1>SAMO STA NAČIN</h1></button></a>"
        "<a href=\"/recon\"><button class=\"g\"><h1>RECONNECT</h1></button></a>"
        "<a href=\"/reboot\"><button class=\"g\"><h1>RESTART</h1></button></a>"
        "<a href=\"/espupdate\"><button class=\"g\"><h1>OTA UPDATE</h1></button></a>"
        "</div>"
        "</BODY></HTML>\n";
        server.send(200, "text/html", webString);
    }

    // #############################  namakanje rastlinjak  #########################################
    void handle_namakanje_rastlinjak_auto_open_time_dec() {
      if (SETTINGS.namakanje_rastlinjak_auto_open_time > 1) {
        SETTINGS.namakanje_rastlinjak_auto_open_time--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_namakanje_rastlinjak_auto_open_time_inc() {
      if (SETTINGS.namakanje_rastlinjak_auto_open_time < 300) {
        SETTINGS.namakanje_rastlinjak_auto_open_time++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_namakanje_rastlinjak_value_dec() {
        if (SETTINGS.namakanje_rastlinjak_value > 0) {
          SETTINGS.namakanje_rastlinjak_value--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_namakanje_rastlinjak_value_inc() {
        if (SETTINGS.namakanje_rastlinjak_value < 100) {
          SETTINGS.namakanje_rastlinjak_value++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_rastlinjak_histereza_dec() {
        if (SETTINGS.namakanje_rastlinjak_hist > 0) {
          SETTINGS.namakanje_rastlinjak_hist--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_namakanje_rastlinjak_histereza_inc() {
        if (SETTINGS.namakanje_rastlinjak_hist < 100) {
          SETTINGS.namakanje_rastlinjak_hist++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_rastlinjak_open_time_inc() {
        if (SETTINGS.namakanje_rastlinjak_open_time < 1000) {
          SETTINGS.namakanje_rastlinjak_open_time++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_rastlinjak_open_time_dec() {
        if (SETTINGS.namakanje_rastlinjak_open_time > 1) {
          SETTINGS.namakanje_rastlinjak_open_time--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_rastlinjak_water_on() {
      digitalWrite(pin_namakanje_rastlinjak, HIGH);
      namakanje_rastlinjak_previous_millis = millis();
      namakanje_rastlinjak_timer = true;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      Serial.println("Rastlinjak www btn set water=1");
    }

    void handle_rastlinjak_water_off() {
      digitalWrite(pin_namakanje_rastlinjak, LOW);
      namakanje_rastlinjak_previous_millis = millis();
      namakanje_rastlinjak_timer = true;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      Serial.println("Rastlinjak www btn set water=0");
    }    
    
    void handle_namakanje_rastlinjak_manual_auto() {
      if (SETTINGS.namakanje_rastlinjak_state) {
        SETTINGS.namakanje_rastlinjak_state = false;
      }
      else {
        SETTINGS.namakanje_rastlinjak_state = true;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
      Serial.print("Rastlinjak www btn set man/auto=");
      Serial.println(SETTINGS.namakanje_rastlinjak_state);
    }
    
    void NamakanjeRastlinjakFunc() {      
      if (SETTINGS.namakanje_rastlinjak_state and not namakanje_rastlinjak_timer) { // state = true = AUTO
        if (SETTINGS.namakanje_rastlinjak_value + SETTINGS.namakanje_rastlinjak_hist > soil_hum and not namakanje_rastlinjak_auto_timer) {
          digitalWrite(pin_namakanje_rastlinjak, HIGH);
//          Serial.print("Rastlinjak auto ctrl set water=1, hist=");
//          Serial.print(SETTINGS.namakanje_rastlinjak_hist);
//          Serial.print(", value=");
//          Serial.print(SETTINGS.namakanje_rastlinjak_value);
//          Serial.print(", current=");
//          Serial.println(soil_hum);
        }
        if (SETTINGS.namakanje_rastlinjak_value - SETTINGS.namakanje_rastlinjak_hist < soil_hum and not namakanje_rastlinjak_auto_timer) {
          digitalWrite(pin_namakanje_rastlinjak, LOW);
          Serial.print("Rastlinjak auto ctrl set water=0, hist=");
          Serial.print(SETTINGS.namakanje_rastlinjak_hist);
          Serial.print(", value=");
          Serial.print(SETTINGS.namakanje_rastlinjak_value);
          Serial.print(", current=");
          Serial.println(soil_hum);
        }
      }
      if (namakanje_rastlinjak_auto_timer) {
        if (millis() > namakanje_rastlinjak_auto_previous_millis + SETTINGS.namakanje_rastlinjak_auto_open_time * 1000 * 60) {
          namakanje_rastlinjak_auto_timer = false; // za vsako spremembo koraka čaka zracenje_auto_open_time [min]
          //Serial.println("Zracenje auto timer expired!");
        }
      }
      if (namakanje_rastlinjak_timer) {
        if (millis() > namakanje_rastlinjak_previous_millis + SETTINGS.namakanje_rastlinjak_open_time * 1000 * 60) {
          namakanje_rastlinjak_timer = false;
          digitalWrite(pin_namakanje_rastlinjak, LOW);
          Serial.println("Rastlinjak timer expired set water=0");
        }
      }
    }
    // #############################  namakanje zunaj  #########################################
    void handle_namakanje_zunaj_auto_open_time_dec() {
      if (SETTINGS.namakanje_zunaj_auto_open_time > 1) {
        SETTINGS.namakanje_zunaj_auto_open_time--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_namakanje_zunaj_auto_open_time_inc() {
      if (SETTINGS.namakanje_zunaj_auto_open_time < 300) {
        SETTINGS.namakanje_zunaj_auto_open_time++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_namakanje_zunaj_value_dec() {
        if (SETTINGS.namakanje_zunaj_value > 0) {
          SETTINGS.namakanje_zunaj_value--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_namakanje_zunaj_value_inc() {
        if (SETTINGS.namakanje_zunaj_value < 100) {
          SETTINGS.namakanje_zunaj_value++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_zunaj_histereza_dec() {
        if (SETTINGS.namakanje_zunaj_hist > 0) {
          SETTINGS.namakanje_zunaj_hist--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_namakanje_zunaj_histereza_inc() {
        if (SETTINGS.namakanje_zunaj_hist < 100) {
          SETTINGS.namakanje_zunaj_hist++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_zunaj_open_time_inc() {
        if (SETTINGS.namakanje_zunaj_open_time < 1000) {
          SETTINGS.namakanje_zunaj_open_time++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_namakanje_zunaj_open_time_dec() {
        if (SETTINGS.namakanje_zunaj_open_time > 1) {
          SETTINGS.namakanje_zunaj_open_time--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    void handle_zunaj_water_on() {
      digitalWrite(pin_namakanje_zunaj, HIGH);
      namakanje_zunaj_previous_millis = millis();
      namakanje_zunaj_timer = true;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      //Serial.println("Zunaj www btn set water=1");
    }

    void handle_zunaj_water_off() {
      digitalWrite(pin_namakanje_zunaj, LOW);
      namakanje_zunaj_previous_millis = millis();
      namakanje_zunaj_timer = true;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      //Serial.println("Zunaj www btn set water=0");
    }    
    
    void handle_namakanje_zunaj_manual_auto() {
      if (SETTINGS.namakanje_zunaj_state) {
        SETTINGS.namakanje_zunaj_state = false;
      }
      else {
        SETTINGS.namakanje_zunaj_state = true;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
      //Serial.print("Zunaj www btn set man/auto=");
      //Serial.println(namakanje_zunaj_state);
    }
    
    void NamakanjeZunajFunc() {
      if (SETTINGS.namakanje_zunaj_state and not namakanje_zunaj_timer) { // state = true = AUTO
        if (SETTINGS.namakanje_zunaj_value + SETTINGS.namakanje_zunaj_hist > zunaj_soil_hum and not namakanje_zunaj_auto_timer) {
          digitalWrite(pin_namakanje_zunaj, HIGH);
          ////Serial.println("Zunaj auto ctrl set water=1"); 
        }
        if (SETTINGS.namakanje_zunaj_value - SETTINGS.namakanje_zunaj_hist < zunaj_soil_hum and not namakanje_zunaj_auto_timer) {
          digitalWrite(pin_namakanje_zunaj, LOW);
          ////Serial.println("Zunaj auto ctrl set water=0"); 
        }
      }
      if (namakanje_zunaj_auto_timer) {
        if (millis() > namakanje_zunaj_auto_previous_millis + SETTINGS.namakanje_zunaj_auto_open_time * 1000 * 60) {
          namakanje_zunaj_auto_timer = false; // za vsako spremembo koraka čaka zracenje_auto_open_time [min]
          //Serial.println("Zracenje auto timer expired!");
        }
      }
      if (namakanje_zunaj_timer) {
        if (millis() > namakanje_zunaj_previous_millis + SETTINGS.namakanje_zunaj_open_time * 1000 * 60) {
          namakanje_zunaj_timer = false;
          digitalWrite(pin_namakanje_zunaj, LOW);
          //Serial.println("Zunaj timer expired set water=0");
        }
      }
    }
    // #############################  zracenje  #########################################
    void handle_zracenje_temp_dec() {
      if (SETTINGS.zracenje_temp_value > 10) {
        SETTINGS.zracenje_temp_value--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_zracenje_temp_inc() {
      if (SETTINGS.zracenje_temp_value < 50) {
        SETTINGS.zracenje_temp_value++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_zracenje_open_time_dec() {
      if (SETTINGS.zracenje_open_time > 1) {
        SETTINGS.zracenje_open_time--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
     void handle_zracenje_open_time_inc() {
      if (SETTINGS.zracenje_open_time < 300) {
        SETTINGS.zracenje_open_time++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_zracenje_auto_open_time_dec() {
      if (SETTINGS.zracenje_auto_open_time > 1) {
        SETTINGS.zracenje_auto_open_time--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
     void handle_zracenje_auto_open_time_inc() {
      if (SETTINGS.zracenje_auto_open_time < 300) {
        SETTINGS.zracenje_auto_open_time++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_zracenje_histereza_dec() {
      if (SETTINGS.zracenje_temp_hist > 1) {
        SETTINGS.zracenje_temp_hist--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
     void handle_zracenje_histereza_inc() {
      if (SETTINGS.zracenje_temp_hist < 300) {
        SETTINGS.zracenje_temp_hist++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }        
    void handle_zracenje_manual_auto() {
      if (SETTINGS.zracenje_state) {
        SETTINGS.zracenje_state = false;
      }
      else {
        SETTINGS.zracenje_state = true;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_window_close_step() {
      if (myServo > 0) {
        myServo--;
      }
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      zracenje_timer = true; // auto kontrol izklopim za x casa
      zracenje_previous_millis = millis();
      //Serial.print("Zracenje www okno dec, value=");
      //Serial.println(myServo);
    }

    void handle_window_open_step() {
      if (myServo < 5) {
        myServo++;
      }
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      zracenje_timer = true; // auto kontrol izklopim za x casa
      zracenje_previous_millis = millis();
      //Serial.print("Zracenje www okno inc, value=");
      //Serial.println(myServo);
    }
    
    void handle_window_close_full() {
      myServo = 0;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      zracenje_timer = true; // auto kontrol izklopim za x casa
      zracenje_previous_millis = millis();
      //Serial.print("Zracenje www okno close, value=");
      //Serial.println(myServo);
    }

    void handle_window_open_full() {
      myServo = 5;
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);
      zracenje_timer = true; // auto kontrol izklopim za x casa
      zracenje_previous_millis = millis();
      //Serial.print("Zracenje www okno open, value=");
      //Serial.println(myServo); 
    }

    void ZracenjeFunc() {
//      Serial.print("state:");
//      Serial.print(SETTINGS.zracenje_state);
//      Serial.print(" timer:");
//      Serial.print(zracenje_timer);
//      Serial.print(" value:");
//      Serial.print(SETTINGS.zracenje_temp_value);
//      Serial.print(" current:");
//      Serial.print(ds_temp_znotraj);
//      Serial.print(" auto_timer:");
//      Serial.println(zracenje_auto_timer);
      if (SETTINGS.zracenje_state and not zracenje_timer) { // state = true = AUTO
        if (SETTINGS.zracenje_temp_value + SETTINGS.zracenje_temp_hist < ds_temp_znotraj and not zracenje_auto_timer) {
//          Serial.print("servo");
//          Serial.println(myServo);
          if (myServo < 5) {
            myServo++;  // odprem za en korak
          }
          zracenje_auto_timer = true;    // aktiviram timer
          zracenje_auto_previous_millis = millis();
        }
        if (SETTINGS.zracenje_temp_value + SETTINGS.zracenje_temp_hist > ds_temp_znotraj and not zracenje_auto_timer) {
          if (myServo > 0) {
            myServo--;  // zaprem za en korak
          }
          zracenje_auto_timer = true;    // aktiviram timer
          zracenje_auto_previous_millis = millis();
        }
        if (zracenje_auto_timer) {
          if (millis() > zracenje_auto_previous_millis + SETTINGS.zracenje_auto_open_time * 1000 * 60) {
            zracenje_auto_timer = false; // za vsako spremembo koraka čaka zracenje_auto_open_time [min]
            //Serial.println("Zracenje auto timer expired!");
          }
        }
        if (zracenje_timer) {
          if (millis() > zracenje_previous_millis + SETTINGS.zracenje_open_time * 1000 * 60) {
            zracenje_timer = false; // ce rocno odpiramo/zapiramo auto control izklopi za zracenje_open_time[min]
            //Serial.println("Zracenje timer expired!");
          }
        }
      }
      SetServoPosition();
    }
    // #############################  beeper  #########################################  
    void handle_beeper_on() {
      digitalWrite(pin_beeper, HIGH);
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);  
    }
    
    void handle_beeper_off() {
      digitalWrite(pin_beeper, LOW);
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);  
    }
    // #############################  led  #########################################
    void handle_led_on() {
      digitalWrite(pin_led, HIGH);
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);  
    }

    void handle_led_off() {
      digitalWrite(pin_led, LOW);
      webString=webMetaRefreshShowdata;
      server.send(200, "text/html", webString);  
    }    
    // #############################  json  #########################################
    void handle_json() {
       webString="{\"RastSoilHum\": \""+ String((float)soil_hum) + "\", \"RastSoilpH\": \""+ String((float)soil_ph) + "\", \"RastLight\": \""+ String((float)soil_light) +
      "\", \"ZunajSoilHum\": \""+ String((float)zunaj_soil_hum) + "\", \"ZunajSoilpH\": \""+ String((float)zunaj_soil_ph) + "\", \"ZunajLight\": \""+ String((float)zunaj_light) + 
      "\", \"WiFi\": \"" + WiFi.RSSI() + "\", \"Baterija\": \"" +String((float)napetost_baterije) + "\", \"Vcc\": \"" + String((float)napajanje) + 
      "\", \"Uptime\": \"" + String((unsigned long)millis()) + "\", \"FreeHeap\": \"" + ESP.getFreeHeap() + "\", \"RastNamakanje\": \"" + String((boolean)digitalRead(pin_namakanje_rastlinjak)) + 
      "\", \"ZunajNamakanje\": \"" + String((boolean)digitalRead(pin_namakanje_zunaj)) + "\", \"Switch1\": \"" + String((boolean)digitalRead(pin_beeper)) + 
      "\", \"Switch2\": \"" + String((boolean)digitalRead(pin_led)) + "\", \"RastOkna\": \"" + String((int)myServo) + 
      "\", \"TempZunaj\": \"" + String((float)ds_temp_zunaj) + "\", \"TempZnotraj\": \"" + String((float)ds_temp_znotraj) + "\", \"TempTla\": \"" + String((float)ds_temp_tla) + "\" }";
        server.send(200, "text/plain", webString);
    }
    // #############################  temp sensors DS18D20  #########################################
    void handle_temp_sensors() {
      webString="["; 
      for (int i=0; i < DS18B20.getDeviceCount(); i++) {
        webString = webString + "{\"id\": \"" + i + "\", \"temp\": \"" + DS18B20.getTempCByIndex(i) + "\"}";
        if (i+1 < DS18B20.getDeviceCount()) {
          webString = webString + ",";
        }
      }
      webString = webString + "]";
      server.send(200, "text/plain", webString);
    }
//    void handle_temp_sensors() {
//      Serial.print("Device count:");
//      Serial.println(DS18B20.getDeviceCount());
//      byte allAddress [DS18B20.getDeviceCount()][8];
//      byte j=0;                                        // search for one wire devices and
//      webString="[";                                   // copy to device address arrays.
//      while ((j < DS18B20.getDeviceCount()) && (oneWire.search(allAddress[j]))) {        
//        j++;
//      }
//      for (byte i=0; i < j; i++) {
//        Serial.print("Device ");
//        Serial.print(i);
//        webString = webString + "{\"id\": \"" + i + "\", \"addr\": \"";
//        Serial.print(": ");                          
//        printAddress(allAddress[i]);                  // print address from each device address arry.
//        webString = webString + "\", \"temp\": \"" + DS18B20.getTempCByIndex(i) + "\"}";
//        if (i+1 < j) {
//          webString = webString + ",";
//        }
//      }
//      Serial.print("\r\n");
//      webString = webString + "]";
//      server.send(200, "text/plain", webString);
//    }
//    
//    void printAddress(DeviceAddress addr) {
//      byte i;
//      for( i=0; i < 8; i++) {                         // prefix the printout with 0x
//          Serial.print("0x");
//          //webString = webString + "0x";
//          if (addr[i] < 16) {
//            Serial.print('0');                        // add a leading '0' if required.
//            webString = webString + "0";
//          }
//          Serial.print(addr[i], HEX);                 // print the actual value in HEX
//          webString = webString + String(addr[i], HEX);
//          if (i < 7) {
//            Serial.print(", ");
//            webString = webString + "-";
//          }
//        }
//      Serial.print("\r\n");
//    }

    // #############################  raw analog  #########################################
    void handle_raw_analog() {
      //[{"id": "0", "temp": "20.88"}]
      webString = "{\"RastSoilHum\": \""+ String((float)soil_hum) + "\", \"RastSoilpH\": \""+ String((float)soil_ph) + "\", \"RastLight\": \"" + String((float)soil_light) +
      "\", \"RawRastSoilHum\": \""+ String((float)raw_soil_hum) + "\", \"RawRastSoilpH\": \""+ String((float)raw_soil_ph) + "\", \"RawRastLight\": \""+ String((float)raw_soil_light) +
      "\", \"AvgRastSoilHum\": \""+ String((float)avg_soil_hum) + "\", \"AvgRastSoilpH\": \""+ String((float)avg_soil_ph) + "\", \"avgRastLight\": \""+ String((float)avg_soil_light) +
      "\", \"ZunajSoilHum\": \""+ String((float)zunaj_soil_hum) + "\", \"ZunajSoilpH\": \""+ String((float)zunaj_soil_ph) + "\", \"ZunajLight\": \""+ String((float)zunaj_light) +
      "\", \"RawZunajSoilHum\": \""+ String((float)raw_zunaj_soil_hum) + "\", \"RawZunajSoilpH\": \""+ String((float)raw_zunaj_soil_ph) + "\", \"RawZunajLight\": \""+ String((float)raw_zunaj_light) +
      "\", \"AvgZunajSoilHum\": \""+ String((float)avg_zunaj_soil_hum) + "\", \"AvgZunajSoilpH\": \""+ String((float)avg_zunaj_soil_ph) + "\", \"AvgZunajLight\": \""+ String((float)avg_zunaj_light) +
      "\", \"Baterija\": \"" + String((float)napetost_baterije) + "\", \"Vcc\": \"" + String((float)napajanje) +
      "\", \"RawBaterija\": \"" + String((float)raw_napetost_baterije) + "\", \"RawVcc\": \"" + String((float)raw_napajanje) +
      "\", \"AvgBaterija\": \"" + String((float)avg_napetost_baterije) + "\", \"AvgVcc\": \"" + String((float)avg_napajanje) + "\"}";
      server.send(200, "text/plain", webString);
    }
    // #############################  korekcije  #########################################
    void handle_light_km() {
      if (SETTINGS.soil_light_k > 0) {
        SETTINGS.soil_light_k--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_light_kp() {
      if (SETTINGS.soil_light_k < 1000) {
        SETTINGS.soil_light_k++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }


    void handle_light_nm() {
      if (SETTINGS.soil_light_n > 0) {
        SETTINGS.soil_light_n--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_light_np() {
      if (SETTINGS.soil_light_n < 1000) {
        SETTINGS.soil_light_n++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }

    void handle_ph_km() {
      if (SETTINGS.soil_ph_k > 0) {
        SETTINGS.soil_ph_k--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_ph_kp() {
      if (SETTINGS.soil_ph_k < 1000) {
        SETTINGS.soil_ph_k++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }

    void handle_ph_nm() {
      if (SETTINGS.soil_ph_n > 0) {
        SETTINGS.soil_ph_n--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_ph_np() {
      if (SETTINGS.soil_ph_n < 1000) {
        SETTINGS.soil_ph_n++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }

    void handle_hum_km() {
      if (SETTINGS.soil_hum_k > 0) {
        SETTINGS.soil_hum_k--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_hum_kp() {
      if (SETTINGS.soil_hum_k < 1000) {
        SETTINGS.soil_hum_k++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_hum_nm() {
      if (SETTINGS.soil_hum_n > 0) {
        SETTINGS.soil_hum_n--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_hum_np() {
      if (SETTINGS.soil_hum_n < 1000) {
        SETTINGS.soil_hum_n++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_voltage_km() {
      if (SETTINGS.voltage_k > 0) {
        SETTINGS.voltage_k--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_voltage_kp() {
      if (SETTINGS.voltage_k < 1000) {
        SETTINGS.voltage_k++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_voltage_nm() {
      if (SETTINGS.voltage_n > 0) {
        SETTINGS.voltage_n--;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }
    void handle_voltage_np() {
      if (SETTINGS.voltage_n < 1000) {
        SETTINGS.voltage_n++;
      }
      webString=webMetaRefreshSettings;
      server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_zunaj_id_dec() {
        if (SETTINGS.ID_ds_temp_zunaj > 0) {
          SETTINGS.ID_ds_temp_zunaj--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_zunaj_id_inc() {
        if (SETTINGS.ID_ds_temp_zunaj < DS18B20.getDeviceCount() - 1) {
          SETTINGS.ID_ds_temp_zunaj++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_znotraj_id_dec() {
        if (SETTINGS.ID_ds_temp_znotraj > 0) {
          SETTINGS.ID_ds_temp_znotraj--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_znotraj_id_inc() {
        if (SETTINGS.ID_ds_temp_znotraj < DS18B20.getDeviceCount() - 1) {
          SETTINGS.ID_ds_temp_znotraj++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_tla_id_dec() {
        if (SETTINGS.ID_ds_temp_tla > 0) {
          SETTINGS.ID_ds_temp_tla--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    
     void handle_temp_senzor_tla_id_inc() {
        if (SETTINGS.ID_ds_temp_tla < DS18B20.getDeviceCount() - 1) {
          SETTINGS.ID_ds_temp_tla++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }

    void handle_temp_senzor_tla_zunaj_id_dec() {
        if (SETTINGS.ID_ds_temp_tla_zunaj > 0) {
          SETTINGS.ID_ds_temp_tla_zunaj--;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    
     void handle_temp_senzor_tla_zunaj_id_inc() {
        if (SETTINGS.ID_ds_temp_tla_zunaj < DS18B20.getDeviceCount() - 1) {
          SETTINGS.ID_ds_temp_tla_zunaj++;
        }
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    
    void handle_reboot() {
        ESP.restart();
        webString=webMetaRefreshSettings;
        server.send(200, "text/html", webString);
    }
    
    void SetServoPosition() {
      if (myServo != myServoHistory) { // samo ce gre za spremembo
        // Using map() function to mapping 0-1023 to 0-179 position. 
        // Why 0-179 (180)? because our Servo just can rotation from 0 to 180 degree. NOTICE: NOT 0-360 DEGREE.
        int pos;
        if (myServo == 0) {
          pos = 0;
        }
        if (myServo == 1) {
          pos = 36;
        }
        if (myServo == 2) {
          pos = 72;
        }
        if (myServo == 3) {
          pos = 108;
        }
        if (myServo == 4) {
          pos = 144;
        }
        if (myServo == 5) {
          pos = 179;
        }
        //pos = map(pos, 0, 1023, 0, 179);
        //Serial.print("Srv pos: ");
        //Serial.println(pos);
        myservo.write(pos);
        myServoHistory = myServo;      
      }
    }

    void Read_DS18B20 () {
      DS18B20.requestTemperatures();
      ds_temp_zunaj = DS18B20.getTempCByIndex(SETTINGS.ID_ds_temp_zunaj);
      ds_temp_znotraj = DS18B20.getTempCByIndex(SETTINGS.ID_ds_temp_znotraj);
      ds_temp_tla = DS18B20.getTempCByIndex(SETTINGS.ID_ds_temp_tla);
      ds_temp_tla_zunaj = DS18B20.getTempCByIndex(SETTINGS.ID_ds_temp_tla_zunaj);
    }

    float AverageFunction(float value, int index_t) {
      // subtract the last reading:
      Average_total[index_t] = Average_total[index_t] - Average_readings[Average_readIndex];
      // read from the sensor:
      Average_readings[Average_readIndex] = value;
      // add the reading to the total:
      Average_total[index_t] = Average_total[index_t] + Average_readings[Average_readIndex];
      // advance to the next position in the array:
//      Average_readIndex = index + 1;
//      // if we're at the end of the array...
//      if (index >= Average_numReadings - index) {
//        // ...wrap around to the beginning:
//        index = 0;
//      }
      // calculate the average:
      return Average_total[index_t] * 8/ Average_numReadings;
    }
    
    void ReadADC() {
      // A0 = Vcc (5V)
      digitalWrite(pin_select_s0, LOW);
      digitalWrite(pin_select_s1, LOW);
      digitalWrite(pin_select_s2, LOW);
      //k = 41.0, n = 0
      //raw_napajanje = (float) analogRead(A0);
      //avg_napajanje = AverageFunction(raw_napajanje,0);
      raw_napajanje = AverageFunction((float) analogRead(A0),0);
      Average_readIndex ++;
      napajanje = raw_napajanje / SETTINGS.voltage_k + SETTINGS.voltage_n;
      // A3 = Bat (12V)
      digitalWrite(pin_select_s0, HIGH);
      digitalWrite(pin_select_s1, HIGH);
      digitalWrite(pin_select_s2, LOW);
      //raw_napetost_baterije = (float) analogRead(A0);
      //avg_napetost_baterije = AverageFunction(raw_napetost_baterije,1);
      raw_napetost_baterije = AverageFunction((float) analogRead(A0),1);
      Average_readIndex ++;
      napetost_baterije = raw_napetost_baterije / SETTINGS.voltage_k + SETTINGS.voltage_n;
      // A1 = Rastlinjak Soil Humidity
      digitalWrite(pin_select_s0, HIGH);
      digitalWrite(pin_select_s1, LOW);
      digitalWrite(pin_select_s2, LOW);
      //raw_soil_hum = (float) analogRead(A0);
      //avg_soil_hum = AverageFunction(raw_soil_hum,2);
      raw_soil_hum = AverageFunction((float) analogRead(A0),2);
      Average_readIndex ++;
      // y = kx + n
      soil_hum = raw_soil_hum / SETTINGS.soil_hum_k + SETTINGS.soil_hum_n;
      // A2 = Rastlinjak Soil pH
      digitalWrite(pin_select_s0, LOW);
      digitalWrite(pin_select_s1, HIGH);
      digitalWrite(pin_select_s2, LOW);
      //raw_soil_ph = (float) analogRead(A0);
      //avg_soil_ph = AverageFunction(raw_soil_ph,3);
      raw_soil_ph = AverageFunction((float) analogRead(A0),3);
      Average_readIndex ++;
      soil_ph = SETTINGS.soil_ph_k / (raw_soil_ph + SETTINGS.soil_ph_n);
      // A4 = Rastlinjak Light
      digitalWrite(pin_select_s0, LOW);
      digitalWrite(pin_select_s1, LOW);
      digitalWrite(pin_select_s2, HIGH);
      //raw_soil_light = (float) analogRead(A0);
      //avg_soil_light = AverageFunction(raw_soil_light,4);
      raw_soil_light = AverageFunction((float) analogRead(A0),4);
      Average_readIndex ++;
      soil_light = raw_soil_light * 10 / SETTINGS.soil_light_k + SETTINGS.soil_light_n;
      // A7 = Zunaj Soil Humidity
      digitalWrite(pin_select_s0, HIGH);
      digitalWrite(pin_select_s1, HIGH);
      digitalWrite(pin_select_s2, HIGH);
      //raw_zunaj_soil_hum = (float) analogRead(A0);
      //avg_zunaj_soil_hum = AverageFunction(raw_zunaj_soil_hum,5);
      raw_zunaj_soil_hum = AverageFunction((float) analogRead(A0),5);
      Average_readIndex ++;
      zunaj_soil_hum = raw_zunaj_soil_hum / SETTINGS.soil_hum_k + SETTINGS.soil_hum_n;
      // A5 = Zunaj Soil pH
      digitalWrite(pin_select_s0, HIGH);
      digitalWrite(pin_select_s1, LOW);
      digitalWrite(pin_select_s2, HIGH);
      //raw_zunaj_soil_ph = (float) analogRead(A0);
      //avg_zunaj_soil_ph = AverageFunction(raw_zunaj_soil_ph,6);
      raw_zunaj_soil_ph = AverageFunction((float) analogRead(A0),6);
      Average_readIndex ++;
      //zunaj_soil_ph = raw_zunaj_soil_ph / SETTINGS.soil_ph_k + SETTINGS.soil_ph_n;
      zunaj_soil_ph = SETTINGS.soil_ph_k / (raw_zunaj_soil_ph + SETTINGS.soil_ph_n);
      // A6 = Zunaj Light
      digitalWrite(pin_select_s0, LOW);
      digitalWrite(pin_select_s1, HIGH);
      digitalWrite(pin_select_s2, HIGH);
      //raw_zunaj_light = (float) analogRead(A0);
      //avg_zunaj_light = AverageFunction(raw_zunaj_light,7);
      raw_zunaj_light = AverageFunction((float) analogRead(A0),7);
      Average_readIndex ++;
      //zunaj_light = raw_zunaj_light / SETTINGS.soil_light_k + SETTINGS.soil_light_n;
      zunaj_light = raw_zunaj_light * 10 / SETTINGS.soil_light_k + SETTINGS.soil_light_n;
      if (Average_readIndex >= Average_numReadings) {
        // ...wrap around to the beginning:
        Average_readIndex = 0;
      }
    }

    void esp_info() {
      webString = "ESP.getBootMode() " + String(ESP.getBootMode()) + "\nESP.getSdkVersion() " + ESP.getSdkVersion() + "\nESP.getBootVersion() " + ESP.getBootVersion() + "\nESP.getChipId() " +
      ESP.getChipId() + "\nESP.getFlashChipSize() " + ESP.getFlashChipSize() + "\nESP.getFlashChipRealSize() " + ESP.getFlashChipRealSize() + "\nESP.getFlashChipSizeByChipId() " +
      ESP.getFlashChipSizeByChipId() + "\nESP.getFlashChipId() " + ESP.getFlashChipId();
      server.send(200, "text/html", webString);
      Serial.println(webString);
    }
    
//    void Read_DHT() {
//      // Wait at least 2 seconds seconds between measurements.
//      // if the difference between the current time and last time you read
//      // the sensor is bigger than the interval you set, read the sensor
//      // Works better than delay for things happening elsewhere als  
//      // Reading temperature for humidity takes about 250 milliseconds!
//      // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
//      dht_hum = dht.readHumidity();          // Read humidity (percent)
//      dht_temp = dht.readTemperature();     // Read temperature as Fahrenheit
//      // Check if any reads failed and exit early (to try again).
//      if (isnan(dht_hum) || isnan(dht_temp)) {
//        //Serial.println("Failed to read from DHT sensor!");
//        return;
//      }
//    }
