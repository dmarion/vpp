/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#include <vlib/vlib.h>
#include <vlib/dma/dma.h>
#include <vppinfra/format_table.h>

u8 *
format_dma_engine_caps (u8 *s, va_list *args)
{
  vlib_dma_caps_t caps = va_arg (*args, vlib_dma_caps_t);
  u8 *t = 0;

  if (0)
    ;
#define _(bit, u, str)                                                        \
  else if (caps & (1 << (bit))) t = format (t, t ? ", %s" : "%s", str);
  foreach_vlib_dma_caps
#undef _

    if (t) s = format (s, "%v", t);
  else s = format (s, "none");

  vec_free (t);
  return s;
}

static clib_error_t *
show_dma_engine_fn (vlib_main_t *vm, unformat_input_t *main_input,
		    vlib_cli_command_t *cmd)
{
  vlib_dma_main_t *dm = &vlib_dma_main;
  vlib_dma_engine_t *e;
  table_t table = {}, *t = &table;
  table_add_header_col (t, 4, "Name", "Numa", "MaxBatchSize", "Caps");
  int i = 0;

  pool_foreach (e, dm->engines)
    {
      int j = 0;
      table_format_cell (t, i, j++, "%v", e->name);
      table_format_cell (t, i, j++, "%u", 0);
      table_format_cell (t, i, j++, "%u", e->max_batch_size);
      table_format_cell (t, i, j++, "%U", format_dma_engine_caps, e->caps);
      i++;
    }

  t->default_body.align = TTAA_LEFT;
  t->default_header_col.align = TTAA_LEFT;
  t->default_header_col.fg_color = TTAC_YELLOW;
  t->default_header_col.flags = TTAF_FG_COLOR_SET;
  vlib_cli_output (vm, "%U", format_table, t);
  table_free (t);
  return 0;
}

VLIB_CLI_COMMAND (show_dma_engine_command, static) = {
  .path = "show dma engines",
  .short_help = "show dma engines",
  .function = show_dma_engine_fn,
};

static clib_error_t *
show_dma_configs_fn (vlib_main_t *vm, unformat_input_t *main_input,
		    vlib_cli_command_t *cmd)
{
  vlib_dma_main_t *dm = &vlib_dma_main;
  vlib_dma_config_t *c;
  table_t table = {}, *t = &table;
  table_add_header_col (t, 5, "ConfigName", "Engine", "Numa", "MaxBatchSize", "Caps");
  int i = 0;

  pool_foreach (c, dm->configs)
    {
      vlib_dma_engine_t *e = pool_elt_at_index (dm->engines, c->engine_index);
      int j = 0;
      table_format_cell (t, i, j++, "%v", c->name);
      table_format_cell (t, i, j++, "%v", e->name);
      table_format_cell (t, i, j++, "%u", 0);
      table_format_cell (t, i, j++, "%u", c->max_batch_size);
      table_format_cell (t, i, j++, "%U", format_dma_engine_caps, c->caps);
      i++;
    }

  t->default_body.align = TTAA_LEFT;
  t->default_header_col.align = TTAA_LEFT;
  t->default_header_col.fg_color = TTAC_YELLOW;
  t->default_header_col.flags = TTAF_FG_COLOR_SET;
  vlib_cli_output (vm, "%U", format_table, t);
  table_free (t);
  return 0;
}

VLIB_CLI_COMMAND (show_dma_configs_command, static) = {
  .path = "show dma configs",
  .short_help = "show dma configs",
  .function = show_dma_configs_fn,
};
