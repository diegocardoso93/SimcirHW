
local ws = {}

ws = {
  -- properties
  sock     = nil,
  is_conn  = false,
  ping_tmr = nil,

  -- callbacks
  ondata_callback       = nil,
  onconnected_callback  = nil, 
  ondisconnect_callback = nil,
  onping_recv_callback  = nil,
  onping_send_callback  = nil,
  onpong_recv_callback  = nil
}

function ws.on(method, func)
  if method == "receive" then
    ws.ondata_callback = func 
  elseif method == "connection" then 
    ws.onconnected_callback = func 
  elseif method == "disconnection" then
    ws.ondisconnect_callback = func
  elseif method == "pingrecv" then
    ws.onping_recv_callback = func
  elseif method == "pingsend" then
    ws.onping_send_callback = func
  elseif method == "pongrecv" then
    ws.onpong_recv_callback = func
  end 
end

function ws.connect(host, port)

  print("Websocket connecting to:", host, "port:", port)
  
  local body = "GET / HTTP/1.1\r\n"
  body = body .. "Host: " .. host .. "\r\n"
  body = body .. "Port: " .. port .. "\r\n"
  body = body .. "Upgrade: websocket\r\n"
  body = body .. "Connection: Upgrade\r\n"
  body = body .. "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
  body = body .. "Sec-WebSocket-Protocol: chat, superchat\r\n"
  body = body .. "Sec-WebSocket-Version: 13\r\n"
  body = body .. "Origin: esp32\r\n"
  body = body .. "\r\n"
  
  local sk = net.createConnection(net.TCP)
  
  local buffer
  local is_hdr_recvd = false
  sk:on("receive", function(sck, c) 
    if is_hdr_recvd == false then 
      if string.match(c, "HTTP/1.1 101(.*)\r\n\r\n") then 
        print("Websocket found hdr")
        is_hdr_recvd = true
        ws.is_conn = true
        if ws.onconnected_callback ~= nil then 
          node.task.post(node.task.LOW_PRIORITY, function()
            ws.onconnected_callback(host, port) 
          end)
        end 
      end 
    else 
      ws.decode_frame(c)
    end 
    
  end)
  sk:on("sent", function()
  end)
  sk:on('disconnection', function(errcode)
    print("Websocket disconnection. err:", errcode)
    ws.is_conn = false
    if ws.ondisconnect_callback then ws.ondisconnect_callback() end 
  end)
  sk:on('reconnection', function(errcode)
    print('Websocket reconnection. err:', errcode)
  end)
  sk:on("connection", function(sck, c)
    print(body)
    sck:send(body)
    --ws.ping_start()
  end)
  
  ws.sock = sk
  sk:connect(port, host)
  
end

ws.lenExpected = 0
ws.buffer = ""
function ws.decode_frame(frame)
  
  local data, fin, opcode
  
  if ws.lenExpected > 0 then 
    data = frame 
    ws.buffer = ws.buffer .. data
  else
  
    fin = string.byte(frame, 1)
    fin = bit.isset(fin, 7)
    
    opcode = string.byte(frame, 1)
    opcode = bit.clear(opcode, 4, 5, 6, 7)

    local plen = string.byte(frame, 2)
    local mask = bit.isset(plen, 7)
    plen = bit.clear(plen, 7)
    ws.lenExpected = plen
    
    if mask then 
      return
    end
    
    data = string.sub(frame, 3)
    
    if plen == 126 then 
      local extlen = string.byte(frame, 3)
      local extlen2 = string.byte(frame, 4)
      extlen = bit.lshift(extlen, 8)
      local actualLen = extlen + extlen2
      ws.lenExpected = actualLen
      
      data = string.sub(data, 3)
      
    elseif plen == 127 then 
      data = string.sub(data, 5)
    end
    
    ws.buffer = data
  end 
  
  ws.lenExpected = ws.lenExpected - string.len(data)
  if ws.lenExpected > 0 then
    print("Websocket expecting " .. ws.lenExpected .. " more chars of data")
  else 
    ws.lenExpected = 0 
  
    if opcode == 0x9 then
      ws.onPingRecv(ws.buffer, fin, opcode)
    elseif opcode == 0xA then 
      ws.onPongRecv(ws.buffer, fin, opcode)
    else
      if ws.ondata_callback ~= nil then 
        ws.ondata_callback(ws.buffer, fin, opcode)
      end 
    end
    ws.buffer = ""
  end

end

function ws.send(data)
  
  print("Websocket doing send. data:", data)
  
  if ws.is_conn == false then 
    print("Websocket not connected, so cannot send.")
    return
  end 
  
  local binstr
  local payloadLen = string.len(data)
  
  binstr = string.char(bit.set(0x1, 7))
  
  if payloadLen < 126 then
    payloadLen = bit.set(payloadLen, 7)
    binstr = binstr .. string.char(payloadLen)
  elseif payloadLen < 65536 then
    binstr = binstr .. string.char(254, bit.rshift(payloadLen, 8), bit.band(payloadLen, 255))
  end
  
  binstr = binstr .. string.char(0x0,0x0,0x0,0x0)
  binstr = binstr .. data

  ws.sock:send(binstr)  
end 

function ws.ping_start()
  if ws.ping_tmr then 
    local running, mode = ws.ping_tmr:state()
    if running then 
      print("Websocket being asked to run tmr a 2nd time. huh?")
      return 
    end
  end 
  
  ws.ping_tmr = tmr.create()
  ws.ping_tmr:alarm(10000, tmr.ALARM_AUTO, ws.onPingSend)
end 

function ws.ping_stop()
  ws.ping_tmr:stop()
  ws.ping_tmr:unregister()
end

ws.isGotPongBack = true
function ws.onPingSend()
  if ws.isGotPongBack then
  else
    ws.is_conn = false
    print("Websocket is dead. Reconnecting...")
    ws.reconnect()
    return
  end
  
  if ws.is_conn == false then
    print("We are not connected, can't send ping")
    ws.reconnect()
    return
  end
  
  ws.isGotPongBack = false
  
  local data = "ping"
  local binstr = string.char(bit.set(0x9, 7))
  local payloadLen = string.len(data)
  payloadLen = bit.set(payloadLen, 7)
  binstr = binstr .. string.char(payloadLen)
  
  binstr = binstr .. string.char(0x0,0x0,0x0,0x0)
  binstr = binstr .. data
  
  ws.sock:send(binstr)
  if ws.onping_send_callback then ws.onping_send_callback() end
end

function ws.onPingRecv()
  print("Ping received")
  if ws.onping_recv_callback then ws.onping_recv_callback() end
end

function ws.onPongRecv(data, fin, opcode)
  ws.isGotPongBack = true
  if ws.onpong_recv_callback then ws.onpong_recv_callback() end
end

function ws.reconnect()
  print("Reconnecting")
  if ws.is_conn then 
    ws.sock:close()
    ws.is_conn = false
  end
  ws.connect()
end  

function ws.disconnect()
  if ws.is_conn then 
    print("Websocket closing...")
    
    ws.ping_stop()
    
    binstr = string.char(bit.set(0x8, 7))
    ws.sock:send(binstr)  
    ws.sock:close()
    ws.is_conn = false
    print("Websocket now closed")
  else 
    print("Websocket was not connected, so not closing")
  end 
end

function ws.is_connected()
  return ws.is_conn
end

return ws