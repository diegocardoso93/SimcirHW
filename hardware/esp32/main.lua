
--require "circuitlib"
--require "utils"

local SimcirHW = require("simcirhw")
local ws = require("websocket")

function handle_ws_message(str_msg)
  print("msg received: " .. str_msg)
  SCH:eval_message(str_msg);
  
  if SCH.message.type == "circuit" then
    SCH:prepare_circuit(SCH.message.circuit)
    SCH:start()
  end
end

ws.on("connection", function() 
  print("got ws connection")
end)
ws.on("receive", function(msg, fin)
  print("Got msg:" .. msg .. ", fin:", fin)
  SCH = SimcirHW:new()
  SCH.ws = ws
  handle_ws_message(msg)
end)
ws.on("close", function(_)
   print("ws connection closed")
end)
ws.connect("192.168.4.2", 9061)
