These Arduinos are set up in an I2C master-slave relationship. The ArduCam program and script enables the user to take a snapshot by pressing the space bar to identify the 
location of the receiving coil and its center coordinates. These coordinates were converted into steps for the motors and sent to the respective X-axis and Y-axis Arduinos. 
The master Arduino would then instruct the X-axis slave to move to the specified coordinates. After the X-axis movement was complete, the master would signal the Y-axis slave 
to move to its specified coordinate. Finally, the master would command the Z-axis slave to extend until the receiving coil was within 5 cm. It would hold that position for 5 
seconds for testing purposes and then retract. This setup allowed for the automation of a sensor-driven, targeted movement of the wireless EV charging system in the X, Y, and 
Z directions using the ArduCam.
