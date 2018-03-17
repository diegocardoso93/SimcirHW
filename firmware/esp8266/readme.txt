https://github.com/nodemcu/nodemcu-firmware/tree/master
http://nodemcu.readthedocs.io/en/master/en/build/

esptool.py v2.2
python esptool.py --port COM5 write_flash -fm dio 0x00000 nodemcu_integer_master_20180303-2228.bin

MODULES:
 - file
 - gpio
 - node
 - tmr
 - uart
 - websocket
 - wifi
 - sjson
