/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#include <vnet/vnet.h>

#include <vnet/plugin/plugin.h>
#include <vpp/app/version.h>
#include <vppinfra/linux/sysfs.h>
#include <vlib/dma/dma.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

VLIB_PLUGIN_REGISTER () = {
  .version = VPP_BUILD_VER,
  .description = "DSA",
};

typedef struct
{
  u32 max_batch_size;
} foo_context_req_t;

VLIB_REGISTER_LOG_CLASS (dsa_default_log, static) = {
  .class_name = "intel-dsa",
  .default_syslog_level = VLIB_LOG_LEVEL_DEBUG,
};

#define log_debug(wq, fmt, ...)                                               \
  vlib_log_debug (dsa_default_log.class, "wq%u.%u: " fmt, wq->device_id,      \
		  wq->wq_id, __VA_ARGS__)
#define log_warn(wq, fmt, ...)                                                \
  vlib_log_warn (dsa_default_log.class, "wq%u.%u: " fmt, wq->device_id,       \
		 wq->wq_id, __VA_ARGS__)
#define log_err(wq, fmt, ...)                                                 \
  vlib_log_err (dsa_default_log.class, "wq%u.%u: " fmt, wq->device_id,        \
		wq->wq_id, __VA_ARGS__)

typedef struct
{
  void *portal;
  int fd;
  u32 max_batches;
  u32 max_batch_size;
  u32 numa_node;
  u16 device_id;
  u16 wq_id;
} dsa_wq_t;

typedef struct
{
  dsa_wq_t *work_queues;
} dsa_main_t;

dsa_main_t dsa_main;

clib_error_t *
dsa_wq_open (vlib_main_t *vm, u16 device_id, u16 wq_id)
{
  dsa_main_t *dm = &dsa_main;
  vlib_dma_engine_t engine = {};
  clib_error_t *err = 0;
  dsa_wq_t *wq;
  u8 *s = 0;

  pool_get_zero (dm->work_queues, wq);
  wq->device_id = device_id;
  wq->wq_id = wq_id;
  wq->fd = -1;

  s = format (s, "/dev/dsa/wq%u.%u%c", device_id, wq_id, 0);

  if ((wq->fd = open ((char *) s, O_RDWR)) < 0)
    {
      err = clib_error_return_unix (0, "open");
      goto done;
    }

  log_debug (wq, "%s opened as fd %d", s, wq->fd);

  wq->portal = mmap (0, 4096, PROT_WRITE, MAP_SHARED, wq->fd, 0);

  if (wq->portal == MAP_FAILED)
    {
      err = clib_error_return_unix (0, "mmap");
      goto done;
    }

  vec_reset_length (s);
  s = format (s, "/sys/bus/dsa/devices/wq%u.%u/size%c", device_id, wq_id, 0);
  err = clib_sysfs_read ((char *) s, "%u", &wq->max_batches);
  vec_reset_length (s);
  s = format (s, "/sys/bus/dsa/devices/wq%u.%u/max_batch_size%c", device_id,
	      wq_id, 0);
  err = clib_sysfs_read ((char *) s, "%u", &wq->max_batch_size);
  vec_reset_length (s);
  s = format (s, "/sys/bus/dsa/devices/dsa%u/numa_node%c", device_id, 0);
  err = clib_sysfs_read ((char *) s, "%u", &wq->numa_node);

  if (err)
    goto done;

  log_debug (wq, "numa_node %u, max_batches %u max_batch_size %u",
	     wq->numa_node, wq->max_batches, wq->max_batch_size);

  engine.caps = VLIB_DMA_CAPS_FORCE_LAST;
  engine.max_batch_size = wq->max_batch_size;
  vlib_dma_register_engine (vm, &engine, "Intel DSA wq%u.%u", device_id,
			    wq_id);

done:
  if (err)
    {
      log_err (wq, "%U", format_clib_error, err);
      if (wq->portal != MAP_FAILED)
	munmap (wq->portal, 4096);
      if (wq->fd > -1)
	close (wq->fd);
      pool_put (dm->work_queues, wq);
    }
  vec_free (s);
  return err;
}

static clib_error_t *
dsa_init (vlib_main_t *vm)
{
  clib_error_t *err = 0;
  err = dsa_wq_open (vm, 0, 0);
  u32 config_index;

  if (err)
    return err;

  vlib_dma_config_t cfg = {};
  cfg.max_batch_size = 256;
  config_index = vlib_dma_configure (vm, &cfg, "test");

  if (config_index == ~0)
    err = clib_error_return (0, "no suitable DMA engine found");

  return err;
}

VLIB_INIT_FUNCTION (dsa_init);
