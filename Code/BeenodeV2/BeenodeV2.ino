/*    
    Build information:  Used chip: ESP32-D0WDQ6-V3 (revision 3)
                        Used programm memory 999406/1966080  Bytes (50%) 
                        Used memory for globale variabel 41884 Bytes (12%)
                        Setting "Minimal SPIFF (1.9MB APP / with OTA/190KB SPIFF)
                        Still free memory for local variable 285796 Bytes (Max 327680 Bytes)
    
    Feature:            (x) Webpage 
                        (x) Wifi Lifecycle
                        (in progress) Configuration Creation (BeeSensors)
                        (x) Configuration management (BeeSensors)
                        (x) Configuration management (MQTT)
                        ( ) MQTT
                        (in progress) Vibration Sensor (ADXL345)
                        ( ) Weight Sensor (HX711)
                        (in progress) Temperature Sensor (DS)
                        ( ) Humadity Sensor ()
                        (in progress) RTC (DS)
                        ( ) Power management
                        ( ) Deep Sleep (ESP)
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
#include <DS3231.h>                 // DS3231
#include <OneWire.h>                // OneWireTemperatur
#include <DallasTemperature.h>      // OneWireTemperatur

// ------------------------------ Definitions (Global) ------------------------------------------
#define FORMAT_ON_FAIL  true        // Autoconnect
#define AUTOCONNECT_MENUCOLOR_TEXT        "#fffacd" // Autoconnect
#define AUTOCONNECT_MENUCOLOR_BACKGROUND  "#696969" // Autoconnect
#define AUTOCONNECT_MENUCOLOR_ACTIVE      "#808080" // Autoconnect
//#define AUTOCONNECT_URI         "/_ac"              // Autoconnect
using WiFiWebServer = WebServer;    // Autoconnect
fs::SPIFFSFS& FlashFS = SPIFFS;     // Autoconnect
WiFiWebServer server;               // Autoconnect
AutoConnect portal(server);         // Autoconnect
AutoConnectConfig config("NewBeeNode", "1234567890"); // Autoconnect
// Declare AutoConnectAux separately as a custom web page to access
// easily for each page in the post-upload handler.
AutoConnectAux auxUpload;           // Autoconnect
AutoConnectAux auxBrowse;           // Autoconnect
RTClib myRTC;                       // DS3231

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 19              // OneWireTemperatur

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);       // OneWireTemperatur 

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire); // OneWireTemperatur


/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);   // ADXL345

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

   Wire.begin(18,23);           // RTC, ADXL234
  SetupAutoConnect();           // Autoconnect
  SetupVibration();             // Vibration
  SetupCommunication();         // Communication
  SetupDeepSleep();             // DeepSleep
  SetupPowerManagement();       // Power
  SetupHumadity();              // Humadity
  SetupRTC();                   // RTC
}

////////// Setup function

void SetupAutoConnect()
{
  SPIFFS.begin();               // Autoconnect
  File page = SPIFFS.open("/mqttpage.json", "r");     // Autoconnect
  portal.load(page);                                  // Autoconnect
  page.close();                                       // Autoconnect
  File page1 = SPIFFS.open("/sensorpage.json", "r");  // Autoconnect
  portal.load(page1);                                 // Autoconnect
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
  config.title = "BeeNode 2A 200320221349";         // Autoconnect
  config.homeUri = "/_ac";                          // Autoconnect
  portal.config(config);                            // Autoconnect
  portal.begin();                                   // Autoconnect
}

void SetupVibration()
{
   Serial.println("Accelerometer Test"); Serial.println("");                    //ADXL345
  
 /* Initialise the sensor */  
    if(!accel.begin())
    {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);
  // accel.setRange(ADXL345_RANGE_8_G);
  // accel.setRange(ADXL345_RANGE_4_G);
  // accel.setRange(ADXL345_RANGE_2_G);
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();
  displayRange();
  Serial.println("");
}

void SetupTemperature()
{
    sensors.begin();  // OneWireTemperatur  
}

void SetupWeigth()
{}

void SetupCommunication()
{}

void SetupDeepSleep()
{}

void SetupPowerManagement()
{}

void SetupHumadity()
{}

void SetupRTC()
{
     
}

void loop() 
{
  HandleWebPage();              // Autoconnect
  HandleTemperature(); 
  HandleWeigth(); 
  HandleVibration(); 
  HandleCommunication(); 
  HandleDeepSleep(); 
  HandlePowerManagement(); 
  HandleHumadity(); 
  HandleRTC();
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

void HandleVibration()
{
 /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);
 
  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print("  ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print("  ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print("  ");Serial.println("m/s^2 ");
 delay(1000);
}

void HandleCommunication()
{}

void HandleDeepSleep()
{}

void HandlePowerManagement()
{}

void HandleHumadity()
{}

void HandleRTC()
{
    delay(10);
    
    DateTime now = myRTC.now();
    
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
    
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
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
