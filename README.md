# Bable
Modbus TCP to CAN bus translator for AMMPS Cummings Generator

# Over View
This is a C program that waits for Modbus TCP messages to come into through Ethernet on a Beaglebone black. In the meantime the application will broadcast messages if connected to an AMMPS Generator. 

When a remote client sends a Modbus TCP message, this message has a specific address. Reads are placed on address number 30000 to 39999. 
Writes Modbus messages are placed on addresses 40000 to 49999. 

The Modbus TCP messages are dealt with on a thread, the information is saved on a map. 
A separate thread then reads the stored commands on the map, finds the associated CANBUS message specified in an ICD. 
Then the canbus message is sent from the Beaglebone Black to AMMPS generator via canbus. 

The AMMPS generator will respond with data in a heartbeat broadcast through canbus, this information is parsed then places on the Modbus map before being sent to the client.


# Command Sturcture 



# Testing


# Modes

