/*
 * Copyright 2016 Dius Computing Pty Ltd. All rights reserved.
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
 *
 * @author Johny Mattsson <jmattsson@dius.com.au>
 */
#include "module.h"
#include "lauxlib.h"
#include "lextra.h"
#include "lmem.h"
#include "nodemcu_esp_event.h"
#include "wifi_common.h"
#include "ip_fmt.h"
#include "nodemcu_esp_event.h"
#include <string.h>

static int scan_cb_ref = LUA_NOREF;

// --- Event handling -----------------------------------------------------

static void sta_conn (lua_State *L, const system_event_t *evt);
static void sta_disconn (lua_State *L, const system_event_t *evt);
static void sta_authmode (lua_State *L, const system_event_t *evt);
static void sta_got_ip (lua_State *L, const system_event_t *evt);
static void empty_arg (lua_State *L, const system_event_t *evt) {}

static const event_desc_t events[] =
{
  { "start",            SYSTEM_EVENT_STA_START,           empty_arg     },
  { "stop",             SYSTEM_EVENT_STA_STOP,            empty_arg     },
  { "connected",        SYSTEM_EVENT_STA_CONNECTED,       sta_conn      },
  { "disconnected",     SYSTEM_EVENT_STA_DISCONNECTED,    sta_disconn   },
  { "authmode_changed", SYSTEM_EVENT_STA_AUTHMODE_CHANGE, sta_authmode  },
  { "got_ip",           SYSTEM_EVENT_STA_GOT_IP,          sta_got_ip    },
};

#define ARRAY_LEN(a) (sizeof(a) / sizeof(a[0]))
static int event_cb[ARRAY_LEN(events)];

static void sta_conn (lua_State *L, const system_event_t *evt)
{
  lua_pushlstring (L,
    (const char *)evt->event_info.connected.ssid,
    evt->event_info.connected.ssid_len);
  lua_setfield (L, -2, "ssid");

  char bssid_str[MAC_STR_SZ];
  macstr (bssid_str, evt->event_info.connected.bssid);
  lua_pushstring (L, bssid_str);
  lua_setfield (L, -2, "bssid");

  lua_pushinteger (L, evt->event_info.connected.channel);
  lua_setfield (L, -2, "channel");

  lua_pushinteger (L, evt->event_info.connected.authmode);
  lua_setfield (L, -2, "auth");
}

static void sta_disconn (lua_State *L, const system_event_t *evt)
{
  lua_pushlstring (L,
    (const char *)evt->event_info.disconnected.ssid,
    evt->event_info.disconnected.ssid_len);
  lua_setfield (L, -2, "ssid");

  char bssid_str[MAC_STR_SZ];
  macstr (bssid_str, evt->event_info.disconnected.bssid);
  lua_pushstring (L, bssid_str);
  lua_setfield (L, -2, "bssid");

  lua_pushinteger (L, evt->event_info.disconnected.reason);
  lua_setfield (L, -2, "reason");
}

static void sta_authmode (lua_State *L, const system_event_t *evt)
{
  lua_pushinteger (L, evt->event_info.auth_change.old_mode);
  lua_setfield (L, -2, "old_mode");
  lua_pushinteger (L, evt->event_info.auth_change.new_mode);
  lua_setfield (L, -2, "new_mode");
}

static void sta_got_ip (lua_State *L, const system_event_t *evt)
{
  char ipstr[IP_STR_SZ] = { 0 };
  ip4str (ipstr, &evt->event_info.got_ip.ip_info.ip);
  lua_pushstring (L, ipstr);
  lua_setfield (L, -2, "ip");

  ip4str (ipstr, &evt->event_info.got_ip.ip_info.netmask);
  lua_pushstring (L, ipstr);
  lua_setfield (L, -2, "netmask");

  ip4str (ipstr, &evt->event_info.got_ip.ip_info.gw);
  lua_pushstring (L, ipstr);
  lua_setfield (L, -2, "gw");
}

static void on_event (const system_event_t *evt)
{
  int idx = wifi_event_idx_by_id (events, ARRAY_LEN(events), evt->event_id);
  if (idx < 0 || event_cb[idx] == LUA_NOREF)
    return;

  lua_State *L = lua_getstate ();
  lua_rawgeti (L, LUA_REGISTRYINDEX, event_cb[idx]);
  lua_pushstring (L, events[idx].name);
  lua_createtable (L, 0, 5);
  events[idx].fill_cb_arg (L, evt);
  lua_call (L, 2, 0);
}

NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_START,           on_event);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_STOP,            on_event);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_CONNECTED,       on_event);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_DISCONNECTED,    on_event);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_AUTHMODE_CHANGE, on_event);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_GOT_IP,          on_event);

void wifi_sta_init (void)
{
  for (unsigned i = 0; i < ARRAY_LEN(event_cb); ++i)
    event_cb[i] = LUA_NOREF;
}


// --- Helper functions -----------------------------------------------------


static void do_connect (const system_event_t *evt)
{
  (void)evt;
  esp_wifi_connect ();
}


// --- Lua API functions ----------------------------------------------------

static int wifi_sta_config (lua_State *L)
{
  luaL_checkanytable (L, 1);
  bool save = luaL_optbool (L, 2, DEFAULT_SAVE);
  lua_settop (L, 1);

  wifi_config_t cfg;
  memset (&cfg, 0, sizeof (cfg));

  lua_getfield (L, 1, "ssid");
  size_t len;
  const char *str = luaL_checklstring (L, -1, &len);
  if (len > sizeof (cfg.sta.ssid))
    len = sizeof (cfg.sta.ssid);
  strncpy ((char *)cfg.sta.ssid, str, len);

  lua_getfield (L, 1, "pwd");
  str = luaL_optlstring (L, -1, "", &len);
  if (len > sizeof (cfg.sta.password))
    len = sizeof (cfg.sta.password);
  strncpy ((char *)cfg.sta.password, str, len);

  lua_getfield (L, 1, "bssid");
  cfg.sta.bssid_set = false;
  if (lua_isstring (L, -1))
  {
    const char *bssid = luaL_checklstring (L, -1, &len);
    const char *fmts[] = {
      "%hhx%hhx%hhx%hhx%hhx%hhx",
      "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
      "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx",
      "%hhx %hhx %hhx %hhx %hhx %hhx",
      NULL
    };
    for (unsigned i = 0; fmts[i]; ++i)
    {
      if (sscanf (bssid, fmts[i],
        &cfg.sta.bssid[0], &cfg.sta.bssid[1], &cfg.sta.bssid[2],
        &cfg.sta.bssid[3], &cfg.sta.bssid[4], &cfg.sta.bssid[5]) == 6)
      {
        cfg.sta.bssid_set = true;
        break;
      }
    }
    if (!cfg.sta.bssid_set)
      return luaL_error (L, "invalid BSSID: %s", bssid);
  }

  lua_getfield (L, 1, "auto");
  bool auto_conn = luaL_optbool (L, -1, true);

  SET_SAVE_MODE(save);
  esp_err_t err = esp_wifi_set_config (WIFI_IF_STA, &cfg);
  if (err != ESP_OK)
    return luaL_error (L, "failed to set wifi config, code %d", err);

  if (auto_conn)
    err = esp_wifi_connect ();
  if (err != ESP_OK)
    return luaL_error (L, "failed to begin connect, code %d", err);

  err = esp_wifi_set_auto_connect (auto_conn);
  return (err == ESP_OK) ?
    0 : luaL_error (L, "failed to set wifi auto-connect, code %d", err);
}


static int wifi_sta_connect (lua_State *L)
{
  esp_err_t err = esp_wifi_connect ();
  return (err == ESP_OK) ? 0 : luaL_error (L, "connect failed, code %d", err);
}


static int wifi_sta_disconnect (lua_State *L)
{
  esp_err_t err = esp_wifi_disconnect ();
  return (err == ESP_OK) ? 0 : luaL_error(L, "disconnect failed, code %d", err);
}


static int wifi_sta_getconfig (lua_State *L)
{
  wifi_config_t cfg;
  esp_err_t err = esp_wifi_get_config (WIFI_IF_STA, &cfg);
  if (err != ESP_OK)
    return luaL_error (L, "failed to get config, code %d", err);

  lua_createtable (L, 0, 3);
  size_t ssid_len = strnlen ((char *)cfg.sta.ssid, sizeof (cfg.sta.ssid));
  lua_pushlstring (L, (char *)cfg.sta.ssid, ssid_len);
  lua_setfield (L, -2, "ssid");

  size_t pwd_len = strnlen ((char *)cfg.sta.password, sizeof (cfg.sta.password));
  lua_pushlstring (L, (char *)cfg.sta.password, pwd_len);
  lua_setfield (L, -2, "pwd");

  if (cfg.sta.bssid_set)
  {
    char bssid_str[MAC_STR_SZ];
    macstr (bssid_str, cfg.sta.bssid);
    lua_pushstring (L, bssid_str);
    lua_setfield (L, -2, "bssid");
  }

  bool auto_conn;
  err = esp_wifi_get_auto_connect (&auto_conn);
  if (err != ESP_OK)
    return luaL_error (L, "failed to get auto-connect, code %d", err);

  lua_pushboolean (L, auto_conn);
  lua_setfield (L, -2, "auto");

  return 1;
}

static int wifi_sta_getmac (lua_State *L)
{
  return wifi_getmac(WIFI_IF_STA, L);
}

static void on_scan_done (const system_event_t *evt)
{
  (void)evt;

  lua_State *L = lua_getstate ();
  lua_rawgeti (L, LUA_REGISTRYINDEX, scan_cb_ref);
  luaL_unref (L, LUA_REGISTRYINDEX, scan_cb_ref);
  scan_cb_ref = LUA_NOREF;
  int nargs = 1;
  if (!lua_isnoneornil (L, -1))
  {
    uint16_t num_ap = 0;
    esp_err_t err = esp_wifi_scan_get_ap_num (&num_ap);
    wifi_ap_record_t *aps = luaM_malloc (L, num_ap * sizeof (wifi_ap_record_t));
    if ((err == ESP_OK) && (aps) &&
        (err = esp_wifi_scan_get_ap_records (&num_ap, aps)) == ESP_OK)
    {
      lua_pushnil (L); // no error

      lua_createtable (L, num_ap, 0); // prepare array
      ++nargs;
      for (unsigned i = 0; i < num_ap; ++i)
      {
        lua_createtable (L, 0, 6); // prepare table for AP entry

        char bssid_str[MAC_STR_SZ];
        macstr (bssid_str, aps[i].bssid);
        lua_pushstring (L, bssid_str);
        lua_setfield (L, -2, "bssid");

        size_t ssid_len =
          strnlen ((const char *)aps[i].ssid, sizeof (aps[i].ssid));
        lua_pushlstring (L, (const char *)aps[i].ssid, ssid_len);
        lua_setfield (L, -2, "ssid");

        lua_pushinteger (L, aps[i].primary);
        lua_setfield (L, -2, "channel");

        lua_pushinteger (L, aps[i].rssi);
        lua_setfield (L, -2, "rssi");

        lua_pushinteger (L, aps[i].authmode);
        lua_setfield (L, -2, "auth");

        lua_pushstring (L, wifi_second_chan_names[aps[i].second]);
        lua_setfield (L, -2, "bandwidth");

        lua_rawseti (L, -2, i + 1); // add table to array
      }
    }
    else
      lua_pushfstring (L, "failure on scan done");
    luaM_free (L, aps);
    lua_call (L, nargs, 0);
  }
}


static int wifi_sta_on (lua_State *L)
{
  return wifi_on (L, events, ARRAY_LEN(events), event_cb);
}

static int wifi_sta_scan (lua_State *L)
{
  if (scan_cb_ref != LUA_NOREF)
    return luaL_error (L, "scan already in progress");

  luaL_checkanytable (L, 1);

  luaL_checkanyfunction (L, 2);
  lua_settop (L, 2);
  scan_cb_ref = luaL_ref (L, LUA_REGISTRYINDEX);

  wifi_scan_config_t scan_cfg;
  memset (&scan_cfg, 0, sizeof (scan_cfg));

  lua_getfield (L, 1, "ssid");
  scan_cfg.ssid = (uint8_t *)luaL_optstring (L, -1, NULL);

  lua_getfield (L, 1, "bssid");
  scan_cfg.bssid = (uint8_t *)luaL_optstring (L, -1, NULL);

  lua_getfield (L, 1, "channel");
  scan_cfg.channel = luaL_optint (L, -1, 0);

  lua_getfield (L, 1, "hidden");
  scan_cfg.show_hidden = luaL_optint (L, -1, 0);

  esp_err_t err = esp_wifi_scan_start (&scan_cfg, false);
  if (err != ESP_OK)
  {
    luaL_unref (L, LUA_REGISTRYINDEX, scan_cb_ref);
    scan_cb_ref = LUA_NOREF;
    return luaL_error (L, "failed to start scan, code %d", err);
  }
  else
    return 0;
}


const LUA_REG_TYPE wifi_sta_map[] = {
  { LSTRKEY( "config" ),      LFUNCVAL( wifi_sta_config )     },
  { LSTRKEY( "connect" ),     LFUNCVAL( wifi_sta_connect )    },
  { LSTRKEY( "disconnect" ),  LFUNCVAL( wifi_sta_disconnect ) },
  { LSTRKEY( "getconfig" ),   LFUNCVAL( wifi_sta_getconfig )  },
  { LSTRKEY( "getmac" ),      LFUNCVAL( wifi_sta_getmac )     },
  { LSTRKEY( "on" ),          LFUNCVAL( wifi_sta_on )         },
  { LSTRKEY( "scan" ),        LFUNCVAL( wifi_sta_scan )       },

  { LNILKEY, LNILVAL }
};


// Currently no auto-connect, so do that in response to events
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_START, do_connect);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_STA_DISCONNECTED, do_connect);
NODEMCU_ESP_EVENT(SYSTEM_EVENT_SCAN_DONE, on_scan_done);
