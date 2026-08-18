# Project presentation - `ft_ping`

This README is organized as follows:
- [Description](#description)
  - [The OSI model: a common language for networked systems](#the-osi-model-a-common-language-for-networked-systems)
    - [Going further: what about ARP?](#going-further-what-about-arp)
- [System Environment](#system-environment)
- [Repo Layout](#repo-layout)
- [How `ft_ping` works](#how-ft_ping-works)
  - [RAW sockets](#raw-sockets)
  - [ICMP packet description](#icmp-packet-description)
    - [Packet send](#packet-send)
    - [Packet received](#packet-received)
    - [Error messages (`-v`)](#error-messages--v)
      - [ICMP header dump](#icmp-header-dump)
  - [TTL (Time To Live)](#ttl-time-to-live)
    - [Values](#values)
  - [RTT and statistics](#rtt-and-statistics)
  - [Byte order and conversion](#byte-order-and-conversion)
  - [ping's source code from inetutils-2.0](#pings-source-code-from-inetutils-20)
- [`ft_ping` usage](#ft_ping-usage)
  - [Allowed arguments](#allowed-arguments)
    - [On `-w` vs `-W`](#on--w-vs--w)
    - [`Ctrl+C` handling](#ctrlc-handling)
- [Simulating network conditions for testing](#simulating-network-conditions-for-testing)
  - [Simulating `Destination Host Unreachable`](#simulating-destination-host-unreachable)
- [How to Use `ft_ping`](#how-to-use-ft_ping)

---

# Description

`ft_ping` is a C project that consists of reimplementing the `ping` program
from `inetutils-2.0`. The main purpose of `ping` is to check whether a
machine is reachable over a network, and to measure how long it takes to
get a response.

`ping` is mainly used to:
- check that a destination is reachable
- check that it responds to ICMP requests
- measure the RTT (Round Trip Time: the time between sending an Echo
  Request and receiving the matching Echo Reply)
- observe packet loss
- retrieve information such as the TTL from the response

```sh
ping 8.8.8.8 # IP address of Google DNS

        ICMP Echo Request
   ┌─────────────────────────►
   │                         │
   │                      8.8.8.8
   │                         │
 PC│                         │
   │                      responds
   │                         │
   ◄─────────────────────────┘
        ICMP Echo Reply
```

## The OSI model: a common language for networked systems

The OSI *(Open Systems Interconnection)* model represents the different
functions required for two machines to communicate over a network. Each
layer relies on the one below it and offers a service to the one above it.

```
┌──────────────────────────────┐
│ 7. Application               │ HTTP, DNS, SSH, ping
├──────────────────────────────┤
│ 6. Presentation              │
├──────────────────────────────┤
│ 5. Session                   │
├──────────────────────────────┤
│ 4. Transport                 │ TCP, UDP
├──────────────────────────────┤
│ 3. Network                   │ IP, ICMP, IPv4
├──────────────────────────────┤
│ 2. Data Link                 │ Ethernet, ARP...
├──────────────────────────────┤
│ 1. Physical                  │ cable, fiber, radio...
└──────────────────────────────┘
```

`ping` mainly relies on the **ICMP** protocol, which lives at the Network
layer, alongside IP itself:

- **ICMP** *(Internet Control Message Protocol)*: used to send control and
  error messages at the IP level ("host unreachable", "TTL expired", Echo Request/Reply).
- **IPv4** *(Internet Protocol version 4)*: handles addressing and routing
  of packets across the network.

An IPv4 packet carrying an ICMP message looks like this:

```
┌──────────────────────┐
│ IPv4 Header          │
├──────────────────────┤
│ ICMP Header          │ ─┐
├──────────────────────┤  ├─ ICMP message
│ ICMP Payload         │ ─┘ (carried inside the IPv4 payload)
└──────────────────────┘
```

The IPv4 header handles addressing and routing (source/destination IP,
TTL, protocol number...), while everything below it (the ICMP header and
payload) is the actual message being transported: type of message
(Echo Request/Reply, Time Exceeded...), sequence number, and optional data.

### Going further: what about ARP?

ARP is not implemented in this project, but it's worth
understanding why `ft_ping`'s packets can leave the machine at all.

**ARP** *(Address Resolution Protocol)* operates one layer below IP, at the
Data Link layer. Before an IP packet can physically travel over a local
network (Ethernet/WiFi), the sending machine needs to know the destination's
**MAC address** (its physical network hardware address); IP addresses
alone mean nothing at that layer. ARP is the protocol that answers the
question *"who has this IP address? what is your MAC address?"* by
broadcasting a request on the local network.

Since `ft_ping` uses a `SOCK_RAW` socket at the IP layer, the kernel
handles ARP resolution and Ethernet framing underneath, completely
transparently. You never interact with ARP directly; it's simply why
raw IP packets are able to actually leave the machine on the wire.

---

# System Environment

VM description:
- **Host**: Debian Bullseye, built with QEMU
- **Reference `ping`**: `inetutils-2.0` (`ping -V`)

```sh
➜  ft_ping git:(main) ✗ ping -V
ping (GNU inetutils) 2.0
Copyright (C) 2021 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Written by Sergey Poznyakoff.
```

`ft_ping` needs to be run with `sudo` because creating a RAW socket
requires the `CAP_NET_RAW` capability, reserved to root by default (see
[RAW sockets](#raw-sockets) for more details).

---

# Repo Layout

```sh
.
├── main.c
├── Makefile
├── README.md
├── include
│   └── ft_ping.h
└── src
    ├── loop.c
    ├── parse_opts.c
    ├── print_stats.c
    ├── receive.c
    ├── resolve_dest.c
    └── send.c
```

---

# How `ft_ping` works

## RAW sockets

A RAW socket is a type of socket that allows direct interaction with the
network layer, bypassing the kernel's usual TCP/UDP handling. It lets a
program craft and read custom packets (such as ICMP ones) instead of
relying on a higher-level transport protocol.

`ping` uses a RAW socket:
```sh
socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

# AF_INET
#    │
#    └──► IPv4

# IPPROTO_ICMP
#    │
#    └──► ICMP protocol
```

Creating a `SOCK_RAW` socket requires elevated privileges: the kernel
restricts it to processes holding the `CAP_NET_RAW` capability (root, or
a binary granted this capability via `setcap cap_net_raw+ep`). This is
why `ft_ping` needs to be run with `sudo` — crafting arbitrary IP-level
packets is a capability the kernel doesn't hand out by default.

Once the packet is sent, the kernel wraps the ICMP message inside an IPv4
packet before it goes out on the wire. This is also what the receiving
side gets back: reading from a RAW ICMP socket returns the **entire IPv4 packet**, not just the ICMP part. This gives access to:
- the TTL
- the source IP
- the destination IP
- the protocol
- the ICMP header
- the identifier
- the sequence number
- the payload

Schematically:
```
    Application
        │
        │ RAW ICMP socket
        ▼
┌───────────────┐
│ Linux Kernel │
└───────┬───────┘
        │
        ▼
    IPv4 + ICMP
```

## ICMP packet description

`ping` mainly uses two ICMP messages: Echo Request and Echo Reply.

|ICMP message | Type |Code |
|---|---|---|
|ICMP Echo Request|8| 0|
|ICMP Echo Reply| 0| 0|

ICMP packet layout:
```
byte  0               1               2               3
bits  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |     Type      |     Code      |           Checksum            |   
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |---- ICMP header
     |           Identifier          |        Sequence Number        |   
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |                          Data / Payload                       |  
```


**Checksum**: detects corruption of the ICMP message. It is computed over
the **entire ICMP message — header and payload together**, not just the
8-byte header. This matters both when building a packet and when
verifying one: using the wrong length on either side produces a checksum
mismatch even though the packet is perfectly valid.

**Identifier**: identifies the requests belonging to one instance of
`ping` (typically the process PID).

**Sequence Number**: identifies each individual packet. It's what allows
matching a given reply to the request it responds to — especially
important since replies are not guaranteed to arrive in order or on time.

Workflow:
```
                ft_ping
                    │
                    │ 1. build ICMP Echo Request
                    ▼
          ┌───────────────────┐
          │ ICMP Echo Request │
          │ type = 8          │
          │ seq = 0           │
          |───────────────────|
          | payload           |
          └─────────┬─────────┘
                    │
                    ▼
          ┌───────────────────┐
          │    IPv4 Header    │    <--- kernel wraps the ICMP echo request in an IPv4 header
          ├───────────────────┤
          │ ICMP Echo Request │
          └─────────┬─────────┘
                    │
                    │ network
                    ▼
              Destination
                    │
                    │ replies
                    ▼
          ┌───────────────────┐
          │    IPv4 Header    │
          ├───────────────────┤
          │  ICMP Echo Reply  │
          └─────────┬─────────┘
                    │
                    ▼
                 ft_ping
                    │
                    ├── checks the source address
                    ├── checks the ID
                    ├── verifies the checksum
                    ├── retrieves the TTL
                    └── computes the RTT
```

### Packet send

Structure of an ICMP header:
```h
/* from #include <netinet/ip_icmp.h> */
struct icmphdr
{
  uint8_t type;		/* message type */
  uint8_t code;		/* type sub-code */
  uint16_t checksum;
  union
  {
    struct
    {
      uint16_t	id;
      uint16_t	sequence;
    } echo;			/* echo datagram */
    uint32_t	gateway;	/* gateway address */
    struct
    {
      uint16_t	__glibc_reserved;
      uint16_t	mtu;
    } frag;			/* path mtu discovery */
  } un;
};
```

Structure of a packet:
```c
/* from send.c */

struct ping_packet
{
    struct icmphdr icmp_hdr;
    char payload[PAYLOAD_SIZE];
};
```

Build the packet:
```c
/* from send.c */
struct ping_packet packet = {
        .icmp_hdr = {
            .type = ICMP_ECHO,
            .code = 0,
            .checksum = 0,
            .un = {.echo = {.id = htons(ping->pid), .sequence = htons(ping->sequence_number)}}},
        .payload = {0}};

gettimeofday(start_time, NULL);
memmove((char *)&packet + sizeof(struct icmphdr), start_time, sizeof(*start_time)); // Copy the start time into the payload
packet.icmp_hdr.checksum = calculate_checksum((unsigned short *)&packet, sizeof(packet));
```

Steps:
1. build a packet with the PID of the program and the sequence number of
   this packet (incremented after each send)
2. copy the current timestamp into the packet's payload — **this is what makes the RTT calculation reliable later**: instead of relying on a
   single "last sent time" variable that would get overwritten by the
   next packet if a reply is delayed, the exact send time travels inside
   the packet itself and comes back unchanged in the reply
3. compute the checksum over the **whole packet** (header + payload)

Packets are sent with a total size of 64 bytes (8-byte ICMP header + 56
bytes of payload), matching the default size used by the reference
`ping`.

### Packet received

IP structure of the received packet:
```h
/* from #include <netinet/ip.h> */
struct iphdr
  {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
  };
```

Retrieve information:
1. locate the IP header:
    ```c
    /* from receive.c */
    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    ```

2. locate the ICMP header within the received packet:
    ```c
    /* from receive.c */
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    ```
    IHL stands for Internet Header Length, expressed in 32-bit (4-byte) words.
    The size of the IP header is therefore: `ihl value × word size (4 bytes)`.
    Note that for a valid minimal IPv4 header, `ihl` is at least `5` (i.e. 20
    bytes, with no IP options).

3. extract the timestamp from the payload:
    ```c
    /* from receive.c */
    char payload[56] = {0};
    memmove(payload, buffer + ip_hdr->ihl * 4 + sizeof(struct icmphdr), sizeof(payload));
    struct timeval *sent_time = (struct timeval *)payload;
    ```

The RTT is then simply `now - *sent_time`. Because the send time is read
back from the reply itself rather than from a local variable, this stays
correct even if replies arrive late or out of order.

This is used to print the ping output:

```
64 bytes from 142.251.39.110: icmp_seq=0 ttl=255 rtt=4.275 ms
```

### Error messages (`-v`)

Unlike an Echo Reply, ICMP error messages (`Time Exceeded`, `Destination Unreachable`, `Parameter Problem`) do **not** carry an `id`/`sequence`
directly in their own ICMP header.  
Instead, the ICMP error message contains the original IP packet that
caused the error. For an `ft_ping` Echo Request, this embedded packet
contains **the original IP header** followed by the **original ICMP Echo header**.

```
┌───────────────────────────┐
│ Outer IP Header (20B)     │ source = router/host that raised the error
├───────────────────────────┤
│ ICMP Error Header (8B)    │ type (11/3/12), code, checksum, unused/pointer
├───────────────────────────┤
│ Original IP Header (20B)  │ copy of the packet that failed
├───────────────────────────┤
│ Original ICMP Header (8B) │ = our original ICMP Echo header
├───────────────────────────┤ (this is where id/sequence are recovered)
│ Original payload          │ 
│ if included               │ 
└───────────────────────────┘
```

To correctly identify which of our own requests triggered the error, we
therefore need to skip **two IP headers and one ICMP header** before
reaching the original `id`/`sequence`:

```c
    struct iphdr *ip_hdr = (struct iphdr *)buffer;
    struct icmphdr *icmp = (struct icmphdr *)(buffer + ip_hdr->ihl * 4);
    struct iphdr *orig_ip_hdr = (struct iphdr *)((unsigned char *)icmp + sizeof(struct icmphdr));
    struct icmphdr *orig_icmp = (struct icmphdr *)((unsigned char *)orig_ip_hdr + sizeof(struct iphdr));

```

Error types handled:

| Type | Value | Meaning |
|------|-------|---------|
| `ICMP_DEST_UNREACH`    | 3  | destination/network/port unreachable |
| `ICMP_TIME_EXCEEDED`   | 11 | TTL reached 0 before arriving at destination |
| `ICMP_PARAMETERPROB`   | 12 | malformed IP header/options in the original packet |

None of these should crash the program — they must be reported (with
`-v`) and the program must keep listening for further replies.

#### ICMP header dump

With `-v`, the reference `ping` prints a hex dump of this ICMP error
packet, which is useful to visually confirm the structure above.

Exemple output:
```sh
IP Hdr Dump:
 45c0 0070 efcd 0000 4001 71e2 0a00 020f 0a00 020f 
Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst     Data
 4  5  00 0054 bba2   2 0000  40  01 36f7 10.0.2.15  203.0.113.0 
ICMP: type: 8, code: 0, size: 64, id: 0x1772, seq: 0x0000
```

| Field           | Value         | Meaning                         |
| --------------- | ------------- | ------------------------------- |
| Version         | `4`           | IPv4                            |
| Header Length   | `5`           | 5 × 4 = 20 bytes                |
| TOS             | `0x00`        | Type of Service / DS field      |
| Length          | `0x0054`      | 84 bytes total IP packet length |
| ID              | `0xbba2`      | IPv4 identification field       |
| Flags           | `2`           | Don't Fragment (`DF`)           |
| Fragment offset | `0`           | Packet is not fragmented        |
| TTL             | `40`          | Initial/default TTL (hex)       |
| Protocol        | `1`           | ICMP                            |
| Checksum        | `0x36f7`      | IPv4 header checksum            |
| Source          | `10.0.2.15`   | Source IP                       |
| Destination     | `203.0.113.0` | Destination IP                  |



## TTL (Time To Live)

The TTL is a field of the IPv4 header (8 bits, values from 0 to 255). It
represents the maximum number of routers (hops) a packet is allowed to
cross before being discarded.

```
┌─────────────────────────────┐
│         IPv4 Header         │
│            ...              │
│          TTL = 64           │
│        Protocol = ICMP      │
│            ...              │
└─────────────────────────────┘
```

Without a TTL, a misrouted packet (e.g. caught in a routing loop between
two misconfigured routers) would circulate on the network indefinitely,
consuming bandwidth without ever reaching its destination or being
reported as lost.

The TTL acts as a safeguard: every router that processes the packet
**decrements the TTL by 1** before forwarding it. If the TTL reaches 0,
the router discards the packet and sends an ICMP **Time Exceeded** (type 11) error message back to the sender, instead of letting the packet keep
circulating.

```
PC                Router            Destination
 │                   │                    │
 │ TTL = 64          │                    │
 ├──────────────────►│                    │
 │                   │ TTL = 63           │
 │                   ├───────────────────►│
 │                   │                    │
```

### Values

- **Starting TTL**: chosen by the sender when the ICMP packet is
  encapsulated into IPv4. This starting value is not standardized and
  depends on the OS/device that generated the packet:

    | System                                  | Typical starting TTL |
    |-------------------------------------------|-----------------------|
    | Linux                                      | 64                    |
    | Windows                                    | 128                   |
    | Solaris / some network equipment            | 255                   |

  This value can be overridden with the `-t`/`--ttl` flag (useful in
  particular to force a low TTL and trigger a Time Exceeded error on
  purpose) :
    ```c
    setsockopt(ping->socket_fd, IPPROTO_IP, IP_TTL, &ping->ttl, sizeof(int));
    ```

- **TTL in the reply**: this is the **remaining** value as read
  from the packet **at the time it is received**, after all the
  decrements performed by the routers it went through. This value never
  needs to be computed — it is simply read directly from `ip_hdr->ttl`.

## RTT and statistics

The RTT measures the time between sending and receiving a packet. At the
end of the program, `ping` prints its statistics:
- **min**: the minimum observed RTT
- **avg**: the average RTT
- **max**: the maximum observed RTT
- **stddev**: the standard deviation of the RTT values, i.e. how much
  individual RTTs vary around the average — a low stddev means a stable
  connection, a high one means the RTT fluctuates a lot between packets.
  It is computed as:

  ``` 
  stddev = sqrt((rtt->total_squared / packets_received) - (avg_time * avg_time))
  ```

Example:
```sh
# ...
64 bytes from 142.251.39.110: icmp_seq=9 ttl=255 rtt=7.629 ms
--- google.com ping statistics ---
10 packets transmitted, 10 received, 0% packet loss
round-trip min/avg/max/stddev = 5.133/7.529/10.941/1.551 ms
```

## Byte order and conversion

The network and the host machine may use a different byte order, so
values need to be converted before being sent or after being received.

- **Host To Network Short** (`htons`): a host typically stores multi-byte
  values in little-endian order (least significant byte first in memory),
  while network protocols always use big-endian ("network byte order",
  most significant byte first). `htons` converts a 16-bit value from host
  order to network order before it's placed in a packet:
```c
  // from send.c
  .id = htons(ping->pid)
```

- **Network To Host Short** (`ntohs`): when reading a 16-bit value from a
  received packet, it must be converted back from network byte order to
  the host's own byte order before being used or printed:
```c
  // from receive.c
  printf("icmp_seq=%u ", ntohs(icmp->un.echo.sequence));
```

## `ping`'s source code from inetutils-2.0

To retrieve the source code of `ping` from `inetutils-2.0` on Debian:

```sh
apt source inetutils
```

Then, inside the extracted folder, run `./configure` and `make`.


---

# `ft_ping` usage

## Allowed arguments

| Flag                          | Description |
|--------------------------------|--------------|
| `-?`, `-h`, `--help`           | display help |
| `-v`, `--verbose`              | enable verbose output (view packet errors and PID) |
| `-w`, `--timeout=N`            | stop the whole program after N seconds have elapsed, regardless of how many packets were sent |
| `-W`, `--linger=N`             | number of seconds to keep waiting for pending replies, once all packets have already been sent |
| `-i`, `--interval=NUMBER`      | wait NUMBER seconds between sending each packet |
| `-c`, `--count=NUMBER`         | stop after sending NUMBER packets (still respecting `-W` linger time afterwards) |
| `-t`, `--ttl=N`                | specify N as the outgoing time-to-live (set via `setsockopt`) |

### On `-w` vs `-W`

These two flags are easy to confuse, so it's worth being precise:

- **`-w` (timeout)**: a hard deadline for the **entire program**. Once N
  seconds have passed since the program started, `ping` stops
  unconditionally and prints its statistics — no matter how many packets
  were sent or how many replies are still pending.
- **`-W` (linger)**: only relevant **after the last packet has already been sent** (either because `-c` was reached, or `-w` fired). It's the
  extra grace period `ping` waits for the *remaining* replies to trickle
  in, before finally printing statistics and exiting.

This is directly reflected in `inetutils-2.0`'s own source:

```c
if (!ping->ping_count || ping->ping_num_xmit < ping->ping_count)
  // still sending packets normally
else if (finishing)
  // already in the linger phase, waiting it out
else
  {
    finishing = 1;
    intvl.tv_sec = linger;   // switch to the -W linger duration
  }
```

> **Important:** `-W` does **not** work as a per-packet "discard if late"
> filter. It only controls how long `ping` actively blocks waiting for a
> given reply before moving on to the next step. A reply that arrives
> after `-W` has already elapsed is still read from the socket and
> displayed with its real (possibly very large) RTT the next time the
> program checks for incoming data — it is never silently dropped just
> for being late.

### `Ctrl+C` handling

`SIGINT` (`Ctrl+C`) is handled gracefully: instead of terminating
abruptly, `ft_ping` prints its final statistics (packets transmitted/
received, packet loss, round-trip min/avg/max/stddev) before exiting
cleanly — matching the reference `ping`'s behavior.

---

# Simulating network conditions for testing

**NetEm** (Network Emulator) is a Linux feature used through `tc`
(traffic control) to emulate degraded or specific network conditions —
useful to test `ft_ping` against packet loss, delay, duplication, or
corruption without needing a real unreliable network.

```sh
sudo tc qdisc replace dev XX root netem
 │   │    │      │     │      │    └── use the Network Emulator
 │   │    │      │     │      └── root queueing discipline of this interface
 │   │    │      │     └── the network interface being configured
 │   │    │      └── replace the existing configuration
 │   │    └── Queueing Discipline (defines how packets are managed)
 │   └── Traffic Control (Linux tool used to configure it)
 └── run with administrator privileges
```

`dev XX`: the interface to modify — e.g. `enp0s1` for the VM's routed
interface, or `lo` for localhost.

```sh
sudo tc qdisc replace dev XX root netem delay XX corrupt XX duplicate XX
```

| Parameter   | Example              | Definition |
|-------------|-----------------------|------------|
| `delay`     | `delay 100ms` or `delay 150ms 50ms` | adds artificial latency to packets (optionally with jitter) |
| `loss`      | `loss 20%`            | simulates random packet loss |
| `duplicate` | `duplicate 50%`       | duplicates a percentage of packets |
| `corrupt`   | `corrupt 30%`         | randomly corrupts packet content (triggers checksum mismatches — useful to test `-v`'s error handling) |

Examples:
```sh
sudo tc qdisc replace dev lo root netem delay 200ms 50ms
sudo tc qdisc replace dev lo root netem duplicate 50% delay 50ms
sudo tc qdisc replace dev lo root netem duplicate 50%
sudo tc qdisc replace dev lo root netem duplicate 50% corrupt 50%
sudo tc qdisc replace dev lo root netem delay 200ms 50ms corrupt 30% duplicate 30%
```

Remove the emulated environment:
```sh
sudo tc qdisc del dev lo root
```

Observe packet traffic while testing:
```sh
sudo tcpdump -i XX -n -tt icmp   # -tt: print raw Unix timestamps
sudo tcpdump -i XX -n -vv icmp   # -vv: verbose output, includes checksum validity
```

`-n` avoids resolving hostnames (faster, and avoids interference from
DNS lookups while testing). `-vv` is particularly useful for checksum
debugging: tcpdump explicitly reports `wrong icmp cksum` if a captured
packet's checksum doesn't match its content, which tells you immediately
whether an "invalid checksum" bug is on the sending side (bad packet on
the wire) or on `ft_ping`'s own verification logic (packet is actually
fine).

## Simulating `Destination Host Unreachable`

A `Destination Host Unreachable` error can also be triggered by adding
an unreachable route inside the VM.

For example:

```sh
sudo ip route add 203.0.113.0/24 via 10.0.2.254
```

- `203.0.113.0/24` is the destination network we want to reach.
via `10.0.2.254` tells Linux to forward packets for this network
through `10.0.2.254`.  
- `10.0.2.254` must be an appropriate next-hop address for the VM's
network. Check the VM's routing table with:  
  ``` sh
  ip route
  ```

Once the route has been added, try to reach an address in that network:

``` sh
sudo ./ft_ping -c 1 203.0.113.1
```

If the configured next-hop cannot reach the destination, the network
stack can generate an ICMP Destination Unreachable error.

For example:
```sh
92 bytes from 10.0.2.15: Destination Unreachable
```

This is useful for testing how `ft_ping` handles ICMP error messages
(type 3) and verifies that the program does not terminate when an
unreachable destination is reported.

---

# How to Use `ft_ping`

1. Clone `ft_ping` in a folder first  : `git clone https://github.com/bibickette/ft_ping.git`
2. Go to folder and compile it : `cd ft_ping && make`
3. Run it with root permission: `sudo ./ft_ping [flags...] <destination>` *(see [allowed arguments](#allowed-arguments) for more details)*


* * *
 
*Project validation date: TBD*
