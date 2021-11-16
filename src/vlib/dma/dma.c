/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#include <vlib/vlib.h>
#include <vlib/dma/dma.h>

vlib_dma_main_t vlib_dma_main;

VLIB_REGISTER_LOG_CLASS (dma_default_log, static) = {
  .class_name = "dma",
  .default_syslog_level = VLIB_LOG_LEVEL_DEBUG,
};

#define log_debug(fmt, ...)                                                   \
  vlib_log_debug (dma_default_log.class, fmt, __VA_ARGS__)

#define log_err(fmt, ...)                                                     \
  vlib_log_err (dma_default_log.class, fmt, __VA_ARGS__)

clib_error_t *
vlib_dma_register_engine (vlib_main_t *vm, vlib_dma_engine_t *et, char *fmt,
			  ...)
{
  vlib_dma_main_t *dm = &vlib_dma_main;
  vlib_dma_engine_t *e;
  va_list va;

  pool_get_zero (dm->engines, e);
  clib_memcpy (e, et, sizeof (vlib_dma_engine_t));
  va_start (va, fmt);
  e->name = va_format (e->name, fmt, &va);
  va_end (va);
  log_debug ("register '%v'", e->name);
  return 0;
}

u32
vlib_dma_configure (vlib_main_t *vm, vlib_dma_config_t *ct, char *fmt, ...)
{
  vlib_dma_main_t *dm = &vlib_dma_main;
  vlib_dma_config_t *c;
  va_list va;
  pool_get_zero (dm->configs, c);
  clib_memcpy (c, ct, sizeof (vlib_dma_config_t));
  va_start (va, fmt);
  c->name = va_format (c->name, fmt, &va);
  va_end (va);
  log_debug ("config '%s'", c->name);
  return 0;
}
