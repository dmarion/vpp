/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2023 Cisco Systems, Inc.
 */

#include "vlib/pci/pci.h"
#include "vppinfra/error.h"
#include <vnet/vnet.h>
#include <vnet/dev/dev.h>
#include <vnet/ethernet/ethernet.h>

u8 *
format_vnet_dev_rv (u8 *s, va_list *args)
{
  vnet_dev_rv_t rv = va_arg (*args, vnet_dev_rv_t);
  u32 index = -rv;

  char *strings[] = { [0] = "OK",
#define _(v, n, d) [v] = #d,
		      foreach_vnet_dev_rv_type
#undef _
  };

  if (index >= ARRAY_LEN (strings))
    return format (s, "unknown error (%d)", rv);
  return format (s, "%s", strings[index]);
}

u8 *
format_vnet_dev_addr (u8 *s, va_list *args)
{
  vnet_dev_main_t *dm = &vnet_dev_main;
  vnet_dev_t *dev = va_arg (*args, vnet_dev_t *);
  vnet_dev_bus_t *bus;

  if (dev == 0)
    return 0;

  bus = pool_elt_at_index (dm->buses, dev->bus_index);
  s = format (s, "%U", bus->ops.format_device_addr, dev);

  return s;
}

u8 *
format_vnet_dev_interface_name (u8 *s, va_list *args)
{
  vnet_dev_main_t *dm = &vnet_dev_main;
  u32 i = va_arg (*args, u32);
  vnet_dev_if_t *intf = pool_elt_at_index (dm->interfaces, i);

  if (intf->name[0])
    return format (s, "%s", intf->name);

  s = format (s, "%s%u/%u", dm->drivers[intf->driver_index].registration->name,
	      intf->dev_index, intf->port_index);
  return s;
}

u8 *
format_vnet_dev_interface_info (u8 *s, va_list *args)
{
  vlib_main_t *vm = vlib_get_main ();
  vnet_dev_main_t *dm = &vnet_dev_main;
  u32 i = va_arg (*args, u32);
  vnet_dev_if_t *intf = pool_elt_at_index (dm->interfaces, i);
  vnet_dev_driver_t *dr = pool_elt_at_index (dm->drivers, intf->driver_index);
  vnet_dev_t *dev = pool_elt_at_index (dm->devices, intf->dev_index)[0];
  vnet_dev_bus_t *bus = pool_elt_at_index (dm->buses, dev->bus_index);
  vnet_dev_port_t *port = pool_elt_at_index (dev->ports, intf->port_index)[0];
  vnet_dev_rx_queue_t *rxq;
  vnet_dev_tx_queue_t *txq;
  u32 indent = format_get_indent (s);

  s = format (s, "Device:");
  s = format (s, "\n%UDriver is '%s'", format_white_space, indent + 2,
	      dr->registration->name);

  if (dev->description)
    s = format (s, ", description '%v'", dev->description);

  if (bus->ops.format_device_info)
    s = format (s, "\n%U%U", format_white_space, indent + 2,
		bus->ops.format_device_info, dev);

  s = format (s, "\n%UProcess node is '%U'", format_white_space, indent + 2,
	      format_vlib_node_name, vm, dev->process_node_index);

  s = format (s, "\n%UPort %u:", format_white_space, indent, port->port_id);
  s = format (s, "\n%UHardware Address is %U", format_white_space, indent + 2,
	      format_ethernet_address, port->config.hw_addr);
  s = format (s, ", %u RX queues (max %u), %u TX queues (max %u)",
	      pool_elts (port->rx_queues), port->config.max_rx_queues,
	      pool_elts (port->tx_queues), port->config.max_tx_queues);
  s = format (s, "\n%UMax frame size is %u", format_white_space, indent + 2,
	      port->config.max_frame_size);
  s = format (s, "\n%URX node is '%U'", format_white_space, indent + 2,
	      format_vlib_node_name, vm, port->rx_node_index);
  if (port->ops.format_status)
    s = format (s, "\n%U%U", format_white_space, indent + 2,
		port->ops.format_status, port);

  pool_foreach_index (i, port->rx_queues)
    {
      rxq = pool_elt_at_index (port->rx_queues, i)[0];
      s = format (s, "\n%URX queue %u:", format_white_space, indent + 2,
		  rxq->queue_id);
      s = format (s, "\n%USize is %u, buffer pool index is %u",
		  format_white_space, indent + 4, rxq->size,
		  rxq->buffer_pool_index);
      s = format (s, "\n%UPolling thread is %u, %sassigned",
		  format_white_space, indent + 4, rxq->rx_thread_index,
		  rxq->rx_thread_assigned ? "" : "not-");
    }

  pool_foreach_index (i, port->tx_queues)
    {
      txq = pool_elt_at_index (port->tx_queues, i)[0];
      s = format (s, "\n%UTX queue %u:", format_white_space, indent + 2,
		  txq->queue_id);
      s = format (s, "\n%USize is %u", format_white_space, indent + 4,
		  txq->size);
    }
  return s;
}