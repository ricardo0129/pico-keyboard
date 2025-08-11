#define BUF_SIZE 2048

struct TCP_CLIENT_T {
  struct tcp_pcb *tcp_pcb;
  ip_addr_t remote_addr;
  uint8_t buffer[BUF_SIZE];
  int buffer_len;
  int sent_len;
  bool complete;
  int run_count;
  bool connected;
};
