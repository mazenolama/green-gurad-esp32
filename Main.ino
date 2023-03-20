#include <HTTPClient.h>
#include "DHT.h"
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#define DHTPIN 22
#define LDR 39
#define Buzzer 23
#define Gas_analog 32
#define Gas_digital 2

#define Moisture  36
#define soil_temp 4  

#define pump 12
#define greenled 14
#define redled 27

float moisture_percentage  ;

#define DHTTYPE DHT22
//create an instance of DHT sensor
DHT dht(DHTPIN , DHTTYPE);

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(soil_temp);

// Pass oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

int ind1 ;
String getMode ;
int ind2 ;
String state;

int httpCodeGet ;
String payloadGet ;

String GETEndPoint, GETLink, getData;

const int id = 1;
      
const char* ssid = "GIS";   
const char* password = "MZ3605005";

//----------------------------------------Web Server address 
const char *API = "https://rms-pos.000webhostapp.com/greenguard_apis/apis";

// Domain name and URL path or IP address with path
String POSTEndPoint = "/POST/post-esp-data.php";
String POSTLink = API + POSTEndPoint; //--> Make a Specify request destination

String apiKeyValue = "tPmAT5Ab3j7F9";
//----------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("Green Guard");
  
   //call begin to start sensor
    dht.begin();
    pinMode(LDR,INPUT);
    pinMode(Buzzer, OUTPUT); 
    pinMode(Gas_digital, INPUT);
    pinMode(pump, OUTPUT);
    pinMode(greenled, OUTPUT);  
    pinMode(redled, OUTPUT);  

    // start connecting to a WiFi network
        
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop() 
  {
  
      HTTPClient getHTTP; //--> Declare object of class HTTPClient
      HTTPClient postHTTP;

      //----------------------------------------Posting Data To MySQL Database
      // soil Temp
      sensors.requestTemperatures(); 
      float temperatureC = sensors.getTempCByIndex(0);
      
      // soil_moisture
      float soil_moisture=getMoisturePercentage();
    
      float g = analogRead(LDR);
      g =map(g,0,4095,0,100);
      //use the functions which are supplied by library.
      float h = dht.readHumidity();
      // Read temperature as Celsius (the default)
      float t = dht.readTemperature();
      //Read Gas percentage and Gas Class
      float gA = analogRead(Gas_analog);
      gA =map(gA,0,9700,0,100);
      
      int gD = digitalRead(Gas_digital);
      
      // Check if any reads failed and exit early (to try again).
        
      if (isnan(h) || isnan(t) || isnan(g) || isnan(gA) || isnan(gD) )
      {
        delay(2000);
        Serial.println("Failed to read from ALL sensors!");
        Serial.println("Please Check Your Connection");
        return;
      }
      else
      {
        gA -=15;
        if(gA < 0)
        {
          gA +=1;
        }
        // set Static GAS % 
        gA= 2.3;
        Serial.print("\n");
        Serial.print("\n");
        Serial.print("\n");
        Serial.print("\n*********************************************************\n");
        
        // print the result to Terminal
        Serial.print("Weather Humidity: ");
        Serial.print(h);
        Serial.print(" %");
        Serial.print("\nWeather Temperature: ");
        Serial.print(t);
        Serial.print("ºC");
        Serial.print("\nSolar Radiation : ");
        Serial.print(g);
        //Serial.print(" w/m^2\n");
        Serial.print(" %");
  
        /*      GAS Sensor             */
        Serial.print("\nGas percentage: ");
        Serial.print(gA);
        Serial.print(" %");
      
        if (gA > 6.00 && gD == 0) 
          {
            //digitalWrite(Buzzer, HIGH); //send tone
            Serial.print("\nGas Class: ");  
            Serial.print(" BUTANE GAS ");
            gD=1;
            Serial.print("\n");
            delay(200);
          }
        else 
          {
            digitalWrite(Buzzer, LOW);  //no tone
            Serial.print("\nGas Class: ");  
            Serial.print(" NO GAS ");
            gD=0;
            Serial.print("\n");
          }

          // Soil Moisture
       
          Serial.print("Soil Moisture : ");
          Serial.print(soil_moisture);
          Serial.print("%");
          Serial.print("\n");

        // Soil Temperature
          Serial.print("Soil Temperature : ");
          Serial.print(temperatureC);
          Serial.print("ºC");
          Serial.print("\n");

         /*******************  Send data to server ****************/

           postHTTP.begin(POSTLink);
  
          // Specify content-type header
          postHTTP.addHeader("Content-Type", "application/x-www-form-urlencoded");
          
          
          // Prepare HTTP POST request data
          String httpRequestData = "api_key=" + apiKeyValue + "&weather_temperature="+ String(t)+
                                   "&weather_humidity="+ String(h)+"&solar_radiation="+ String(g)+
                                   "&gas_percentage="+ String(gA)+"&gas_class="+ String(gD)+
                                   "&soil_moisture="+ String(soil_moisture)+
                                   "&soil_temperature="+ String(temperatureC)+    
                                   "";

                                     
          Serial.print("httpRequestData: ");
          Serial.println(httpRequestData);
          
          //========================================\\
          //        Send HTTP POST request
          
          int httpResponseCode = postHTTP.POST(httpRequestData);
          
          //==========================================\\
              
          if (httpResponseCode>0) 
          {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
          }
          else 
          {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
          }
          // Free resources
          postHTTP.end();

          // =============================== END of POSTING DATA ======================== //
          // delay a little bit for next read
          delay(10);
        }
        
      //----------------------------------------Getting Data from MySQL Database
      
      GETEndPoint = "/GET/GetData.php"; 
      GETLink = API + GETEndPoint; //--> Make a Specify request destination
      getData = "id=" + String(id);
      
      Serial.println("----------------Connect to Server-----------------");
      Serial.println("Get LED Status from Server or Database");
      Serial.print("Request Link : ");
      Serial.println(GETLink);
    
      getHTTP.begin(GETLink); //--> Specify request destination
      getHTTP.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
             
           
      httpCodeGet = getHTTP.POST(getData); //--> Send the request
      payloadGet = getHTTP.getString(); //--> Get the response payload from server

      
      ind1 = payloadGet.indexOf(" ");  //finds location of first ,
      getMode = payloadGet.substring(0, ind1);   //captures first data String
      ind2 = payloadGet.indexOf(" ", ind1+1 );   //finds location of second 
      state = payloadGet.substring(ind1+1, ind2+1);

       
      Serial.print("Response Code : "); //--> If Response Code = 200 means Successful connection, if -1 means connection failed. 
      Serial.println(httpCodeGet); //--> Print HTTP return code
      Serial.print("Returned mode from Server : ");
      Serial.println(getMode); //--> Print request response payload
      Serial.print("Returned state from Server : ");
      Serial.println(state); //--> Print request response payload

    //  Nooooooootee  -> state == "on " fe space || getMode = "auto" mafesh

      /**********            Water Pump                  *************/

      if (getMode == "auto") // compare the input of led status received from firebase
      {                         
        funcPumpAuto(soil_moisture,temperatureC);
      }
      else if (getMode == "manual") // compare the input of led status received from firebase
      {              
        funcPumpManual();                             
      }
        
    //----------------------------------------
    
      Serial.println("----------------Closing Connection----------------");
    
      getHTTP.end(); //--> Close connection
      
      Serial.println();
      Serial.println();
      delay(10); //--> GET Data at every 5 seconds
  }

/*************** Functions ***************************/


// Soil Moisture
float getMoisturePercentage()
{
  moisture_percentage = analogRead(Moisture);
  moisture_percentage  =( 100 - ( (moisture_percentage /4095.00) * 100 ) ) ;
  if ( moisture_percentage > 60 )
  {
    moisture_percentage+=30;
  }
  
  return moisture_percentage;
}

void funcPumpManual()
{
  if (state == "on ") // compare the input of led status received from firebase
  {                         
    Serial.println("Pump Turned ON");                 
    digitalWrite(pump, LOW);
    digitalWrite(greenled, HIGH); 
    digitalWrite(redled, LOW);                                                         
  }
  else if (state == "off ") // compare the input of led status received from firebase
  {              
    Serial.println("Pump Turned OFF");
    digitalWrite(pump, HIGH);
    digitalWrite(greenled, LOW); 
    digitalWrite(redled, HIGH);                                                          
  }
}

void funcPumpAuto(float soil_moisture,float temperatureC)
{
  if(soil_moisture < 20.10 || temperatureC > 35.5)
  {
    Serial.println("Pump Turned ON");                 
    digitalWrite(pump, LOW);
    digitalWrite(greenled, HIGH); 
    digitalWrite(redled, LOW);      
  }
  else
  {
    Serial.println("Pump Turned OFF");
    digitalWrite(pump, HIGH);
    digitalWrite(greenled, LOW); 
    digitalWrite(redled, HIGH);                                                   
  }
}
