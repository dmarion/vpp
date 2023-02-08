/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2023 Cisco Systems, Inc.
 */

#ifndef _ENA_DEFS_H_
#define _ENA_DEFS_H_

#define foreach_ena_regs                                                      \
  _ (VERSION, 0x0)                                                            \
  _ (CONTROLLER_VERSION, 0x4)                                                 \
  _ (CAPS, 0x8)                                                               \
  _ (CAPS_EXT, 0xc)                                                           \
  _ (AQ_BASE_LO, 0x10)                                                        \
  _ (AQ_BASE_HI, 0x14)                                                        \
  _ (AQ_CAPS, 0x18)                                                           \
  _ (ACQ_BASE_LO, 0x20)                                                       \
  _ (ACQ_BASE_HI, 0x24)                                                       \
  _ (ACQ_CAPS, 0x28)                                                          \
  _ (AQ_DB, 0x2c)                                                             \
  _ (ACQ_TAIL, 0x30)                                                          \
  _ (AENQ_CAPS, 0x34)                                                         \
  _ (AENQ_BASE_LO, 0x38)                                                      \
  _ (AENQ_BASE_HI, 0x3c)                                                      \
  _ (AENQ_HEAD_DB, 0x40)                                                      \
  _ (AENQ_TAIL, 0x44)                                                         \
  _ (INTR_MASK, 0x4c)                                                         \
  _ (DEV_CTL, 0x54)                                                           \
  _ (DEV_STS, 0x58)                                                           \
  _ (MMIO_REG_READ, 0x5c)                                                     \
  _ (MMIO_RESP_LO, 0x60)                                                      \
  _ (MMIO_RESP_HI, 0x64)                                                      \
  _ (RSS_IND_ENTRY_UPDATE, 0x68)

typedef enum
{
#define _(n, v) ENA_REGS_##n##_OFF = (v),
  foreach_ena_regs
#undef _
} ena_regs_t;

#define foreach_ena_admin_aq_opcode                                           \
  _ (CREATE_SQ, 1)                                                            \
  _ (DESTROY_SQ, 2)                                                           \
  _ (CREATE_CQ, 3)                                                            \
  _ (DESTROY_CQ, 4)                                                           \
  _ (GET_FEATURE, 8)                                                          \
  _ (SET_FEATURE, 9)                                                          \
  _ (GET_STATS, 11)

typedef enum
{
#define _(n, v) ENA_ADMIN_##n = (v)
  foreach_ena_admin_aq_opcode
#undef _
} ena_admin_aq_opcode_t;

#endif /* ENA_H */
