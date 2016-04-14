# udp-rxtx

Yet another network traffic generator...

Could be used to send udp traffic in bursts or equally spread out packets.

| Option | explanation |
| ------------- | ------------- |
| -b | send packets in bursts |
| -c | max pps per core, will use yet another core when more pps is requested. Always starting from core 1 (not core 0) as core 0 might be the one doing irq. |
| -l | packet length, defaults to 64 |
| -p | pps (packets per second) |
| -r | run as reciever |
| -t <ip> | run as sender |
