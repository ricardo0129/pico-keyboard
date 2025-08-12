#pragma once

#include "lwip/pbuf.h"
#include "pico_net/tcp_struct.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

namespace pico_net {

    err_t tcp_client_close(struct TCP_CLIENT_T* state);

    err_t tcp_result(void* arg, int status);

    err_t tcp_client_sent(void* arg, struct tcp_pcb *tpcb, u16_t len);

    err_t tcp_client_connected(void* arg, struct tcp_pcb *tpcb, err_t err);

    err_t tcp_client_poll(void* arg, struct tcp_pcb *tpcb);

    void tcp_client_err(void* arg, err_t err);

    err_t tcp_client_recv(void* arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

    bool tcp_client_open(TCP_CLIENT_T* state);

    TCP_CLIENT_T* tcp_client_init();

    void run_tcp_client_test();
}
