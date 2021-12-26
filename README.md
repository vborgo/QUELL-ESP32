# QUELL-FIRMWARE
Firmware: The early prototype of our hardware was wired, with the chest unit connected to both hand units via UART. This connection was used to send inertial measurement unit (IMU) data from both hand units and collect this together with IMU data from the chest unit to create a single history of the last 32 measurements from all 3 units for further processing and classification. 

Create a protocol for sending messages via UART using an ESP32. This could be in the form of a message sent from one set of pins to another on a single ESP32, or between two ESP32 modules. The message should be the string “Marco” and should be acknowledged with the string “Polo”. This should be clearly displayed on the debug terminal.

Would this protocol be suitable for creating the IMU history as described here? What issues might you expect to encounter when creating this history? What checks could we include to ensure the data is valid?
