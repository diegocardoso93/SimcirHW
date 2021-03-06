
-- init.lua --
-- NodeMCU
-- https://github.com/nodemcu/nodemcu-firmware
-- branch: master
-- commit: 2e67ff5a639a13260fd4fb3c3b627ccdc2845616
-- modules: file,gpio,node,tmr,uart,websocket,wifi,sjson
-- powered by Lua 5.1.4

print("\n UNISC \n")

print("Chip ID: " .. node.chipid() .. "\n")
print("Heap Size: " .. node.heap() .. "\n")
print("MAC Address: " .. wifi.ap.getmac() .. "\n")

print("Initializing...\n")

wifi.setmode(wifi.SOFTAP)
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

print("Configuring IP...\n")
ip_cfg = {
    ip = "192.168.1.1",
    netmask = "255.255.255.0",
    gateway = "192.168.1.1"
}
wifi.ap.setip(ip_cfg)

dhcp_config = {start = "192.168.1.100"}
wifi.ap.dhcp.config(dhcp_config)

print("Starting DHCP Service...\n")
wifi.ap.dhcp.start()

wifi.eventmon.register(wifi.eventmon.AP_STACONNECTED, function(T)
  print("\n\tAP: Station connected. \n\tMAC: " .. T.MAC .. "\n\tAID: " .. T.AID)
  tmr.create():alarm(6000, tmr.ALARM_SINGLE, function()
    dofile("main.lua")
  end)
end)

wifi.eventmon.register(wifi.eventmon.AP_STADISCONNECTED, function(T)
  print("\n\tAP: Station disconnected. \n\tMAC: " .. T.MAC .. "\n\tAID: " .. T.AID)
end)
