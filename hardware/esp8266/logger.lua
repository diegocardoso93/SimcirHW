
local logger = {}

local Logger = {}

function Logger:push_state(data)
  -- data.timestamp = tmr.get_time()
  _t = {outputs={}, inputs={}, localtime=nil}
  for outk, outv in pairs(data.outputs) do
    _t.outputs[outk] = outv
  end
  for inpk, inpv in pairs(data.inputs) do
    _t.inputs[inpk] = inpv
  end
  _t.localtime = tmr.now() -- microseconds
  
  self.data[#self.data+1] = _t
end

-- free memmory after cycle
function Logger:clean()
  self.data = {}
  --collectgarbage()
end

function Logger:dump()
  for k, data in pairs(self.data) do
    io.write(k .. " |")
    for x, inp_val in pairs(data.inputs) do
      io.write(" " .. x .. ":" .. inp_val)
    end
    for x, out_val in pairs(data.outputs) do
      io.write(" " .. x .. ":" .. out_val)
    end
    io.write(" " .. data.localtime)
    print()
  end
end


--[[
template is:
{
  type="datalog",
  data={
    {
      inputs={},
      outputs={},
      localtime=nil
    },
    [...]
  }
}
]]
function Logger:format_message_to_send()
  self.message = {
    type="datalog",
    data=self.data
  }
  self.message = sjson.encode(self.message)
end

function logger:new()
  local self = {}
  setmetatable(self, { __index = Logger})
  
  self.data = {}
  self.message = {}
  return self
end

return logger