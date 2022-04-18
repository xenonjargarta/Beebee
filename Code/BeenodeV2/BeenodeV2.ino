/*
      Build information:  Used chip: ESP32-D0WDQ6-V3 (revision 3)
                          Used programm memory 1088654/1966080  Bytes (55%)
                          Used memory for globale variabel 46860 Bytes (14%)
                          Setting "Minimal SPIFF (1.9MB APP / with OTA/190KB SPIFF)
                          Still free memory for local variable 280820 Bytes (Max 327680 Bytes)

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

struct CfgDevice {
  String beenodename;             // Autoconnect
  String hivename;                // Autoconnect, Message
  bool needToReboot;              // Autoconnect
  String sdaio;                   // ADXL345, RTC
  String sdlio;                   // ADXL345, RTC
  bool useDeepSleep;              // DeepSleep
  int deepSleepTime;              // DeepSleep
  bool usePowerOff;               // PowerSwitch
  String esphostname;             // Autoconnect
  String OneWireBusPin;           // OneWireBus
  String MISOPIN;                 // SDCard
  String MOSIPIN;                 // SDCard
  String CSPIN;                   // SDCard
  String CLKPIN;                  // SDCard  
  String DOUTPIN;                 // HDX711
  String SCKPIN;                  // HDX711  
  String POWEROFFPIN;             // PowerSwitch  
  String TXPIN;                   // SIM  
  String RXPIN;                   // SIM  
  String devicetype;              // DeviceType
};

struct CfgMessage 
{
  bool useMQTT;                   // MQTT
  String mqtt_SSID;               // MQTT
  String mqtt_wifi_pwd;           // MQTT
  String mqttusername;            // MQTT
  String mqttpassword;            // MQTT
  String mqtt_topic;              // MQTT
  String mqtt_server;             // MQTT
  String mqtt_port;               // MQTT
  bool useSDLogging;              // SDLogging
  String mqtt_messagedelay;       // MQTT
  String sd_logfilepath;          // SDLogging
  bool useESPNow;                 // ESPNOW
  String espnow_receivermac;      // ESPNOW
  String msg_coding;              // Message
};

struct CfgStorage 
{
  bool useTemperatureSensor;      // OneWireTemperatur
  bool useVibrationSensor;        // ADXL345
  bool useRTCSensor;              // RTC
  bool setupReadyVibration;       // ADXL345
  float acc_datarate;             // ADXL345
  bool acc_usefullres;            // ADXL345
  int acc_range;                  // ADXL345
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

CfgDevice  _CfgDevice  = {"", "", false, "", "", false, 30,  false, "", "", "", "", "", "", "", "", "", "", "", ""};
CfgStorage _CfgStorage = {false, false, false, false, 3200, false, 16, false, false, false};
CfgMessage _CfgMessage = { false, "", "", "", "", "", "", "", false, "1000", "/values.txt", false, "", "l"};

SensorValues _SensorValues = {"", "", "", "", "", "", "", ""};

String CreateMessage()
{
  String message = "";
  char codingcase = _CfgMessage.msg_coding.charAt(1);
    
  switch(codingcase)
  {
    case 'l':
      Serial.println("Plain text coding selected");
      // Message part 1; ID
      message += _CfgDevice.hivename;
      message += ";";
      
      if (_CfgStorage.useRTCSensor)                     // RTC
      {                                                 // RTC
        message += _SensorValues.senortime;             // RTC
      }
      message += ";";
      
      if (_CfgStorage.useVibrationSensor)               // ADXL345
      { // ADXL345
        //message += "H1)";                             // ADXL345
        message += _SensorValues.vibration_x;           // ADXL345
        message += "&";                                 // ADXL345
        message += _SensorValues.vibration_x;           // ADXL345
        message += "&";                                 // ADXL345
        message += _SensorValues.vibration_y;           // ADXL345
      }                                                 // ADXL345
      message += ";";
      
      if (_CfgStorage.useTemperatureSensor)             // OneWireTemperatur
      { // OneWireTemperatur
        message += _SensorValues.temperatur;            // OneWireTemperatur
      }                                                 // OneWireTemperatur
      message += ";";
      
      if (_CfgStorage.useWeigthSensor)                  // HDX117
      { // HDX117
        message += _SensorValues.weigth;                // HDX117
      }                                                 // HDX117
      message += ";";
      
      if (_CfgStorage.useTempSensorTwo)                 // BME280
      { // BME280
        message += _SensorValues.temperatur2;           // BME280
      }                                                 // BME280
      message += ";";
      
      if (_CfgStorage.useHumidity)                      // BME280
      { // BME280
        message += _SensorValues.humidity;              // BME280        
      }                                                 // BME280
      message += ";";
      
      break;
    case 'i':
      Serial.println("Bit coding selected");
      break;
    case 'y':
      Serial.println("Byte coding selected");
      break;
    default:
      break;    
  }
  Serial.print("message:"); 
  Serial.print(message); 
  Serial.print(" Message length:");
  Serial.println(message.length());
  
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
#define SCK   14                                              // SDCARD
#define MISO  12                                              // SDCARD
#define MOSI  13                                              // SDCARD
#define CS    15                                              // SDCARD
#define SEALEVELPRESSURE_HPA (1013.25)                        // BME280
#define AUTOCONNECT_STARTUPTIME 10

using WiFiWebServer = WebServer;    // Autoconnect
fs::SPIFFSFS& FlashFS = SPIFFS;     // Autoconnect
WiFiWebServer server;               // Autoconnect
AutoConnect portal(server);         // Autoconnect
AutoConnectConfig config("NewBeeNode", "1234567890");         // Autoconnect
// Declare AutoConnectAux separately as a custom web page to access
// easily for each page in the post-upload handler.
AutoConnectAux auxUpload;           // Autoconnect
AutoConnectAux auxBrowse;           // Autoconnect
AutoConnectAux auxDev;              // Autoconnect
RTClib myRTC;                       // DS3231-RTC
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);       // OneWireTemperatur
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire); // OneWireTemperatur
RTC_DATA_ATTR int bootCount = 0;     // DeepSleep
SPIClass spi;                        // SDLogging
Adafruit_BME280 bme; // I2C          // BME280
unsigned bmestatus;                  // BME280

/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);   // ADXL345

const char* PARAM_SENSOR_FILE       = "/param_sensor.json";  // Autoconnect
const char* PARAM_MESSAGE_FILE      = "/param_message.json"; // Autoconnect
const char* PARAM_DEVICE_FILE       = "/param_device.json";  // Autoconnect
const char* AUX_SENSOR_SETTING_URI  = "/sensor_setting";     // Autoconnect
const char* AUX_SENSOR_SAVE_URI     = "/sensor_save";        // Autoconnect
const char* AUX_DEVICE_SETTING_URI  = "/device_settings";    // Autoconnect
const char* AUX_DEVICE_SAVE_URI     = "/device_save";        // Autoconnect
const char* AUX_MESSAGE_SETTING_URI = "/message_settings";   // Autoconnect
const char* AUX_MESSAGE_SAVE_URI    = "/message_save";       // Autoconnect
const char* AUX_SENSOR_CLEAR_URI    = "/sensor_clear";       // Autoconnect
const int HX711_dout = 0;                                    // HX711 mcu > HX711 dout pin
const int HX711_sck = 2;                                     // HX711 mcu > HX711 sck pin

EspMQTTClient client;          // using the default constructor

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Upload request custom Web page
static const char PAGE_UPLOAD[] PROGMEM = R"(
{
  "uri": "/upload",
  "title": "Upload",
  "menu": false,
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
      "uri": "/uploaddone"
    }
  ]
}
)";

// Upload result display
static const char PAGE_BROWSE[] PROGMEM = R"(
{
  "uri": "/uploaddone",
  "title": "Upload",
  "menu": false,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h4>Upload done. You need to reboot the device.</h4>",
      "style": "text-align:center;color:#2f4f4f;padding:10px;"
    },
    {
      "name": "reset",
      "type": "ACSubmit",
      "value": "Reset",
      "uri": "/_ac#rdlg"
    },
    {
      "name": "newline20",
      "type": "ACElement",
      "value": "<hr>"
    },
    {
      "name": "caption5",
      "type": "ACText",
      "value": "Parameters saved as:",
      "style": "font-family:serif;color:#4682b4;"
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

// Manage Devices
static const char PAGE_DEVICE[] PROGMEM = R"(
{
  "uri": "/device",
  "title": "Device Management",
  "menu": true,
  "element": [
    {
      "name": "reset",
      "type": "ACSubmit",
      "value": "Reset",
      "uri": "/_ac#rdlg"
    },
    {
      "name": "upload",
      "type": "ACSubmit",
      "value": "UPLOAD",
      "uri": "/upload"
    },
    {
      "name": "update",
      "type": "ACSubmit",
      "value": "Firmware update",
      "uri": "/_ac/update"
    },
    {
      "name": "disconnect",
      "type": "ACSubmit",
      "value": "Wifi disconnect",
      "uri": "/_ac/disc"
    },
    {
      "name": "newAP",
      "type": "ACSubmit",
      "value": "Configure new AP",
      "uri": "/_ac/config"
    },
    {
      "name": "openssid",
      "type": "ACSubmit",
      "value": "Open SSID",
      "uri": "/_ac/open"
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
  delay(10);                    // ESP startup
  Serial.begin(115200);         // ESP Console
  SetupAutoConnect();           // Autoconnect
  Serial.println(GET_CHIPID());        // ESP Console
  Serial.println(GET_HOSTNAME());         // ESP Console  

  if(_CfgStorage.useRTCSensor == true || _CfgStorage.useVibrationSensor == true || _CfgStorage.useTempSensorTwo == true || _CfgStorage.useHumidity == true) // DS3231-RTC, ADXL234, BME280
   {                                                                             // DS3231-RTC, ADXL234, BME280
    int sda = _CfgDevice.sdaio.substring(0,2).toInt();                          // DS3231-RTC, ADXL234, BME280
    int sdl = _CfgDevice.sdlio.substring(0,2).toInt();                          // DS3231-RTC, ADXL234, BME280
      Wire.begin(sda,sdl);                                                       // DS3231-RTC, ADXL234, BME280
      Serial.println("SDA/ SDL done " + _CfgDevice.sdaio + "/" + _CfgDevice.sdlio); // DS3231-RTC, ADXL234, BME280
   }                                                                            // DS3231-RTC, ADXL234, BME280  
  
  if(_CfgStorage.useVibrationSensor) { SetupVibration(); }        // Vibration
  SetupCommunication();         // MQTT, SDLogging
  if(_CfgDevice.usePowerOff)   { SetupPowerManagement(); }      // Power
  if(_CfgStorage.useHumidity)  { SetupHumadity();  }              // Humadity
  if(_CfgMessage.useSDLogging) { SetupLogging();   }              // SDLogging
  if(_CfgStorage.useRTCSensor) { SetupRTC();       }              // RTC
  if(_CfgDevice.useDeepSleep)  { SetupDeepSleep(); }              // DeepSleep
  if(_CfgStorage.useTemperatureSensor || _CfgStorage.useTempSensorTwo) { SetupTemperature(); }  // Temperatur
  if(_CfgStorage.useWeigthSensor) { SetupWeigth(); }              // Weigth  
}

////////// Setup function
void SetupLogging()                            // SDLogging
{                                              // SDLogging
  spi = SPIClass(VSPI);                        // SDLogging
  spi.begin(SCK, MISO, MOSI, CS);              // SDLogging

  if (!SD.begin(CS,spi,80000000))              // SDLogging
  {                                            // SDLogging
    Serial.println("Card Mount Failed");       // SDLogging
    _CfgMessage.useSDLogging = false;          // SDLogging
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

void SetupAutoConnect()
{ 
  SPIFFS.begin();                                     // Autoconnect
  File devicepage = SPIFFS.open("/devicepage.json", "r");  // Autoconnect
  if(portal.load(devicepage))                              // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& device_setting = *portal.aux(AUX_DEVICE_SETTING_URI);// Autoconnect
    loadDeviceParams(device_setting, args);                              // Autoconnect
    portal.on(AUX_DEVICE_SETTING_URI, loadDeviceParams);                 // Autoconnect
    portal.on(AUX_DEVICE_SAVE_URI, saveParamsDevice);                    // Autoconnect
  }
  else
  {
    Serial.println("Unable to load devicepage.json. Load now default device managemnt page");// Autoconnect
    auxDev.load(PAGE_DEVICE);      // Autoconnect
    portal.join({auxDev});  // Autoconnect
  }
  devicepage.close();                                 // Autoconnect
 
  File sensorpage = SPIFFS.open("/sensorpage.json", "r");  // Autoconnect
  if(portal.load(sensorpage))                              // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& sensor_setting = *portal.aux(AUX_SENSOR_SETTING_URI);// Autoconnect
    loadSensorParams(sensor_setting, args);                              // Autoconnect
    portal.on(AUX_SENSOR_SETTING_URI, loadSensorParams);                 // Autoconnect
    portal.on(AUX_SENSOR_SAVE_URI, saveParamsSensor);                    // Autoconnect
  }                                                                      // Autoconnect
  sensorpage.close();                                 // Autoconnect

  File messagepage = SPIFFS.open("/messagepage.json", "r");  // Autoconnect
  if(portal.load(messagepage))                              // Autoconnect
  {                                                   // Autoconnect
    PageArgument  args;                               // Autoconnect
    AutoConnectAux& message_setting = *portal.aux(AUX_MESSAGE_SETTING_URI);// Autoconnect
    loadMessageParams(message_setting, args);                              // Autoconnect
    portal.on(AUX_MESSAGE_SETTING_URI, loadMessageParams);                 // Autoconnect
    portal.on(AUX_MESSAGE_SAVE_URI, saveMessageSensor);                    // Autoconnect
  }                                                                      // Autoconnect
  messagepage.close();                                 // Autoconnect
  SPIFFS.end();                                       // Autoconnect

   // Start the filesystem
  FlashFS.begin(FORMAT_ON_FAIL);  // Autoconnect

  // Attach the custom web pages
  auxUpload.load(PAGE_UPLOAD);   // Autoconnect
  auxBrowse.load(PAGE_BROWSE);   // Autoconnect
  auxBrowse.on(postUpload);      // Autoconnect
  portal.join({auxUpload, auxBrowse});  // Autoconnect

  // The handleFileRead callback function provides an echo back of the
  // uploaded file to the client. You can include the uploaded file in
  // the response by embedding the object HTML tag in your custom web page.
  // The client browser will request to get the content according to
  // the link of the object tag, and the request can be caught by onNotFound handler.
  portal.onNotFound(handleFileRead);                // Autoconnect
  config.ota = AC_OTA_BUILTIN;                      // Autoconnect
  config.title = _CfgDevice.beenodename + " v2.05.4"; // Autoconnect
  config.homeUri = "/_ac";                          // Autoconnect
  config.bootUri = AC_ONBOOTURI_HOME;               // Autoconnect
  // Reconnect and continue publishing even if WiFi is disconnected.
  config.autoReconnect = true;                      // Autoconnect
  config.reconnectInterval = 1;                     // Autoconnect
  portal.config(config);                            // Autoconnect
  portal.begin();                                   // Autoconnect
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
  if(_CfgMessage.useMQTT)                                                   // MQTT
  {                                                                         // MQTT
      client.enableDebuggingMessages();                                     // MQTT
      const char *ssidchar = _CfgMessage.mqtt_SSID.c_str();                 // MQTT
      const char *wifipwdChar = _CfgMessage.mqtt_wifi_pwd.c_str();          // MQTT
      client.setWifiCredentials(ssidchar, wifipwdChar);                     // MQTT
      
      const char *beenodechar = _CfgDevice.beenodename.c_str();            // MQTT
      client.setMqttClientName(beenodechar);                                // MQTT
        
      const char *serverChar = _CfgMessage.mqtt_server.c_str();             // MQTT
      const char *usernameChar = _CfgMessage.mqttusername.c_str();          // MQTT
      const char *passwordChar = _CfgMessage.mqttpassword.c_str();          // MQTT
      client.setMqttServer(serverChar, usernameChar, passwordChar, _CfgMessage.mqtt_port.toInt());   // MQTT
  }
}

//Required implementation for MQTT Client
void onConnectionEstablished()
{
    
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
  esp_sleep_enable_timer_wakeup(_CfgDevice.deepSleepTime * uS_TO_S_FACTOR);        //DeepSleep
  Serial.println("Setup ESP32 to sleep for every " + String(_CfgDevice.deepSleepTime) +      
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
{
    pinMode(_CfgDevice.POWEROFFPIN.toInt(), OUTPUT);
      delay(1000);            // Wartet eine Sekunde
  digitalWrite(_CfgDevice.POWEROFFPIN.toInt(), LOW);
  Serial.println(_CfgDevice.POWEROFFPIN.toInt());
}

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

void loop() 
{
  HandleWebPage();                                                    // Autoconnect

  if(!_CfgDevice.needToReboot)
  {
    if(_CfgStorage.useTemperatureSensor || _CfgStorage.useTempSensorTwo ) { HandleTemperature(); }     // Temperatur
    if(_CfgStorage.useWeigthSensor) { HandleWeigth(); }               // HX711
    if(_CfgStorage.useVibrationSensor && _CfgStorage.setupReadyVibration) { HandleVibration(); }  //ADXL345
    { HandleCommunication();  }                                       // MQTT, SDLogging
    if(_CfgStorage.useHumidity)  { HandleHumadity();        }         // BME280
    if(_CfgStorage.useRTCSensor) { HandleRTC();             }         // DS3231-RTC
    if(_CfgDevice.usePowerOff)   { HandlePowerManagement(); }         // PowerOff
    if(_CfgDevice.useDeepSleep)  { HandleDeepSleep();       }         // DeepSleep
    if(_CfgMessage.useMQTT) { delay(_CfgMessage.mqtt_messagedelay.toInt()); } // MQTT
  }
  else
  {
        // System waits for reboot just wbepage is enabled
  }
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
  if(_CfgStorage.useTempSensorTwo)                          // BME280
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

void HandleCommunication()                                                                      // MQTT, SDLOgging                                                   
{                                                                                               // MQTT, SDLOgging
  String message = CreateMessage();                                                             // MQTT, SDLOgging
  const char *mqtttopicChar = _CfgMessage.mqtt_topic.c_str();                                   // MQTT, SDLOgging
  if(_CfgMessage.useMQTT)                                                                       // MQTT,
  {                                                                                             // MQTT,
    if(client.isConnected())                                                                    // MQTT,
    {                                                                                           // MQTT,
      // Subscribe to "mytopic/Hives" and display received message to Serial
      // client.subscribe("test/topic", [](const String & payload) { Serial.println(payload); });
    
      // Publish a message                                       
      client.publish(mqtttopicChar, message); // You can activate the retain flag by setting the third parameter to true   // MQTT,
    }                                                                                             // MQTT,
    client.loop();                                                                                // MQTT,
  }                                                                                               // MQTT,
  if(_CfgMessage.useSDLogging)                                                                    // SDLOgging
  {                                                                                               // SDLOgging
    message += "\n";                                                                              // SDLOgging
    const char *SDsd_logfilepath = _CfgMessage.sd_logfilepath.c_str();                            // SDLOgging
    const char *sdmessage = message.c_str();                                                      // SDLOgging
    appendFile(SD, SDsd_logfilepath, sdmessage);                                                  // SDLOgging
    Serial.printf("Appending to file: %s\n", _CfgMessage.sd_logfilepath);                         // SDLOgging
  }                                                                                               // SDLOgging
}                                                                                                 // MQTT, SDLOgging

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
}                                                                                         // SDLOgging

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
    Serial.println("Going to sleep now");                                             // DeepSleep
    delay(1000);                                                                      // DeepSleep
    Serial.flush();                                                                   // DeepSleep
    esp_deep_sleep_start();                                                           // DeepSleep
    Serial.println("This will never be printed");                                     // DeepSleep                                                                                 // DeepSleep
}                                                                                     // DeepSleep

void HandlePowerManagement()
{
  Serial.println("In per mode");
  digitalWrite(_CfgDevice.POWEROFFPIN.toInt(), HIGH);
  delay(1000);            // Wartet eine Sekunde
  digitalWrite(_CfgDevice.POWEROFFPIN.toInt(), LOW);
  delay(1000);            // Wartet eine Sekunde
  digitalWrite(_CfgDevice.POWEROFFPIN.toInt(), HIGH);
}

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

// publishInterval = period.value().substring(0, 2).toInt() * 1000;

void getDeviceParams(AutoConnectAux& aux) 
{
  _CfgDevice.beenodename = aux[F("beenodename")].value;                           // Autoconnect
  _CfgDevice.beenodename.trim();                                                  // Autoconnect
  _CfgDevice.hivename = aux[F("hiveid")].value;                                 // Autoconnect
  _CfgDevice.hivename.trim();                                                     // Autoconnect
  _CfgDevice.useDeepSleep  = aux[F("useDeepSleep")].as<AutoConnectCheckbox>().checked; // DeepSleep 
  _CfgDevice.deepSleepTime = aux[F("deepSleepTime")].value.toInt();               // DeepSleep
  _CfgDevice.sdaio = aux[F("sdaio")].value;                                    // ADXL, RTC
  _CfgDevice.sdaio.trim();                                                     // ADXL, RTC
  _CfgDevice.sdlio = aux[F("sdlio")].value;                                    // Autoconnect
  _CfgDevice.sdlio.trim();                                                     // ADXL, RTC

  _CfgDevice.esphostname = aux[F("esphostname")].value;                        // 
  _CfgDevice.esphostname.trim();                                               // 
  _CfgDevice.OneWireBusPin = aux[F("OneWireBusPin")].value;                    // 
  _CfgDevice.OneWireBusPin.trim();                                             // 

  _CfgDevice.MISOPIN = aux[F("MISOPIN")].value;                             // 
  _CfgDevice.MISOPIN.trim();                                               // 
  _CfgDevice.MOSIPIN = aux[F("MOSIPIN")].value;                    // 
  _CfgDevice.MOSIPIN.trim();                                             // 

    _CfgDevice.CSPIN = aux[F("CSPIN")].value;                        // 
  _CfgDevice.CSPIN.trim();                                               // 
  _CfgDevice.CLKPIN = aux[F("CLKPIN")].value;                    // 
  _CfgDevice.CLKPIN.trim();                                             // 

  _CfgDevice.POWEROFFPIN = aux[F("POWEROFFPIN")].value;                        // 
  _CfgDevice.POWEROFFPIN.trim();                                               // 
  _CfgDevice.DOUTPIN = aux[F("DOUTPIN")].value;                    // 
  _CfgDevice.DOUTPIN.trim();                                             // 

  _CfgDevice.SCKPIN = aux[F("SCKPIN")].value;                    // 
  _CfgDevice.SCKPIN.trim();                                             // 

  _CfgDevice.TXPIN = aux[F("TXPIN")].value;                    // 
  _CfgDevice.TXPIN.trim();                                             // 

  _CfgDevice.RXPIN = aux[F("RXPIN")].value;                    // 
  _CfgDevice.RXPIN.trim();                                             // 

  _CfgDevice.usePowerOff  = aux[F("usePowerOff")].as<AutoConnectCheckbox>().checked; // DeepSleep 
  
  AutoConnectRadio& dr = aux[F("devicetype")].as<AutoConnectRadio>();         // MESSAGE
  _CfgDevice.devicetype = dr.value();                                          // MESSAGE
  
  Serial.println(" ");                                              // Autoconnect 
  Serial.println("Curren Configuration:");                          // Autoconnect 
  Serial.print("Bee node name: ");                                  // Autoconnect 
  Serial.println(_CfgDevice.beenodename);                          // Autoconnect 
  Serial.print("Hive name: ");                                      // Autoconnect 
  Serial.println(_CfgDevice.hivename);                             // Autoconnect 
  Serial.print("Used deep sleep: ");                                // Autoconnect 
  Serial.println(_CfgDevice.useDeepSleep);                         // Autoconnect 
  Serial.print("Deep sleep time: ");                                // Autoconnect 
  Serial.println(_CfgDevice.deepSleepTime);                        // Autoconnect 

  Serial.print("esphostname: ");                                // Autoconnect 
  Serial.println(_CfgDevice.esphostname);                        // Autoconnect 

  Serial.print("MISOPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.MISOPIN);                        // Autoconnect 
  Serial.print("MOSIPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.MOSIPIN);                        // Autoconnect 

  Serial.print("CSPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.CSPIN);                        // Autoconnect 
  Serial.print("CLKPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.CLKPIN);                        // Autoconnect 

  
  Serial.print("POWEROFFPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.POWEROFFPIN);                        // Autoconnect 
  Serial.print("DOUTPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.DOUTPIN);                        // Autoconnect 

  Serial.print("SCKPIN: ");                                // Autoconnect 
  Serial.println(_CfgDevice.SCKPIN);                        // Autoconnect 
  
  Serial.print("sdaio: ");                                          // ADXL345, RTC  
  Serial.println(_CfgDevice.sdaio);                                // ADXL345, RTC 
  Serial.print("sdlio: ");                                          // ADXL345, RTC 
  Serial.println(_CfgDevice.sdlio);                                // ADXL345, RTC 

  
  Serial.print("RXPIN: ");                                          // ADXL345, RTC  
  Serial.println(_CfgDevice.RXPIN);                                // ADXL345, RTC 
  Serial.print("TXPIN: ");                                          // ADXL345, RTC 
  Serial.println(_CfgDevice.TXPIN);                                // ADXL345, RTC 

  Serial.print("devicetype: ");                                          // 
  Serial.println(_CfgDevice.devicetype);                                // 

  
  Serial.print("usePowerOff: ");                                          // 
  Serial.println(_CfgDevice.usePowerOff);                                // 
  
  Serial.println("CFG Loaded end");                                  // Autoconnect 
  Serial.println(" ");                                               // Autoconnect 

  Serial.println(GET_CHIPID());
  Serial.println(GET_HOSTNAME());

}

void getMessageParams(AutoConnectAux& aux) 
{
  _CfgMessage.useSDLogging = aux[F("useSDLogging")].as<AutoConnectCheckbox>().checked; // SDLogging
  _CfgMessage.mqtt_server = aux[F("mqtt_server")].value;                          // MQTT
  _CfgMessage.mqtt_server.trim();                                                 // MQTT

  _CfgMessage.useMQTT = aux[F("useMQTT")].as<AutoConnectCheckbox>().checked;    // MQTT
  _CfgMessage.mqtt_SSID = aux[F("mqtt_SSID")].value;                            // MQTT
  _CfgMessage.mqtt_SSID.trim();                                                 // MQTT
  _CfgMessage.mqtt_wifi_pwd = aux[F("mqtt_wifi_pwd")].value;                    // MQTT
  _CfgMessage.mqtt_wifi_pwd.trim();                                             // MQTT
  _CfgMessage.mqttusername = aux[F("mqttusername")].value;                      // MQTT
  _CfgMessage.mqttusername.trim();                                              // MQTT
  _CfgMessage.mqttpassword = aux[F("mqttpassword")].value;                      // MQTT
  _CfgMessage.mqttpassword.trim();                                              // MQTT
  _CfgMessage.mqtt_topic = aux[F("mqtt_topic")].value;                          // MQTT
  _CfgMessage.mqtt_topic.trim();                                                // MQTT
  _CfgMessage.mqtt_port = aux[F("mqtt_port")].value;                            // MQTT
  _CfgMessage.mqtt_port.trim();                                                 // MQTT
  _CfgMessage.mqtt_messagedelay = aux[F("mqtt_messagedelay")].value;            // MQTT
  _CfgMessage.mqtt_messagedelay.trim();                                         // MQTT
  _CfgMessage.sd_logfilepath = aux[F("sd_logfilepath")].value;                  // SDLogging
  _CfgMessage.sd_logfilepath.trim();                                            // SDLOgging   
  _CfgMessage.useESPNow = aux[F("useESPNow")].as<AutoConnectCheckbox>().checked;// ESPNOW
  _CfgMessage.espnow_receivermac = aux[F("espnow_receivermac")].value;          // ESPNOW
  _CfgMessage.espnow_receivermac.trim();                                        // ESPNOW
  AutoConnectRadio& dr = aux[F("msg_coding")].as<AutoConnectRadio>();         // MESSAGE
  _CfgMessage.msg_coding = dr.value();                                          // MESSAGE


  Serial.print("useMQTT: ");                                        // MQTT 
  Serial.println(_CfgMessage.useMQTT);                              // MQTT

  Serial.print("mqtt_SSID: ");                                      // MQTT 
  Serial.println(_CfgMessage.mqtt_SSID);                            // MQTT

  Serial.print("mqtt_wifi_pwd: ");                                  // MQTT 
  Serial.println(_CfgMessage.mqtt_wifi_pwd);                        // MQTT

  Serial.print("mqttusername: ");                                   // MQTT 
  Serial.println(_CfgMessage.mqttusername);                         // MQTT

  Serial.print("mqttpassword: ");                                   // MQTT 
  Serial.println(_CfgMessage.mqttpassword);                         // MQTT
    
  Serial.print("mqtt_topic: ");                                     // MQTT 
  Serial.println(_CfgMessage.mqtt_topic);                           // MQTT

  Serial.print("mqtt_server: ");                                    // MQTT 
  Serial.println(_CfgMessage.mqtt_server);                          // MQTT

  Serial.print("mqtt_server: ");                                    // MQTT 
  Serial.println(_CfgMessage.mqtt_port);                            // MQTT

  Serial.print("mqtt_messagedelay: ");                              // MQTT 
  Serial.println(_CfgMessage.mqtt_messagedelay);                    // MQTT
  
  Serial.print("sd_logfilepath: ");                                 // SDLogging 
  Serial.println(_CfgMessage.sd_logfilepath);                       // SDLogging

  Serial.print("useESPNow: ");                              // ESPNOW 
  Serial.println(_CfgMessage.useESPNow);                    // ESPNOW
  
  Serial.print("espnow_receivermac: ");                                 // ESPNOW 
  Serial.println(_CfgMessage.espnow_receivermac);                       // ESPNOW

  Serial.print("msg_coding: ");                                 // MESSAGE 
  Serial.println(_CfgMessage.msg_coding);                       // MESSAGE
  
  
  Serial.println("CFG Loaded end");                                  // Autoconnect 
  Serial.println(" ");                                               // Autoconnect 
}

void getSensorParams(AutoConnectAux& aux) 
{
  _CfgStorage.useTemperatureSensor  = aux[F("useTemperatureSensor")].as<AutoConnectCheckbox>().checked; // Autoconnect 
  _CfgStorage.useVibrationSensor  = aux[F("useVibrationSensor")].as<AutoConnectCheckbox>().checked; // ADXL 
  _CfgStorage.useRTCSensor  = aux[F("useRTCSensor")].as<AutoConnectCheckbox>().checked; // RTC 
  AutoConnectRadio& dr = aux[F("acc_datarate")].as<AutoConnectRadio>();            // ADXL
  _CfgStorage.acc_datarate = dr.value().toDouble();                                // ADXL
  _CfgStorage.acc_usefullres  = aux[F("acc_usefullres")].as<AutoConnectCheckbox>().checked; // ADXL
  AutoConnectRadio& range = aux[F("acc_range")].as<AutoConnectRadio>();           // ADXL
  _CfgStorage.acc_range = range.value().toInt();                                  // ADXL
  _CfgStorage.useWeigthSensor = aux[F("useWeigthSensor")].as<AutoConnectCheckbox>().checked;       // HDX711  
  _CfgStorage.useTempSensorTwo = aux[F("useTempSensorTwo")].as<AutoConnectCheckbox>().checked;    // BME280
  _CfgStorage.useHumidity = aux[F("useHumidity")].as<AutoConnectCheckbox>().checked;    // BME280

  Serial.println(" ");                                              // Autoconnect 
  Serial.println("Curren Configuration:");                          // Autoconnect 
  
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

  Serial.print("useTempSensorTwo: ");                          // BME280
  Serial.println(_CfgStorage.useTempSensorTwo);                // BME280

  Serial.print("useHumidity: ");                                    // BME280 
  Serial.println(_CfgStorage.useHumidity);                          // BME280ogging

  Serial.print("useWeigthSensor: ");                          // HDX711 
  Serial.println(_CfgStorage.useWeigthSensor);                // HDX711
  
  Serial.println("CFG Loaded end");                                  // Autoconnect 
  Serial.println(" ");                                               // Autoconnect 

  Serial.println(GET_CHIPID());
  Serial.println(GET_HOSTNAME());

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


// Load parameters saved with saveParams from SPIFFS into the
// elements defined in /message_setting JSON.
String loadMessageParams(AutoConnectAux& aux, PageArgument& args)     // Autoconnect
{                                                                     // Autoconnect
  (void)(args);                                                       // Autoconnect
  Serial.print(PARAM_MESSAGE_FILE);                                   // Autoconnect
  File param = FlashFS.open(PARAM_MESSAGE_FILE, "r");                 // Autoconnect
  if (param) {                                                        // Autoconnect
    if (aux.loadElement(param)) 
    {                                                                 // Autoconnect
      getMessageParams(aux);                                          // Autoconnect
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
  sensor_setting.saveElement(param, {"useTemperatureSensor", "useVibrationSensor", "useRTCSensor", +
                                     "acc_datarate", "acc_range", "acc_usefullres", "useSDLogging", +
                                     "useWeigthSensor" ,"useTempSensorTwo",   "useHumidity"});     // Autoconnect
  param.close();                                                            // Autoconnect
  _CfgDevice.needToReboot = true;                                          // Autoconnect
  Serial.println("Need to reboot device");                                  // Autoconnect

  // Echo back saved parameters to AutoConnectAux page.
  aux[F("useTemperatureSensor")].value = _CfgStorage.useTemperatureSensor;     // Autoconnect
  aux[F("useVibrationSensor")].value = _CfgStorage.useVibrationSensor;         // Autoconnect
  aux[F("useRTCSensor")].value = _CfgStorage.useRTCSensor;                     // Autoconnect
  aux[F("acc_datarate")].value = _CfgStorage.acc_datarate;                     // Autoconnect
  aux[F("acc_range")].value = _CfgStorage.acc_range;                           // Autoconnect
  aux[F("acc_usefullres")].value = _CfgStorage.acc_usefullres;                 // Autoconnect
  aux[F("useWeigthSensor")].value = _CfgStorage.useWeigthSensor;               // HDX711
  aux[F("useTempSensorTwo")].value = _CfgStorage.useTempSensorTwo;             // BME280
  aux[F("useHumidity")].value = _CfgStorage.useHumidity;                       // BME280

  return String();                                                             // Autoconnect
}                                                                              // Autoconnect

// Load parameters saved with saveParams from SPIFFS into the
// elements defined in /device_settings JSON.
String loadDeviceParams(AutoConnectAux& aux, PageArgument& args)      // Autoconnect
{                                                                     // Autoconnect
  (void)(args);                                                       // Autoconnect
  Serial.print(PARAM_DEVICE_FILE);                                    // Autoconnect
  File param = FlashFS.open(PARAM_DEVICE_FILE, "r");                  // Autoconnect
  if (param) {                                                        // Autoconnect
    if (aux.loadElement(param)) {                                     // Autoconnect
      getDeviceParams(aux);                                           // Autoconnect
      Serial.println(" loaded");                                      // Autoconnect
    }                                                                 // Autoconnect
    else                                                              // Autoconnect
      Serial.println(" failed to load");                              // Autoconnect
    param.close();                                                    // Autoconnect
  }                                                                   // Autoconnect
  else                                                                // Autoconnect
    Serial.println(" open failed");                                   // Autoconnect
  return String("");                                                  // Autoconnect
}        // Autoconnect
                                                                                
// Save the value of each element entered by '/device_setting' to the
// parameter file. The saveParamsSensor as below is a callback function of
// /sensor_save. When invoking this handler, the input value of each
// element is already stored in '/device_setting'.
// In the Sketch, you can output to stream its elements specified by name.
String saveParamsDevice(AutoConnectAux& aux, PageArgument& args) {         // Autoconnect
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   device_setting = *portal.aux(portal.where());          // Autoconnect
  getDeviceParams(device_setting);                                         // Autoconnect

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /sensor_setting, it is necessary to get
  // the AutoConnectAux object of /sensor_setting.
  File param = FlashFS.open(PARAM_DEVICE_FILE, "w");                        // Autoconnect
  device_setting.saveElement(param, {"beenodename", "hiveid" , "useDeepSleep" , "deepSleepTime", "sdaio", "sdlio",+
                                     "SCKPIN",      "DOUTPIN", "POWEROFFPIN",   "CLKPIN",        "CSPIN", "MOSIPIN",+
                                     "MISOPIN",     "OneWireBusPin",            "esphostname", "TXPIN", "RXPIN" ,"devicetype" , "usePowerOff"});     // Autoconnect*/
  param.close();                                                            // Autoconnect
  _CfgDevice.needToReboot = true;                                           // Autoconnect
  Serial.println("Need to reboot device");                                  // Autoconnect

  // Echo back saved parameters to AutoConnectAux page.
  aux[F("beenodename")].value = _CfgDevice.beenodename;                       // Autoconnect
  aux[F("hiveid")].value = _CfgDevice.hivename;                             // Autoconnect
  aux[F("deepSleepTime")].value = _CfgDevice.deepSleepTime;                   // Autoconnect
  aux[F("useDeepSleep")].value = _CfgDevice.useDeepSleep;                     // Autoconnect
  aux[F("sdaio")].value = _CfgDevice.sdaio;                                   // AXDL345, RTC
  aux[F("sdlio")].value = _CfgDevice.sdlio;                                   // AXDL345, RTC
  aux[F("SCKPIN")].value = _CfgDevice.SCKPIN;                       // 
  aux[F("DOUTPIN")].value = _CfgDevice.DOUTPIN;                             // 
  aux[F("POWEROFFPIN")].value = _CfgDevice.POWEROFFPIN;                   // 
  aux[F("CLKPIN")].value = _CfgDevice.CLKPIN;                     // 
  aux[F("CSPIN")].value = _CfgDevice.CSPIN;                                   //                                   // 
  aux[F("MOSIPIN")].value = _CfgDevice.MOSIPIN;                       // 
  aux[F("MISOPIN")].value = _CfgDevice.MISOPIN;                             // 
  aux[F("OneWireBusPin")].value = _CfgDevice.OneWireBusPin;                                   // 
  aux[F("esphostname")].value = _CfgDevice.esphostname;                                  //
  aux[F("TXPIN")].value = _CfgDevice.TXPIN;                                   // 
  aux[F("RXPIN")].value = _CfgDevice.RXPIN;                                  //
  aux[F("devicetype")].value = _CfgDevice.devicetype;                                  //
  aux[F("usePowerOff")].value = _CfgDevice.usePowerOff;                                  //

    
  return String();                                                             // Autoconnect
}      // Autoconnect



// Save the value of each element entered by '/sensor_setting' to the
// parameter file. The saveParamsSensor as below is a callback function of
// /sensor_save. When invoking this handler, the input value of each
// element is already stored in '/sensor_setting'.
// In the Sketch, you can output to stream its elements specified by name.
String saveMessageSensor(AutoConnectAux& aux, PageArgument& args) {         // Autoconnect
  // The 'where()' function returns the AutoConnectAux that caused
  // the transition to this page.
  AutoConnectAux&   message_setting = *portal.aux(portal.where());          // Autoconnect
  getMessageParams(message_setting);                                        // Autoconnect

  // The entered value is owned by AutoConnectAux of /mqtt_setting.
  // To retrieve the elements of /sensor_setting, it is necessary to get
  // the AutoConnectAux object of /sensor_setting.
  File param = FlashFS.open(PARAM_MESSAGE_FILE, "w");                        // Autoconnect
  message_setting.saveElement(param, {"useSDLogging", "useMQTT", "mqtt_SSID", "mqtt_wifi_pwd", "mqttusername", "mqttpassword", "mqtt_topic",  +
                                      "mqtt_server", "mqtt_port", "mqtt_messagedelay" , "sd_logfilepath", "espnow_receivermac", "useESPNow", "msg_coding"});     // Autoconnect
  param.close();                                                             // Autoconnect
  _CfgDevice.needToReboot = true;                                            // Autoconnect
  Serial.println("Need to reboot device");                                   // Autoconnect

  // Echo back saved parameters to AutoConnectAux page.
  aux[F("useSDLogging")].value = _CfgMessage.useSDLogging;                     // SD Logging
  aux[F("mqtt_topic")].value = _CfgMessage.mqtt_topic;                         // MQTT
  aux[F("mqtt_server")].value = _CfgMessage.mqtt_server;                       // MQTT
  aux[F("mqtt_messagedelay")].value = _CfgMessage.mqtt_messagedelay;           // MQTT
  aux[F("useMQTT")].value = _CfgMessage.useMQTT;                               // MQTT
  aux[F("mqtt_SSID")].value = _CfgMessage.mqtt_SSID;                           // MQTT
  aux[F("mqtt_wifi_pwd")].value = _CfgMessage.mqtt_wifi_pwd;                   // MQTT
  aux[F("mqttusername")].value = _CfgMessage.mqttusername;                     // MQTT
  aux[F("mqttpassword")].value = _CfgMessage.mqttpassword;                     // MQTT

  aux[F("sd_logfilepath")].value = _CfgMessage.sd_logfilepath;                 // SD Logging

  aux[F("mqtt_port")].value = _CfgMessage.mqtt_port;                           // MQTT
  aux[F("espnow_receivermac")].value = _CfgMessage.espnow_receivermac;         // ESPNOW
  aux[F("useESPNow")].value = _CfgMessage.useESPNow;                           // ESPNOW
  aux[F("msg_coding")].value = _CfgMessage.msg_coding;                           // ESPNOW
  
  return String();                                                             // Autoconnect
}                                                                              // Autoconnect

void listAllFiles()
{
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file)
  {
      Serial.print("FILE: ");
      Serial.println(file.name());
 
      file = root.openNextFile();
  }
}

void formatspiff()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
 
  Serial.println("\n\n----Listing files before format----");
  listAllFiles();
 
  bool formatted = SPIFFS.format();
 
  if(formatted)
  {
    Serial.println("\n\nSuccess formatting");
  }
  else
  {
    Serial.println("\n\nError formatting");
  }
 
  Serial.println("\n\n----Listing files after format----");
  listAllFiles();
}
