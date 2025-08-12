#include "pico_net/pico_net.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "pico_net/tcp_struct.h"

using namespace pico_net;

#if !defined(TEST_TCP_SERVER_IP)
#error TEST_TCP_SERVER_IP not defined
#endif

#define TCP_PORT 4242

#define TEST_ITERATIONS 10
#define POLL_TIME_S 5


void dump_bytes(void *arg, uint32_t len) {
    uint8_t* bptr = (uint8_t*) arg;
    unsigned int i = 0;

    printf("dump_bytes %d", len);
    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            printf("\n");
        } else if ((i & 0x07) == 0) {
            printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}

err_t pico_net::tcp_client_close(struct TCP_CLIENT_T* state) {
    err_t err = ERR_OK;
    if (state->tcp_pcb != NULL) {
        tcp_arg(state->tcp_pcb, NULL);
        tcp_poll(state->tcp_pcb, NULL, 0);
        tcp_sent(state->tcp_pcb, NULL);
        tcp_recv(state->tcp_pcb, NULL);
        tcp_err(state->tcp_pcb, NULL);
        err = tcp_close(state->tcp_pcb);
        if (err != ERR_OK) {
            printf("close failed %d, calling abort\n", err);
            tcp_abort(state->tcp_pcb);
            err = ERR_ABRT;
        }
        state->tcp_pcb = NULL;
    }
  return err;
}

// Called with results of operation
err_t pico_net::tcp_result(void* arg, int status) {
    TCP_CLIENT_T* state = (TCP_CLIENT_T*)arg;
    if (status == 0) {
        printf("test success\n");
    } 
    else {
        printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_client_close(state);
}

err_t pico_net::tcp_client_sent(void* arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_CLIENT_T* state = (TCP_CLIENT_T*)arg;
    printf("tcp_client_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {

        state->run_count++;
        if (state->run_count >= TEST_ITERATIONS) {
            tcp_result(arg, 0);
            return ERR_OK;
        }

        // We should receive a new buffer from the server
        state->buffer_len = 0;
        state->sent_len = 0;
        printf("Waiting for buffer from server\n");
    }
    return ERR_OK;
}

err_t pico_net::tcp_client_connected(void* arg, struct tcp_pcb *tpcb, err_t err) {
    TCP_CLIENT_T* state = (TCP_CLIENT_T*)arg;
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
        return tcp_result(arg, err);
    }
    state->connected = true;
    printf("Waiting for buffer from server\n");
    return ERR_OK;
}

err_t pico_net::tcp_client_poll(void* arg, struct tcp_pcb *tpcb) {
    TCP_CLIENT_T* state = (TCP_CLIENT_T*)arg;
    printf("tcp_client_poll\n");
    return tcp_result(state, -1); // no response is an error?
}

void pico_net::tcp_client_err(void* arg, err_t err) {
    if (err != ERR_ABRT) {
        printf("tcp_client_err %d\n", err);
        tcp_result(arg, err);
    }
}

err_t pico_net::tcp_client_recv(void* arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_CLIENT_T* state = (TCP_CLIENT_T*)arg;
    if (!p) {
        return tcp_result(state, -1);
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not
    // required, however you can use this method to cause an assertion in debug
    // mode, if this method is called when cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            dump_bytes(q->payload, q->len);
        }
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->buffer_len;
        state->buffer_len += pbuf_copy_partial(
            p, state->buffer + state->buffer_len,
            p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // If we have received the whole buffer, send it back to the server
    if (state->buffer_len == BUF_SIZE) {
        printf("Writing %d bytes to server\n", state->buffer_len);
        err_t err = tcp_write(tpcb, state->buffer, state->buffer_len, TCP_WRITE_FLAG_COPY);
        if (err != ERR_OK) {
            printf("Failed to write data %d\n", err);
            return tcp_result(state, -1);
        }
    }
    return ERR_OK;
}

bool pico_net::tcp_client_open(TCP_CLIENT_T* state) {
    printf("Connecting to %s port %u\n", ip4addr_ntoa(&state->remote_addr),
         TCP_PORT);
    state->tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(&state->remote_addr));
    if (!state->tcp_pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    tcp_arg(state->tcp_pcb, state);
    tcp_poll(state->tcp_pcb, tcp_client_poll, POLL_TIME_S * 2);
    tcp_sent(state->tcp_pcb, tcp_client_sent);
    tcp_recv(state->tcp_pcb, tcp_client_recv);
    tcp_err(state->tcp_pcb, tcp_client_err);

    state->buffer_len = 0;

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure
    // correct locking. You can omit them if you are in a callback from lwIP. Note
    // that when using pico_cyw_arch_poll these calls are a no-op and can be
    // omitted, but it is a good practice to use them in case you switch the
    // cyw43_arch type later.
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(state->tcp_pcb, &state->remote_addr, TCP_PORT,
                          tcp_client_connected);
    cyw43_arch_lwip_end();

    return err == ERR_OK;
}

// Perform initialisation
TCP_CLIENT_T* pico_net::tcp_client_init() {
    TCP_CLIENT_T *state = new TCP_CLIENT_T();
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    ip4addr_aton(TEST_TCP_SERVER_IP, &state->remote_addr);
    return state;
}

void pico_net::run_tcp_client_test(void) {
    TCP_CLIENT_T *state = tcp_client_init();
    if (!state) {
        return;
    }
    if (!tcp_client_open(state)) {
        tcp_result(state, -1);
        return;
    }
    while (!state->complete) {
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
    }
    free(state);
}
