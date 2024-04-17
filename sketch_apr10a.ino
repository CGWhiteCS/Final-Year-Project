#include <array>
#include <vector>
#include <EEPROM.h>
#include <string>
#include <Crypto.h>
#include <SHA256.h>
#include <iostream>
#include <cstdint>

#include <DHT.h>
#define DHTPIN 8       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // Type of the DHT sensor
#define SHA256_HASH_LENGTH 32
#define MINUTE 60000 
#define TIMEADJUST 0.1666 
#define MAXBLOCKS 8
#define CHAINSPACE MAXBLOCKS*(200+SHA256_HASH_LENGTH)


DHT dht(DHTPIN, DHTTYPE);

int moistureReading; 
const int SMSPin = A0; // Analog pin connected to the LDR


int lightReading; 
const int LDRPin = A1; // Analog pin connected to the LDR


int waterLevelReading;
const int waterLevelPin = 9;


SHA256 sha256;
int eepromSize = 8096;
unsigned long previousMillis = 0;

const int lightPin = 4;
const int waterPin = 5;
const int heatPin = 6;

class chainController {
    

public:
  String lastBlock = "";
  // Constructor
  
  chainController() {
    clearData();

    testData();
    std::array<int, 2> LastBlockPointers = findLastBlockPointers();

    if (LastBlockPointers[0] != -1 && LastBlockPointers[1] != -1) {
      getLastBlock(LastBlockPointers[0], LastBlockPointers[1]);
    } else {
      Serial.print("you have no blocks");
    }
  }




  std::array<int, 2> findLastBlockPointers() {
    int i = 0;
    int value;
    int lastBlockStartPointer = -1;
    int lastBlockEndPointer = -1;
    while (i < CHAINSPACE) {
      value = EEPROM.read(i);
      if (value == '\n') {
        lastBlockStartPointer = lastBlockEndPointer;
        lastBlockEndPointer = i;
      }
      //Serial.print(i);
      //Serial.print("\t");
      //Serial.print(value);
      //Serial.println();
      i = i + 1;
    }


    if (lastBlockStartPointer == -1 && lastBlockEndPointer != -1) {
      lastBlockStartPointer = 0;
    }

    std::array<int, 2> points = { lastBlockStartPointer, lastBlockEndPointer };
    return points;
  }




  void getLastBlock(int startPoint, int endPoint) {
    String tempString = "";
    int i = startPoint;
    if (i != 0) {
      i = i + 1;
    }
    char currrentChar;
    while (i <= endPoint) {
      currrentChar = char(EEPROM.read(i));

      tempString = tempString + currrentChar;

      i = i + 1;
    }
    lastBlock = tempString;
    //Serial.print(lastBlock);
  }


  String extractBlockData(String newBlock, int columnChoice) {
     int i = 0;
    String blockNum;
    String blockTime;
    String blockDire;
    String blockData;
    String blockSig;


    
    
    int columnBoundry = newBlock.indexOf(",");
    blockNum = newBlock.substring(0,columnBoundry);
    newBlock.remove(0,columnBoundry+1);

    columnBoundry = newBlock.indexOf(",");
    blockTime = newBlock.substring(0,columnBoundry);
    newBlock.remove(0,columnBoundry+1);

    columnBoundry = newBlock.indexOf(",");
    blockDire = newBlock.substring(0,columnBoundry);
    newBlock.remove(0,columnBoundry+1);


    columnBoundry = newBlock.indexOf(",");
    blockData = newBlock.substring(0,columnBoundry);
    newBlock.remove(0,columnBoundry+1);

    columnBoundry = newBlock.indexOf(",");
    blockSig = newBlock.substring(0,columnBoundry);
    newBlock.remove(0,columnBoundry+1);

    switch(columnChoice) {
          case 1:
              return blockNum;
              break;
          case 2:
              return blockTime;
              break;
          case 3:
              return blockDire;
              break;
          case 4:
              return blockData;
              break;
          case 5:
              return blockSig;
              break;
          case 6:
              return blockNum+blockTime+blockDire+blockData;
              break;
            }
  }
  


  void addBlock(String blockToAdd) {
    Serial.println("Attempting to add block : " + blockToAdd);
    String cleanFormat = "";
    String hash1 = extractBlockData(blockToAdd,5);

    if (lastBlock.length() >= 2) {
    cleanFormat =  lastBlock.substring(0, lastBlock.length() - 1);
    } 
    String hash2 = calculateSHA256(cleanFormat);
    Serial.println("Hash of last block data : " + hash2);




    if (hash2.equals(hash1)) {
    Serial.println("Hashes match: adding block to chain");
    std::array<int, 2> LastBlockPointers = findLastBlockPointers();
    int lastMemPos = LastBlockPointers[1] + 1;

    for (int i = 0; i < blockToAdd.length(); i++) {
    EEPROM.write(lastMemPos + i, blockToAdd[i]);
    }
    EEPROM.write(lastMemPos + blockToAdd.length(), '\n');

    LastBlockPointers = findLastBlockPointers();
    getLastBlock(LastBlockPointers[0], LastBlockPointers[1]);
    smartContractEx();
    

  } else {
    Serial.println("Hashes do not match: Block rejected");
  }
}





String calculateSHA256(const String &input) {

 char hashResult[SHA256_HASH_LENGTH * 2 + 1]; // +1 for the null terminator

  // Compute SHA256 hash
  SHA256 sha256;
  uint8_t hash[SHA256_HASH_LENGTH];
  sha256.update(input.c_str(), input.length());
  sha256.finalize(hash, SHA256_HASH_LENGTH);

  // Convert hash to hexadecimal string
  for (int i = 0; i < SHA256_HASH_LENGTH; i++) {
    sprintf(&hashResult[i * 2], "%02x", hash[i]);
  }
  hashResult[SHA256_HASH_LENGTH * 2] = '\0'; // Null-terminate the string
  return String(hashResult);

}




void smartContractEx(){
  int c = 0;
  int i = 0;
  int value = 0; 

  while (i < CHAINSPACE) {
      value = EEPROM.read(i);
      if (value == '\n') {
        c = c + 1;
      }
      //Serial.print(i);
      //Serial.print("\t");
      //Serial.print(value);
      //Serial.println();
      i = i + 1;
    }

  if (c > MAXBLOCKS){
  for (int d = 0; d < CHAINSPACE; d++) {
    EEPROM.write(d, 0);
  }
  Serial.println("too many blocks : clearing Chain Data");

  int address = 0;
  for (int i = 0; i < lastBlock.length(); i++) {
    EEPROM.write(address + i, lastBlock[i]);
  }
  EEPROM.write(address + lastBlock.length(), '\n');
  }

  if (extractBlockData(lastBlock, 3).equals("sensors")){
    addBlock(calACT(extractBlockData(lastBlock, 4)));

  }
  
  if (extractBlockData(lastBlock, 3).equals("acts")){
    updateACTS(extractBlockData(lastBlock, 4));

  }
}


  


String calACT(String cData){
  String water;
  String lights;
  String moisture;
  String humid;
  String temperature;

  int firstDataPoint = cData.indexOf(":");
  int lastDataPoint = cData.indexOf("]");
  water = cData.substring(firstDataPoint+1,lastDataPoint);
  cData.remove(0,lastDataPoint+1);

  firstDataPoint = cData.indexOf(":");
  lastDataPoint = cData.indexOf("]");
  lights = cData.substring(firstDataPoint+1,lastDataPoint);
  cData.remove(0,lastDataPoint+1);

  firstDataPoint = cData.indexOf(":");
  lastDataPoint = cData.indexOf("]");
  moisture = cData.substring(firstDataPoint+1,lastDataPoint);
  cData.remove(0,lastDataPoint+1);

  firstDataPoint = cData.indexOf(":");
  lastDataPoint = cData.indexOf("]");
  humid = cData.substring(firstDataPoint+1,lastDataPoint);
  cData.remove(0,lastDataPoint+1);

  firstDataPoint = cData.indexOf(":");
  lastDataPoint = cData.indexOf("]");
  temperature = cData.substring(firstDataPoint+1,lastDataPoint);
  cData.remove(0,lastDataPoint+1);

  String waterACT = water;
  String lightACT = "";
  String moistureACT = "";
  String humidityACT = humid;
  String temperatureACT = "";

  if (lights.toInt() < 100){
        lightACT = "on";
      }
      else{
        lightACT = "off";
      }

  if (moisture.toInt() < 100){
        moistureACT = "on";
      }
      else{
        moistureACT = "off";
      }

  
  
  if (temperature.toInt() < 20){
        temperatureACT = "on";
      }
      else{
        temperatureACT = "off";
      }     
    

    String blockIDStr;
    String timeStamp;
    blockIDStr = extractBlockData(lastBlock, 1);
    int blockID = blockIDStr.toInt() + 1;
    blockIDStr = String(blockID);


    long currentMillis = millis();
    timeStamp = String(currentMillis);

    String actData = "[water level message:"+waterACT+"]"+"[light strip:"+lightACT+"]"+"[water pump:"+moistureACT+"]"+"[humid level message:"+humidityACT+"]"+"[heat pad:"+temperatureACT+"]";

    return blockIDStr+","+timeStamp+","+"acts"+","+actData+","+calculateSHA256(lastBlock.substring(0, lastBlock.length() - 1));
}

void updateACTS(String instros){
  String water;
  String lights;
  String pump;
  String humid;
  String heat;


  int firstDataPoint = instros.indexOf(":");
  int lastDataPoint = instros.indexOf("]");
  water = instros.substring(firstDataPoint+1,lastDataPoint);
  instros.remove(0,lastDataPoint+1);

  firstDataPoint = instros.indexOf(":");
  lastDataPoint = instros.indexOf("]");
  lights = instros.substring(firstDataPoint+1,lastDataPoint);
  instros.remove(0,lastDataPoint+1);

  firstDataPoint = instros.indexOf(":");
  lastDataPoint = instros.indexOf("]");
  pump = instros.substring(firstDataPoint+1,lastDataPoint);
  instros.remove(0,lastDataPoint+1);

  firstDataPoint = instros.indexOf(":");
  lastDataPoint = instros.indexOf("]");
  humid = instros.substring(firstDataPoint+1,lastDataPoint);
  instros.remove(0,lastDataPoint+1);

  firstDataPoint = instros.indexOf(":");
  lastDataPoint = instros.indexOf("]");
  heat = instros.substring(firstDataPoint+1,lastDataPoint);
  instros.remove(0,lastDataPoint+1);

  Serial.println("==============ACTIONS NEEDED==============");


  if (water == "0"){
    Serial.println("Water levels are low : add more water");
  }

  if (lights == "on"){
          digitalWrite(lightPin, HIGH);
        }else if (lights == "off"){
          digitalWrite(lightPin, LOW);
        }

  if (pump == "on"){
          digitalWrite(waterPin, HIGH);
          delay(10000);
          digitalWrite(waterPin, LOW);
        }else if (pump == "off"){
          digitalWrite(waterPin, LOW);
      }
 
  if (humid.toInt() > 300){
    Serial.println("humidity to too high : reduce humidity");
  }

  if (heat == "on"){
          digitalWrite(heatPin, HIGH);
        }else{
          digitalWrite(heatPin, LOW);
        }
  
  Serial.println("====================================");

  
 
}
void testData() {
  int address = 0;
  String testSting = "1,1709921090,none,test:more of a test,genesis";
  Serial.println("Adding genesis block : " + testSting);
  for (int i = 0; i < testSting.length(); i++) {
    EEPROM.write(address + i, testSting[i]);
  }
  EEPROM.write(address + testSting.length(), '\n');



  //address = address + testSting.length() + 1;
  //String testSting2 = "this is the second test String";
  //for (int i2 = 0; i2 < testSting2.length(); i2++) {
  //  EEPROM.write(address + i2, testSting2[i2]);
  //}
  //EEPROM.write(address + testSting2.length(), '\n');
}


void clearData() {
  for (int i = 0; i < CHAINSPACE; i++) {
    EEPROM.write(i, 0);
  }
  Serial.println("data has been cleared");
}

};

String getSensorData(){
  
  waterLevelReading = digitalRead(waterLevelPin);

  lightReading = analogRead(LDRPin);

  moistureReading = (analogRead(SMSPin)/10);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  String water = "water level:"+String(waterLevelReading);
  String light = "light level:"+String(lightReading);
  String moisture = "moisture level:"+String(moistureReading);
  String humid = "humidity level:"+String(humidity);
  String temp = "temperature :"+String(temperature);



  return "["+water+"]"+"["+light+"]"+"["+moisture+"]"+"["+humid+"]"+"["+temp+"]";

}



void setup() {
  Serial.begin(9600);
  pinMode(waterLevelPin, INPUT);

  pinMode(lightPin, OUTPUT);  // sets the digital pin 13 as output
  pinMode(waterPin, OUTPUT);  // sets the digital pin 13 as output
  pinMode(heatPin, OUTPUT);  // sets the digital pin 13 as output
  dht.begin();


}

void loop() {
  chainController bChain;
  
  String blockIDStr = "";
  String timeStamp = "";
  String sensorData = "";

  while (true){
    blockIDStr = bChain.extractBlockData(bChain.lastBlock, 1);
    int blockID = blockIDStr.toInt() + 1;
    blockIDStr = String(blockID);


    long currentMillis = millis();
    timeStamp = String(currentMillis);

    sensorData = getSensorData();
    


    bChain.addBlock(blockIDStr+","+timeStamp+","+"sensors"+","+sensorData+","+bChain.calculateSHA256(bChain.lastBlock.substring(0, bChain.lastBlock.length() - 1)));

    delay(MINUTE*TIMEADJUST);
  }
  // Set myVar to 10

  // Get and print the value of myVar

  // Delay for 1 second
  
}
