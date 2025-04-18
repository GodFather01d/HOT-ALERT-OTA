  #include <WiFiManager.h>  
  #include <EEPROM.h>
  #include <FirebaseESP8266.h>
  #include <addons/TokenHelper.h>
  #include <addons/RTDBHelper.h>
  #include <Ticker.h>
  #include <WiFiUdp.h>
  #include <NTPClient.h>
  #include <TimeLib.h>
  #include <WiFiClientSecure.h>
  #include <ESP8266httpUpdate.h>
  #include <ESP8266HTTPClient.h>

  #define API_KEY "AIzaSyCccXF84hXZ5eS5bsNVQExAfMPviOBaKMc"
  #define DATABASE_URL "temprature-controller-default-rtdb.asia-southeast1.firebasedatabase.app/"

  const char* JT_UPDATE_URL = "https://jyotiautomation.com/backend/services/";


  #define wifiLed 2                 //   D4     wifi led pin  
  #define TRIGGER_PIN 14           //    D5     WIFI SETUP BUTTON PIN 



  WiFiClientSecure client;  
  WiFiClientSecure secureClient;

  #define EEPROM_SIZE 512

  #define sys_on_time 20
  #define sys_off_time 15
  #define SYS_ID_ADDR 0

  #define EEPROM_UPDATE 30
  #define UPDATE_LINK_ADDER 35
  
  char sys_id[10]; 
  float on_time;
  float off_time;
  
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;
  bool signupOK = false;
  volatile bool buttonPressed = false;
   void IRAM_ATTR handleButtonPress() {
      // Set flag to indicate button press
      buttonPressed = true;
    }
  const long utcOffsetInSeconds = 19800;
  char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

  Ticker timer,timer1;
  unsigned int sys_ON = 0,sys_OFF = 0, sys_PANIC_OFF = 0 ,sys_PANIC_ON = 0 ,hours_ontime = 0,mint_ontime = 0,hours_offtime = 0,notify = 0,DOOR_OPEN = 0;;
  unsigned int mint_offtime = 0, ontime_flag = 0, offtime_flag = 0, time_hours = 0, time_mint = 0, panic = 0, SYS_STATE,RESET = 0, panic_counter = 0;
  unsigned int restart = 0,SET_HUMD = 0,currentYear = 0,currentMonth = 0,currentDay = 0,PANIC_DELAY = 5;
  unsigned int online_flag = 0, offline_flag = 0, online_flag2 = 0, offline_flag2 = 0,ALERT_SENSOR = 0;
  float SET_TEMP = 0,ALERT_TEMP = 0,ALERT_HUMD = 0;
  WiFiManager wm;
  unsigned char update_ota = 0,webupdate_flag = 0,JT_UPDATE_UNSUCCESS_FLAG = 0,sendhistory_flag = 0,send_noti_flag = 0,doorNumber = 0;
  unsigned int ON_OFF_STATE = 0;
  
  void setup()
  {
    
    pinMode(wifiLed ,OUTPUT);
    pinMode(TRIGGER_PIN ,INPUT_PULLUP);
    EEPROM.begin(EEPROM_SIZE);
    update_ota =  EEPROM.read(EEPROM_UPDATE);
    readSysIdFromEEPROM();

    wm.setConfigPortalTimeout(10);
    wm.autoConnect();
    
    Serial.begin(9600);
    delay(1000);
    if(WiFi.status() == WL_CONNECTED ) 
     {
      if(update_ota == 1) 
      {
        String link = readUpdateLinkFromEEPROM();
        EEPROM.put(EEPROM_UPDATE, 0);
        EEPROM.commit();
        checkForUpdates(link);
      }
      else
      {
         String link = readUpdateLinkFromEEPROM();
         delay(500);
         Serial.println(link);
         EEPROM.put(EEPROM_UPDATE, 0);
         EEPROM.commit();
         makeHttpRequest(link);
         if(JT_UPDATE_UNSUCCESS_FLAG == 1)
         {
           makeHttpRequest(link);
           
         }
          if(JT_UPDATE_UNSUCCESS_FLAG == 1)
         {
           makeHttpRequest(link);
           
         }
        
 
      }
      
    }
    if(WiFi.status() == WL_CONNECTED ) 
    { 
      timeClient.begin();
      config.api_key = API_KEY;
      config.database_url = DATABASE_URL;
      if (Firebase.signUp(&config, &auth, "", ""))
      {
        signupOK = true;
        config.token_status_callback = tokenStatusCallback; 
        Firebase.begin(&config, &auth);
        Firebase.reconnectWiFi(true);
        Firebase.reconnectNetwork(true); 
        delay(1000);
        String path = "SYSTEM/" + String (sys_id) + "/JT_SERVER_LINK";

        
      }
      else 
      {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
      }
      digitalWrite(wifiLed, LOW);  

      delay(1000);
    }
    else
    {
      digitalWrite(wifiLed, HIGH); 

    }
    timer.attach(60*5, task);
    attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), handleButtonPress, CHANGE);

  }

  void task()
  {
    if(WiFi.status() == WL_CONNECTED) 
    {
      Firebase.begin(&config, &auth);
    }
  }
  void loop()
  {
   if (buttonPressed || digitalRead(TRIGGER_PIN) == LOW)
     {
     // delay(500); 
        buttonPressed = false;

        setupConfigPortal();
      
     }
    
        delay(500);
        settime();
        delay(500);
        readata();
        delay(500);
        handledata();
        delay(500);
     
     if(RESET == 1)
     {
      
        ESP.restart();
     }
     
  }
  void settime()
  {
    if(buttonPressed)
    {
      return;
    }
    if(WiFi.status() == WL_CONNECTED) 
    {
      online_flag2 = 1;
      timeClient.update();
      time_hours = timeClient.getHours();
      time_mint = timeClient.getMinutes();
      unsigned long epochTime = timeClient.getEpochTime();
      currentYear = year(epochTime);
      currentMonth = month(epochTime);
      currentDay = day(epochTime);
    }
  }
  void sendhistory(String Path)
  {
    if(buttonPressed)
    {
      return;
    }
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/MONTH", currentMonth);  
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/DAY", currentDay);  
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/YEAR", currentYear); 
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/HR", time_hours); 
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/MIN", time_mint); 
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_OFF", 1);
    if(Path == "DOOR_HISTORY")
    {
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/" + Path + "/DOOR_OPEN", doorNumber);  
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/DOOR_OPEN", doorNumber);
    }
      if(Path == "TEMP_HISTORY")
    {

      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ Path +"/TEMP", ALERT_TEMP);  
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_TEMP", 1);
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/DOOR_OPEN", 0);
      
    }
    if(Path == "HUMD_HISTORY")
    {
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/"+ Path +"/TEMP", ALERT_HUMD);  
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_HUMD", 1);
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/DOOR_OPEN", 0);
      
    }
    if(Path == "SMOKE_HISTORY")
    {
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/SENSOR_ALERT", 1);
    }
      if(Path == "WATER_HISTORY")
    {
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/SENSOR_ALERT", 2);
    }
    sendhistory_flag = 0;
    
  }
  void sendnotification(String Path)
  {
    if(buttonPressed)
    {
      return;
    }
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/HR", time_hours);
    Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/MIN", time_mint);
    
    if(Path == "DOOR_OPEN")
    {
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/DOOR_NOTI", doorNumber);
    }
    if(Path == "EXCEED_TEMP")
    {
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/EXCEED_TEMP", 1);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/TEMP", ALERT_TEMP);

    }
    if(Path == "EXCEED_HUMD")
    {
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/EXCEED_HUMD", 1);
        Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/TEMP", ALERT_HUMD);

    }
    else
    {
      Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/" + Path + "/SENSOR_ALERT_NOTI", ALERT_SENSOR);
    }
          
    send_noti_flag = 0;
  }
  void handledata()
   {
    if(WiFi.status() == WL_CONNECTED) 
    {
     if(Serial.available()) 
      {
        String s =  Serial.readStringUntil('\n');
        char inputCharArray[s.length() + 1];
        s.toCharArray(inputCharArray, s.length() + 1);

         if (strncmp(inputCharArray, "JT_DOOR", 7) == 0) {
        // Extract the door number (assuming it's always in the form "XXX")
        doorNumber = atoi(inputCharArray + 8); // Extract digits starting from position 8

        // Check if the number is between 1 and 25
        if (doorNumber >= 1 && doorNumber <= 99) {


            // Firebase set operation including the full input string in the path
         
            send_noti_flag = 1;
            sendhistory_flag = 1;
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_TEMP", 0);
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_HUMD", 0);

        }
       }

       if (strncmp(inputCharArray, "JT_CUR", 6) == 0) {
          char *tempPart = strtok(inputCharArray + 6, "_"); // Extract temperature part (e.g., "225")
          char *humidityPart = strtok(NULL, "_");     
    
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/DOOR_OPEN", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_HUMD", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_TEMP", 0);

          if (tempPart != NULL && humidityPart != NULL) {
              // Convert extracted values to appropriate types
              float temp = atoi(tempPart) / 10.0;  // Convert "225" to 22.5
              int humidity = atoi(humidityPart);  

              // Update time to ensure timestamp accuracy
              timeClient.update();
      
              // Update the temperature in Firebase
              if (!Firebase.setFloat(fbdo, "SYSTEM/" + String(sys_id) + "/CURRENT_VALUES/TEMP", temp)) {
                  Serial.println("Failed to update TEMP in Firebase");
                //  Serial.println(fbdo.errorReason());
              } else {
                  Serial.println("Temperature updated successfully in Firebase: " + String(temp));
              }
      
              // Update the humidity in Firebse
              if (!Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/CURRENT_VALUES/HUMD", humidity)) {
                  Serial.println("Failed to update HUMIDITY in Firebase");
                 Serial.println(fbdo.errorReason());
              } else {
                 Serial.println("Humidity updated successfully in Firebase: " + String(humidity));
                 Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_OFF", 0);
                 Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/SENSOR_ALERT", 0);
              }

          } 
          else 
          {
              Serial.println("Invalid signal format.");
          }
        }

        if (strncmp(inputCharArray, "JT_UP", 5) == 0) {
          char *tempPart = strtok(inputCharArray + 5, "_"); // Extract temperature part (e.g., "225")
          char *humidityPart = strtok(NULL, "_");     
          char *alert = strtok(NULL, "_");          // Extract humidity part (e.g., "50")
          

          if (tempPart != NULL && humidityPart != NULL) {
              // Convert extracted values to appropriate types
              ALERT_TEMP = atoi(tempPart) / 10.0;  // Convert "225" to 22.5
              ALERT_HUMD = atoi(humidityPart);  
              ALERT_SENSOR = atoi(alert);   // Convert "50" to 50
              timeClient.update();
             if (!Firebase.setFloat(fbdo, "SYSTEM/" + String(sys_id) + "/CURRENT_VALUES/TEMP", ALERT_TEMP)) 
               {
                  Serial.println("Failed to update TEMP in Firebase");
                // Serial.println(fbdo.errorReason());
               } else {
                  Serial.println("Temperature updated successfully in Firebase: " + String(ALERT_TEMP));
               }
      
              // Update the humidity in Firebse
              if (!Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/CURRENT_VALUES/HUMD", ALERT_HUMD)) {
                  Serial.println("Failed to update HUMIDITY in Firebase");
                  Serial.println(fbdo.errorReason());
               } else {
                 Serial.println("Humidity updated successfully in Firebase: " + String(ALERT_HUMD));
               }

              if(SET_TEMP <= ALERT_TEMP)
              {
           
                  sendhistory_flag = 2;
                  send_noti_flag = 2;
              }
              else
              {
                Firebase.setFloat(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_TEMP", 0);
              }
              if(SET_HUMD <= ALERT_HUMD)
              {
                 
                  sendhistory_flag = 3;
                  send_noti_flag = 3;
                
              }
              else
              {
                Firebase.setFloat(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_HUMD", 0);
              }
              if(ALERT_SENSOR == 1)
              {

                  sendhistory_flag = 4;
                  send_noti_flag = 4;
              }
              if(ALERT_SENSOR == 2 )
              {
                  sendhistory_flag = 5;
                  send_noti_flag = 5;
              }

              String url = JT_UPDATE_URL + String(sys_id) + "/" + String(ALERT_TEMP) + "/" + String(SET_TEMP) +"/" + String(ALERT_HUMD) + "/" + String(SET_HUMD) +"/" + String(ON_OFF_STATE) + "/" + String(ALERT_SENSOR);
              String path = "SYSTEM/" + String (sys_id) + "/JT_SERVER_LINK";
              bool responce = Firebase.setString(fbdo, path, url);
              Serial.println("Link fetched successfully: " + url);
              Serial.println("Free Heap Before: " + String(ESP.getFreeHeap()));
              makeHttpRequest(url);
          } 
          else 
          {
              Serial.println("Invalid signal format.");
          }
    
        }
       if (strcmp(inputCharArray, "JT_STS") == 0) 
        {
          Serial.println("JT_ONLINE");
        }
       if (strcmp(inputCharArray, "JT_SIREN_OFF") == 0) 
        {
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/DOOR_OPEN", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_HUMD", 0);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ALERT/ALERT_TEMP", 0);
           Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SIREN_OFF", 0);
        }
        if (strcmp(inputCharArray, "JT_PWRON") == 0) 
        {
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_ON/HR", time_hours);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_ON/MIN", time_mint);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_ON/POWER_ON", 1);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ON_OFF_STATE", 1);

        }
        if (strcmp(inputCharArray, "JT_PWR_FAIL") == 0) 
        {
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_OFF/HR", time_hours);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_OFF/MIN", time_mint);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/PWR_OFF/POWER_OFF", 1);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ON_OFF_STATE", 0);

        }
      
        if (strcmp(inputCharArray, "JT_LOWBAT") == 0) 
        {
           Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_LOW/HR", time_hours);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_LOW/MIN", time_mint);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_LOW/BATTERY_LOW", 1);
        }
         if (strcmp(inputCharArray, "JT_BATOK") == 0) 
        {
           Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_OK/HR", time_hours);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_OK/MIN", time_mint);
          Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/BAT_OK/BATTERY_OK", 1);
        }

      }

    }
    else
    {
      if(Serial.available()) 
      {
        String s =  Serial.readStringUntil('\n');
        char inputCharArray[s.length() + 1];
        s.toCharArray(inputCharArray, s.length() + 1);
        if (strcmp(inputCharArray, "JT_STS") == 0) 
        {
          delay(3000);
          Serial.println("JT_OFFLINE");
        }
      }
    }
  }


  void readata()
  {
    if (WiFi.status() == WL_CONNECTED) 
      {
        if(buttonPressed)
        {
          return;
        }
        digitalWrite(wifiLed, LOW);
        
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/SYSTEM_ON_OFF_TIME/RESET"))
        {
          RESET = fbdo.intData();
        } 
         if (Firebase.getFloat(fbdo,"SYSTEM/" + String(sys_id) + "/SET_TEMP_HUMD/TEMP_SET"))
        {
          SET_TEMP = fbdo.floatData();
        }
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/SET_TEMP_HUMD/HUMIDITY_SET"))
        {
          SET_HUMD = fbdo.intData();
        }
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/ON_OFF_STATE"))
        {
          ON_OFF_STATE = fbdo.intData();
        }
        if(buttonPressed)
        {
          return;
        }
         if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/SET_TEMP_HUMD/SET_BUTTON"))
        {
          int val = fbdo.intData();
          if(val == 1)
          {
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/SET_TEMP_HUMD/SET_BUTTON", 0);
            int settemp = SET_TEMP * 10;
            String setdata_temp = "SET_" + String(settemp) + "_" + String(SET_HUMD);
            Serial.println();
            Serial.println(setdata_temp);
            timeClient.update();
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/SET_VALUES/HR", time_hours);
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/SET_VALUES/MIN", time_mint);
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/SET_VALUES/TEMP", SET_TEMP);
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/SET_VALUES/HUMD", SET_HUMD);
            Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/NOTIFICATION/SET_VALUES/SET_VALUES_NOTI",1);
          }
        }
        if (Firebase.getInt(fbdo,"SYSTEM/" + String(sys_id) + "/UPDATE")) 
          {
            int value = fbdo.intData();
            EEPROM.put(EEPROM_UPDATE, value);
          //  Serial.print("update : " + value);

            if(value == 1)
            {
              String path = "SYSTEM/UPDATE_LINK";
             if (Firebase.getString(fbdo, path)) {
              if (fbdo.dataType() == "string") {
                String link = fbdo.stringData(); // Store the string from Firebase
                Serial.println("Link fetched successfully: " + link);
            
                // Convert the String to a char array
                char linkArray[link.length() + 1]; // Ensure enough space for null terminator
                link.toCharArray(linkArray, sizeof(linkArray));
            
                // Write the char array to EEPROM
                for (int i = 0; i < link.length() && i < (EEPROM_SIZE - UPDATE_LINK_ADDER); i++) {
                  EEPROM.write(UPDATE_LINK_ADDER + i, linkArray[i]);
                }
                EEPROM.write(UPDATE_LINK_ADDER + link.length(), '\0'); // Null-terminate the stored string
              } else {
                Serial.println("The data type is not a string!");
              }
            } else {
              Serial.println("Failed to fetch update link: " + fbdo.errorReason());
            }

              String update_time = (String(time_hours)+" : "+String(time_mint)+ " d : " );
              String update_date = (String(currentDay)+" / "+String(currentMonth)+" / "+String(currentYear));
              Firebase.setString(fbdo, "SYSTEM/" + String(sys_id) + "/LAST_UPDATE_TIME/",update_time + update_date);
              Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/UPDATE", 0);
              EEPROM.commit();
              delay(1000);
              ESP.restart();
            }
    
          }
        if(buttonPressed)
        {
          return;
        }
        bool status = Firebase.setInt(fbdo, "SYSTEM/" + String(sys_id) + "/ONLINE_STATUS", 1);
        if (status == 0)
        {
          restart++;
        }
        else
        {
          restart = 0;
        }
        if(restart >= 3)
        {
          ESP.restart();
        }
        if(currentYear > 2000)
        {
          if(sendhistory_flag == 1)
          {
            sendhistory("DOOR_HISTORY");
          }
          if(sendhistory_flag == 2)
          {
            sendhistory("TEMP_HISTORY");
          }
          if(sendhistory_flag == 3)
          {
            sendhistory("HUMD_HISTORY");
          }
          if(sendhistory_flag == 4)
          {
            sendhistory("SMOKE_HISTORY");
          }
          if(sendhistory_flag == 5)
          {
            sendhistory("WATER_HISTORY");
          }
          if(send_noti_flag == 1)
          {
            sendnotification("DOOR_OPEN");
          }
          if(send_noti_flag == 2)
          {
            sendnotification("EXCEED_TEMP");
          }
          if(send_noti_flag == 3)
          {
            sendnotification("EXCEED_HUMD");
          }
          if(send_noti_flag == 4 || send_noti_flag == 5)
          {
            sendnotification("SENSOR_ALERT");
          }
        }
    
      }
      else
      {
          digitalWrite(wifiLed, HIGH);  
    
      }
  }

 void setupConfigPortal() 
 {
  for(int i = 0;i<= 5;i++)
  {
    digitalWrite(wifiLed,HIGH);
    delay(500);
    digitalWrite(wifiLed,LOW);
    delay(500);
  }
  WiFiManagerParameter sys_id_param("sys_id", "System ID", sys_id, 32);
  wm.addParameter(&sys_id_param);
  wm.setConfigPortalTimeout(180);

  if (!wm.startConfigPortal("JT_WIFI_SETUP","Jyoti@2000") )
  {
   Serial.println("Failed to connect through Config Portal");
  }
  strcpy(sys_id, sys_id_param.getValue());
  for (int i = 0; i < sizeof(sys_id); i++) 
  {
    EEPROM.write(SYS_ID_ADDR + i, sys_id[i]);
  }
  EEPROM.commit();
  for(int i = 0;i<= 5;i++)
  {
    digitalWrite(wifiLed,HIGH);
    delay(500);
    digitalWrite(wifiLed,LOW);
    delay(500);
  }
  ESP.restart();
 }
 unsigned long startTime;
 const unsigned long timeout = 20000;
 void checkForUpdates(String GITHUB_BIN_URL) {
    Serial.println("Checking for updates...");
  
    client.setInsecure();
  
  
    // Start the update process and check for timeout
    startTime = millis();
    t_httpUpdate_return result = ESPhttpUpdate.update(client, GITHUB_BIN_URL);
  
  
    if (millis() - startTime >= timeout) {
      Serial.println("Update timeout occurred");
    }
  
    switch (result) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("Update failed. Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
  
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("No updates available.");
        break;
  
      case HTTP_UPDATE_OK:
        Serial.println("Update successful. Rebooting...");
        break;
    
    }
  }


  
  void makeHttpRequest(String serverName) 
  {
    
      WiFiClientSecure wifiClient; 
      wifiClient.setInsecure(); 
      HTTPClient http;
      Serial.println("\nSending HTTP GET request...");
      if (http.begin(wifiClient, serverName)) 
      { 
        int httpResponseCode = http.GET(); // Send the request
        if (httpResponseCode > 0) 
        {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.println("Response payload: ");
         // Serial.println(payload);
  
          if (payload.indexOf("successfully added") != -1) 
          {
            Serial.println("The response indicates success!");
            JT_UPDATE_UNSUCCESS_FLAG = 0;
          }
          else 
          {
            Serial.println("Unexpected response.");
            JT_UPDATE_UNSUCCESS_FLAG = 1;
          }
        } 
        else 
        {
          Serial.print("Error on HTTP request: ");
          Serial.println(http.errorToString(httpResponseCode));
          JT_UPDATE_UNSUCCESS_FLAG = 1;
        }
  
        http.end(); // Free resources
      } 
      else 
      {
        Serial.println("Unable to connect to server");
        JT_UPDATE_UNSUCCESS_FLAG = 1;
      }
     
  
  }

 void readSysIdFromEEPROM() 
  {
    for (int i = 0; i < sizeof(sys_id); i++) 
    {
      sys_id[i] = EEPROM.read(SYS_ID_ADDR + i);
    }          
    sys_id[sizeof(sys_id) - 1] = '\0'; 
  }
    String readUpdateLinkFromEEPROM() {
    char linkBuffer[EEPROM_SIZE - UPDATE_LINK_ADDER];
    int i = 0;
      for (; i < (EEPROM_SIZE - UPDATE_LINK_ADDER); i++) {
        linkBuffer[i] = EEPROM.read(UPDATE_LINK_ADDER + i);
        if (linkBuffer[i] == '\0') break;
      }

   
      linkBuffer[i] = '\0';
      return String(linkBuffer);
  }
