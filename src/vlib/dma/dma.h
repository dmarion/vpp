/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#ifndef __included_vlib_dma_dma_h
#define __included_vlib_dma_dma_h
#include <vlib/vlib.h>

#define foreach_vlib_dma_caps _ (0, FORCE_LAST, "force-last")

typedef enum
{
#define _(bit, u, s) VLIB_DMA_CAPS_##u = (1 << bit),
  foreach_vlib_dma_caps
#undef _
} vlib_dma_caps_t;

typedef struct
{
  vlib_dma_caps_t caps;
  u32 max_batch_size;
} vlib_dma_template_t;

typedef u32 (vlib_dma_request_template_fn_t) (vlib_main_t *vm,
					      vlib_dma_template_t *t);

typedef struct
{
  u8 *name;
  vlib_dma_caps_t caps;
  u32 max_batch_size;
  vlib_dma_request_template_fn_t *req_template_fn;
} vlib_dma_engine_t;

clib_error_t *vlib_dma_register_engine (vlib_main_t *vm,
					vlib_dma_engine_t *reg, char *fmt,
					...);

typedef struct
{
  u8 *name;
  u32 max_batch_size;
  u32 engine_index;
  vlib_dma_caps_t caps;
} vlib_dma_config_t;

u32 vlib_dma_configure (vlib_main_t *vm, vlib_dma_config_t *cfg, char *fmt,
			...);

typedef struct
{
  vlib_dma_engine_t *engines;
  vlib_dma_config_t *configs;
} vlib_dma_main_t;

extern vlib_dma_main_t vlib_dma_main;

#endif
