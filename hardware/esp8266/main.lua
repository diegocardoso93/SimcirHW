
local SimcirHW = require("simcirhw")

function handle_ws_message(str_msg)
  print("msg received: " .. str_msg)
  SCH:eval_message(str_msg);
  
  if SCH.message.type == "circuit" and not SCH.executing then
    SCH:prepare_circuit(SCH.message.circuit)
    SCH:start()
  elseif SCH.message.type == "datalog_stop" then
    SCH:stop()
  end
end

local ws = websocket.createClient()
ws:on("connection", function()
  print("got ws connection")
end)
ws:on("receive", function(_, msg, code)
  if SCH == nil then
    SCH = SimcirHW:new()
    SCH.ws = ws
  end
  handle_ws_message(msg)
end)
ws:on("close", function(_)
  print("ws connection closed")
  ws:connect("ws://192.168.1.100:9061")
end)
ws:connect("ws://192.168.1.100:9061")
