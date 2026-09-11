# CAN Telemetry and OTA 

This markdown is a step-by-step for student engineers who want to use the tool for their FSAE projects down the line. For a more detailed engineering analysis, see reports/capstone-report.pdf

## Overview

This was our capstone project for 2026 at the University of Windsor, Department of ECE. 
We are team 21 - Brooklyn Boucher, Bansari Patel, Laith Al-Kinani. Our advisor was Dr. Roberto Muscedere. 
The motivation for this project arose from the need for a remote data visualization/debugging tool for our school's Formula SAE vehicle. The state-of-the-art prior to this project was engineers having to manually go into the control box of the vehicle, remove an SD card from the vehicle ECU (an RPi as of 2025), plug it into their laptops, and pray to God the vehicle data of interest was captured. Obviously, the procedure was invasive, time-consuming, and most importantly: was not a real-time data analysis procedure. So, we wanted to develop a tool where engineers could see important vehicle data in real-time. It's similar to a telematics control unit (TCU) that some modern vehicles use, minus the GPS feature. 
For our capstone demo, we built a hardware-in-loop (HiL) setup. This is what we suggest doing first before testing on an actual FSAE vehicle.
What you'll get: a web app that displays live incoming data from the vehicle CAN bus with timestamps.
Two limitations that stand out right now: we have to use the driver's phone's hotspot connection, and we haven't tested the OTA feature yet. More on that in the "Open Issues" section at the end.

## First Step: Read the Report

Read the report. Feel free to ask your AI of choice to clone this repo and help you set things up.

## Second Step: Clone the Repo + Install ESP32 / VSCode Extension (Optional)

Clone the repo using this command:

`git clone repo-name`

We used the ESP IDF extension on VSCode as our toolchain, but you could use another toolchain if you'd like.


## Third Step: Required Hardware

For a breadboard setup, the required hardware is: an ESP32-WROOM 30-pin variant, a TJA1050T transceiver breakout board, an MCP2515 controller + TJA1050T transceiver module, Arduino Nano, a green LED and a 330 ohm resistor.
The Nano and the MCP2515 module constitute the "second node" of our CAN bus.
GPIO 21 and 22 are used for CAN TX and RX respectively on the ESP32. 
GPIO 18 is our Wi-Fi status LED that glows green when the tool has successfully connected to a phone hotspot. 
The MCP2515->Arduino connections can be found online.
The breadboard setup is as shown:

## Fourth Step: The PCB

The PCB files are included in the repo. You would obviously have to print the PCB and then solder the parts on yourself - or just ask us and we can give you the board that we used for our demo (this is for Windsor FSAE students only lol). Other than the ESP32 setup, the outward connections to other CAN nodes don't change much. The inputs are still power (5V, GND) and the two CAN lines. Just make sure everything is connected correctly.


## Fifth Step: Make a New Branch and Change the Hotspot Address (Important)

Right now, the ESP32 connects to my phone's - "Laith's iPhone" with password "laithiscool" (I know) in wifi_stuff.c
You obviously will have to change this. *Be careful with the apostrophe* in iPhone names because Apple uses a specific code for apostrophes: xE2\x80\x99s
So to make this change cleanly, open the repo in your favourite IDE after you cloned it in step one, then in the terminal make a new branch from the main branch:

`git checkout -b origin/main/your-branch-name`

Then apply the changes to wifi_stuff.c on *lines 27 and 28* to match your phone's hotspot username and password. Save the file.

Keep the branch open for all your testing. If you wish to merge it into main, please open a pull request so we can see the merge history cleanly!


## Sixth Step: Compile and Flash the Nano and the ESP32

Open the Arduino IDE (or preferred toolchain of choice), open the demo_send.ino file under ARDUINO_MCP2515/, connect the Nano Board to your computer and flash the board. This Arduino sketch transmits simulated vehicle data such as RPM, battery pack SOC, and power draw to the ESP32 which then broadcasts it online. I'd like to emphasize: the Arduino Nano sketch is a *stand-in* for the FSAE vehicle, and builds CAN frames in the exact format that the actual vehicle CAN bus should transmit them based on the DBC files.

Then connect the ESP32 to your computer, open the VSCode IDE, open the CAN-ESP32-SOCKET/ folder, build the binary using the wrench symbol, then flash using the lightning bolt symbol. You may have to play with the config settings in the IDE to build and flash. Make sure the COM port is populated. 
Once it's compiled and flashed (may take a few minutes), pay attention to what you see in the serial port (make sure to set baud rate to 115200):
You should see CAN_OK acknowledgments buzzing by on both the Nano and ESP serial ports. This means that the CAN bus is connected and messages are being sent and acknowledged. 
You should also see MQTT_OK messages in the serial stream from the ESP32. I tried to add as many debug statements as reasonably possible to help with debugging.



## Seventh Step: Upload the Correct DBC File

The DBC file is what the frontend app uses to decode the incoming MQTT packets. Right now the app supports two DBC files: the Orion BMS and the DTI Inverter. Make sure any new DBC files don't break the app, so consider adequate testing here.

## Eighth Step: Run the App
With the hardware connected and CAN messages publishing to the MQTT topic, once you run the .py app you should see data start to populate on the web app.
*Note*: we wrote this app specifically for the demo, so change it to fit your data telemetry needs i.e pick what signals you want to display. Our demo uses simulated data from the Arduino Nano and not real data because we didn't have the car available to test on.

## Troubleshooting

In general, always check the CAN connection -> then the connection between the ESP32 to the MQTT broker -> then the frontend app to the MQTT broker. 
This is the general debugging pipeline. Make sure each step of the pipeline is working properly before moving to the next one. Debug statements are your best friend here.
You have to make sure that the frontend app is using the right DBC file. 
*Note:* the MQTT broker we're using is HiveMQ, a free and public MQTT broker. Pay attention to the status of the HiveMQ and make sure it's not down.

## Open Issues

### OTA

We have not implemented the OTA feature yet. We've thought about the system architecture and designed it on a high-level (see report), but we haven't tested it even on a small scale. We just didn't have enough time. One limitation that we didn't really consider is the memory bank on the current ESP32 WROOM variation that we're using. There just isn't enough flash memory (or RAM, for that matter) to allocate a bank of memory for the binary that we want to flash, without causing a headache with memory partitions. The ESP32-S3 has considerably more memory that we can play with. There's also a few security and quality-of-life considerations with OTA updates (like secure flashing, geneology, dual-banks, etc.).
I've went into more detail on this feature in the report so check that out for ideas about where to go from here.

### Security

On the topic of hardware, security is also an important matter. Any encryption should be done with a hardware accelerator on the device, and although the ESP32 WROOM variant likely does have an accelerator (I haven't done my due research on this honestly), we didn't get to using or incorporating it. We were too scared it would consume too much power or otherwise break someting. Anyways, another ESP32 variant could help here. Encryption capabilities would be necessary for incorporating TLS into the MQTT connections. 

### FreeRTOS Porting

One of our justifications for using FreeRTOS was that we could easily port the firmware to other devices. This is something to consider when upgrading to a more powerful or memory-spacious device. 

### Hotspot

Right now the device connects to a phone's hotspot for an LTE/5G connection. This is...easy... but if we want to take this to the next level, the hardware and firmware should be modified to connect to a carrier. This is detailed a bit more in the report, but for our project was way out of scope, so it will be cool to see in future iterations.

### HiveMQ
HiveMQ as our broker and temporary data storage is cool because it's easy and reliable. No reason to change this, only if you're extremely bored. But the alternative idea is hosting our own broker and server on a separate device like an RPi.




