/***********************************************************************//**
  \file   M5AtomVL53L1XWaterLevel.ino
  \brief  NMEA2000 library example for M5 Atom with VL53L1X Time of Flight sensor.
          Broadcasts water level based on distance.

  This example reads distance from an M5Stack ToF unit (VL53L1X) via I2C,
  calculates a simulated tank level, and sends it to the NMEA2000 bus as
  a fresh water tank level (PGN 127505).
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L1X.h>

#ifndef ARDUINO_M5Stack_ATOM
#define ARDUINO_M5Stack_ATOM
#endif

#include <NMEA2000_CAN.h>
#include <N2kMessages.h>

// VL53L1X Sensor Setup
Adafruit_VL53L1X vl53; // No XSHUT or IRQ pins used on M5Stack ToF unit

// Tank dimensions for level calculation
const double TANK_DEPTH_MM = 1000.0; // Total depth of tank
const double TANK_CAPACITY_L = 100.0; // Total capacity in Liters

// List here messages your device will transmit.
const unsigned long TransmitMessages[] PROGMEM={127505L, 0};

void setup() {
  Serial.begin(115200);

  // Initialize I2C for M5 Atom (SDA=26, SCL=32)
  Wire.begin(26, 32);

  if (!vl53.begin(0x29, &Wire)) {
    Serial.println(F("Error on init of VL sensor"));
    // while (1) delay(10); // Halt if sensor not found
  } else {
    Serial.println(F("VL53L1X sensor OK!"));
    if (!vl53.startRanging()) {
      Serial.print(F("Couldn't start ranging: "));
      Serial.println(vl53.vl_status);
    }
  }

  // Set Product information
  NMEA2000.SetProductInformation("00000002", // Manufacturer's Model serial code
                                 101, // Manufacturer's product code
                                 "M5 Atom Water Level",  // Manufacturer's Model ID
                                 "1.0.0.0 (2024-03-13)",  // Manufacturer's Software version code
                                 "1.0.0.0 (2024-03-13)" // Manufacturer's Model version
                                 );

  // Set device information
  NMEA2000.SetDeviceInformation(112244, // Unique number.
                                140, // Device function = Fluid Level
                                75, // Device class = Sensor Communication Interface
                                2040 // Manufacturer Code
                               );

  NMEA2000.SetMode(tNMEA2000::N2km_NodeOnly, 23);
  NMEA2000.EnableForward(false);
  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  NMEA2000.Open();
}

void SendN2kWaterLevel() {
  static unsigned long LastTime = 0;
  if (LastTime + 2500 < millis()) {
    LastTime = millis();

    double levelPercentage = 0;

    if (vl53.dataReady()) {
      int16_t distance = vl53.distance();
      if (distance == -1) {
        Serial.print(F("Couldn't get distance: "));
        Serial.println(vl53.vl_status);
      } else {
        // Calculate percentage (distance from top of tank)
        double fillDepth = TANK_DEPTH_MM - distance;
        if (fillDepth < 0) fillDepth = 0;
        levelPercentage = (fillDepth / TANK_DEPTH_MM) * 100.0;
        if (levelPercentage > 100.0) levelPercentage = 100.0;

        Serial.print("Distance: "); Serial.print(distance); Serial.print(" mm, ");
        Serial.print("Level: "); Serial.print(levelPercentage); Serial.println(" %");

        vl53.clearInterrupt();

        tN2kMsg N2kMsg;
        // Instance 1, Fresh Water
        SetN2kFluidLevel(N2kMsg, 1, N2kft_Water, levelPercentage, TANK_CAPACITY_L);
        NMEA2000.SendMsg(N2kMsg);
      }
    }
  }
}

void loop() {
  SendN2kWaterLevel();
  NMEA2000.ParseMessages();
}
