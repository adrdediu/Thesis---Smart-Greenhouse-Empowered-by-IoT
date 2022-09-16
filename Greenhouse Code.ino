#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// Network Credentials
const char* ssid = "nett";
const char* password = "krownett";

//Pins
#define FAN_PIN D0
#define DHT_1_PIN D1    
#define DHT_2_PIN D2
#define LED_PIN D3
#define WS_TRIGGER_PIN D6
#define WS_ECHO_PIN  D5
#define WATER_PUMP_PIN D7
#define LDR_PIN A0

//Generic constants used
#define DHT_SENSORS_TYPE    DHT11     
#define speed_of_sound 0.034 //29us for 1 cm => 0.034 cm in 1us
#define sensor_to_tank_distance 2 //in cm

DHT dht_sensor_1(DHT_1_PIN, DHT_SENSORS_TYPE);
DHT dht_sensor_2(DHT_2_PIN, DHT_SENSORS_TYPE);

//Climate System variables//////////
float temp_1 = 0.0;
float humi_1 = 0.0;
float temp_2 = 0.0;
float humi_2 = 0.0;
boolean fan_status = false;
//////////////////////////////////////////

//Lighting System variables///////////////
boolean lights_status=false;
uint8 ldr_day_stage=0;
/////////////////////////////////////////

//Water System variables/////////////////
//Internal Variables
long ws_time_interval;
float distance_in_Cm;
float tank_level=10.0;
byte valid_tank_level=0;

//External Variables for the Website
boolean water_pump_status=false;
String water_tank_level ="75-100";
///////////////////////////////////////////////

//Time variables/////////////////
unsigned long previous_time = 0; 
/////////////////////////////////

// Create AsyncWebServer object on port 80
AsyncWebServer server_greenhouse(80);


   


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
 <script src="https://kit.fontawesome.com/80e2384ad9.js" crossorigin="anonymous"></script>
 <style>
    html {
     font-family: cursive,"Times New Roman", "Arial";
     display: inline-block;
     margin: 0px auto;
     text-align: center;
     background-color:#3cb371;
     margin-left: 2.5vw;margin-right: 2.5vw;
    }
    h2 { font-size:6vmin;background-color:white; padding: 20px;border-radius: 15px; }
    p { font-size:3vmin;padding:3vh 0; }
    .sensor-label{
      font-size:3vmin;
      vertical-align:middle;
    }
  .button {
    background-color: #4CAF50;
    color: white;
    border:none;
    padding: 2vh 2vw;
    text-align: center;
    display: inline-block;
    font-size: 2.5vmin;
  border-radius: 15px;
    margin:2vh 0;
  }
  .grid-container_light {
  display:grid;
  grid-template-columns: auto auto;
  gap: 1vmin;
  background-color: #2196F5;
  padding: 2vmin;
  border:2px solid black;
  margin-top:20px;
  border-radius: 15px;
  }
  .grid-container_light > div {
  background-color: white;
  border: 2px solid black;
  text-align: center;
  font-size: 3vmin;
  padding: 0.3vmin;
  border-radius: 15px;
  overflow-wrap: anywhere;
  }
  

  .grid-container {
  display: grid;
  border:2px solid black;
  margin-top:20px;
  grid-template-columns: auto auto auto;
  gap: 1vmin;
  background-color: #2196F9;
  padding: 2vmin;
  border-radius: 15px;
  }
  .grid-container > div {
  background-color: white;
  border: 2px solid black;
  text-align: center;
  font-size: 3vmin;
  padding: 0.3vmin;
  border-radius: 15px;
  overflow-wrap: anywhere;
  }
  
  .div_system{
  margin-top:3vmin;
  padding:2vmin;
  font-size:3.5vmin;
  background-color:white;
  border-radius: 15px;
  }

  </style>
</head>
<body>
  <h2 id="top">Smart Greenhouse</h2>
  <!-- Climate System -->
  <div class="div_system">Climate System<br><button id="temp_button" class="button">Show / Hide</button>
   <div id="dispTempSystem" style="display: none;">
   <div class="grid-container">
      <div>Front Side Sensor Data</div>
      <div>Cooling Fan</div>
      <div>Back Side Sensor Data</div>
      <div>
        <p>
        <i class="fas fa-thermometer-half" style="color:#059e8a;" id="temp1_icon"></i> 
        <span class="sensor-label">Temperature :</span> 
        <span id="temperature_sensor_front">%temperature_sensor_front%</span>
        <span>&deg;C<span>
        </p>
      </div>
      <div>
         <p style="padding:3.7vmin 0;">
        <i class="fa-solid fa-fan" style="color:red" id="fan_icon"></i>
        <span class="sensor-label">Status :<br></span>
        <span id="fan_status">%fan_status%</span>
        </p>
      </div>
      <div>
        <p>
        <i class="fas fa-thermometer-half" style="color:#059e8a;" id="temp2_icon"></i> 
        <span class="sensor-label">Temperature :</span> 
        <span id="temperature_sensor_back">%temperature_sensor_back%</span>
        <span>&deg;C<span>
        </p>
      </div>
      <div id="humidity_div_left">
        <p>
        <i class="fas fa-tint" style="color:#00add6;" id="humi1_icon"></i> 
        <span class="sensor-label">Humidity :</span>
        <span id="humidity_sensor_front">%humidity_sensor_front%</span>
        <span>&#37;</span>
        </p>
      </div>
    <div><p style="font-size:1.8vmin;font-style:italic;padding:3.5vmin 0;">Sensors accuracy :<br>Temperature:0-50&deg;C /&plusmn;2&deg;C<br>Humidity: 20-80&#37;/&plusmn;5&#37;</p></div>
     <div id="humidity_div_right">
        <p>
        <i class="fas fa-tint" style="color:#00add6;" id="humi2_icon"></i> 
        <span class="sensor-label">Humidity :</span>
        <span id="humidity_sensor_back">%humidity_sensor_back%</span>
        <span>&#37;</span>
        </p>
      </div>

    </div>
   </div>
   </div>
   <!-- Lighting System -->
   <div class="div_system">Lighting System<br><button id="light_button" class="button">Show / Hide</button>
   <div id="dispLightSystem" style="display: none;">
    <div class="grid-container_light">
      <div>Lights</div>
      <div>Light Detector</div>
      <div>
         <p>
        <i class="fas fa-lightbulb" style="color:red" id="lights_icon"></i>
        <span class="sensor-label">Status :</span>
        <span id="lights_status">%lights_status%</span>
        </p>
      </div>
      <div>
         <p>
        <i class="fa-solid fa-sun" style="color:yellow" id="sun_icon"></i>
        <span class="sensor-label">Time of Day :</span>
        <span id="ldr_day_stage">%ldr_day_stage%</span>
        </p>
      </div>
    </div>
   </div>
   </div>
   <!-- Water System -->
   <div class="div_system">Water System<br><button id="water_button" class="button">Show / Hide</button>
   <div id="dispWaterSystem" style="display: none;">
    <div class="grid-container_light">
      <div>Reservoir</div>
      <div>Water Pump</div>
      <div>
         <p>
        <i class="fa-solid fa-glass-water-droplet" style="color:red" id="water_icon"></i>
        <span class="sensor-label">Water Level:</span>
        <span id="water_tank_level">%water_tank_level%</span>
        </p>
      </div>
      <div>
         <p>
        <i class="fa-solid fa-faucet" style="color:red" id="water_2_icon"></i>
        <span class="sensor-label">Status :</span>
        <span id="water_pump_status">%water_pump_status%</span>
        </p>
      </div>
    </div>
   </div>
   </div>
   
</body>
<script>
var temp_button =  document.getElementById("temp_button");
var light_button = document.getElementById("light_button");
var water_button = document.getElementById("water_button");
temp_button.addEventListener("click",function(){
  var x = document.getElementById("dispTempSystem");
  if (x.style.display === "none") {
    x.style.display = "block";
  } else {
    x.style.display = "none";
  }
  const temp_div = document.getElementById("top");
  temp_div.scrollIntoView({behavior: 'smooth' });
});
light_button.addEventListener("click",function(){
    var x = document.getElementById("dispLightSystem");
    if (x.style.display === "none") {
    x.style.display = "block";
    } else {
    x.style.display = "none";
    }
  const light_div = document.getElementById("sun_icon");
  light_div.scrollIntoView({behavior: 'smooth' });
});
water_button.addEventListener("click",function(){
    var x = document.getElementById("dispWaterSystem");
    if (x.style.display === "none") {
    x.style.display = "block";
    } else {
    x.style.display = "none";
    }
  const water_div = document.getElementById("water_icon");
  water_div.scrollIntoView({behavior: 'smooth' });
});
<!-- Climate System -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var value = parseFloat(this.responseText);
      if(value>-20)
        {
          document.getElementById("temperature_sensor_front").innerHTML = this.responseText;
          if(value>25){document.getElementById("temp1_icon").style.color = "red";document.getElementById("temperature_sensor_front").style.color = "red";}
          else {document.getElementById("temp1_icon").style.color = "green";document.getElementById("temperature_sensor_front").style.color = "green";}
        }
      else {document.getElementById("temperature_sensor_front").innerHTML = "Error &#9888";document.getElementById("temp1_icon").style.color = "black"; }
    }
  };
  xhttp.open("GET", "/temperature_sensor_front", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var value = parseFloat(this.responseText);
      if(value>-20)
        {
          document.getElementById("humidity_sensor_front").innerHTML = this.responseText;
          if(value>65){document.getElementById("humi1_icon").style.color = "blue";document.getElementById("humidity_sensor_front").style.color = "blue";}
          else {document.getElementById("humi1_icon").style.color = "cyan";document.getElementById("humidity_sensor_front").style.color = "cyan";}
        }
      else {document.getElementById("humidity_sensor_front").innerHTML = "Error &#9888";document.getElementById("humi1_icon").style.color = "black"; }
    }
  };
  xhttp.open("GET", "/humidity_sensor_front", true);
  xhttp.send();
}, 10000 ) ;

<!-- Sensor at the back -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var value = parseFloat(this.responseText);
      if(value>-20)
        {
          document.getElementById("temperature_sensor_back").innerHTML = this.responseText;
          if(value>25){document.getElementById("temp2_icon").style.color = "red";document.getElementById("temperature_sensor_back").style.color = "red";}
          else {document.getElementById("temp2_icon").style.color = "green";document.getElementById("temperature_sensor_back").style.color = "green";}
        }
      else {document.getElementById("temperature_sensor_back").innerHTML = "Error &#9888";document.getElementById("temp2_icon").style.color = "black"; }
    }
  };
  xhttp.open("GET", "/temperature_sensor_back", true);
  xhttp.send();
}, 10000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var value = parseFloat(this.responseText);
      if(value>-20)
        {
          document.getElementById("humidity_sensor_back").innerHTML = this.responseText;
          if(value>65){document.getElementById("humi2_icon").style.color = "blue";document.getElementById("humidity_sensor_back").style.color = "blue";}
          else {document.getElementById("humi2_icon").style.color = "cyan";document.getElementById("humidity_sensor_back").style.color = "cyan";}
        }
      else {document.getElementById("humidity_sensor_back").innerHTML = "Error &#9888";document.getElementById("humi2_icon").style.color = "black"; }
    }
  };
  xhttp.open("GET", "/humidity_sensor_back", true);
  xhttp.send();
}, 10000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if(parseInt(this.responseText))
            {document.getElementById("fan_status").innerHTML = "Active";document.getElementById("fan_icon").style.color = "green";}
      else  {document.getElementById("fan_status").innerHTML = "Inactive";document.getElementById("fan_icon").style.color = "red";}
    }
  };
  xhttp.open("GET", "/fan_status", true);
  xhttp.send();
}, 10000 ) ;

<!-- Light System -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if(parseInt(this.responseText))
           {document.getElementById("lights_status").innerHTML = "Active";document.getElementById("lights_icon").style.color = "yellow";}
      else {document.getElementById("lights_status").innerHTML = "Inactive";document.getElementById("lights_icon").style.color = "black";}
    }
  };
  xhttp.open("GET", "/lights_status", true);
  xhttp.send();
}, 10000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if(parseInt(this.responseText))
            {document.getElementById("ldr_day_stage").innerHTML = "Day";document.getElementById("sun_icon").style.color = "yellow";}
      else  {document.getElementById("ldr_day_stage").innerHTML = "Night";document.getElementById("sun_icon").style.color = "black";}
    }
  };
  xhttp.open("GET", "/ldr_day_stage", true);
  xhttp.send();
}, 10000 ) ;

<!--Water System -->
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    var value = parseFloat(this.responseText)
      if(value >= 0){
        document.getElementById("water_tank_level").innerHTML = value +"-"+(value+25)+"&#37";;
        if(value>=75)     document.getElementById("water_icon").style.color = "green";
        else if(value>=50)document.getElementById("water_icon").style.color = "lightgreen";
        else if(value>=25)document.getElementById("water_icon").style.color = "yellow";
        else if(value>=0) document.getElementById("water_icon").style.color = "red";
      }
      else {
        document.getElementById("water_tank_level").innerHTML = "Error";
        document.getElementById("water_icon").style.color = "black";
      }
    }
  };
  xhttp.open("GET", "/water_tank_level", true);
  xhttp.send();
}, 10000 ) ;
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if(parseInt(this.responseText))
            {document.getElementById("water_pump_status").innerHTML = "Active";document.getElementById("water_2_icon").style.color = "green";}
      else  {document.getElementById("water_pump_status").innerHTML = "Inactive";document.getElementById("water_2_icon").style.color = "red";}
    }
  };
  xhttp.open("GET", "/water_pump_status", true);
  xhttp.send();
}, 10000 ) ;
</script>

</html>)rawliteral";

// Replaces text in html page with sensor values
String pre_processor_placeholder(const String& upcoming_data){

  if(upcoming_data == "temperature_sensor_front"){
    return String(temp_1);
  }
  else if(upcoming_data == "humidity_sensor_front"){
    return String(humi_1);
  }
  else if(upcoming_data == "temperature_sensor_back"){
    return String(temp_2);
  }
  else if(upcoming_data == "humidity_sensor_back"){
    return String(humi_2);
  }
  else if(upcoming_data == "fan_status")
  {
    return String("Inactive");
  }
  else if(upcoming_data == "lights_status")
  {
    return String("Inactive");
  }
  else if(upcoming_data == "ldr_day_stage")
  {
    return String("Waiting to get data..");
  }
  else if(upcoming_data == "water_tank_level")
  {
    return String("Waiting for data..");
  }
  else if(upcoming_data == "water_pump_status")
  {
    return String("Inactive");
  }
  return String();
}

void ultrasonic_sensor_init()
{
  // Clears the WS_TRIGGER_PIN
  digitalWrite(WS_TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  // Sets the WS_TRIGGER_PIN on HIGH state for 10 micro seconds
  digitalWrite(WS_TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(WS_TRIGGER_PIN, LOW);
}

void sensor_setup()
{
  dht_sensor_1.begin();
  dht_sensor_2.begin();
  pinMode(FAN_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(WS_TRIGGER_PIN, OUTPUT);
  pinMode(WS_ECHO_PIN, INPUT);
  pinMode(WATER_PUMP_PIN,OUTPUT);
  digitalWrite(FAN_PIN,LOW);
  digitalWrite(LED_PIN,LOW);
  digitalWrite(WATER_PUMP_PIN,HIGH);
}

void wifi_server_setup()
{
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  Serial.println(WiFi.localIP());

  // Route for root / web page
  server_greenhouse.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, pre_processor_placeholder);
  });
  server_greenhouse.on("/temperature_sensor_front", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp_1).c_str());
  });
  server_greenhouse.on("/humidity_sensor_front", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humi_1).c_str());
  });
  server_greenhouse.on("/temperature_sensor_back", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp_2).c_str());
  });
  server_greenhouse.on("/humidity_sensor_back", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humi_2).c_str());
  });
  server_greenhouse.on("/fan_status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(fan_status).c_str());
  });
  //Light System
  server_greenhouse.on("/lights_status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(lights_status).c_str());
  });
  server_greenhouse.on("/ldr_day_stage", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(ldr_day_stage).c_str());
  });
  //Water System
  server_greenhouse.on("/water_pump_status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(water_pump_status).c_str());
  });
  server_greenhouse.on("/water_tank_level", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(water_tank_level).c_str());
  });
  // Start server_greenhouse
  server_greenhouse.begin();
}

void climate_system()
{
  // Read data from front sensor
    float newTemp_1 = dht_sensor_1.readTemperature();
    float newHumi_1 = dht_sensor_1.readHumidity()+5;
    if (isnan(newTemp_1)||isnan(newHumi_1)) {
      Serial.println("Failed to read from DHT sensor at the front side!");
      temp_1= -100;
      humi_1= -100;
    }
    else {
      temp_1 = newTemp_1;
       humi_1 = newHumi_1;
      Serial.print(temp_1);
      Serial.println(humi_1);
      
    }
    // Read data from back sensor
    float newTemp_2 = dht_sensor_2.readTemperature();
    float newHumi_2 = dht_sensor_2.readHumidity()-5;//after calibration
    if (isnan(newTemp_2)||isnan(newHumi_2)) {
      Serial.println("Failed to read from DHT sensor at the back side!");
      temp_2= -100;
      humi_2= -100;
    }
    else {
      temp_2 = newTemp_2;
      humi_2 = newHumi_2;
      Serial.print(temp_2);
      Serial.println(humi_2);
    }
    //If either the temperatures or the humitidies are higher than the treshold, activate fan
    
    if(temp_2>=25 || temp_1>=25 || humi_1> 80 || humi_2 > 80)
    {
      fan_status = true;
      digitalWrite(FAN_PIN,LOW);
      Serial.println("Fan is on");
    }
    else {
      fan_status = false;
      digitalWrite(FAN_PIN,HIGH);
      Serial.println("Fan is off");
    }
}
void lighting_system()
{
  int ldr_analog_value = analogRead(LDR_PIN);

  Serial.println(ldr_analog_value);
  if(ldr_analog_value < 20){
      lights_status = true;
      ldr_day_stage=0;
      digitalWrite(LED_PIN,HIGH);
    }
  else{
    lights_status=false;
    ldr_day_stage=1;
    digitalWrite(LED_PIN,LOW);
    }
}

void water_system()
{
    ultrasonic_sensor_init();
  ws_time_interval = pulseIn(WS_ECHO_PIN, HIGH);
  // Calculate the distance
  distance_in_Cm = ws_time_interval * speed_of_sound/2;
  distance_in_Cm = distance_in_Cm-sensor_to_tank_distance;
  Serial.println(distance_in_Cm);
    if(distance_in_Cm<=12&&distance_in_Cm>=0)
    { 
      if(distance_in_Cm>=7.5){
        tank_level=0;
        valid_tank_level++;
      }
      else if(distance_in_Cm>=5.5)   {tank_level=2.5;valid_tank_level++;}
      else if(distance_in_Cm>=3.5)   {tank_level=5.0;valid_tank_level++;}
      else {
        tank_level=7.5;
        water_pump_status=false;
      }
  
      water_tank_level = String(tank_level*10);
      
      if(valid_tank_level==3){
        valid_tank_level=0;     
        water_pump_status=true;
      }
   }
   else {
      valid_tank_level=0; 
      water_pump_status=false;
      water_tank_level="-100";
   }//turn off pump, due to sensor error

  Serial.println(water_tank_level);
  
  if(water_pump_status){
    digitalWrite(WATER_PUMP_PIN,LOW);
    Serial.println("Activating Water Pump");
  }
  else digitalWrite(WATER_PUMP_PIN,HIGH);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  sensor_setup();
  wifi_server_setup();
}
 
void loop(){  
  unsigned long current_time = millis();
  if (current_time - previous_time >= 10000) 
  {
    previous_time = current_time;

    //System functions
    climate_system();
    lighting_system();
    water_system();
  }
 
}
