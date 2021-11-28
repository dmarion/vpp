/* SPDX-License-Identifier: Apache-2.0
 * Copyright(c) 2021 Cisco Systems, Inc.
 */

#include <vppinfra/format.h>
#include <vppinfra/vector/test/test.h>
#include <vppinfra/error.h>

test_registration_t *test_registrations[CLIB_MARCH_TYPE_N_VARIANTS] = {};

int
test_march_supported (clib_march_variant_type_t type)
{
#define _(s, n)                                                               \
  if (CLIB_MARCH_VARIANT_TYPE_##s == type)                                    \
    return clib_cpu_march_priority_##s ();
  foreach_march_variant
#undef _
    return 0;
}

clib_error_t *
test_func ()
{
  for (int i = 0; i < CLIB_MARCH_TYPE_N_VARIANTS; i++)
    {
      test_registration_t *r = test_registrations[i];

      if (r == 0 || test_march_supported (i) < 0)
	continue;

      fformat (stdout, "\nMultiarch Variant: %U\n", format_march_variant, i);
      fformat (stdout,
	       "-------------------------------------------------------\n");
      while (r)
	{
	  clib_error_t *err;
	  err = (r->fn) (0);
	  fformat (stdout, "%-50s %s\n", r->name, err ? "FAIL" : "PASS");
	  if (err)
	    {
	      clib_error_report (err);
	      fformat (stdout, "\n");
	    }

	  r = r->next;
	}
    }

  fformat (stdout, "\n");
  return 0;
}

#define TEST_PERF_MAX_EVENTS 7
typedef struct
{
  u64 config[TEST_PERF_MAX_EVENTS];
  u8 n_events;
  format_function_t *format_fn;
} test_perf_event_bundle_t;

static u8 *
format_test_perf_bundle_default (u8 *s, va_list *args)
{
  test_perf_event_bundle_t __clib_unused *b =
    va_arg (*args, test_perf_event_bundle_t *);
  test_perf_t *tp = va_arg (*args, test_perf_t *);
  u64 *data = va_arg (*args, u64 *);

  if (data)
    s = format (s, "%5.2f", (f64) data[1] / data[0]);
  else
    s = format (s, "%5s", "IPC");

  if (data)
    s = format (s, "%8.2f", (f64) data[0] / tp->n_ops);
  else
    s = format (s, "%8s", "Clks/Op");

  if (data)
    s = format (s, "%8.2f", (f64) data[1] / tp->n_ops);
  else
    s = format (s, "%8s", "Inst/Op");

  if (data)
    s = format (s, "%9.2f", (f64) data[2] / tp->n_ops);
  else
    s = format (s, "%9s", "Brnch/Op");

  if (data)
    s = format (s, "%10.2f", (f64) data[3] / tp->n_ops);
  else
    s = format (s, "%10s", "BrMiss/Op");
  return s;
}

test_perf_event_bundle_t perf_bundles[] = { {
  .config[0] = PERF_COUNT_HW_CPU_CYCLES,
  .config[1] = PERF_COUNT_HW_INSTRUCTIONS,
  .config[2] = PERF_COUNT_HW_BRANCH_INSTRUCTIONS,
  .config[3] = PERF_COUNT_HW_BRANCH_MISSES,
  .config[4] = PERF_COUNT_HW_CACHE_REFERENCES,
  .config[5] = PERF_COUNT_HW_CACHE_MISSES,
  .n_events = 6,
  .format_fn = format_test_perf_bundle_default,
} };

#ifdef __linux__
clib_error_t *
test_perf ()
{
  clib_error_t *err = 0;
  test_perf_event_bundle_t *b = perf_bundles;
  int group_fd = -1, fds[TEST_PERF_MAX_EVENTS];
  u64 count[TEST_PERF_MAX_EVENTS + 3] = {};
  struct perf_event_attr pe = {
    .size = sizeof (struct perf_event_attr),
    .type = PERF_TYPE_HARDWARE,
    .disabled = 1,
    .exclude_kernel = 1,
    .exclude_hv = 1,
    .pinned = 1,
    .exclusive = 1,
    .read_format = (PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED |
		    PERF_FORMAT_TOTAL_TIME_RUNNING),
  };

  for (int i = 0; i < TEST_PERF_MAX_EVENTS; i++)
    fds[i] = -1;

  for (int i = 0; i < b->n_events; i++)
    {
      pe.config = b->config[i];
      int fd = syscall (__NR_perf_event_open, &pe, /* pid */ 0, /* cpu */ -1,
			/* group_fd */ group_fd, /* flags */ 0);
      if (fd < 0)
	{
	  err = clib_error_return_unix (0, "perf_event_open");
	  goto done;
	}

      if (group_fd == -1)
	{
	  group_fd = fd;
	  pe.pinned = 0;
	  pe.exclusive = 0;
	}
      fds[i] = fd;
    }

  for (int i = 0; i < CLIB_MARCH_TYPE_N_VARIANTS; i++)
    {
      test_registration_t *r = test_registrations[i];

      if (r == 0 || test_march_supported (i) < 0)
	continue;

      fformat (stdout, "\nMultiarch Variant: %U\n", format_march_variant, i);
      fformat (stdout,
	       "-------------------------------------------------------\n");
      while (r)
	{
	  if (r->perf_tests)
	    {
	      test_perf_t *pt = r->perf_tests;
	      fformat (stdout, "%-22s%-12s%U\n", r->name, "OpType",
		       b->format_fn, b, pt, 0UL);
	      do
		{
		  u32 read_size = (b->n_events + 3) * sizeof (u64);
		  for (int i = 0; i < 5; i++)
		    {
		      perf_event_reset (group_fd);
		      pt->fn (group_fd, pt);
		      if ((read (group_fd, &count, read_size) != read_size))
			{
			  err = clib_error_return_unix (0, "read");
			  goto done;
			}
		      if (count[1] != count[2])
			clib_warning (
			  "perf counters were not running all the time."
#ifdef __x86_64__
			  "\nConsider turning NMI watchdog off ('sysctl -w "
			  "kernel.nmi_watchdog=0')."
#endif
			);
		      fformat (stdout, "  %-20s%-12s%U\n", pt->name,
			       pt->op_name ? pt->op_name : "", b->format_fn, b,
			       pt, count + 3);
		    }
		}
	      while ((++pt)->fn);
	    }
	  r = r->next;
	}
    }

done:
  for (int i = 0; i < TEST_PERF_MAX_EVENTS; i++)
    if (fds[i] != -1)
      close (fds[i]);
  return err;
}
#endif

int
main (int argc, char *argv[])
{
  unformat_input_t _i = {}, *i = &_i;
  clib_mem_init (0, 64ULL << 20);
  clib_error_t *err;
  int perf = 0;

  unformat_init_command_line (i, argv);

  while (unformat_check_input (i) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (i, "perf"))
	perf = 1;
      else
	{
	  clib_warning ("unknown input '%U'", format_unformat_error, i);
	  exit (1);
	}
    }

  if (perf)
    err = test_perf ();
  else
    err = test_func ();

  if (err)
    {
      clib_error_report (err);
      fformat (stderr, "\n");
      return 1;
    }
  return 0;
}
