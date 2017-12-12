
local logger = {}

local Logger = {}

function Logger:push_state(data)
  self.data[#self.data+1] = data
end

-- free memmory after cycle
function Logger:clean()
  self.data = nil
  collectgarbage()
end

function Logger:__tostring()
  
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