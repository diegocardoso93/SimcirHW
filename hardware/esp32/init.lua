
-- init.lua --
-- NodeMCU
-- https://github.com/nodemcu/nodemcu-firmware
-- branch: master
-- commit: 2e67ff5a639a13260fd4fb3c3b627ccdc2845616
-- modules: file,gpio,net,node,tmr,uart,websocket,wifi
-- powered by Lua 5.1.4

print("\n UNISC \n")

print("Chip ID: " .. node.chipid() .. "\n")
print("Heap Size: " .. node.heap() .. "\n")
print("MAC Address: " .. wifi.ap.getmac() .. "\n")

print("Initializing...\n")

wifi.mode(wifi.SOFTAP)
print("set mode=SOFTAP (mode=" .. wifi.getmode() .. ")\n")

ap_cfg = {
    ssid = "SimcirHW",
    pwd = "unisc123",
    auth = wifi.WPA2_PSK,
    channel = 6,
    hidden = false,
    max = 4,
    beacon = 100,
    save = false
}
wifi.ap.config(ap_cfg)
wifi.start()

wifi.ap.on("sta_connected", function(mac, id)
  print("sta connected")
  --dofile("main.lua")
end)
