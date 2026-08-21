# Reliable UDP Lab: Detailed Notes

## Messages and Flags
- Header fields: `seq`/`ack` (byte-based), `len`, `wnd` (receiver free window in packets), `flags` (SYN/ACK/FIN/META), `sack` (32-bit bitmap after `ack`), `checksum` (16-bit internet checksum over header+payload).
- Handshake: SYN → SYN|ACK → ACK. Teardown: FIN|ACK → final ACK. FIN confirmation is best-effort; data integrity is unaffected.
- Segmentation: fixed payload (default 1000B, configurable), pipelined sends; out-of-window data is dropped.

## Modules
- `src/reliable_udp.cpp`
  - Sender: handshake, send META (filename|size), pipeline data, Reno congestion control, SACK-aware selective repeat, timeout + retransmit, throughput stats, FIN best-effort close.
  - Receiver: checksum verification, in-order write with out-of-order buffer, SACK construction, window advertisement, replies FIN|ACK and extra ACKs, auto-create output dir, UTF-8 console, loops to serve multiple connections.
- `src/reliable_udp.h`: constants, config, stats.
- `src/sender_main.cpp` / `src/receiver_main.cpp`: CLI entry points.
- `CMakeLists.txt`: CMake build, links `ws2_32` on Windows, `/utf-8` for MSVC.

## Key Algorithms
- Congestion control (Reno): `cwnd=1`, `ssthresh=window`; slow start → additive increase; 3 dup ACK → fast retransmit/recovery; timeout → slow start.
- SACK: receiver marks up to 32 segments after `ack`; sender skips already received blocks to reduce redundant retransmits.
- Timeouts: based on oldest unacked send time, uses `rto_ms`; bounded by `max_retransmit`.

## Build & Run
```powershell
cmake -S . -B build
cmake --build build
```
- Receiver (looping service): `.\\build\\Debug\\rudp_receiver.exe 127.0.0.1 9000 .\\recv_out --wnd 32 --payload 1000`
- Sender (direct): `.\\build\\Debug\\rudp_sender.exe 127.0.0.1 9000 .\\lab2测试环境\\测试文件\\1.jpg --wnd 32 --payload 1000 --rto 300`
- With Router: sender targets Router listen port (e.g., 8000), Router forwards to receiver (e.g., 127.0.0.1:9000); ensure Router has no bandwidth cap and correct forwarding.

## Tuning
- `--wnd`/`--payload`: for moderate loss (≈5%), try `wnd=64`, `payload=1200`; too large a window can inflate retransmits under high loss.
- `--rto`: increase for higher RTT/loss (e.g., 600–1000ms) to reduce spurious timeouts.
- FIN warnings: disabled (best-effort close). Data integrity is determined by ACKed bytes, not FIN.

## Possible Improvements
- RTT estimation with adaptive RTO.
- Larger or offset-based SACK representation.
- Dynamic receive window; rate-based pacing.
- Batch META for multi-file transfers; TIME_WAIT/delayed ACK handling.
