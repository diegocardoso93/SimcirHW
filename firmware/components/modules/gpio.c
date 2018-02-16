// vim: ts=2 sw=2 et ai
/*
 * Copyright (c) 2016 Johny Mattsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "module.h"
#include "lauxlib.h"
#include "lmem.h"

#include "driver/gpio.h"
#include "task/task.h"

#include <assert.h>

#define PULL_UP 1
#define PULL_DOWN 2

static int *gpio_cb_refs = NULL; // Lazy init
static task_handle_t cb_task;

static int check_err (lua_State *L, esp_err_t err)
{
  switch (err)
  {
    case ESP_ERR_INVALID_ARG: luaL_error (L, "invalid argument");
    case ESP_ERR_INVALID_STATE: luaL_error (L, "internal logic error");
    case ESP_OK: break;
  }
  return 0;
}

// TODO: can/should we attempt to guard against task q overflow?
_Static_assert(GPIO_PIN_COUNT<256, "task post encoding assumes < 256 gpios");
static void single_pin_isr (void *p)
{
  gpio_num_t gpio_num = (gpio_num_t)p;
  gpio_intr_disable (gpio_num);
  task_post_low (cb_task, (gpio_num) | (gpio_get_level (gpio_num) << 8));
}


/* Lua: gpio.config({
 *   gpio= x || { x, y ... }
 *   dir= IN || OUT || IN_OUT
 *   opendrain= 0 || 1 (output only)
 *   pull= UP || DOWN || UP_DOWN || FLOATING
 * }, ...
 */
static int lgpio_config (lua_State *L)
{
  const int n = lua_gettop (L);
  luaL_checkstack (L, 5, "out of mem");
  for (int i = 1; i <= n; ++i)
  {
    luaL_checkanytable (L, i);
    gpio_config_t cfg;
    cfg.intr_type = GPIO_INTR_DISABLE;

    lua_getfield (L, i, "dir");
    cfg.mode = luaL_checkinteger (L, -1);
    lua_getfield(L, i, "opendrain");
    if (luaL_optint (L, -1, 0) && (cfg.mode & GPIO_MODE_DEF_OUTPUT))
      cfg.mode |= GPIO_MODE_DEF_OD;

    lua_getfield(L, i, "pull");
    int pulls = luaL_optint (L, -1, 0);
    cfg.pull_down_en =
      (pulls & PULL_DOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en =
      (pulls & PULL_UP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;

    lua_pop (L, 3);

    cfg.pin_bit_mask = 0;
    lua_getfield(L, i, "gpio");
    int type = lua_type (L, -1);
    if (type == LUA_TNUMBER)
      cfg.pin_bit_mask = 1ULL << lua_tointeger (L, -1);
    else if (type == LUA_TTABLE)
    {
      lua_pushnil (L);
      while (lua_next (L, -2) != 0)
      {
        lua_pushvalue (L, -1); // copy, so lua_tonumber() doesn't break iter
        int pin = lua_tointeger (L, -1);
        lua_pop (L, 2); // leave key
        cfg.pin_bit_mask |= 1ULL << pin;
      }
    }
    else
      return luaL_error (L, "missing/bad 'gpio' field");
    lua_settop (L, n); // discard remaining temporaries
    check_err (L, gpio_config (&cfg));
  }
  return 0;
}


// Lua: gpio.read(gpio) => 0 || 1
static int lgpio_read (lua_State *L)
{
  int gpio = luaL_checkint (L, 1);
  if (!GPIO_IS_VALID_GPIO(gpio))
    return check_err (L, ESP_ERR_INVALID_ARG);
  lua_pushinteger (L, gpio_get_level (gpio));
  return 1;
}


// Lua: gpio.trig(gpio, UP/DOWN/etc, function(gpio, level) ... end || nil)
//   up, down, both, low, high
static int lgpio_trig (lua_State *L)
{
  int gpio = luaL_checkint (L, 1);
  int intr_type = luaL_checkint (L, 2);
  if (!lua_isnoneornil (L, 3))
    luaL_checkanyfunction (L, 3);

  lua_settop (L, 3);

  if (gpio < 0 || gpio >= GPIO_PIN_COUNT)
    return luaL_error (L, "invalid gpio");

  if (!gpio_cb_refs)
  {
    gpio_cb_refs = luaM_newvector (L, GPIO_PIN_COUNT, int);
    for (unsigned i = 0; i < GPIO_PIN_COUNT; ++i)
      gpio_cb_refs[i] = LUA_NOREF;
  }

  // Set/update interrupt callback
  if (!lua_isnoneornil (L, 3))
  {
    luaL_unref (L, LUA_REGISTRYINDEX, gpio_cb_refs[gpio]);
    gpio_cb_refs[gpio] = luaL_ref (L, LUA_REGISTRYINDEX);
  }

  // Disable interrupt while reconfiguring
  check_err (L, gpio_intr_disable (gpio));

  if (gpio_cb_refs[gpio] == LUA_NOREF)
  {
    check_err (L, gpio_set_intr_type (gpio, GPIO_INTR_DISABLE));
    check_err (L, gpio_isr_handler_remove (gpio));
  }
  else
  {
    check_err (L, gpio_set_intr_type (gpio, intr_type));
    check_err (L, gpio_isr_handler_add (gpio, single_pin_isr, (void *)gpio));
    check_err (L, gpio_intr_enable (gpio));
  }
  return 0;
}


// Lua: gpio.wakeup(gpio, INTR_NONE | INTR_LOW | INTR_HIGH)
static int lgpio_wakeup (lua_State *L)
{
  int gpio = luaL_checkint (L, 1);
  int intr_type = luaL_optint (L, 2, GPIO_INTR_DISABLE);
  if (intr_type == GPIO_INTR_DISABLE)
    check_err (L, gpio_wakeup_disable (gpio));
  else
    check_err (L, gpio_wakeup_enable (gpio, intr_type));
  return 0;
}


// Lua: gpio.write(gpio, 0 || 1)
static int lgpio_write (lua_State *L)
{
  int gpio = luaL_checkint (L, 1);
  int level = luaL_checkint (L, 2);
  check_err (L, gpio_set_level (gpio, level));
  return 0;
}



static void nodemcu_gpio_callback_task (task_param_t param, task_prio_t prio)
{
  (void)prio;
  uint32_t gpio = (uint32_t)param & 0xffu;
  int level = ((int)param) & 0x100u;

  lua_State *L = lua_getstate ();
  if (gpio_cb_refs[gpio] != LUA_NOREF)
  {
    lua_rawgeti (L, LUA_REGISTRYINDEX, gpio_cb_refs[gpio]);
    lua_pushinteger (L, gpio);
    lua_pushinteger (L, level);
    lua_call (L, 2, 0);
    gpio_intr_enable (gpio);
  }
}


static int nodemcu_gpio_init (lua_State *L)
{
  cb_task = task_get_id (nodemcu_gpio_callback_task);
  check_err (L,
    gpio_install_isr_service (ESP_INTR_FLAG_LOWMED));
  return 0;
}


static const LUA_REG_TYPE lgpio_map[] =
{
  { LSTRKEY( "config" ),            LFUNCVAL( lgpio_config )          },
  { LSTRKEY( "read"   ),            LFUNCVAL( lgpio_read )            },
  { LSTRKEY( "trig" ),              LFUNCVAL( lgpio_trig )            },
  { LSTRKEY( "wakeup" ),            LFUNCVAL( lgpio_wakeup )          },
  { LSTRKEY( "write"  ),            LFUNCVAL( lgpio_write )           },


  { LSTRKEY( "OUT" ),               LNUMVAL( GPIO_MODE_OUTPUT )       },
  { LSTRKEY( "IN" ),                LNUMVAL( GPIO_MODE_INPUT )        },
  { LSTRKEY( "IN_OUT" ),            LNUMVAL( GPIO_MODE_INPUT_OUTPUT ) },

  { LSTRKEY( "FLOATING"),           LNUMVAL( 0 )                      },
  { LSTRKEY( "PULL_UP" ),           LNUMVAL( PULL_UP )                },
  { LSTRKEY( "PULL_DOWN" ),         LNUMVAL( PULL_DOWN )              },
  { LSTRKEY( "PULL_UP_DOWN"),       LNUMVAL( PULL_UP | PULL_DOWN )    },

  { LSTRKEY( "INTR_NONE" ),         LNUMVAL( GPIO_INTR_DISABLE )      },
  { LSTRKEY( "INTR_UP" ),           LNUMVAL( GPIO_INTR_POSEDGE )      },
  { LSTRKEY( "INTR_DOWN" ),         LNUMVAL( GPIO_INTR_NEGEDGE )      },
  { LSTRKEY( "INTR_UP_DOWN" ),      LNUMVAL( GPIO_INTR_ANYEDGE )      },
  { LSTRKEY( "INTR_LOW" ),          LNUMVAL( GPIO_INTR_LOW_LEVEL )    },
  { LSTRKEY( "INTR_HIGH" ),         LNUMVAL( GPIO_INTR_HIGH_LEVEL )   },

  { LNILKEY, LNILVAL }
};

NODEMCU_MODULE(GPIO, "gpio", lgpio_map, nodemcu_gpio_init);
