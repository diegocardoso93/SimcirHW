// Module for interfacing with the SPI master hardware

#include <string.h>
#include "module.h"
#include "lauxlib.h"
#include "lextra.h"

#include "driver/spi_master.h"
#include "esp_heap_caps.h"

#include "esp_log.h"

#include "spi_common.h"

#define SPI_MASTER_TAG "spi.master"

#define UD_HOST_STR "spi.master"
#define UD_DEVICE_STR "spi.device"


static int no_err( esp_err_t err )
{
  switch (err)
  {
  default:
    ESP_LOGI(SPI_MASTER_TAG, "unknown error");
    return 0;
  case ESP_ERR_INVALID_ARG:
    ESP_LOGI(SPI_MASTER_TAG, "invalid argument");
    return 0;
  case ESP_ERR_INVALID_STATE: 
    ESP_LOGI(SPI_MASTER_TAG, "internal logic error");
    return 0;
  case ESP_ERR_NO_MEM:
    ESP_LOGI(SPI_MASTER_TAG, "no memory");
    return 0;
  case ESP_OK:
    return 1;
  }
}

// ****************************************************************************
// Device related functions
//
typedef struct {
  spi_device_handle_t device;
  int host_ref, host;
} lspi_device_t;

#define GET_UD_DEVICE \
  lspi_device_t *ud = (lspi_device_t *)luaL_checkudata( L, 1, UD_DEVICE_STR );

#define CONFIG_TRANS_FROM_FIELD(field) \
  lua_getfield( L, stack, #field ); \
  trans.field = luaL_optint( L, -1, 0 );

static int lspi_device_free( lua_State *L )
{
  GET_UD_DEVICE;

  if (ud->device) {
    spi_bus_remove_device( ud->device );
    ud->device = NULL;

    // unref host to unblock automatic gc from this object
    luaL_unref( L, LUA_REGISTRYINDEX, ud->host_ref );
    ud->host_ref = LUA_NOREF;
  }

  return 0;
}

// Lua:
//   recv = dev:transfer(trans_desc)
//   recv = dev:transfer(data)
static int lspi_device_transfer( lua_State *L )
{
  GET_UD_DEVICE;
  int stack = 1;

  luaL_argcheck( L, ud->device, stack, "no device" );

  spi_transaction_t trans;
  memset( &trans, 0, sizeof( trans ) );
  size_t data_len, rx_len;
  const char *data;

  int type = lua_type( L, ++stack );
  luaL_argcheck( L, type == LUA_TSTRING || type == LUA_TTABLE, stack, "string or table expected" );

  if (type == LUA_TSTRING) {

    data = luaL_checklstring( L, stack, &data_len );
    rx_len = data_len;

  } else {
    const char * const options[] = {"std", "dio", "qio"};
    const uint32_t options_flags[] = {0, SPI_TRANS_MODE_DIO, SPI_TRANS_MODE_QIO};

    CONFIG_TRANS_FROM_FIELD(cmd);
    CONFIG_TRANS_FROM_FIELD(addr);
    //
    lua_getfield( L, stack, "rxlen" );
    rx_len = luaL_optint( L, -1, 0 );
    //
    lua_getfield( L, stack, "addr_mode" );
    trans.flags |= luaL_optbool( L, -1, false ) ? SPI_TRANS_MODE_DIOQIO_ADDR : 0;
    //
    lua_getfield( L, stack, "mode" );
    trans.flags |= options_flags[ luaL_checkoption( L, -1, options[0], options ) ];
    //
    lua_getfield( L, stack, "txdata" );
    data = luaL_optlstring( L, -1, "", &data_len );

    lua_settop( L, stack );
  }

  const char *msg = NULL;

  trans.length = data_len * 8;
  if (data_len == 0) {
    //no MOSI phase requested
    trans.tx_buffer = NULL;
  } else if (data_len <=4 ) {
    // use local tx data buffer
    trans.flags |= SPI_TRANS_USE_TXDATA;
    memcpy( trans.tx_data, data, data_len );

  } else {
    // use DMA'able buffer
    if ((trans.tx_buffer = heap_caps_malloc( data_len, MALLOC_CAP_DMA ))) {
      memcpy( (void *)trans.tx_buffer, data, data_len );
    } else {
      msg = "no memory";
      goto free_mem;
    }
  }

  trans.rxlength = rx_len * 8;
  if (rx_len == 0) {
    // no MISO phase requested
    trans.rx_buffer = NULL;

  } else if (rx_len <= 4) {
    // use local rx data buffer
    trans.flags |= SPI_TRANS_USE_RXDATA;

  } else {
    // use DMA'able buffer
    if (!(trans.rx_buffer = heap_caps_malloc( rx_len, MALLOC_CAP_DMA ))) {
      msg = "no mem";
      goto free_mem;
    }
  }

  // finally perform the transaction
  if (no_err( spi_device_transmit( ud->device, &trans ) )) {
    // evaluate receive data
    if (trans.flags & SPI_TRANS_USE_RXDATA) {
      lua_pushlstring( L, (const char *)&(trans.rx_data[0]), rx_len );
    } else {
      lua_pushlstring( L, trans.rx_buffer, rx_len );
    }

  } else
    msg = "transfer failed";

free_mem:
  if (!(trans.flags & SPI_TRANS_USE_TXDATA) && trans.tx_buffer)
    heap_caps_free( (void *)trans.tx_buffer );
  if (!(trans.flags & SPI_TRANS_USE_RXDATA) && trans.rx_buffer)
    heap_caps_free( (void *)trans.rx_buffer );

  if (msg)
    return luaL_error( L, msg );

  return 1;
}


static const LUA_REG_TYPE lspi_device_map[] = {
  { LSTRKEY( "transfer" ), LFUNCVAL( lspi_device_transfer ) },
  { LSTRKEY( "remove" ),   LFUNCVAL( lspi_device_free ) },
  { LSTRKEY( "__gc" ),     LFUNCVAL( lspi_device_free ) },
  { LSTRKEY( "__index" ),  LROVAL( lspi_device_map ) },
  {LNILKEY, LNILVAL}
};


// ****************************************************************************
// Host related functions
//
#define GET_UD_HOST \
  lspi_host_t *ud = (lspi_host_t *)luaL_checkudata( L, 1, UD_HOST_STR );
//
#define CONFIG_BUS_PIN_FROM_FIELD(pin) \
  lua_getfield( L, stack, #pin ); \
  config.pin ## _io_num = luaL_optint( L, -1, -1 );
//
#define CONFIG_DEVICE_FROM_INT_FIELD(field) \
  lua_getfield( L, stack, #field ); \
  config.field = luaL_optint( L, -1, 0 );
#define CONFIG_DEVICE_FROM_BOOL_FIELD(field, mask) \
  lua_getfield( L, stack, #field ); \
  config.flags |= luaL_optbool( L, -1, false ) ? mask : 0;

static int lspi_host_free( lua_State *L )
{
  GET_UD_HOST;

  if (ud->host >= 0) {
    spi_bus_free( ud->host );
    ud->host = -1;
  }

  return 0;
}

// Lua: master = spi.master(host, config)
int lspi_master( lua_State *L )
{
  int stack = 0;

  int host = luaL_checkint( L, ++stack );
  luaL_argcheck( L,
                 host == SPI_HOST || host == HSPI_HOST || host == VSPI_HOST,
                 stack,
                 "invalid host" );

  luaL_checktype( L, ++stack, LUA_TTABLE );

  spi_bus_config_t config;
  memset( &config, 0, sizeof( config ) );
  //
  CONFIG_BUS_PIN_FROM_FIELD(sclk);
  CONFIG_BUS_PIN_FROM_FIELD(mosi);
  CONFIG_BUS_PIN_FROM_FIELD(miso);
  CONFIG_BUS_PIN_FROM_FIELD(quadwp);
  CONFIG_BUS_PIN_FROM_FIELD(quadhd);
  lua_pop( L, 5 );

  int use_dma = luaL_optint( L, ++stack, 1 );
  luaL_argcheck( L, use_dma >= 0 && use_dma <= 2, stack, "out of range" );

  if (no_err( spi_bus_initialize( host, &config, use_dma ) )) {
    lspi_host_t *ud = (lspi_host_t *)lua_newuserdata( L, sizeof( lspi_host_t ) );
    luaL_getmetatable( L, UD_HOST_STR );
    lua_setmetatable( L, -2 );
    ud->host = host;

    return 1;
  }

  return luaL_error( L, "bus init failed" );
}

// Lua: dev = master:device(config)
static int lspi_host_device( lua_State *L )
{
  GET_UD_HOST;
  int stack = 1;

  luaL_argcheck( L, ud->host >= 0, stack, "no active bus host" );

  luaL_checktype( L, ++stack, LUA_TTABLE );

  spi_device_interface_config_t config;
  memset( &config, 0, sizeof( config ) );

  // mandatory fields
  lua_getfield( L, stack, "cs" );
  config.spics_io_num = luaL_optint( L, -1, -1 );
  //
  lua_getfield( L, stack, "mode" );
  int mode = luaL_optint( L, -1, -1 );
  luaL_argcheck( L, mode >= 0, stack, "mode setting missing" );
  config.mode = (uint8_t)mode;
  //
  lua_getfield( L, stack, "freq" );
  int freq = luaL_optint( L, -1, -1 );
  luaL_argcheck( L, freq >= 0, stack, "freq setting missing" );
  config.clock_speed_hz = freq;
  //
  // optional fields
  CONFIG_DEVICE_FROM_INT_FIELD(command_bits);
  CONFIG_DEVICE_FROM_INT_FIELD(address_bits);
  CONFIG_DEVICE_FROM_INT_FIELD(dummy_bits);
  CONFIG_DEVICE_FROM_INT_FIELD(cs_ena_pretrans);
  CONFIG_DEVICE_FROM_INT_FIELD(cs_ena_posttrans);
  CONFIG_DEVICE_FROM_INT_FIELD(duty_cycle_pos);
  CONFIG_DEVICE_FROM_BOOL_FIELD(tx_lsb_first, SPI_DEVICE_TXBIT_LSBFIRST);
  CONFIG_DEVICE_FROM_BOOL_FIELD(rx_lsb_first, SPI_DEVICE_RXBIT_LSBFIRST);
  CONFIG_DEVICE_FROM_BOOL_FIELD(wire3, SPI_DEVICE_3WIRE);
  CONFIG_DEVICE_FROM_BOOL_FIELD(positive_cs, SPI_DEVICE_POSITIVE_CS);
  CONFIG_DEVICE_FROM_BOOL_FIELD(halfduplex, SPI_DEVICE_HALFDUPLEX);
  CONFIG_DEVICE_FROM_BOOL_FIELD(clk_as_cs, SPI_DEVICE_CLK_AS_CS);
  lua_settop( L, stack );
  //
  // fill remaining config entries
  config.queue_size = 1;

  lspi_device_t *dev = (lspi_device_t *)lua_newuserdata( L, sizeof( lspi_device_t ) );
  dev->device = NULL;
  if (no_err( spi_bus_add_device( ud->host, &config, &(dev->device) ) )) {
    luaL_getmetatable( L, UD_DEVICE_STR );
    lua_setmetatable( L, -2 );

    // reference host object to avoid automatic gc
    lua_pushvalue( L, 1 );
    dev->host_ref = luaL_ref( L, LUA_REGISTRYINDEX );
    dev->host = ud->host;

    return 1;
  }

  lua_pop( L, 1 );

  return luaL_error( L, "failed to add device" );
}


static const LUA_REG_TYPE lspi_master_map[] = {
  { LSTRKEY( "device" ),  LFUNCVAL( lspi_host_device ) },
  { LSTRKEY( "close" ),   LFUNCVAL( lspi_host_free ) },
  { LSTRKEY( "__gc" ),    LFUNCVAL( lspi_host_free ) },
  { LSTRKEY( "__index" ), LROVAL( lspi_master_map ) },
  {LNILKEY, LNILVAL}
};


// ****************************************************************************
// Generic
//
int luaopen_spi_master( lua_State *L ) {
  luaL_rometatable(L, UD_HOST_STR, (void *)lspi_master_map);
  luaL_rometatable(L, UD_DEVICE_STR, (void *)lspi_device_map);
  return 0;
}
