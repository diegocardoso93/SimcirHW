
-- custom operator metatable hack
-- see more: http://lua-users.org/wiki/CustomOperators
local function infix(f)
  local metatable = { __sub = function(self, b) return f(self[1], b) end }
  return setmetatable({}, { __sub = function(a, _) return setmetatable({ a }, metatable) end })
end

 bitnot = function (x) return x == 0 and 1 or 0 end
  bitor = infix(function(a, b) return (a == 1 or b == 1) and 1 or 0 end)
 bitand = infix(function(a, b) return (a == b and a == 1) and 1 or 0 end)
 bitxor = infix(function(a, b) return a ~= b and 1 or 0 end)
 bitnor = infix(function(a, b) return bitnot(a -bitor- b) end)
bitnand = infix(function(a, b) return bitnot(a -bitand- b) end)

function convert_circuit(str)
  local map_replace = {
    [  'NOT' ]=  'bitnot'   ,
    [ ' AND ']=' -bitand- ' ,
    [  ' OR ']=' -bitor- '  ,
    [' NAND ']=' -bitnand- ',
    [ ' NOR ']=' -bitnor- ' ,
    [ ' XOR ']=' -bitxor- ' ,
  }
  for old, new in pairs(map_replace) do
    str = string.gsub(str, old, new)
  end
  return str
end
