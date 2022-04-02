/*    
      Build information:  Used chip: ESP32-D0WDQ6-V3 (revision 3)
                          Used programm memory 1205606/1966080  Bytes (61%) 
                          Used memory for globale variabel 46692 Bytes (12%)
                          Setting "Minimal SPIFF (1.9MB APP / with OTA/190KB SPIFF)
                          Still free memory for local variable 280988 Bytes (Max 327680 Bytes)
      
      Feature:            (x) Webpage 
                          (x) Wifi Lifecycle
                          (in progress) Configuration management (BeeSensors)
                          (x) Configuration management (MQTT)
                          ( ) MQTT
                          (in progress) Vibration Sensor (ADXL345)
                          ( ) Weight Sensor (HX711)
                          (in progress) Temperature Sensor (DS)
                          ( ) Humadity Sensor ()
                          (in progress) RTC (DS3231)
                          ( ) Power management
                          (in progress) Deep Sleep (ESP)
                          ( ) Lora Communication
                          ( ) SIM Communication
                          (in progress) SD Card
                          
      Scenario supported: (X) Always On with webserver
                          ( ) Sleep on always power
                          ( ) Sleep on battery
                          ( ) One time startup
*/

// ------------------------------ Includes ------------------------------------------
#include <Wire.h>                   // ADXL345, DS3231
#include <Adafruit_Sensor.h>        // ADXL345
#include <Adafruit_ADXL345_U.h>     // ADXL345
#include <AutoConnect.h>            // Autoconnect
#include <WiFi.h>                   // Autoconnect
#include <WebServer.h>              // Autoconnect
#include <FS.h>                     // Autoconnect
#include <SPIFFS.h>                 // Autoconnect
#include <DS3231.h>                 // DS3231-RTC
#include <OneWire.h>                // OneWireTemperatur
#include <DallasTemperature.h>      // OneWireTemperatur

// ------------------------------ Definitions (Global) ------------------------------------------
#define FORMAT_ON_FAIL  true        // Autoconnect
#define AUTOCONNECT_MENUCOLOR_TEXT        "#fffacd" // Autoconnect
#define AUTOCONNECT_MENUCOLOR_BACKGROUND  "#696969" // Autoconnect
#define AUTOCONNECT_MENUCOLOR_ACTIVE      "#808080" // Autoconnect
#define GET_CHIPID()    ((uint16_t)(ESP.getEfuseMac()>>32))      // Autoconnect
#define GET_HOSTNAME()  (WiFi.getHostname())        // Autoconnect
//#define AUTOCONNECT_URI         "/_ac"            // Autoconnect
using WiFiWebServer = WebServer;    // Autoconnect
fs::SPIFFSFS& FlashFS = SPIFFS;     // Autoconnect
WiFiWebServer server;               // Autoconnect
AutoConnect portal(server);         // Autoconnect
AutoConnectConfig config("NewBeeNode", "1234567890"); // Autoconnect
// Declare AutoConnectAux separately as a custom web page to access
// easily for each page in the post-upload handler.
AutoConnectAux auxUpload;           // Autoconnect
AutoConnectAux auxBrowse;           // Autoconnect
RTClib myRTC;                       // DS3231-RTC

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 19              // OneWireTemperatur

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);       // OneWireTemperatur 

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire); // OneWireTemperatur

/* Conversion factor for micro seconds to seconds */
#define uS_TO_S_FACTOR 1000000      //DeepSleep
/* Time ESP32 will go to sleep (in seconds) */ 
#define TIME_TO_SLEEP  10           //DeepSleep

RTC_DATA_ATTR int bootCount = 0;    //DeepSleep
bool useDeepSleep = false;          //DeepSleep

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);   // ADXL345

const char* PARAM_FILE              = "/param.json";         // Autoconnect
const char* PARAM_SENSOR_FILE       = "/param_sensor.json";  // Autoconnect
const char* AUX_SENSOR_SETTING_URI  = "/sensor_setting";     // Autoconnect
const char* AUX_SENSOR_SAVE_URI     = "/sensor_save";        // Autoconnect
const char* AUX_SENSOR_CLEAR_URI    = "/sensor_clear";       // Autoconnect
const char* AUX_SETTING_URI         = "/mqtt_setting";       // Autoconnect
const char* AUX_SAVE_URI            = "/mqtt_save";          // Autoconnect
const char* AUX_CLEAR_URI           = "/mqtt_clear";         // Autoconnect

String  apId;         // Autoconnect
String  hostName;     // Autoconnect

String  serverName;   // Autoconnect
String  channelId;    // Autoconnect
String  userKey;      // Autoconnect
String  apiKey;       // Autoconnect
bool  uniqueid;       // Autoconnect
unsigned long publishInterval = 0;   // Autoconnect
const char* userId = "anyone";       // Autoconnect

String beenodename;    // Autoconnect
String hivename;       // Autoconnect

// Upload request custom Web page
static const char PAGE_UPLOAD[] PROGMEM = R"(
{
  "uri": "/",
  "title": "Upload",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>File uploading platform</h2>"
    },
    {
      "name": "upload_file",
      "type": "ACFile",
      "label": "Select file: ",
      "store": "fs"
    },
    {
      "name": "upload",
      "type": "ACSubmit",
      "value": "UPLOAD",
      "uri": "/upload"
    }
  ]
}
)";

// Upload result display
static const char PAGE_BROWSE[] PROGMEM = R"(
{
  "uri": "/upload",
  "title": "Upload",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>Uploading ended</h2>"
    },
    {
      "name": "filename",
      "type": "ACText",
      "posterior": "br"
    },
    {
      "name": "size",
      "type": "ACText",
      "format": "%s bytes uploaded",
      "posterior": "br"
    },
    {
      "name": "content_type",
      "type": "ACText",
      "format": "Content: %s",
      "posterior": "br"
    },
    {
      "name": "object",
      "type": "ACElement"
    }
  ]
}
)";

String getContentType(const String& filename) {
  if (filename.endsWith(".txt")) {
    return "text/plain";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  }else if (filename.endsWith(".ini")) {
    return "text/plain";
  }
  return "application/octet-stream";
}

/**
 * Post uploading, AutoConnectFile's built-in upload handler reads the
 * file saved in SPIFFS and displays the file contents on /upload custom
 * web page. However, only files with mime type uploaded as text are
 * displayed. A custom web page handler is called after upload.
 * @param  aux  AutoConnectAux(/upload)
 * @param  args PageArgument
 * @return Uploaded text content
 */
String postUpload(AutoConnectAux& aux, PageArgument& args) {
  String  content;
  // Explicitly cast to the desired element to correctly extract
  // the element using the operator [].
  AutoConnectFile&  upload = auxUpload["upload_file"].as<AutoConnectFile>();
  AutoConnectText&  aux_filename = aux["filename"].as<AutoConnectText>();
  AutoConnectText&  aux_size = aux["size"].as<AutoConnectText>();
  AutoConnectText&  aux_contentType = aux["content_type"].as<AutoConnectText>();
  // Assignment operator can be used for the element attribute.
  aux_filename.value = upload.value;
  Serial.printf("uploaded file saved as %s\n", aux_filename.value.c_str());
  aux_size.value = String(upload.size);
  aux_contentType.value = upload.mimeType;

  // Include the uploaded content in the object tag to provide feedback
  // in case of success.
  String  uploadFileName = String("/") + aux_filename.value;
  if (FlashFS.exists(uploadFileName.c_str()))
    auxBrowse["object"].value = String("<object data=\"") + uploadFileName + String("\"></object>");
  else
    auxBrowse["object"].value = "Not saved";
  return String();
}

/**
 * Read the given file from the filesystem and stream it back to the client
 */
void handleFileRead(void) {
  const String& filePath = server.uri();
  if (FlashFS.exists(filePath.c_str())) 
  {
    File  uploadedFile = FlashFS.open(filePath, "r");
    String  mime =  getContentType(filePath);
    server.streamFile(uploadedFile, mime);
    uploadedFile.close();
  }
}

void setup() 
{
  delay(1000);                  // ESP startup
  Serial.begin(115200);         // ESP Console
  Serial.println();             // ESP Console
  Wire.begin(18,23);            // DS3231-RTC, ADXL234
  SetupAutoConnect();           // Autoconnect
  SetupVibration();             // Vibration
  SetupCommunication();         // Communication
  SetupPowerManagement();       // Power
  SetupHumadity();              // Humadity
  SetupRTC();                   // RTC
  if(useDeepSleep)  SetupDeepSleep();             // DeepSleep
}

////////// Setup function

void SetupAutoConnect()
{
  SPIFFS.begin();                                     // Autoconnect
  File page = SPIFFS.open("/mqttpage.json", "r");     // Autoconnect
  if(portal.load(page))                               // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& mqtt_setting = *portal.aux(AUX_SETTING_URI);// Autoconnect
    loadParams(mqtt_setting, args);                             // Autoconnect
    if (uniqueid)                                               // Autoconnect
    {                                                           // Autoconnect
      config.apid = "ESP-" + String(GET_CHIPID(), HEX);         // Autoconnect
      Serial.println("apid set to " + config.apid);             // Autoconnect
    }                                                           // Autoconnect
    if (hostName.length())                                      // Autoconnect
    {                                                           // Autoconnect
      config.hostName = hostName;                               // Autoconnect
      Serial.println("hostname set to " + config.hostName);     // Autoconnect
    }                                                           // Autoconnect
    portal.on(AUX_SETTING_URI, loadParams);                     // Autoconnect
    portal.on(AUX_SAVE_URI, saveParams);                        // Autoconnect
  }                                                             // Autoconnect
  
  page.close();                                       // Autoconnect
  File page1 = SPIFFS.open("/sensorpage.json", "r");  // Autoconnect
  if(portal.load(page1))                              // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& sensor_setting = *portal.aux(AUX_SENSOR_SETTING_URI);// Autoconnect
    loadSensorParams(sensor_setting, args);                             // Autoconnect
    portal.on(AUX_SENSOR_SETTING_URI, loadSensorParams);        // Autoconnect
    portal.on(AUX_SENSOR_SAVE_URI, saveParamsSensor);           // Autoconnect
  }                                                             // Autoconnect
  page1.close();                                      // Autoconnect
  SPIFFS.end();                                       // Autoconnect

  // Start the filesystem
  FlashFS.begin(FORMAT_ON_FAIL);  // Autoconnect

  // Attach the custom web pages
  auxUpload.load(PAGE_UPLOAD);   // Autoconnect
  auxBrowse.load(PAGE_BROWSE);   // Autoconnect
  auxBrowse.on(postUpload);      // Autoconnect
  portal.join({ auxUpload, auxBrowse });  // Autoconnect

  // The handleFileRead callback function provides an echo back of the
  // uploaded file to the client. You can include the uploaded file in
  // the response by embedding the object HTML tag in your custom web page.
  // The client browser will request to get the content according to
  // the link of the object tag, and the request can be caught by onNotFound handler.
  portal.onNotFound(handleFileRead);                // Autoconnect
  config.ota = AC_OTA_BUILTIN;                      // Autoconnect
  config.title = beenodename + " 020420221625";     // Autoconnect
  config.homeUri = "/_ac";                          // Autoconnect
  config.bootUri = AC_ONBOOTURI_HOME;               // Autoconnect
  // Reconnect and continue publishing even if WiFi is disconnected.
  config.autoReconnect = true;                      // Autoconnect
  config.reconnectInterval = 1;                     // Autoconnect
  
  portal.config(config);                            // Autoconnect
  portal.begin();                                   // Autoconnect
}

void SetupVibration()                                                   //ADXL345
{                                                                       //ADXL345
   Serial.println("Accelerometer Test"); Serial.println("");            //ADXL345
  
 /* Initialise the sensor */  
    if(!accel.begin())                                                  //ADXL345
    {                                                                   //ADXL345
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");//ADXL345
    while(1);                                                           //ADXL345
  }                                                                     //ADXL345

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);                                   //ADXL345
  // accel.setRange(ADXL345_RANGE_8_G);                                 //ADXL345
  // accel.setRange(ADXL345_RANGE_4_G);                                 //ADXL345
  // accel.setRange(ADXL345_RANGE_2_G);                                 //ADXL345
  
  /* Display some basic information on this sensor */
  displaySensorDetails();                                               //ADXL345
  
  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();                                                    //ADXL345
  displayRange();                                                       //ADXL345
  Serial.println("");                                                   //ADXL345
}

void SetupTemperature()
{
    sensors.begin();  // OneWireTemperatur  
}

void SetupWeigth()
{}

void SetupCommunication()
{}

void SetupDeepSleep()                                                               //DeepSleep
{
    //Increment boot number and print it every reboot                           
  ++bootCount;                                                                      //DeepSleep
  Serial.println("Boot number: " + String(bootCount));
  
  //Print the wakeup reason for ESP32
  print_wakeup_reason();                                                            //DeepSleep

  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);                    //DeepSleep
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +      
  " Seconds");                                                                      //DeepSleep

  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);          //DeepSleep
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");   //DeepSleep
}                                                                                   //DeepSleep

void SetupPowerManagement()
{}

void SetupHumadity()
{}

void SetupRTC()
{
     // Wire.begin was moved to Setup(). It is requird by ADXL and RTC
}

void loop() 
{
  HandleWebPage();              // Autoconnect
  HandleTemperature(); 
  HandleWeigth(); 
  HandleVibration(); 
  HandleCommunication(); 
  HandlePowerManagement(); 
  HandleHumadity(); 
  HandleRTC();                  //DS3231-RTC
  if(useDeepSleep) HandleDeepSleep();            //DeepSleep
}

////////// Loop Functions 

void HandleWebPage()
{
    portal.handleClient();       // Autoconnect
}

void HandleTemperature()
{
  // Send the command to get temperatures
  sensors.requestTemperatures();                          //OneWireTemperature

  //print the temperature in Celsius
  Serial.print("Temperature: ");                          //OneWireTemperature
  Serial.print(sensors.getTempCByIndex(0));               //OneWireTemperature
  Serial.print((char)176);//shows degrees character       //OneWireTemperature
  Serial.print("C  |  ");                                 //OneWireTemperature
}

void HandleWeigth()
{}

void HandleVibration()                                                   //ADXL345
{
 /* Get a new sensor event */ 
  sensors_event_t event;                                                 //ADXL345
  accel.getEvent(&event);                                                //ADXL345
 
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");                            //ADXL345
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");                            //ADXL345
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");   //ADXL345
 delay(1000);
}

void HandleCommunication()
{}

void HandleDeepSleep()
{
  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */
  Serial.println("Going to sleep now");                                             //DeepSleep
  delay(1000);                                                                      //DeepSleep
  Serial.flush();                                                                   //DeepSleep
  esp_deep_sleep_start();                                                           //DeepSleep
  Serial.println("This will never be printed");                                     //DeepSleep
}

void HandlePowerManagement()
{}

void HandleHumadity()
{}

void HandleRTC()
{
    delay(10);                                        //DS3231-RTC
    
    DateTime now = myRTC.now();                       //DS3231-RTC
    
    Serial.print(now.year(), DEC);                    //DS3231-RTC
    Serial.print('/');                                //DS3231-RTC
    Serial.print(now.month(), DEC);                   //DS3231-RTC
    Serial.print('/');                                //DS3231-RTC
    Serial.print(now.day(), DEC);                     //DS3231-RTC
    Serial.print(' ');                                //DS3231-RTC
    Serial.print(now.hour(), DEC);                    //DS3231-RTC
    Serial.print(':');                                //DS3231-RTC
    Serial.print(now.minute(), DEC);                  //DS3231-RTC
    Serial.print(':');                                //DS3231-RTC
    Serial.print(now.second(), DEC);                  //DS3231-RTC
    Serial.println();                                 //DS3231-RTC
    
    Serial.print(" since midnight 1/1/1970 = ");      //DS3231-RTC
    Serial.print(now.unixtime());                     //DS3231-RTC
    Serial.print("s = ");                             //DS3231-RTC
    Serial.print(now.unixtime() / 86400L);            //DS3231-RTC
    Serial.println("d");                              //DS3231-RTC
}

///////////// Functions

void displaySensorDetails(void)                                                     //ADXL345
{                                                                                   //ADXL345
  sensor_t sensor;                                                                  //ADXL345
  accel.getSensor(&sensor);                                                         //ADXL345
  Serial.println("------------------------------------");                           //ADXL345
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);                    //ADXL345
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);                 //ADXL345
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);               //ADXL345
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");                 //ADXL345
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");                 //ADXL345
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");                //ADXL345
  Serial.println("------------------------------------");                 //ADXL345
  Serial.println("");                                                     //ADXL345
  delay(500);                                                             //ADXL345
}                                                                         //ADXL345

void displayDataRate(void)                      //ADXL345
{                                               //ADXL345
  Serial.print  ("Data Rate:    ");             //ADXL345
                                                //ADXL345
  switch(accel.getDataRate())                   //ADXL345
  {                                             //ADXL345
    case ADXL345_DATARATE_3200_HZ:              //ADXL345
      Serial.print  ("3200 ");                  //ADXL345 
      break;                                    //ADXL345
    case ADXL345_DATARATE_1600_HZ:              //ADXL345
      Serial.print  ("1600 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_800_HZ:               //ADXL345
      Serial.print  ("800 ");                   //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_400_HZ:               //ADXL345
      Serial.print  ("400 ");                   //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_200_HZ:               //ADXL345
      Serial.print  ("200 ");                   //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_100_HZ:               //ADXL345
      Serial.print  ("100 ");                   //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_50_HZ:                //ADXL345
      Serial.print  ("50 ");                    //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_25_HZ:                //ADXL345
      Serial.print  ("25 ");                    //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_12_5_HZ:              //ADXL345
      Serial.print  ("12.5 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_6_25HZ:               //ADXL345
      Serial.print  ("6.25 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_3_13_HZ:              //ADXL345
      Serial.print  ("3.13 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_1_56_HZ:              //ADXL345
      Serial.print  ("1.56 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_0_78_HZ:              //ADXL345
      Serial.print  ("0.78 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_0_39_HZ:              //ADXL345
      Serial.print  ("0.39 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_0_20_HZ:              //ADXL345
      Serial.print  ("0.20 ");                  //ADXL345
      break;                                    //ADXL345
    case ADXL345_DATARATE_0_10_HZ:              //ADXL345
      Serial.print  ("0.10 ");                  //ADXL345
      break;                                    //ADXL345
    default:                                    //ADXL345
      Serial.print  ("???? ");                  //ADXL345
      break;                                    //ADXL345
  }                                             //ADXL345
  Serial.println(" Hz");                        //ADXL345
}                                               //ADXL345

void displayRange(void)                     //ADXL345
{                                           //ADXL345
  Serial.print  ("Range:         +/- ");    //ADXL345
  
  switch(accel.getRange())                  //ADXL345
  {                                         //ADXL345
    case ADXL345_RANGE_16_G:                //ADXL345
      Serial.print  ("16 ");                //ADXL345
      break;                                //ADXL345
    case ADXL345_RANGE_8_G:                 //ADXL345
      Serial.print  ("8 ");                 //ADXL345
      break;                                //ADXL345
    case ADXL345_RANGE_4_G:                 //ADXL345
      Serial.print  ("4 ");                 //ADXL345
      break;                                //ADXL345
    case ADXL345_RANGE_2_G:                 //ADXL345
      Serial.print  ("2 ");                 //ADXL345
      break;                                //ADXL345
    default:                                //ADXL345
      Serial.print  ("?? ");                //ADXL345
      break;                                //ADXL345
  }                                         //ADXL345
  Serial.println(" g");                     //ADXL345
}                                           //ADXL345

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason()                            //DeepSleep
{                                                     //DeepSleep
  esp_sleep_wakeup_cause_t wakeup_reason;             //DeepSleep

  wakeup_reason = esp_sleep_get_wakeup_cause();       //DeepSleep

  switch(wakeup_reason)                               //DeepSleep
  {                                                   //DeepSleep
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;  //DeepSleep
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;//DeepSleep
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;                        //DeepSleep
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;                  //DeepSleep
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;                    //DeepSleep
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;            //DeepSleep
  }                                                                                                       //DeepSleep
}                                                                                                         //DeepSleep


void getParams(AutoConnectAux& aux) 
{
  serverName = aux[F("mqttserver")].value;                            // Autoconnect
  serverName.trim();                                                  // Autoconnect
  channelId = aux[F("channelid")].value;                              // Autoconnect
  channelId.trim();                                                   // Autoconnect
  userKey = aux[F("userkey")].value;                                  // Autoconnect  
  userKey.trim();                                                     // Autoconnect
  apiKey = aux[F("apikey")].value;                                    // Autoconnect
  apiKey.trim();                                                      // Autoconnect
  AutoConnectRadio& period = aux[F("period")].as<AutoConnectRadio>(); // Autoconnect
  publishInterval = period.value().substring(0, 2).toInt() * 1000;    // Autoconnect
  uniqueid = aux[F("uniqueid")].as<AutoConnectCheckbox>().checked;    // Autoconnect
  hostName = aux[F("hostname")].value;                                // Autoconnect
  hostName.trim();                                                    // Autoconnect
}


void getSensorParams(AutoConnectAux& aux) 
{
  beenodename = aux[F("beenodename")].value;                           // Autoconnect
  beenodename.trim();                                                  // Autoconnect
  hivename = aux[F("hivename")].value;                               // Autoconnect
  hivename.trim();                                                     // Autoconnect
}

// Load parameters saved with saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args)            // Autoconnect
{                                                                     // Autoconnect
  (void)(args);                                                       // Autoconnect
  Serial.print(PARAM_FILE);                                           // Autoconnect
  File param = FlashFS.open(PARAM_FILE, "r");                         // Autoconnect
  if (param) {                                                        // Autoconnect
    if (aux.loadElement(param)) {                                     // Autoconnect
      getParams(aux);                                                 // Autoconnect
      Serial.println(" loaded");                                      // Autoconnect
    }                                                                 // Autoconnect
    else                                                              // Autoconnect
      Serial.println(" failed to load");                              // Autoconnect
    param.close();                                                    // Autoconnect
  }                                                                   // Autoconnect
  else                                                                // Autoconnect
    Serial.println(" open failed");                                   // Autoconnect
  return String("");                                                  // Autoconnect
}


// Load parameters saved with saveParams from SPIFFS into the
// elements defined in /sensor_setting JSON.
String loadSensorParams(AutoConnectAux& aux, PageArgument& args)    // Autoconnect
{                                                                     // Autoconnect
  (void)(args);                                                       // Autoconnect
  Serial.print(PARAM_SENSOR_FILE);                                    // Autoconnect
  File param = FlashFS.open(PARAM_SENSOR_FILE, "r");                  // Autoconnect
  if (param) {                                                        // Autoconnect
    if (aux.loadElement(param)) {                                     // Autoconnect
      getSensorParams(aux);                                           // Autoconnect
      Serial.println(" loaded");                                      // Autoconnect
    }                                                                 // Autoconnect
    else                                                              // Autoconnect
      Serial.println(" failed to load");                              // Autoconnect
    param.close();                                                    // Autoconnect
  }                                                                   // Autoconnect
  else                                                                // Autoconnect
    Serial.println(" open failed");                                   // Autoconnect
  return String("");                                                  // Autoconnect
}

// Save the value of each element entered by '/mqtt_setting' to the
// parameter file. The saveParams as below is a callback function of
// /mqtt_save. When invoking this handler, the input value of each
// element is already stored in '/mqtt_setting'.
// In the Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux& aux, PageArgument& args) {          // Autoconnect
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   mqtt_setting = *portal.aux(portal.where());       // Autoconnect
  getParams(mqtt_setting);                                            // Autoconnect

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = FlashFS.open(PARAM_FILE, "w");                         // Autoconnect
  mqtt_setting.saveElement(param, {"mqttserver", "channelid", "userkey", "apikey", "uniqueid", "period", "hostname"}); // Autoconnect
  param.close();                                                      // Autoconnect

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectInput& mqttserver = mqtt_setting[F("mqttserver")].as<AutoConnectInput>();         // Autoconnect
  aux[F("mqttserver")].value = serverName + String(mqttserver.isValid() ? " (OK)" : " (ERR)"); // Autoconnect
  aux[F("channelid")].value = channelId;                                                       // Autoconnect
  aux[F("userkey")].value = userKey;                                                           // Autoconnect
  aux[F("apikey")].value = apiKey;                                                             // Autoconnect
  aux[F("period")].value = String(publishInterval / 1000);                                     // Autoconnect

  return String();                                                                             // Autoconnect
}                                                                                              // Autoconnect

// Save the value of each element entered by '/sensor_setting' to the
// parameter file. The saveParamsSensor as below is a callback function of
// /sensor_save. When invoking this handler, the input value of each
// element is already stored in '/sensor_setting'.
// In the Sketch, you can output to stream its elements specified by name.
String saveParamsSensor(AutoConnectAux& aux, PageArgument& args) {         // Autoconnect
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   sensor_setting = *portal.aux(portal.where());          // Autoconnect
  getSensorParams(sensor_setting);                                         // Autoconnect

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /sensor_setting, it is necessary to get
  // the AutoConnectAux object of /sensor_setting.
  File param = FlashFS.open(PARAM_SENSOR_FILE, "w");                  // Autoconnect
  sensor_setting.saveElement(param, {"beenodename", "hivename"});     // Autoconnect
  param.close();                                                      // Autoconnect

  // Echo back saved parameters to AutoConnectAux page.
  aux[F("beenodename")].value = beenodename;                                                   // Autoconnect
  aux[F("hivename")].value = hivename;                                                         // Autoconnect

  return String();                                                                             // Autoconnect
}                                                                                              // Autoconnect
