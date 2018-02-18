
ws = require('../esp32_websocket')

ws.on("receive", function(data, fin)
  print("Got data:" .. data .. ", fin:", fin)
end)
ws.on("connection", function(host, port, path)
  print("Websocket connected to host:", host, "port:", port)
  ws.send("list") -- teste < 126
  local buffSend = ""
  for i=0, 100, 1 do
	buffSend = buffSend .. "ABCDE"
  end
  ws.send(buffSend) -- teste > 126
end)
ws.on("disconnection", function()
  print("Websocket got disconnect from:", ws.wsUrl)
end)
ws.on("pingsend", function()
  print("Ping")
end)
ws.on("pongrecv", function()
  print("Got pong. We're alive.")
end)

ws.connect("192.168.4.2", "9061")