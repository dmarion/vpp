/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#ifndef included_test_test_h
#define included_test_test_h

#include <vppinfra/cpu.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#endif

typedef clib_error_t *(test_fn_t) (clib_error_t *);

struct test_perf_;
typedef void (test_perf_fn_t) (int fd, struct test_perf_ *tp);

typedef struct test_perf_
{
  u64 n_ops;
  char *op_name;
  char *name;
  test_perf_fn_t *fn;
} test_perf_t;

typedef struct test_registration_
{
  char *name;
  u8 multiarch : 1;
  test_fn_t *fn;
  test_perf_t *perf_tests;
  u32 n_perf_tests;
  struct test_registration_ *next;
} test_registration_t;

extern test_registration_t *test_registrations[CLIB_MARCH_TYPE_N_VARIANTS];

#define __clib_test_fn static __clib_noinline __clib_section (".test_wrapper")

#define REGISTER_TEST(x)                                                      \
  test_registration_t CLIB_MARCH_SFX (__test_##x);                            \
  static void __clib_constructor CLIB_MARCH_SFX (__test_registration_##x) (   \
    void)                                                                     \
  {                                                                           \
    test_registration_t *r = &CLIB_MARCH_SFX (__test_##x);                    \
    r->next = test_registrations[CLIB_MARCH_SFX (CLIB_MARCH_VARIANT_TYPE)];   \
    test_registrations[CLIB_MARCH_SFX (CLIB_MARCH_VARIANT_TYPE)] = r;         \
  }                                                                           \
  test_registration_t CLIB_MARCH_SFX (__test_##x)

#define TEST_PERF_TESTS(...)                                                  \
  (test_perf_t[])                                                             \
  {                                                                           \
    __VA_ARGS__, {}                                                           \
  }

static_always_inline void
perf_event_ioctl (int fd, u32 req)
{
#ifdef __x86_64__
  asm inline("syscall"
	     :
	     : "D"(fd), "S"(req), "a"(__NR_ioctl), "d"(PERF_IOC_FLAG_GROUP)
	     : "rcx", "r11" /* registers modified by kernel */);
#else
  ioctl (fd, req, PERF_IOC_FLAG_GROUP);
#endif
}

static_always_inline void
perf_event_reset (int fd)
{
  perf_event_ioctl (fd, PERF_EVENT_IOC_RESET);
}
static_always_inline void
perf_event_enable (int fd)
{
  perf_event_ioctl (fd, PERF_EVENT_IOC_ENABLE);
}
static_always_inline void
perf_event_disable (int fd)
{
  perf_event_ioctl (fd, PERF_EVENT_IOC_DISABLE);
}

#endif
