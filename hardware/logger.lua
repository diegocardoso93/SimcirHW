
local logger = {}

local Logger = {}

function Logger:push_state(data)
  -- data.timestamp = tmr.get_time()
  _t = {outputs={}, inputs={}}
  for outk, outv in pairs(data.outputs) do
    _t.outputs[outk] = outv
  end
  for inpk, inpv in pairs(data.inputs) do
    _t.inputs[inpk] = inpv
  end
  self.data[#self.data+1] = _t
end

-- free memmory after cycle
function Logger:clean()
  self.data = nil
  collectgarbage()
end

function Logger:dump()
  for k, state in pairs(self.data) do
    io.write(k)
    for x, inout in pairs(state) do
      io.write("| " .. x .. ": ")
      for y, v in pairs(inout) do
        io.write(y .. " " .. v .. " ")
      end
    end
    print()
  end
end

function Logger:format_message_to_send()
  self.message = 
  [===[
  {
    type="datalog",
    payload={
      inputs={}
      outputs={}
    }
  }
  ]===];
end

function logger:new()
  local self = {}
  setmetatable(self, { __index = Logger})
  
  self.data = {}
  self.message = {}
  return self
end

return logger