# Red en malla con ESP32 (ESP-NOW)

Tres proyectos ESP-IDF (v5.3 o superior) para ESP32: coordinator, router y end_device.

    cd coordinator
    idf.py set-target esp32
    idf.py build
    idf.py -p COM3 flash monitor

Repetir en router y end_device. Antes, copiar las direcciones MAC que imprime el coordinador y el router en nodes.h de router y end_device. En end_device cambiar NODE_ID a 3 o 4.

Sensor: SHT31 por I2C (SDA GPIO21, SCL GPIO22).
