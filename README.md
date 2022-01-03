# QUELL-FIRMWARE
Firmware: The early prototype of our hardware was wired, with the chest unit connected to both hand units via UART. This connection was used to send inertial measurement unit (IMU) data from both hand units and collect this together with IMU data from the chest unit to create a single history of the last 32 measurements from all 3 units for further processing and classification. 

Create a protocol for sending messages via UART using an ESP32. This could be in the form of a message sent from one set of pins to another on a single ESP32, or between two ESP32 modules. The message should be the string “Marco” and should be acknowledged with the string “Polo”. This should be clearly displayed on the debug terminal.

Would this protocol be suitable for creating the IMU history as described here? What issues might you expect to encounter when creating this history? What checks could we include to ensure the data is valid?

2021-12-03:
Without a reliable data delivery control, I would not trust any protocol. It would be ok to use something like this in a PCB with both chips connected close to each other on a RS-232 communication, the reliability of 115200 baud is still good if you dont need more than 12kb/s. But if you need more speed, and speacially with wire connection, I would recommend something like I2C or CAN, at least RS-422. But lets say we only have available pins for RX and TX (not even CTS and RTS flow control), the packets must have, at least, a header with Packet Size and Checksum or CRC check, or both, there is other algorithms, but this are the most widely used. A packet counter is an option, but it is more recommended for UDP that tends to deliver in packets, the Packet Size in the header already does the job.

-----------------------------------------------------------------------------------------

# Protocol:
PACKET DESCRIPTION (Big Endian):
PACKET ITEM: | LENGTH: | DESCRIPTION: | CONST VALUE:
--- | --- | --- | ---
SOH | u8| Start of heading | 0X01
Packet Size | u16 | Packet size | NO
SOT | u8 | Start of text | 0x02
Message | Variable  |  Message content | NO
EOT | u8 | End of Text | 0x03
CRC16 | u16 | CRC16-CCITT (XMODEM) from SOH to EOT | NO

# Messages:
MESSAGE: | ACK MESSAGE RESPONSE:
--- | ---
"marco" | "polo"
"polo" | "ok"
"ok" | n/a
"error" | n/a
unknown | "error"

----------------------------------------------------------------------------------------

# Test Procedure:
Minimum requirements:
* 1 or 2 ESP32-WROOM-32;
* Wire between UART1 Tx (GPIO4) and Rx (GPIO5) - Same one or multiple boards;
* Serial Terminal for USB port (UART0 - debug) of ESP32 dev-kit;

1. Connect the usb and open the Serial Terminal on the PC to get debug and control one or multiple devices. There is an embedded command terminal on UART0, type "?" and ENTER to get the commands available;
2. Use the command "marco" to inject a marco message in UART1 Tx;
3. Follow the debug with the communication flow in the Serial Terminal of PC;
