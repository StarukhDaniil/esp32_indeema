Learning project for ESP32S3

cpu_loader:
Loads CPU by creating 2 tasks. Time for loading and resting CPU for each task can be modified by changing workerInfo structures.

joystick_reader:
Component for reading input from joystick.
Has configurable callbacks for each side of joy position and button press

wifi_manager:
Component for controlling wifi.
Handles events from joy_reader and throws events to led_manager.


led_manager:
Component for controlling LED