/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2023 Cisco Systems, Inc.
 */

#include <vlib/vlib.h>
#include <vlib/unix/unix.h>
#include <vlib/pci/pci.h>
#include <vnet/ethernet/ethernet.h>

#include <ena/ena.h>

u8 *
format_ena_device_name (u8 *s, va_list *args)
{
  vlib_main_t *vm = vlib_get_main ();
  u32 i = va_arg (*args, u32);
  ena_device_t *ad = ena_get_device (i);
  vlib_pci_addr_t *addr = vlib_pci_get_addr (vm, ad->pci_dev_handle);

  if (ad->name)
    return format (s, "%s", ad->name);

  s = format (s, "ena-%x/%x/%x/%x", addr->domain, addr->bus, addr->slot,
	      addr->function);
  return s;
}

u8 *
format_ena_device_flags (u8 *s, va_list *args)
{
  ena_device_t *ad = va_arg (*args, ena_device_t *);
  u8 *t = 0;

#define _(a, b, c)                                                            \
  if (ad->flags & (1 << a))                                                   \
    t = format (t, "%s%s", t ? " " : "", c);
  foreach_ena_device_flags
#undef _
    s = format (s, "%v", t);
  vec_free (t);
  return s;
}

u8 *
format_ena_device (u8 *s, va_list *args)
{
  u32 i = va_arg (*args, u32);
  ena_device_t *ad = ena_get_device (i);
  u32 indent = format_get_indent (s);
  u8 *a = 0;
  ena_rxq_t *rxq = vec_elt_at_index (ad->rxqs, 0);
  ena_txq_t *txq = vec_elt_at_index (ad->txqs, 0);

  s = format (s, "rx: queues %u, desc %u (min %u max %u)", ad->n_rx_queues,
	      rxq->size, 0, 0);
  s = format (s, "\n%Utx: queues %u, desc %u (min %u max %u)",
	      format_white_space, indent, ad->n_tx_queues, txq->size, 0, 0);
  s = format (s, "\n%Uflags: %U", format_white_space, indent,
	      format_ena_device_flags, ad);
  if (ad->error)
    s = format (s, "\n%Uerror %U", format_white_space, indent,
		format_clib_error, ad->error);

  if (a)
    s = format (s, "\n%Ustats:%v", format_white_space, indent, a);

  vec_free (a);
  return s;
}

u8 *
format_ena_input_trace (u8 *s, va_list *args)
{
  vlib_main_t *vm = va_arg (*args, vlib_main_t *);
  vlib_node_t *node = va_arg (*args, vlib_node_t *);
  ena_input_trace_t *t = va_arg (*args, ena_input_trace_t *);
  vnet_main_t *vnm = vnet_get_main ();
  vnet_hw_interface_t *hi = vnet_get_hw_interface (vnm, t->hw_if_index);

  s = format (s, "ena: %v (%d) qid %u next-node %U", hi->name, t->hw_if_index,
	      t->qid, format_vlib_next_node_name, vm, node->index,
	      t->next_index);

  return s;
}
