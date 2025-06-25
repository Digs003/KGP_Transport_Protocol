# Reliable Flow Control on top of UDP

This project implements a reliable, flow-controlled communication protocol over the inherently unreliable UDP protocol. It uses user-space socket emulation, shared memory, and custom protocol headers for in-order, reliable message delivery.

---

## 📊 Packet Loss Probability vs Average Number of Transmissions

| Packet Loss Probability (P) | No. of Transmissions | No. of Packets | Avg. No. of Transmissions |
| --------------------------- | -------------------- | -------------- | ------------------------- |
| 0.05                        | 471                  | 401            | 1.1646                    |
| 0.10                        | 536                  | 401            | 1.3366                    |
| 0.15                        | 610                  | 401            | 1.5212                    |
| 0.20                        | 642                  | 401            | 1.6001                    |
| 0.25                        | 761                  | 401            | 1.8977                    |
| 0.30                        | 820                  | 401            | 2.0445                    |
| 0.35                        | 861                  | 401            | 2.1471                    |
| 0.40                        | 1012                 | 401            | 2.5237                    |
| 0.45                        | 1094                 | 401            | 2.7282                    |
| 0.50                        | 1231                 | 401            | 3.0698                    |

---

## 🧱 Data Structures

### 1. `NET_SOCKET`

- `int sock_id`: UDP socket ID
- `char ip_addr[INET_ADDRSTRLEN]`: IP address
- `uint16_t port`: Port number
- `int err_code`: Global error code

### 2. `window`

- `int size`: Window size
- `int start`: Start sequence number
- `int wnd[256]`:
  - **Send window (swnd)**: `i` is index for message sent but not acknowledged; `-1` otherwise
  - **Receive window (rwnd)**: `i` is expected sequence; `-1` otherwise

### 3. `SHARED_MEM`

- Connection metadata and buffer management:
  - `int free`, `pid_t pid`, `int udp_sockid`, `char ip_addr[]`, `uint16_t port`
- Buffers:
  - `char send_buffer[10][512]`, `int sendbufferlen[10]`
  - `time_t last_send_time[256]`
  - `char recv_buffer[10][512]`, `int recv_buffer_live[10]`, `int rcvbufferlen[10]`, `int recv_buffer_base`
- Windows:
  - `window swnd`, `window rwnd`
- `int full`: Indicates receive buffer full

---

## 🔧 Library Functions

### `int k_socket(int domain, int type, int protocol)`

Creates a new KTP socket by finding a free shared memory slot.

### `int k_bind(char src_ip[], uint16_t src_port, char dest_ip[], uint16_t dest_port)`

Binds the KTP socket to source/destination IP and port.

### `ssize_t k_sendto(...)`

Checks and fills send buffer; updates send window. Returns message length or -1.

### `ssize_t k_recvfrom(...)`

Fetches a message from the receive buffer if available; otherwise returns error.

### `int k_close(int sockfd)`

Marks shared memory as free. Returns 0.

### `int drop_Message()`

Simulates packet loss using a random probability.

---

## ⚙️ initksocket Functions

### `main()`

Initializes shared memory and semaphores; starts `create_and_bind()`.

### `create_and_bind()`

Maintains at least N active UDP sockets and binds them.

### `void* R()` (Receiver Thread)

- Waits on `select()` with timeout `T`
- On timeout or data reception:
  - Sends ACKs
  - Updates sequence numbers
  - Handles reordering and buffer updates

### `void* S()` (Sender Thread)

- Wakes every `T/2` seconds
- Resends timed-out packets
- Sends new buffered messages

### `handler()`

Performs cleanup on `Ctrl+C`.

---

## 📦 Packet Structure

### 📨 Acknowledgment Packet (13 bytes)

| Bytes | Description                             |
| ----- | --------------------------------------- |
| 0     | Type (0 = ACK)                          |
| 1–8   | Sequence number of last received packet |
| 9–12  | Receive window size                     |

### 📤 Data Packet (531 bytes)

| Bytes  | Description               |
| ------ | ------------------------- |
| 0      | Type (1 = DATA)           |
| 1–8    | Sequence number of packet |
| 9–18   | Length of the message     |
| 19–530 | Message contents          |

---

## 📝 Notes

- Custom sockets operate via shared memory and semaphores.
- Provides reliability, in-order delivery, and flow control using custom protocol headers.
- Packet loss is simulated to test retransmission and ACK mechanisms.

---

## 📎 License

This project is for educational purposes and simulates TCP-like reliability over UDP for understanding transport-layer protocols.
