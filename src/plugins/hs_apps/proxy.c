/*
* Copyright (c) 2017-2019 Cisco and/or its affiliates.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at:
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <vnet/vnet.h>
#include <vlibmemory/api.h>
#include <vnet/session/application.h>
#include <vnet/session/application_interface.h>
#include <hs_apps/proxy.h>
#include <vnet/tcp/tcp.h>

proxy_main_t proxy_main;

#define TCP_MSS 1460

typedef struct
{
  char uri[128];
  u32 app_index;
  u32 api_context;
} proxy_connect_args_t;

static void
proxy_cb_fn (void *data, u32 data_len)
{
  proxy_connect_args_t *pa = (proxy_connect_args_t *) data;
  vnet_connect_args_t a;

  memset (&a, 0, sizeof (a));
  a.api_context = pa->api_context;
  a.app_index = pa->app_index;
  a.uri = pa->uri;
  vnet_connect_uri (&a);
}

static void
proxy_call_main_thread (vnet_connect_args_t * a)
{
  if (vlib_get_thread_index () == 0)
    {
      vnet_connect_uri (a);
    }
  else
    {
      proxy_connect_args_t args;
      args.api_context = a->api_context;
      args.app_index = a->app_index;
      clib_memcpy (args.uri, a->uri, vec_len (a->uri));
      vl_api_rpc_call_main_thread (proxy_cb_fn, (u8 *) & args, sizeof (args));
    }
}

static void
delete_proxy_session (session_t * s, int is_active_open)
{
  proxy_main_t *pm = &proxy_main;
  proxy_session_t *ps = 0;
  vnet_disconnect_args_t _a, *a = &_a;
  session_t *active_open_session = 0;
  session_t *server_session = 0;
  uword *p;
  u64 handle;

  clib_spinlock_lock_if_init (&pm->sessions_lock);

  handle = session_handle (s);

  if (is_active_open)
    {
      p = hash_get (pm->proxy_session_by_active_open_handle, handle);
      if (p == 0)
	{
	  clib_warning ("proxy session for %s handle %lld (%llx) AWOL",
			is_active_open ? "active open" : "server",
			handle, handle);
	}
      else if (!pool_is_free_index (pm->sessions, p[0]))
	{
	  active_open_session = s;
	  ps = pool_elt_at_index (pm->sessions, p[0]);
	  if (ps->vpp_server_handle != ~0)
	    server_session = session_get_from_handle (ps->vpp_server_handle);
	}
    }
  else
    {
      p = hash_get (pm->proxy_session_by_server_handle, handle);
      if (p == 0)
	{
	  clib_warning ("proxy session for %s handle %lld (%llx) AWOL",
			is_active_open ? "active open" : "server",
			handle, handle);
	}
      else if (!pool_is_free_index (pm->sessions, p[0]))
	{
	  server_session = s;
	  ps = pool_elt_at_index (pm->sessions, p[0]);
	  if (ps->vpp_active_open_handle != ~0)
	    active_open_session = session_get_from_handle
	      (ps->vpp_active_open_handle);
	}
    }

  if (ps)
    {
      if (CLIB_DEBUG > 0)
	clib_memset (ps, 0xFE, sizeof (*ps));
      pool_put (pm->sessions, ps);
    }

  if (active_open_session)
    {
      a->handle = session_handle (active_open_session);
      a->app_index = pm->active_open_app_index;
      hash_unset (pm->proxy_session_by_active_open_handle,
		  session_handle (active_open_session));
      vnet_disconnect_session (a);
    }

  if (server_session)
    {
      a->handle = session_handle (server_session);
      a->app_index = pm->server_app_index;
      hash_unset (pm->proxy_session_by_server_handle,
		  session_handle (server_session));
      vnet_disconnect_session (a);
    }

  clib_spinlock_unlock_if_init (&pm->sessions_lock);
}

static int
proxy_accept_callback (session_t * s)
{
  proxy_main_t *pm = &proxy_main;

  s->session_state = SESSION_STATE_READY;

  clib_spinlock_lock_if_init (&pm->sessions_lock);

  return 0;
}

static void
proxy_disconnect_callback (session_t * s)
{
  delete_proxy_session (s, 0 /* is_active_open */ );
}

static void
proxy_reset_callback (session_t * s)
{
  clib_warning ("Reset session %U", format_session, s, 2);
  delete_proxy_session (s, 0 /* is_active_open */ );
}

static int
proxy_connected_callback (u32 app_index, u32 api_context,
			  session_t * s, u8 is_fail)
{
  clib_warning ("called...");
  return -1;
}

static int
proxy_add_segment_callback (u32 client_index, u64 segment_handle)
{
  clib_warning ("called...");
  return -1;
}

static int
proxy_rx_callback (session_t * s)
{
  u32 max_dequeue;
  int actual_transfer __attribute__ ((unused));
  svm_fifo_t *tx_fifo, *rx_fifo;
  proxy_main_t *pm = &proxy_main;
  u32 thread_index = vlib_get_thread_index ();
  vnet_connect_args_t _a, *a = &_a;
  proxy_session_t *ps;
  int proxy_index;
  uword *p;
  svm_fifo_t *ao_tx_fifo;

  ASSERT (s->thread_index == thread_index);

  clib_spinlock_lock_if_init (&pm->sessions_lock);
  p = hash_get (pm->proxy_session_by_server_handle, session_handle (s));

  if (PREDICT_TRUE (p != 0))
    {
      clib_spinlock_unlock_if_init (&pm->sessions_lock);
      ao_tx_fifo = s->rx_fifo;

      /*
       * Send event for active open tx fifo
       */
      if (svm_fifo_set_event (ao_tx_fifo))
	{
	  u32 ao_thread_index = ao_tx_fifo->master_thread_index;
	  u32 ao_session_index = ao_tx_fifo->master_session_index;
	  if (session_send_io_evt_to_thread_custom (&ao_session_index,
						    ao_thread_index,
						    SESSION_IO_EVT_TX))
	    clib_warning ("failed to enqueue tx evt");
	}

      if (svm_fifo_max_enqueue (ao_tx_fifo) <= TCP_MSS)
	svm_fifo_add_want_deq_ntf (ao_tx_fifo, SVM_FIFO_WANT_DEQ_NOTIF);
    }
  else
    {
      rx_fifo = s->rx_fifo;
      tx_fifo = s->tx_fifo;

      ASSERT (rx_fifo->master_thread_index == thread_index);
      ASSERT (tx_fifo->master_thread_index == thread_index);

      max_dequeue = svm_fifo_max_dequeue_cons (s->rx_fifo);

      if (PREDICT_FALSE (max_dequeue == 0))
	return 0;

      max_dequeue = clib_min (pm->rcv_buffer_size, max_dequeue);
      actual_transfer = svm_fifo_peek (rx_fifo, 0 /* relative_offset */ ,
				       max_dequeue, pm->rx_buf[thread_index]);

      /* $$$ your message in this space: parse url, etc. */

      clib_memset (a, 0, sizeof (*a));

      pool_get (pm->sessions, ps);
      clib_memset (ps, 0, sizeof (*ps));
      ps->server_rx_fifo = rx_fifo;
      ps->server_tx_fifo = tx_fifo;
      ps->vpp_server_handle = session_handle (s);

      proxy_index = ps - pm->sessions;

      hash_set (pm->proxy_session_by_server_handle, ps->vpp_server_handle,
		proxy_index);

      clib_spinlock_unlock_if_init (&pm->sessions_lock);

      a->uri = (char *) pm->client_uri;
      a->api_context = proxy_index;
      a->app_index = pm->active_open_app_index;
      proxy_call_main_thread (a);
    }

  return 0;
}

static int
proxy_tx_callback (session_t * proxy_s)
{
  proxy_main_t *pm = &proxy_main;
  transport_connection_t *tc;
  session_handle_t handle;
  proxy_session_t *ps;
  session_t *ao_s;
  u32 min_free;
  uword *p;

  min_free = clib_min (proxy_s->tx_fifo->nitems >> 3, 128 << 10);
  if (svm_fifo_max_enqueue (proxy_s->tx_fifo) < min_free)
    {
      svm_fifo_add_want_deq_ntf (proxy_s->tx_fifo, SVM_FIFO_WANT_DEQ_NOTIF);
      return 0;
    }

  clib_spinlock_lock_if_init (&pm->sessions_lock);

  handle = session_handle (proxy_s);
  p = hash_get (pm->proxy_session_by_server_handle, handle);
  if (!p)
    return 0;

  if (pool_is_free_index (pm->sessions, p[0]))
    return 0;

  ps = pool_elt_at_index (pm->sessions, p[0]);
  if (ps->vpp_active_open_handle == ~0)
    return 0;

  ao_s = session_get_from_handle (ps->vpp_active_open_handle);

  /* Force ack on active open side to update rcv wnd */
  tc = session_get_transport (ao_s);
  tcp_send_ack ((tcp_connection_t *) tc);

  clib_spinlock_unlock_if_init (&pm->sessions_lock);

  return 0;
}

static session_cb_vft_t proxy_session_cb_vft = {
  .session_accept_callback = proxy_accept_callback,
  .session_disconnect_callback = proxy_disconnect_callback,
  .session_connected_callback = proxy_connected_callback,
  .add_segment_callback = proxy_add_segment_callback,
  .builtin_app_rx_callback = proxy_rx_callback,
  .builtin_app_tx_callback = proxy_tx_callback,
  .session_reset_callback = proxy_reset_callback
};

static int
active_open_connected_callback (u32 app_index, u32 opaque,
				session_t * s, u8 is_fail)
{
  proxy_main_t *pm = &proxy_main;
  proxy_session_t *ps;
  u8 thread_index = vlib_get_thread_index ();

  if (is_fail)
    {
      clib_warning ("connection %d failed!", opaque);
      return 0;
    }

  /*
   * Setup proxy session handle.
   */
  clib_spinlock_lock_if_init (&pm->sessions_lock);

  ps = pool_elt_at_index (pm->sessions, opaque);
  ps->vpp_active_open_handle = session_handle (s);

  s->tx_fifo = ps->server_rx_fifo;
  s->rx_fifo = ps->server_tx_fifo;

  /*
   * Reset the active-open tx-fifo master indices so the active-open session
   * will receive data, etc.
   */
  s->tx_fifo->master_session_index = s->session_index;
  s->tx_fifo->master_thread_index = s->thread_index;

  /*
   * Account for the active-open session's use of the fifos
   * so they won't disappear until the last session which uses
   * them disappears
   */
  s->tx_fifo->refcnt++;
  s->rx_fifo->refcnt++;

  hash_set (pm->proxy_session_by_active_open_handle,
	    ps->vpp_active_open_handle, opaque);

  clib_spinlock_unlock_if_init (&pm->sessions_lock);

  /*
   * Send event for active open tx fifo
   */
  ASSERT (s->thread_index == thread_index);
  if (svm_fifo_set_event (s->tx_fifo))
    session_send_io_evt_to_thread (s->tx_fifo, SESSION_IO_EVT_TX);

  return 0;
}

static void
active_open_reset_callback (session_t * s)
{
  delete_proxy_session (s, 1 /* is_active_open */ );
}

static int
active_open_create_callback (session_t * s)
{
  return 0;
}

static void
active_open_disconnect_callback (session_t * s)
{
  delete_proxy_session (s, 1 /* is_active_open */ );
}

static int
active_open_rx_callback (session_t * s)
{
  svm_fifo_t *proxy_tx_fifo;

  proxy_tx_fifo = s->rx_fifo;

  /*
   * Send event for server tx fifo
   */
  if (svm_fifo_set_event (proxy_tx_fifo))
    {
      u8 thread_index = proxy_tx_fifo->master_thread_index;
      u32 session_index = proxy_tx_fifo->master_session_index;
      return session_send_io_evt_to_thread_custom (&session_index,
						   thread_index,
						   SESSION_IO_EVT_TX);
    }

  if (svm_fifo_max_enqueue (proxy_tx_fifo) <= TCP_MSS)
    svm_fifo_add_want_deq_ntf (proxy_tx_fifo, SVM_FIFO_WANT_DEQ_NOTIF);

  return 0;
}

static int
active_open_tx_callback (session_t * ao_s)
{
  proxy_main_t *pm = &proxy_main;
  transport_connection_t *tc;
  session_handle_t handle;
  proxy_session_t *ps;
  session_t *proxy_s;
  u32 min_free;
  uword *p;

  min_free = clib_min (ao_s->tx_fifo->nitems >> 3, 128 << 10);
  if (svm_fifo_max_enqueue (ao_s->tx_fifo) < min_free)
    {
      svm_fifo_add_want_deq_ntf (ao_s->tx_fifo, SVM_FIFO_WANT_DEQ_NOTIF);
      return 0;
    }

  clib_spinlock_lock_if_init (&pm->sessions_lock);

  handle = session_handle (ao_s);
  p = hash_get (pm->proxy_session_by_active_open_handle, handle);
  if (!p)
    return 0;

  if (pool_is_free_index (pm->sessions, p[0]))
    return 0;

  ps = pool_elt_at_index (pm->sessions, p[0]);
  if (ps->vpp_server_handle == ~0)
    return 0;

  proxy_s = session_get_from_handle (ps->vpp_server_handle);

  /* Force ack on proxy side to update rcv wnd */
  tc = session_get_transport (proxy_s);
  tcp_send_ack ((tcp_connection_t *) tc);

  clib_spinlock_unlock_if_init (&pm->sessions_lock);

  return 0;
}

/* *INDENT-OFF* */
static session_cb_vft_t active_open_clients = {
  .session_reset_callback = active_open_reset_callback,
  .session_connected_callback = active_open_connected_callback,
  .session_accept_callback = active_open_create_callback,
  .session_disconnect_callback = active_open_disconnect_callback,
  .builtin_app_rx_callback = active_open_rx_callback,
  .builtin_app_tx_callback = active_open_tx_callback,
};
/* *INDENT-ON* */

static int
proxy_server_attach ()
{
  proxy_main_t *pm = &proxy_main;
  u64 options[APP_OPTIONS_N_OPTIONS];
  vnet_app_attach_args_t _a, *a = &_a;
  u32 segment_size = 512 << 20;

  clib_memset (a, 0, sizeof (*a));
  clib_memset (options, 0, sizeof (options));

  if (pm->private_segment_size)
    segment_size = pm->private_segment_size;
  a->name = format (0, "proxy-server");
  a->api_client_index = pm->server_client_index;
  a->session_cb_vft = &proxy_session_cb_vft;
  a->options = options;
  a->options[APP_OPTIONS_SEGMENT_SIZE] = segment_size;
  a->options[APP_OPTIONS_RX_FIFO_SIZE] = pm->fifo_size;
  a->options[APP_OPTIONS_TX_FIFO_SIZE] = pm->fifo_size;
  a->options[APP_OPTIONS_PRIVATE_SEGMENT_COUNT] = pm->private_segment_count;
  a->options[APP_OPTIONS_PREALLOC_FIFO_PAIRS] =
    pm->prealloc_fifos ? pm->prealloc_fifos : 0;

  a->options[APP_OPTIONS_FLAGS] = APP_OPTIONS_FLAGS_IS_BUILTIN;

  if (vnet_application_attach (a))
    {
      clib_warning ("failed to attach server");
      return -1;
    }
  pm->server_app_index = a->app_index;

  vec_free (a->name);
  return 0;
}

static int
active_open_attach (void)
{
  proxy_main_t *pm = &proxy_main;
  vnet_app_attach_args_t _a, *a = &_a;
  u64 options[16];

  clib_memset (a, 0, sizeof (*a));
  clib_memset (options, 0, sizeof (options));

  a->api_client_index = pm->active_open_client_index;
  a->session_cb_vft = &active_open_clients;
  a->name = format (0, "proxy-active-open");

  options[APP_OPTIONS_ACCEPT_COOKIE] = 0x12345678;
  options[APP_OPTIONS_SEGMENT_SIZE] = 512 << 20;
  options[APP_OPTIONS_RX_FIFO_SIZE] = pm->fifo_size;
  options[APP_OPTIONS_TX_FIFO_SIZE] = pm->fifo_size;
  options[APP_OPTIONS_PRIVATE_SEGMENT_COUNT] = pm->private_segment_count;
  options[APP_OPTIONS_PREALLOC_FIFO_PAIRS] =
    pm->prealloc_fifos ? pm->prealloc_fifos : 0;

  options[APP_OPTIONS_FLAGS] = APP_OPTIONS_FLAGS_IS_BUILTIN
    | APP_OPTIONS_FLAGS_IS_PROXY;

  a->options = options;

  if (vnet_application_attach (a))
    return -1;

  pm->active_open_app_index = a->app_index;

  vec_free (a->name);

  return 0;
}

static int
proxy_server_listen ()
{
  proxy_main_t *pm = &proxy_main;
  vnet_listen_args_t _a, *a = &_a;
  clib_memset (a, 0, sizeof (*a));
  a->app_index = pm->server_app_index;
  a->uri = (char *) pm->server_uri;
  return vnet_bind_uri (a);
}

static int
proxy_server_create (vlib_main_t * vm)
{
  proxy_main_t *pm = &proxy_main;
  vlib_thread_main_t *vtm = vlib_get_thread_main ();
  u32 num_threads;
  int i;

  num_threads = 1 /* main thread */  + vtm->n_threads;
  vec_validate (proxy_main.server_event_queue, num_threads - 1);
  vec_validate (proxy_main.active_open_event_queue, num_threads - 1);
  vec_validate (pm->rx_buf, num_threads - 1);

  for (i = 0; i < num_threads; i++)
    vec_validate (pm->rx_buf[i], pm->rcv_buffer_size);

  if (proxy_server_attach ())
    {
      clib_warning ("failed to attach server app");
      return -1;
    }
  if (proxy_server_listen ())
    {
      clib_warning ("failed to start listening");
      return -1;
    }
  if (active_open_attach ())
    {
      clib_warning ("failed to attach active open app");
      return -1;
    }

  for (i = 0; i < num_threads; i++)
    {
      pm->active_open_event_queue[i] = session_main_get_vpp_event_queue (i);

      ASSERT (pm->active_open_event_queue[i]);

      pm->server_event_queue[i] = session_main_get_vpp_event_queue (i);
    }

  return 0;
}

static clib_error_t *
proxy_server_create_command_fn (vlib_main_t * vm, unformat_input_t * input,
				vlib_cli_command_t * cmd)
{
  proxy_main_t *pm = &proxy_main;
  char *default_server_uri = "tcp://0.0.0.0/23";
  char *default_client_uri = "tcp://6.0.2.2/23";
  int rv;
  u64 tmp;

  pm->fifo_size = 64 << 10;
  pm->rcv_buffer_size = 1024;
  pm->prealloc_fifos = 0;
  pm->private_segment_count = 0;
  pm->private_segment_size = 0;
  pm->server_uri = 0;
  pm->client_uri = 0;

  while (unformat_check_input (input) != UNFORMAT_END_OF_INPUT)
    {
      if (unformat (input, "fifo-size %d", &pm->fifo_size))
	pm->fifo_size <<= 10;
      else if (unformat (input, "rcv-buf-size %d", &pm->rcv_buffer_size))
	;
      else if (unformat (input, "prealloc-fifos %d", &pm->prealloc_fifos))
	;
      else if (unformat (input, "private-segment-count %d",
			 &pm->private_segment_count))
	;
      else if (unformat (input, "private-segment-size %U",
			 unformat_memory_size, &tmp))
	{
	  if (tmp >= 0x100000000ULL)
	    return clib_error_return
	      (0, "private segment size %lld (%llu) too large", tmp, tmp);
	  pm->private_segment_size = tmp;
	}
      else if (unformat (input, "server-uri %s", &pm->server_uri))
	vec_add1 (pm->server_uri, 0);
      else if (unformat (input, "client-uri %s", &pm->client_uri))
	vec_add1 (pm->client_uri, 0);
      else
	return clib_error_return (0, "unknown input `%U'",
				  format_unformat_error, input);
    }

  if (!pm->server_uri)
    {
      clib_warning ("No server-uri provided, Using default: %s",
		    default_server_uri);
      pm->server_uri = format (0, "%s%c", default_server_uri, 0);
    }
  if (!pm->client_uri)
    {
      clib_warning ("No client-uri provided, Using default: %s",
		    default_client_uri);
      pm->client_uri = format (0, "%s%c", default_client_uri, 0);
    }

  vnet_session_enable_disable (vm, 1 /* turn on session and transport */ );

  rv = proxy_server_create (vm);
  switch (rv)
    {
    case 0:
      break;
    default:
      return clib_error_return (0, "server_create returned %d", rv);
    }

  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (proxy_create_command, static) =
{
  .path = "test proxy server",
  .short_help = "test proxy server [server-uri <tcp://ip/port>]"
      "[client-uri <tcp://ip/port>][fifo-size <nn>][rcv-buf-size <nn>]"
      "[prealloc-fifos <nn>][private-segment-size <mem>]"
      "[private-segment-count <nn>]",
  .function = proxy_server_create_command_fn,
};
/* *INDENT-ON* */

clib_error_t *
proxy_main_init (vlib_main_t * vm)
{
  proxy_main_t *pm = &proxy_main;
  pm->server_client_index = ~0;
  pm->active_open_client_index = ~0;
  pm->proxy_session_by_active_open_handle = hash_create (0, sizeof (uword));
  pm->proxy_session_by_server_handle = hash_create (0, sizeof (uword));

  return 0;
}

VLIB_INIT_FUNCTION (proxy_main_init);

/*
* fd.io coding-style-patch-verification: ON
*
* Local Variables:
* eval: (c-set-style "gnu")
* End:
*/
