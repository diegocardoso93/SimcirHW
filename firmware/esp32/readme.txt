http://nodemcu.readthedocs.io/en/dev-esp32/en/build/
https://github.com/nodemcu/nodemcu-firmware/tree/dev-esp32

esptool.py v2.2
python esptool.py --chip esp32 --port COM6 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 bootloader.bin 0x10000 NodeMCU.bin 0x8000 partitions_singleapp.bin


MODULES:
 - file
 - gpio
 - net
 - node
 - tmr
 - wifi

---- modifications
nodemcu-firmware-esp32/components/modules/tmr.c
- include lib:
#include <sys/time.h>

- add the following function:
// Lua: tmr.now(), return system time in us
static int tmr_now(lua_State* L)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  lua_pushinteger(L, tv.tv_sec * 1e6 + tv.tv_usec);
  return 1;
}

- register function (variable tmr_map):
{ LSTRKEY( "now" ),          LFUNCVAL( tmr_now ) },


