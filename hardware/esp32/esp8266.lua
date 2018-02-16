
local Esp8266 = {}

Esp8266.NodeMCU_V2 = {
  named_pins = {
    D0  =  19, -- GPIO16
    D1  =  21, -- GPIO5
    D2  =  2, -- GPIO4
    D3  =  3, -- GPIO0
    D4  =  4, -- GPIO2
    D5  =  5, -- GPIO14
    D6  =  6, -- GPIO12
    D7  =  7, -- GPIO13
    D8  =  8, -- GPIO15
    -- RX  =  9, -- GPIO3
    -- TX  = 10, -- GPIO1
    -- SD2 = 11, -- GPIO9
    SD3 = 12  -- GPIO10
  }
}

Esp8266.Wemos_D1 = {
  named_pins = {
    D2  = 0, -- GPIO16
    D3  = 1, -- GPIO5
    D4  = 2, -- GPIO4
    D8  = 3, -- GPIO0
    D9  = 4, -- GPIO2
    D13 = 5, -- GPIO14
    D12 = 6, -- GPIO12
    D11 = 7, -- GPIO13
    D10 = 8, -- GPIO15
    -- RX  =  9, -- GPIO3
    -- TX  = 10, -- GPIO1
    -- SD2 = 11, -- GPIO9
    -- SD3 = 12  -- GPIO10
  }
}

function Esp8266.get_name()
  return "esp8266"
end

function Esp8266.set_board(board_name)
  Esp8266.board = Esp8266[board_name]
end

function Esp8266.get_pin(label)
  return Esp8266.board.named_pins[label]
end

return Esp8266