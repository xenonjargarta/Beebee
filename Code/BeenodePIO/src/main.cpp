/*    
      Build information:  Used chip: ESP32-D0WDQ6-V3 (revision 3)
                          Used programm memory 1081290/1966080  Bytes (54%) 
                          Used memory for globale variabel 46412 Bytes (14%)
                          Setting "Minimal SPIFF (1.9MB APP / with OTA/190KB SPIFF)
                          Still free memory for local variable 281268 Bytes (Max 327680 Bytes)
      
      Features:           (x) Webpage 
                          (x) Wifi Lifecycle
                          (x) Configuration management 
                          (x) MQTT
                          (x) Vibration Sensor (ADXL345)
                          (x) Weight Sensor (HX711)
                          (x) Temperature Sensor (DS)
                          (in progress7) Humadity Sensor ()
                          (x) RTC (DS3231)
                          ( ) Power management
                          (in progress) Deep Sleep (ESP)
                          ( ) Lora Communication
                          ( ) SIM Communication
                          (x) SD Card

      Libaries            Express if ESP32 Boards - ESP32 by Espressif Systems - (https://dl.espressif.com/dl/package_esp32_index.json) 1.0.6
                          OneWire 2.3.6
                          DallasTemperatur 3.9.0
                          Autoconnect 1.3.4 (+ dependencies)
                          Adafruit 1.3.1 (+dependencies)
                          DS3231 (by Andrew Wickert; 1.0.1)
                          EspMQTTClient 1.13.3 (+dependencies)
                          SPI/SD/FS default by Arduino
                          Adafruit BMP280 2.6.0
                          
      Scenario supported: (X) Always On with webserver
                          (X) Sleep on always power
                          ( ) Sleep on battery
                          ( ) One time st startup
                          
*/

// ------------------------------ Includes ------------------------------------------
#include <Arduino.h>                // Arduino.h
#include <Wire.h>                   // ADXL345, DS3231
#include <Adafruit_Sensor.h>        // ADXL345
#include <Adafruit_ADXL345_U.h>     // ADXL345
#include <Adafruit_BME280.h>        // BME280

#include "SD.h"                     // SDCARD
#include "SPI.h"                    // SDCARD
#include <AutoConnect.h>            // Autoconnect
#include <WiFi.h>                   // Autoconnect
#include <WebServer.h>              // Autoconnect
#include <FS.h>                     // Autoconnect
#include <SPIFFS.h>                 // Autoconnect
#include <DS3231.h>                 // DS3231-RTC
#include <OneWire.h>                // OneWireTemperatur
#include <DallasTemperature.h>      // OneWireTemperatur
#include "EspMQTTClient.h"          // MQTT
#include <HX711_ADC.h>              // HDX711

struct CfgStorage {
  String sdaio;                   // ADXL345, RTC
  String sdlio;                   // ADXL345, RTC
  String beenodename;             // Autoconnect
  String hivename;                // Autoconnect
  int deepSleepTime;              // Autoconnect
  bool useTemperatureSensor;      // OneWireTemperatur
  bool useVibrationSensor;        // ADXL345
  bool useRTCSensor;              // RTC
  bool setupReadyVibration;       // ADXL345 
  bool needToReboot;              // Autoconnect
  float acc_datarate;             // ADXL345
  bool acc_usefullres;            // ADXL345
  int acc_range;                  // ADXL345
  bool useMQTT;                   // MQTT
  String mqtt_SSID;               // MQTT
  String mqtt_wifi_pwd;           // MQTT
  String mqttusername;            // MQTT
  String mqttpassword;            // MQTT
  String mqtt_topic;              // MQTT
  String mqtt_server;             // MQTT
  String mqtt_port;               // MQTT
  bool useDeepSleep;              // DeepSleep
  bool useSDLogging;              // SDLogging
  String mqtt_messagedelay;       // MQTT
  String sd_logfilepath;          // SDLogging
  bool useWeigthSensor;           // HDX711
  bool useTempSensorTwo;          // BME280
  bool useHumidity;               // BME280
};

struct SensorValues 
{
  String temperatur;      // DS 
  String vibration_z;     // ADXL345 
  String vibration_x;     // ADXL345 
  String vibration_y;     // ADXL345 
  String weigth;          // HDX117 
  String senortime;       // RTC 
  String temperatur2;     // BME280 
  String humidity;        // BME280
};

CfgStorage _CfgStorage = {"", "", "", "", 20, false, false, false, false, false, 3200, false, 16, false, "", "", "", "", "", "", "", false, false, "1000", "/values.txt", false, false, false};
SensorValues _SensorValues = {"", "", "", "", "", "", "", ""};

String CreateMessage()
{
  String message ="";
  if(_CfgStorage.useRTCSensor)                      // RTC
  {                                                 // RTC
    //message += "T)";                              // RTC
    message += _SensorValues.senortime;             // RTC
  }                                                 // RTC
  if(_CfgStorage.useVibrationSensor)                // ADXL345
  {                                                 // ADXL345
    message += ";";                                 // ADXL345
    //message += "H1)";                             // ADXL345
    message += _SensorValues.vibration_x;           // ADXL345
    message += ";";                                 // ADXL345
    message += _SensorValues.vibration_x;           // ADXL345
    message += ";";                                 // ADXL345
    message += _SensorValues.vibration_y;           // ADXL345
  }                                                 // ADXL345
  if(_CfgStorage.useTemperatureSensor)              // OneWireTemperatur
  {                                                 // OneWireTemperatur
    message += ";";                                 // OneWireTemperatur
    message += _SensorValues.temperatur;            // OneWireTemperatur
  }                                                 // OneWireTemperatur
  if(_CfgStorage.useWeigthSensor)                   // HDX117
  {                                                 // HDX117
    message += ";";                                 // HDX117
    message += _SensorValues.weigth;                // HDX117
  }                                                 // HDX117
  if(_CfgStorage.useTempSensorTwo)                  // BME280
  {                                                 // BME280
    message += ";";                                 // BME280
    message += _SensorValues.temperatur2;           // BME280
  }                                                 // BME280
  if(_CfgStorage.useHumidity)                       // BME280
  {                                                 // BME280
    message += ";";                                 // BME280
    message += _SensorValues.humidity;              // BME280
  }                                                 // BME280
  return message;
}

// ------------------------------ Definitions (Global) ------------------------------------------
#define FORMAT_ON_FAIL  true        // Autoconnect
#define GET_CHIPID()    ((uint16_t)(ESP.getEfuseMac()>>32))   // Autoconnect
#define GET_HOSTNAME()  (WiFi.getHostname())                  // Autoconnect
// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 19                                       // OneWireTemperatur
/* Conversion factor for micro seconds to seconds */
#define uS_TO_S_FACTOR 1000000                                //DeepSleep
#define BUTTON_PIN 37                                         // GIOP21 pin connected to button
#define SCK   14                                              // SDCARD
#define MISO  12                                              // SDCARD
#define MOSI  13                                              // SDCARD
#define CS    15                                              // SDCARD
#define SEALEVELPRESSURE_HPA (1013.25)                        // BME280

using WiFiWebServer = WebServer;    // Autoconnect
fs::SPIFFSFS& FlashFS = SPIFFS;     // Autoconnect
WiFiWebServer server;               // Autoconnect
AutoConnect portal(server);         // Autoconnect
AutoConnectConfig config("NewBeeNode", "1234567890");         // Autoconnect
// Declare AutoConnectAux separately as a custom web page to access
// easily for each page in the post-upload handler.
AutoConnectAux auxUpload;           // Autoconnect
AutoConnectAux auxBrowse;           // Autoconnect
RTClib myRTC;                       // DS3231-RTC
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);       // OneWireTemperatur 
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire); // OneWireTemperatur
RTC_DATA_ATTR int bootCount = 0;     // DeepSleep
SPIClass spi;                        // SDLogging
Adafruit_BME280 bme; // I2C          // BME280
unsigned bmestatus;                     // BME280

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);   // ADXL345

const char* PARAM_SENSOR_FILE       = "/param_sensor.json";  // Autoconnect
const char* AUX_SENSOR_SETTING_URI  = "/sensor_setting";     // Autoconnect
const char* AUX_SENSOR_SAVE_URI     = "/sensor_save";        // Autoconnect
const char* AUX_SENSOR_CLEAR_URI    = "/sensor_clear";       // Autoconnect      
const int HX711_dout = 0;                                    // HX711 mcu > HX711 dout pin
const int HX711_sck = 2;                                     // HX711 mcu > HX711 sck pin

EspMQTTClient client;          // using the default constructor

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

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

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

////////// Setup function
void SetupLogging()                            // SDLogging 
{                                              // SDLogging
  spi = SPIClass(VSPI);                        // SDLogging
  spi.begin(SCK, MISO, MOSI, CS);              // SDLogging

  if (!SD.begin(CS,spi,80000000))              // SDLogging
  {                                            // SDLogging
    Serial.println("Card Mount Failed");       // SDLogging
    _CfgStorage.useSDLogging = false;          // SDLogging
  }                                            // SDLogging
  else                                         // SDLogging
  {                                            // SDLogging
    uint8_t cardType = SD.cardType();          // SDLogging
  
    if(cardType == CARD_NONE){                 // SDLogging
      Serial.println("No SD card attached");   // SDLogging
      return;                                  // SDLogging
    }                                          // SDLogging
  
    Serial.print("SD Card Type: ");            // SDLogging
    if(cardType == CARD_MMC){                  // SDLogging
      Serial.println("MMC");                   // SDLogging
    } else if(cardType == CARD_SD){            // SDLogging
      Serial.println("SDSC");                  // SDLogging
    } else if(cardType == CARD_SDHC){          // SDLogging
      Serial.println("SDHC");                  // SDLogging
    } else {                                   // SDLogging
      Serial.println("UNKNOWN");               // SDLogging
    }                                          // SDLogging
  
    uint64_t cardSize = SD.cardSize() / (1024 * 1024); // SDLogging
    Serial.printf("SD Card Size: %lluMB\n", cardSize); // SDLogging
  
    listDir(SD, "/", 0);                       // SDLogging
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));    // SDLogging
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));      // SDLogging
  }                                            // SDLogging
}                                              // SDLogging


void getSensorParams(AutoConnectAux& aux) 
{
  _CfgStorage.beenodename = aux[F("beenodename")].value;                           // Autoconnect
  _CfgStorage.beenodename.trim();                                                  // Autoconnect
  _CfgStorage.hivename = aux[F("hivename")].value;                                 // Autoconnect
  _CfgStorage.hivename.trim();                                                     // Autoconnect
  _CfgStorage.useDeepSleep  = aux[F("useDeepSleep")].as<AutoConnectCheckbox>().checked; // DeepSleep 
  _CfgStorage.deepSleepTime = aux[F("deepSleepTime")].value.toInt();               // DeepSleep
  _CfgStorage.useTemperatureSensor  = aux[F("useTemperatureSensor")].as<AutoConnectCheckbox>().checked; // Autoconnect 
  _CfgStorage.useVibrationSensor  = aux[F("useVibrationSensor")].as<AutoConnectCheckbox>().checked; // ADXL 
  _CfgStorage.useRTCSensor  = aux[F("useRTCSensor")].as<AutoConnectCheckbox>().checked; // RTC 
  AutoConnectRadio& dr = aux[F("acc_datarate")].as<AutoConnectRadio>();            // ADXL
  _CfgStorage.acc_datarate = dr.value().toDouble();                                // ADXL
  _CfgStorage.acc_usefullres  = aux[F("acc_usefullres")].as<AutoConnectCheckbox>().checked; // ADXL
  AutoConnectRadio& range = aux[F("acc_range")].as<AutoConnectRadio>();            // ADXL
  _CfgStorage.acc_range = range.value().toInt();                                   // ADXL
  _CfgStorage.useSDLogging = aux[F("useSDLogging")].as<AutoConnectCheckbox>().checked; // SDLogging
  _CfgStorage.mqtt_server = aux[F("mqtt_server")].value;                           // MQTT
  _CfgStorage.mqtt_server.trim();                                                  // MQTT
 
  _CfgStorage.useMQTT = aux[F("useMQTT")].as<AutoConnectCheckbox>().checked;        // SDLogging
  _CfgStorage.mqtt_SSID = aux[F("mqtt_SSID")].value;                               // Autoconnect
  _CfgStorage.mqtt_SSID.trim();                                                    // MQTT
  _CfgStorage.mqtt_wifi_pwd = aux[F("mqtt_wifi_pwd")].value;                       // MQTT
  _CfgStorage.mqtt_wifi_pwd.trim();                                                // MQTT
  _CfgStorage.mqttusername = aux[F("mqttusername")].value;                         // MQTT
  _CfgStorage.mqttusername.trim();                                                 // MQTT
  _CfgStorage.mqttpassword = aux[F("mqttpassword")].value;                         // MQTT
  _CfgStorage.mqttpassword.trim();                                                 // MQTT
  _CfgStorage.mqtt_topic = aux[F("mqtt_topic")].value;                             // MQTT
  _CfgStorage.mqtt_topic.trim();                                                   // MQTT
  _CfgStorage.mqtt_port = aux[F("mqtt_port")].value;                               // MQTT
  _CfgStorage.mqtt_port.trim();                                                    // MQTT
  _CfgStorage.mqtt_messagedelay = aux[F("mqtt_messagedelay")].value;               // MQTT
  _CfgStorage.mqtt_messagedelay.trim();                                            // MQTT
  _CfgStorage.sd_logfilepath = aux[F("sd_logfilepath")].value;                     // SDLogging
  _CfgStorage.sd_logfilepath.trim();                                               // SDLOgging   
  _CfgStorage.useWeigthSensor = aux[F("useWeigthSensor")].as<AutoConnectCheckbox>().checked;       // HDX711  
  _CfgStorage.sdaio = aux[F("sdaio")].value;                                       // ADXL, RTC
  _CfgStorage.sdaio.trim();                                                        // ADXL, RTC
  _CfgStorage.sdlio = aux[F("sdlio")].value;                                       // Autoconnect
  _CfgStorage.sdlio.trim();                                                        // ADXL, RTC
  _CfgStorage.useTempSensorTwo = aux[F("useTempSensorTwo")].as<AutoConnectCheckbox>().checked;    // BME280
  _CfgStorage.useHumidity = aux[F("useHumidity")].as<AutoConnectCheckbox>().checked;    // BME280

  Serial.println(" ");                                              // Autoconnect 
  Serial.println("Curren Configuration:");                          // Autoconnect 
  Serial.print("Bee node name: ");                                  // Autoconnect 
  Serial.println(_CfgStorage.beenodename);                          // Autoconnect 
  Serial.print("Hive name: ");                                      // Autoconnect 
  Serial.println(_CfgStorage.hivename);                             // Autoconnect 
  Serial.print("Used deep sleep: ");                                // Autoconnect 
  Serial.println(_CfgStorage.useDeepSleep);                         // Autoconnect 
  Serial.print("Deep sleep time: ");                                // Autoconnect 
  Serial.println(_CfgStorage.deepSleepTime);                        // Autoconnect 
  Serial.print("Use temperature sensor: ");                         // Autoconnect 
  Serial.println(_CfgStorage.useTemperatureSensor);                 // Autoconnect 
  Serial.print("Use vibration sensor: ");                           // Autoconnect 
  Serial.println(_CfgStorage.useVibrationSensor);                   // Autoconnect 
  Serial.print("Use RTC sensor: ");                                 // Autoconnect 
  Serial.println(_CfgStorage.useRTCSensor);                         // Autoconnect 

  Serial.print("acc_datarate: ");                                   // Autoconnect 
  Serial.println(_CfgStorage.acc_datarate);                         // Autoconnect 

  Serial.print("acc_usefullres: ");                                 // Autoconnect 
  Serial.println(_CfgStorage.acc_usefullres);                       // Autoconnect 

  Serial.print("acc_range: ");                                      // Autoconnect 
  Serial.println(_CfgStorage.acc_range);                            // Autoconnect 

  Serial.print("sdaio: ");                                          // ADXL345, RTC  
  Serial.println(_CfgStorage.sdaio);                                // ADXL345, RTC 
  Serial.print("sdlio: ");                                          // ADXL345, RTC 
  Serial.println(_CfgStorage.sdlio);                                // ADXL345, RTC 

  Serial.print("useMQTT: ");                                        // MQTT 
  Serial.println(_CfgStorage.useMQTT);                              // MQTT

  Serial.print("mqtt_SSID: ");                                      // MQTT 
  Serial.println(_CfgStorage.mqtt_SSID);                            // MQTT

  Serial.print("mqtt_wifi_pwd: ");                                  // MQTT 
  Serial.println(_CfgStorage.mqtt_wifi_pwd);                        // MQTT

  Serial.print("mqttusername: ");                                   // MQTT 
  Serial.println(_CfgStorage.mqttusername);                         // MQTT

  Serial.print("mqttpassword: ");                                   // MQTT 
  Serial.println(_CfgStorage.mqttpassword);                         // MQTT
    
  Serial.print("mqtt_topic: ");                                     // MQTT 
  Serial.println(_CfgStorage.mqtt_topic);                           // MQTT

  Serial.print("mqtt_server: ");                                    // MQTT 
  Serial.println(_CfgStorage.mqtt_server);                          // MQTT

  Serial.print("mqtt_server: ");                                    // MQTT 
  Serial.println(_CfgStorage.mqtt_port);                            // MQTT

  Serial.print("mqtt_messagedelay: ");                              // MQTT 
  Serial.println(_CfgStorage.mqtt_messagedelay);                    // MQTT
  
  Serial.print("sd_logfilepath: ");                                 // SDLogging 
  Serial.println(_CfgStorage.sd_logfilepath);                       // SDLogging

  Serial.print("useTempSensorTwo: ");                          // BME280
  Serial.println(_CfgStorage.useTempSensorTwo);                // BME280

  Serial.print("useHumidity: ");                                    // BME280 
  Serial.println(_CfgStorage.useHumidity);                          // BME280ogging

  Serial.print("useWeigthSensor: ");                          // HDX711 
  Serial.println(_CfgStorage.useWeigthSensor);                // HDX711
  
  Serial.println("CFG Loaded end");                                  // Autoconnect 
  Serial.println(" ");                                               // Autoconnect 
}

// Load parameters saved with saveParams from SPIFFS into the
// elements defined in /sensor_setting JSON.
String loadSensorParams(AutoConnectAux& aux, PageArgument& args)      // Autoconnect
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
}                                                                     // Autoconnect
                                                                                
//  aux[F("mqttserver")].value = serverName + String(mqttserver.isValid() ? " (OK)" : " (ERR)"); // Autoconnect


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
  File param = FlashFS.open(PARAM_SENSOR_FILE, "w");                        // Autoconnect
  sensor_setting.saveElement(param, {"beenodename", "hivename", "useDeepSleep" , "deepSleepTime", "useTemperatureSensor", "useVibrationSensor", "useRTCSensor", +
  "acc_datarate","acc_range","acc_usefullres","sdaio","sdlio","useSDLogging","useMQTT","mqtt_SSID","mqtt_wifi_pwd","mqttusername","mqttpassword","mqtt_topic",  +
  "mqtt_server", "mqtt_port", "useWeigthSensor" ,"mqtt_messagedelay" ,"sd_logfilepath", "useTempSensorTwo",   "useHumidity"});     // Autoconnect
  param.close();                                                            // Autoconnect
  _CfgStorage.needToReboot = true;                                          // Autoconnect
  Serial.println("Need to reboot device");                                  // Autoconnect

 // Echo back saved parameters to AutoConnectAux page.
  aux[F("beenodename")].value = _CfgStorage.beenodename;                       // Autoconnect
  aux[F("hivename")].value = _CfgStorage.hivename;                             // Autoconnect
  aux[F("deepSleepTime")].value = _CfgStorage.deepSleepTime;                   // Autoconnect
  aux[F("useDeepSleep")].value = _CfgStorage.useDeepSleep;                     // Autoconnect
  aux[F("useTemperatureSensor")].value = _CfgStorage.useTemperatureSensor;     // Autoconnect
  aux[F("useVibrationSensor")].value = _CfgStorage.useVibrationSensor;         // Autoconnect
  aux[F("useRTCSensor")].value = _CfgStorage.useRTCSensor;                     // Autoconnect
  aux[F("acc_datarate")].value = _CfgStorage.acc_datarate;                     // Autoconnect
  aux[F("acc_range")].value = _CfgStorage.acc_range;                           // Autoconnect
  aux[F("acc_usefullres")].value = _CfgStorage.acc_usefullres;                 // Autoconnect
  aux[F("sdaio")].value = _CfgStorage.sdaio;                                   // AXDL345, RTC
  aux[F("sdlio")].value = _CfgStorage.sdlio;                                   // AXDL345, RTC
  aux[F("useSDLogging")].value = _CfgStorage.useSDLogging;                     // MQTT  
  aux[F("useMQTT")].value = _CfgStorage.useMQTT;                               // MQTT  
  aux[F("mqtt_topic")].value = _CfgStorage.mqtt_topic;                         // MQTT  
  aux[F("mqtt_SSID")].value = _CfgStorage.mqtt_SSID;                           // MQTT  
  aux[F("mqtt_wifi_pwd")].value = _CfgStorage.mqtt_wifi_pwd;                   // MQTT  
  aux[F("mqttusername")].value = _CfgStorage.mqttusername;                     // MQTT  
  aux[F("mqttpassword")].value = _CfgStorage.mqttpassword;                     // MQTT  
  aux[F("mqtt_server")].value = _CfgStorage.mqtt_server;                       // MQTT  
  aux[F("mqtt_server")].value = _CfgStorage.mqtt_port;                         // MQTT 
  aux[F("mqtt_messagedelay")].value = _CfgStorage.mqtt_messagedelay;           // MQTT 
  aux[F("sd_logfilepath")].value = _CfgStorage.sd_logfilepath;                 // SD Logging
  aux[F("useWeigthSensor")].value = _CfgStorage.useWeigthSensor;               // HDX711
  aux[F("useTempSensorTwo")].value = _CfgStorage.useTempSensorTwo;             // BME280
  aux[F("useHumidity")].value = _CfgStorage.useHumidity;                       // BME280
 
  return String();                                                             // Autoconnect
}                                                                              // Autoconnect


void SetupAutoConnect()
{
  SPIFFS.begin();                                     // Autoconnect
  File page1 = SPIFFS.open("/sensorpage.json", "r");  // Autoconnect
  if(portal.load(page1))                              // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& sensor_setting = *portal.aux(AUX_SENSOR_SETTING_URI);// Autoconnect
    loadSensorParams(sensor_setting, args);                              // Autoconnect
    portal.on(AUX_SENSOR_SETTING_URI, loadSensorParams);                 // Autoconnect
    portal.on(AUX_SENSOR_SAVE_URI, saveParamsSensor);                    // Autoconnect
  }                                                                      // Autoconnect
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
  config.title = _CfgStorage.beenodename + " v2.04.14b"; // Autoconnect
  config.homeUri = "/_ac";                          // Autoconnect
  config.bootUri = AC_ONBOOTURI_HOME;               // Autoconnect
  // Reconnect and continue publishing even if WiFi is disconnected.
  config.autoReconnect = true;                      // Autoconnect
  config.reconnectInterval = 1;                     // Autoconnect
  
  portal.config(config);                            // Autoconnect
  portal.begin();                                   // Autoconnect
}


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

void SetupVibration()                                                   //ADXL345
{                                                                       //ADXL345
   Serial.println("Accelerometer Test"); Serial.println("");            //ADXL345
  
 /* Initialise the sensor */  
    if(!accel.begin())                                                  //ADXL345
    {                                                                   //ADXL345
        /* There was a problem detecting the ADXL345 ... check your connections */
        Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");//ADXL345
        _CfgStorage.useVibrationSensor = false;                                         //ADXL345
    }
    else                                                                  //ADXL345
    {                                                                    //ADXL345
        /* Set the range to whatever is appropriate for your project */
        switch(_CfgStorage.acc_range)
        {
          case 16:
             accel.setRange(ADXL345_RANGE_16_G); 
             Serial.println("ADXL345_RANGE_16_G");
          break;
          case 8:
             accel.setRange(ADXL345_RANGE_8_G); 
             Serial.println("ADXL345_RANGE_8_G");
          break;
          case 4:
             accel.setRange(ADXL345_RANGE_4_G); 
             Serial.println("ADXL345_RANGE_4_G");
          break;
          case 2:
             accel.setRange(ADXL345_RANGE_2_G); 
             Serial.println("ADXL345_RANGE_2_G");
          break;
          default:
             accel.setRange(ADXL345_RANGE_16_G); 
             Serial.println("Unknown range. Use now ADXL345_RANGE_16_G");
          break;    
        }

      //  accel.setFullRes(acc_usefullres);
      //  Serial.print("Full resolution");Serial.println(acc_usefullres);
          int sel = _CfgStorage.acc_datarate*100;
          switch(sel)                                   // ADXL345
          {                                             // ADXL345
            case 320000:                                // ADXL345
              accel.setDataRate(ADXL345_DATARATE_3200_HZ);
              Serial.print  ("3200 ");                  // ADXL345 
              break;                                    // ADXL345
            case 160000:              //ADXL345
              accel.setDataRate(ADXL345_DATARATE_1600_HZ);
              Serial.print  ("1600 ");                  // ADXL345 
              break;                                    // ADXL345
            case 80000:              //ADXL345
              accel.setDataRate(ADXL345_DATARATE_800_HZ);
              Serial.print  ("800 ");                   // ADXL345 
              break;                                    // ADXL345
            case 40000:              //ADXL345
              accel.setDataRate(ADXL345_DATARATE_400_HZ);
              Serial.print  ("400 ");                   // ADXL345 
              break;                                    // ADXL345
            case 20000:                                 // ADXL345
              accel.setDataRate(ADXL345_DATARATE_200_HZ);
              Serial.print  ("200 ");                   // ADXL345 
              break;                                    // ADXL345
            case 10000:                                 // ADXL345
              accel.setDataRate(ADXL345_DATARATE_100_HZ);
              Serial.print  ("100 ");                   // ADXL345 
              break;                                    // ADXL345
            case 5000:                                  // ADXL345
              accel.setDataRate(ADXL345_DATARATE_50_HZ);
              Serial.print  ("50 ");                    // ADXL345 
              break;                                    // ADXL345
            case 2500:                                  // ADXL345
              accel.setDataRate(ADXL345_DATARATE_25_HZ);
              Serial.print  ("25 ");                    // ADXL345 
              break;                                    // ADXL345
            case 1250:                                  // ADXL345
              accel.setDataRate(ADXL345_DATARATE_12_5_HZ);
              Serial.print  ("12.5 ");                  // ADXL345 
              break;                                    // ADXL345

            case 625:                                   // ADXL345
              accel.setDataRate(ADXL345_DATARATE_6_25HZ);
              Serial.print  ("6.25 ");                  // ADXL345 
              break;                                    // ADXL345
            case 313:                                   // ADXL345
              accel.setDataRate(ADXL345_DATARATE_3_13_HZ);
              Serial.print  ("3.13 ");                  // ADXL345 
              break;                                    // ADXL345
            case 156:                                   // ADXL345
              accel.setDataRate(ADXL345_DATARATE_1_56_HZ);
              Serial.print  ("1.56 ");                  // ADXL345 
              break;                                    // ADXL345
            case 78:                                    // ADXL345
              accel.setDataRate(ADXL345_DATARATE_0_78_HZ);
              Serial.print  ("0.78 ");                  // ADXL345 
              break;                                    // ADXL345

            case 39:                              //ADXL345
              accel.setDataRate(ADXL345_DATARATE_0_39_HZ);
              Serial.print  ("0.39 ");                  // ADXL345 
              break;                                    // ADXL345
            case 20:                                    // ADXL345
              accel.setDataRate(ADXL345_DATARATE_0_20_HZ);
              Serial.print  ("0.20 ");                  // ADXL345 
              break;                                    // ADXL345
            case 10:                                    // ADXL345
              accel.setDataRate(ADXL345_DATARATE_0_10_HZ);
              Serial.print  ("0.10 ");                  // ADXL345 
              break;                                    // ADXL345
            default:                                    // ADXL345
              accel.setDataRate(ADXL345_DATARATE_3200_HZ);
              Serial.print  ("???? set to 3200 ");      // ADXL345
              break;                                    // ADXL345
            
        }        
        
        /* Display some basic information on this sensor */
        displaySensorDetails();                                          // ADXL345
        
        /* Display additional settings (outside the scope of sensor_t) */
        displayDataRate();                                               // ADXL345
        displayRange();                                                  // ADXL345
        Serial.println("");                                              // ADXL345
        _CfgStorage.setupReadyVibration = true;                          // ADXL345
    }
}

void SetupTemperature()
{
  if(_CfgStorage.useTemperatureSensor) { sensors.begin(); }         // OneWireTemperatur  
  if(_CfgStorage.useTempSensorTwo) 
  { 
    if (!bmestatus) 
    {
       bmestatus = bme.begin(0x76);
      if (!bmestatus) 
      {
         Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
         Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
         Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
         Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
         Serial.print("        ID of 0x60 represents a BME 280.\n");
         Serial.print("        ID of 0x61 represents a BME 680.\n");
         _CfgStorage.useTempSensorTwo = false;
         delay(10);
      }
      else
      {
            Serial.println("BME Ready to use");
      }
    }
  }
}         

void SetupWeigth()                                                  // HX711
{                                                                   // HX711
  LoadCell.begin();                                                 // HX711
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = false; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);                           // HX711
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) // HX711
  {                                                                 // HX711
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations"); // HX711
    _CfgStorage.useWeigthSensor = false;                            // HX711
  }                                                                 // HX711
  else                                                              // HX711
  {                                                                 // HX711
    LoadCell.setCalFactor(1.0); // user set calibration value (float), initial value 1.0 may be used for this sketch  // HX711
    Serial.println("Startup is complete");                          // HX711
  }                                                                 // HX711
  if (_CfgStorage.useWeigthSensor == true) { while (!LoadCell.update());} // HX711
}                                                                   // HX711

void SetupCommunication()
{
  if(_CfgStorage.useMQTT)                                                   // MQTT
  {                                                                         // MQTT
      client.enableDebuggingMessages();                                     // MQTT
      const char *ssidchar = _CfgStorage.mqtt_SSID.c_str();                 // MQTT
      const char *wifipwdChar = _CfgStorage.mqtt_wifi_pwd.c_str();          // MQTT
      client.setWifiCredentials(ssidchar, wifipwdChar);                     // MQTT
      
      const char *beenodechar = _CfgStorage.beenodename.c_str();            // MQTT
      client.setMqttClientName(beenodechar);                                // MQTT
        
      const char *serverChar = _CfgStorage.mqtt_server.c_str();             // MQTT
      const char *usernameChar = _CfgStorage.mqttusername.c_str();          // MQTT
      const char *passwordChar = _CfgStorage.mqttpassword.c_str();          // MQTT
      client.setMqttServer(serverChar, usernameChar, passwordChar, _CfgStorage.mqtt_port.toInt());   // MQTT
  }
}

//Required implementation for MQTT Client
void onConnectionEstablished()
{
    
}

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
}                   

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
  esp_sleep_enable_timer_wakeup(_CfgStorage.deepSleepTime * uS_TO_S_FACTOR);        //DeepSleep
  Serial.println("Setup ESP32 to sleep for every " + String(_CfgStorage.deepSleepTime) +      
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
{
   if (!bmestatus) 
    {
      bmestatus = bme.begin(0x76);
      if (!bmestatus) 
      {
         Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
         Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
         Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
         Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
         Serial.print("        ID of 0x60 represents a BME 280.\n");
         Serial.print("        ID of 0x61 represents a BME 680.\n");
         _CfgStorage.useHumidity = false;
         delay(10);
      }
      else
      {
            Serial.println("BME Ready to use");
      }
    }
}

void SetupRTC()
{
     // Wire.begin was moved to Setup(). It is requird by ADXL and RTC
}
  
////////// Loop Functions 

void HandleWebPage()             // Autoconnect
{                                // Autoconnect
    portal.handleClient();       // Autoconnect
}                                // Autoconnect
      
void HandleTemperature()                                    //OneWireTemperature, BME280
{                                                           //OneWireTemperature, BME280
  if(_CfgStorage.useTemperatureSensor)//OneWireTemperature  //OneWireTemperature
  {                                                         //OneWireTemperature
    // Send the command to get temperatures
    sensors.requestTemperatures();                          //OneWireTemperature
    _SensorValues.temperatur = sensors.getTempCByIndex(0);  //OneWireTemperature
    //print the temperature in Celsius
    Serial.print("Temperature: ");                          //OneWireTemperature
    Serial.print(_SensorValues.temperatur);                 //OneWireTemperature
    Serial.print((char)176);//shows degrees character       //OneWireTemperature
    Serial.print("C  |  ");                                 //OneWireTemperature
  }                                                         //OneWireTemperature
  if(_CfgStorage.useTempSensorTwo)                     // BME280
  {                                                         // BME280
    Serial.print("Temperature = ");                         // BME280
    _SensorValues.temperatur2 = bme.readTemperature();      // BME280
    Serial.print(_SensorValues.temperatur2);                // BME280
    Serial.println(" °C");                                  // BME280
  }                                                         // BME280
}                                                           //OneWireTemperature, BME280

unsigned long t = 0;                             // HX711
void HandleWeigth()                              // HX711
{                                                // HX711
  static boolean newDataReady = 0;               // HX711
  const int serialPrintInterval = 0; //increase value to slow down serial print activity                              // HX711
  
  // check for new data/start next conversion:  
  if (LoadCell.update()) newDataReady = true;    // HX711

  // get smoothed value from the dataset:
  if (newDataReady)                              // HX711
  {                                              // HX711
    if (millis() > t + serialPrintInterval)      // HX711
    {                                            // HX711
      float i = LoadCell.getData();              // HX711
      Serial.print("Load_cell output val: ");    // HX711
      Serial.println(i);                         // HX711
      newDataReady = 0;                          // HX711
      _SensorValues.weigth = i;                  // HX711
      t = millis();                              // HX711
    }                                            // HX711
  }                                              // HX711
}                                                // HX711

void HandleVibration()                                                   //ADXL345
{
 /* Get a new sensor event */ 
  sensors_event_t event;                                                 //ADXL345
  accel.getEvent(&event);                                                //ADXL345

  _SensorValues.vibration_x =event.acceleration.x;                       //ADXL345
  _SensorValues.vibration_y =event.acceleration.y;                       //ADXL345
  _SensorValues.vibration_z =event.acceleration.z;                       //ADXL345
  
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(_SensorValues.vibration_x); Serial.print("  ");                            //ADXL345
  Serial.print("Y: "); Serial.print(_SensorValues.vibration_y); Serial.print("  ");                            //ADXL345
  Serial.print("Z: "); Serial.print(_SensorValues.vibration_z); Serial.print("  ");Serial.println("m/s^2 ");   //ADXL345
  delay(10);                                                                                                   //ADXL345
}

void appendFile(fs::FS &fs, const char * path, const char * message)                      // SDLOgging
{                                                                                         // SDLOgging
  Serial.printf("Appending to file: %s\n", path);                                         // SDLOgging
  Serial.println(message);                                                                // SDLOgging
  File file = fs.open(path, FILE_APPEND);                                                 // SDLOgging
  if(!file){                                                                              // SDLOgging
    Serial.println("Failed to open file for append");                                     // SDLOgging
    return;                                                                               // SDLOgging
  }                                                                                       // SDLOgging
  if(file.print(message)){                                                                // SDLOgging
    Serial.println("Message appended");                                                   // SDLOgging
  } else {                                                                                // SDLOgging
    Serial.println("Append failed");                                                      // SDLOgging
  }                                                                                       // SDLOgging
  file.close();                                                                           // SDLOgging
}  

void HandleCommunication()                                                                      // MQTT, SDLOgging                                                   
{                                                                                               // MQTT, SDLOgging
  String message = CreateMessage();                                                             // MQTT, SDLOgging
  const char *mqtttopicChar = _CfgStorage.mqtt_topic.c_str();                                   // MQTT, SDLOgging
  if(_CfgStorage.useMQTT)                                                                       // MQTT,
  {                                                                                             // MQTT,
    if(client.isConnected())                                                                    // MQTT,
    {                                                                                           // MQTT,
      // Subscribe to "mytopic/test" and display received message to Serial
      // client.subscribe("test/topic", [](const String & payload) { Serial.println(payload); });
    
      // Publish a message                                       
      client.publish(mqtttopicChar, message); // You can activate the retain flag by setting the third parameter to true   // MQTT,
    }                                                                                             // MQTT,
    client.loop();                                                                                // MQTT,
  }                                                                                               // MQTT,
  if(_CfgStorage.useSDLogging)                                                                    // SDLOgging
  {                                                                                               // SDLOgging
    message += "\n";                                                                              // SDLOgging
    const char *SDsd_logfilepath = _CfgStorage.sd_logfilepath.c_str();                            // SDLOgging
    const char *sdmessage = message.c_str();                                                      // SDLOgging
    appendFile(SD, SDsd_logfilepath, sdmessage);                                                  // SDLOgging
    Serial.printf("Appending to file: %s\n", SDsd_logfilepath);                                   // SDLOgging
  }                                                                                               // SDLOgging
}                                                                                                 // MQTT, SDLOgging                                                                                       // SDLOgging

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
  Serial.println(digitalRead(BUTTON_PIN));                                            // DeepSleep
  if(digitalRead(BUTTON_PIN) == 0 && _CfgStorage.useDeepSleep == true)                // DeepSleep
  {                                                                                   // DeepSleep
    _CfgStorage.useDeepSleep = false;                                                 // DeepSleep
  }                                                                                   // DeepSleep
  else                                                                                // DeepSleep
  {                                                                                   // DeepSleep
    Serial.println("Going to sleep now");                                             // DeepSleep
    delay(1000);                                                                      // DeepSleep
    Serial.flush();                                                                   // DeepSleep
    esp_deep_sleep_start();                                                           // DeepSleep
    Serial.println("This will never be printed");                                     // DeepSleep
  }                                                                                   // DeepSleep
}                                                                                     // DeepSleep

void HandlePowerManagement()
{}

void HandleHumadity()                                                                 // BME280
{                                                                                     // BME280
    Serial.print("Humidity = ");                                                      // BME280
    _SensorValues.humidity = bme.readHumidity();                                      // BME280
    Serial.print(_SensorValues.humidity);                                             // BME280
    Serial.println(" %");                                                             // BME280
}                                                                                     // BME280

void HandleRTC()
{
    delay(10);                                        // DS3231-RTC
    char buf[20];                                     // DS3231
    DateTime now = myRTC.now();                       // DS3231-RTC
    snprintf(buf,sizeof(buf),"%02d.%02d.%4d/%02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
    _SensorValues.senortime = buf;

    Serial.println(buf);                              // DS3231-RTC
    
    Serial.print(" since midnight 1/1/1970 = ");      // DS3231-RTC
    Serial.print(now.unixtime());                     // DS3231-RTC
    Serial.print("s = ");                             // DS3231-RTC
    Serial.print(now.unixtime() / 86400L);            // DS3231-RTC
    Serial.println("d");                              // DS3231-RTC
}

///////////// Functions


                                                                                      //DeepSleep


void setup() 
{
  delay(1000);                  // ESP startup
  Serial.begin(115200);         // ESP Console
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("TST");        // ESP Console
  SetupAutoConnect();           // Autoconnect
  if(_CfgStorage.useRTCSensor == true || _CfgStorage.useVibrationSensor == true || _CfgStorage.useTempSensorTwo == true || _CfgStorage.useHumidity == true) // DS3231-RTC, ADXL234, BME280
   {                                                                             // DS3231-RTC, ADXL234, BME280
    int sda = _CfgStorage.sdaio.substring(0,2).toInt();                          // DS3231-RTC, ADXL234, BME280
    int sdl = _CfgStorage.sdlio.substring(0,2).toInt();                          // DS3231-RTC, ADXL234, BME280
      Wire.begin(sda,sdl);                                                       // DS3231-RTC, ADXL234, BME280
      Serial.println("SDA/ SDL done " + _CfgStorage.sdaio + "/" + _CfgStorage.sdlio); // DS3231-RTC, ADXL234, BME280
   }                                                                            // DS3231-RTC, ADXL234, BME280  
  
  if(_CfgStorage.useVibrationSensor) { SetupVibration(); }        // Vibration
  SetupCommunication();         // MQTT, SDLogging
  SetupPowerManagement();       // Power
  if(_CfgStorage.useHumidity)  { SetupHumadity();  }              // Humadity
  if(_CfgStorage.useSDLogging) { SetupLogging();   }              // SDLogging
  if(_CfgStorage.useRTCSensor) { SetupRTC();       }              // RTC
  if(_CfgStorage.useDeepSleep) { SetupDeepSleep(); }              // DeepSleep
  if(_CfgStorage.useTemperatureSensor || _CfgStorage.useTempSensorTwo) { SetupTemperature(); }  // Temperatur
  if(_CfgStorage.useWeigthSensor) { SetupWeigth(); }              // Weigth  
}

void loop() 
{
  delay(_CfgStorage.mqtt_messagedelay.toInt());
  HandleWebPage();                                                    // Autoconnect
  if(!_CfgStorage.needToReboot)
  {
    if(_CfgStorage.useTemperatureSensor || _CfgStorage.useTempSensorTwo ) { HandleTemperature(); }     // Temperatur
    if(_CfgStorage.useWeigthSensor) { HandleWeigth(); }               // HX711
    if(_CfgStorage.useVibrationSensor && _CfgStorage.setupReadyVibration) { HandleVibration(); }  //ADXL345
    { HandleCommunication();  }                                       // MQTT, SDLogging
    HandlePowerManagement(); 
    if(_CfgStorage.useHumidity)  { HandleHumadity();        }         // BME280
    if(_CfgStorage.useRTCSensor) { HandleRTC();             }         // DS3231-RTC
    if(_CfgStorage.useDeepSleep) { HandleDeepSleep();       }         // DeepSleep
  }
  else
  {

  }
}