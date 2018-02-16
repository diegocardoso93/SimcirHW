
local m = {}

m.wsUrl = "ws://10.0.0.104:8989/ws"

-- private properties
m.isInitted = false
m.sock = nil
m.myIp = nil
m.isConn = false

function m.init(myip)
  
  if myip then m.myIp = myip end

  if m.myIp == nil then
    print("Websocket: You need to connect to wifi, or if you are, give me the IP address of this ESP32 in the init method.")
  else 
    print("Websocket initted. My IP: " .. m.myIp)
    m.isInitted = true
  end
end

m.onDataCallback = nil
m.onConnectedCallback = nil 
m.onDisconnectCallback = nil
function m.on(method, func)
  if method == "receive" then
    m.onDataCallback = func 
  elseif method == "connection" then 
    m.onConnectedCallback = func 
  elseif method == "disconnection" then
    m.onDisconnectCallback = func
  end 
end

function m.connect(host, port, callbackOnConnected)
  
  if wsurl ~= nil then m.wsUrl = wsurl end
  if callbackOnConnected ~= nil then m.callbackOnConnected = callbackOnConnected end 
  
  print("Websocket connecting to:", host, "port:", port)
  
  local body = "GET / HTTP/1.1\r\n"
  body = body .. "Host: " .. host .. "\r\n"
  body = body .. "Upgrade: websocket\r\n"
  body = body .. "Connection: Upgrade\r\n"
  body = body .. "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"
  body = body .. "Sec-WebSocket-Protocol: chat, superchat\r\n"
  body = body .. "Sec-WebSocket-Version: 13\r\n"
  body = body .. "Origin: esp32\r\n"
  body = body .. "\r\n"
  
  local sk = net.createConnection(net.TCP)
  
  local buffer
  local isHdrRecvd = false
  sk:on("receive", function(sck, c) 
    print("Got receive: " .. c)
    
    -- see if we are good to go here 
    if isHdrRecvd == false then 
      -- look for header    
      -- look for HTTP/1.1 101
      if string.match(c, "HTTP/1.1 101(.*)\r\n\r\n") then 
        print("Websocket found hdr")
        isHdrRecvd = true
        m.isConn = true
        if m.onConnectedCallback ~= nil then 
          node.task.post(node.task.LOW_PRIORITY, function()
            m.onConnectedCallback(host, port) 
          end)
        end 
      end 
    else 
      -- we can start to deliver incoming data 
  
      m.decodeFrame(c)
    end 
    
  end)
  sk:on("sent", function()
    -- print("Websocket sent data")
  end)
  sk:on('disconnection', function(errcode)
    print("Websocket disconnection. err:", errcode)
    m.isConn = false
    if m.onDisconnectCallback then m.onDisconnectCallback() end 
  end)
  sk:on('reconnection', function(errcode)
    print('Websocket reconnection. err:', errcode)
  end)
  sk:on("connection", function(sck, c)
    -- print("Websocket got connection. Sending TCP msg. body:")
    print(body)
    -- m.sock = sck 
    sck:send(body)
    
  end)
  
  m.sock = sk
  sk:connect(port, host)
  
end

-- currently only supports minimal frame decode
m.lenExpected = 0
m.buffer = ""
function m.decodeFrame(frame)
  
  local data, fin, opcode
  
  if m.lenExpected > 0 then 
    -- we should just raw append
    data = frame 
    -- print("Raw len:", string.len(data))
    -- m.lenExpected = m.lenExpected - string.len(data)
    
    -- append the new data to the previous data
    m.buffer = m.buffer .. data

  else
    -- we need to decode the headr from the frame 
  
    -- get FIN. 1 means msg is complete. 0 means multi-part
    fin = string.byte(frame, 1)
    fin = bit.isset(fin, 7)
    -- print("FIN:", fin)
    
    -- get opcode
    opcode = string.byte(frame, 1)
    opcode = bit.clear(opcode, 4, 5, 6, 7) -- clear FIN and RSV
    -- print("Opcode:", opcode)
    

    -- get 2nd byte as it has the payload length
    -- msb of byte is mask, remaining 7 bytes is len 
    local plen = string.byte(frame, 2)
    local mask = bit.isset(plen, 7)
    plen = bit.clear(plen, 7) -- remove the mask from the length
    -- print("Frame len:", plen, " Mask:", mask)
    m.lenExpected = plen
    
    if mask then 
      -- print("We should not get a mask from server. Error.")
      return
    end
    
    data = string.sub(frame, 3) -- remove first 2 bytes, i.e. start at 3rd char
  
    if plen == 126 then 
      -- read next 2 bytes
      local extlen = string.byte(frame, 3)
      local extlen2 = string.byte(frame, 4)
      -- bit shift by one byte extlen 
      extlen = bit.lshift(extlen, 8)
      local actualLen = extlen + extlen2
      -- print("ActualLen:", actualLen)
      m.lenExpected = actualLen
      
      data = string.sub(data, 3) -- remove first 2 bytes
      
    elseif plen == 127 then 
      -- print("Websocket lib does not support longer payloads yet")
      -- return
      data = string.sub(data, 5) -- remove first 4 bytes
    end
    
    -- set the buffer to the current data since it's new
    m.buffer = data
  end 
  
  -- print("Our own count of len:", string.len(data))
  
  -- calc m.lenExpected for next time back into this method
  m.lenExpected = m.lenExpected - string.len(data)
  -- print("m.lenExpected:", m.lenExpected)
  
  -- we need to see if next time into decodeFrame we are just 
  -- expecting more raw data without a frame header, i.e. happens if 
  -- the TCP packet is > 1024
  if m.lenExpected > 0 then
    print("Websocket expecting " .. m.lenExpected .. " more chars of data")
  else 
    -- done with data. do callback 
    m.lenExpected = 0 
  
    -- print("Payload:", data, "opcode:", opcode)
    
    if m.onDataCallback ~= nil then 
      m.onDataCallback(m.buffer, fin, opcode)
    end 
    -- set buffer to empty for next time into this method
    m.buffer = ""
  end

end

-- currently only supports sending short frames less than 126 chars
function m.send(data)
  
  print("Websocket doing send. data:", data)
  
  if m.isConn == false then 
    print("Websocket not connected, so cannot send.")
    return
  end 
  
  local binstr, payloadLen 
  
  -- we need to create the frame headers
  --if string.len(data) > 126 then 
  --  print("Websocket lib only supports max len 126 currently")
  --  return
  --end
  
  -- print("Len: ", string.len(data))
  
  -- 1st byte
  -- binstr = string.char(0x1) -- opcode set to 0x1 for txt
  binstr = string.char(bit.set(0x1, 7)) -- set FIN to 1 meaning we will not multi-part this msg

  -- 2nd byte mask and payload length
  payloadLen = string.len(data) 
  payloadLen = bit.set(payloadLen, 7) -- set mask to on for 8th msb
  binstr = binstr .. string.char(tonumber(payloadLen))
  
  -- 3rd, 4th, 5th, and 6th byte is masking key
  -- just use mask of 0 to cheat so no need to xor
  binstr = binstr .. string.char(0x0,0x0,0x0,0x0)
  
  -- Now add payload 
  binstr = binstr .. data

  -- print out the bytes in decimal
  -- print(string.byte(binstr, 1, string.len(binstr)))
  -- print("Len binstr:", string.len(binstr))
  -- print(binstr)

  m.sock:send(binstr)  
end 

function m.reconnect()
  print("Reconnecting")
  if m.isConn then 
    m.sock:close()
    m.isConn = false
  end
  m.connect()
end  

function m.disconnect()
  if m.isConn then 
    print("Websocket closing...")
    
    -- 1st byte
    -- opcode set to 0x8 for close
    binstr = string.char(bit.set(0x8, 7)) -- set FIN to 1 meaning we will not multi-part this msg
    m.sock:send(binstr)  
    m.sock:close()
    m.isConn = false
    print("Websocket now closed")
  else 
    print("Websocket was not connected, so not closing")
  end 
end 

function m.getUrl()
  return m.wsUrl
end 

function m.isConnected()
  return m.isConn
end

return m