import socket
import struct
import time
from pathlib import Path


def checksum(data: bytes) -> int:
    if len(data) % 2:
        data += b"\x00"
    words = struct.unpack("!%dH" % (len(data) // 2), data)
    total = sum(words)
    while total >> 16:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def mac_bytes(mac: str) -> bytes:
    return bytes(int(part, 16) for part in mac.split(":"))


def tcp_segment(src_ip: str, dst_ip: str, src_port: int, dst_port: int,
                seq: int, ack: int, flags: int, window: int = 64240, payload: bytes = b"") -> bytes:
    data_offset = 5  # 20-byte header, no options
    offset_flags = (data_offset << 12) | flags
    header = struct.pack("!HHIIHHHH", src_port, dst_port, seq, ack, offset_flags, window, 0, 0)
    pseudo = struct.pack("!4s4sBBH", socket.inet_aton(src_ip), socket.inet_aton(dst_ip), 0, 6, len(header) + len(payload))
    chksum = checksum(pseudo + header + payload)
    header = struct.pack("!HHIIHHHH", src_port, dst_port, seq, ack, offset_flags, window, chksum, 0)
    return header + payload


def ipv4_packet(src_ip: str, dst_ip: str, payload: bytes, identification: int, ttl: int = 64, proto: int = 6) -> bytes:
    version_ihl = 0x45
    total_length = 20 + len(payload)
    flags_fragment = 0x4000  # do not fragment
    header = struct.pack("!BBHHHBBH4s4s",
                         version_ihl, 0, total_length, identification, flags_fragment,
                         ttl, proto, 0, socket.inet_aton(src_ip), socket.inet_aton(dst_ip))
    chksum = checksum(header)
    header = struct.pack("!BBHHHBBH4s4s",
                         version_ihl, 0, total_length, identification, flags_fragment,
                         ttl, proto, chksum, socket.inet_aton(src_ip), socket.inet_aton(dst_ip))
    return header + payload


def ethernet_frame(dst_mac: str, src_mac: str, eth_type: int, payload: bytes) -> bytes:
    return mac_bytes(dst_mac) + mac_bytes(src_mac) + struct.pack("!H", eth_type) + payload


def pcap_global_header() -> bytes:
    return struct.pack("=IHHiIII", 0xA1B2C3D4, 2, 4, 0, 0, 65535, 1)


def pcap_packet_header(ts: float, length: int) -> bytes:
    sec = int(ts)
    usec = int((ts - sec) * 1_000_000)
    return struct.pack("=IIII", sec, usec, length, length)


def build_capture() -> bytes:
    client_ip = "192.168.1.10"
    server_ip = "192.168.1.20"
    client_mac = "0a:00:27:00:00:0a"
    server_mac = "0a:00:27:00:00:14"
    client_port = 54832
    server_port = 80

    seq_c = 1000
    seq_s = 5000

    req_payload = (
        b"GET /index.html HTTP/1.1\r\n"
        b"Host: 192.168.1.20\r\n"
        b"User-Agent: LabClient/1.0\r\n"
        b"Accept: text/html\r\n"
        b"\r\n"
    )

    html_body = (
        b"<!doctype html><html><body><h1>Lab 3 Page</h1>"
        b"<p>HTTP trace for networking lab.</p></body></html>"
    )
    res_payload = (
        b"HTTP/1.1 200 OK\r\n"
        b"Content-Type: text/html\r\n"
        b"Content-Length: " + str(len(html_body)).encode() + b"\r\n"
        b"Connection: close\r\n"
        b"\r\n" + html_body
    )

    packets = []
    timestamp = time.time()
    ident = 1

    def add_packet(frame: bytes, delta: float):
        nonlocal timestamp, ident
        timestamp += delta
        packets.append(pcap_packet_header(timestamp, len(frame)) + frame)
        ident += 1

    # 1) SYN from client
    syn_tcp = tcp_segment(client_ip, server_ip, client_port, server_port, seq_c, 0, flags=0x02)
    syn_ip = ipv4_packet(client_ip, server_ip, syn_tcp, identification=ident)
    syn_frame = ethernet_frame(server_mac, client_mac, 0x0800, syn_ip)
    add_packet(syn_frame, 0.001)
    seq_c_next = seq_c + 1

    # 2) SYN-ACK from server
    synack_tcp = tcp_segment(server_ip, client_ip, server_port, client_port, seq_s, seq_c_next, flags=0x12)
    synack_ip = ipv4_packet(server_ip, client_ip, synack_tcp, identification=ident)
    synack_frame = ethernet_frame(client_mac, server_mac, 0x0800, synack_ip)
    add_packet(synack_frame, 0.02)
    seq_s_next = seq_s + 1

    # 3) ACK from client
    ack_tcp = tcp_segment(client_ip, server_ip, client_port, server_port, seq_c_next, seq_s_next, flags=0x10)
    ack_ip = ipv4_packet(client_ip, server_ip, ack_tcp, identification=ident)
    ack_frame = ethernet_frame(server_mac, client_mac, 0x0800, ack_ip)
    add_packet(ack_frame, 0.02)

    # 4) HTTP GET request (PSH+ACK)
    req_tcp = tcp_segment(client_ip, server_ip, client_port, server_port, seq_c_next, seq_s_next, flags=0x18, payload=req_payload)
    req_ip = ipv4_packet(client_ip, server_ip, req_tcp, identification=ident)
    req_frame = ethernet_frame(server_mac, client_mac, 0x0800, req_ip)
    add_packet(req_frame, 0.03)
    seq_c_data_end = seq_c_next + len(req_payload)

    # 5) HTTP 200 response (PSH+ACK)
    res_tcp = tcp_segment(server_ip, client_ip, server_port, client_port, seq_s_next, seq_c_data_end, flags=0x18, payload=res_payload)
    res_ip = ipv4_packet(server_ip, client_ip, res_tcp, identification=ident)
    res_frame = ethernet_frame(client_mac, server_mac, 0x0800, res_ip)
    add_packet(res_frame, 0.05)
    seq_s_data_end = seq_s_next + len(res_payload)

    # 6) ACK from client for response
    ack2_tcp = tcp_segment(client_ip, server_ip, client_port, server_port, seq_c_data_end, seq_s_data_end, flags=0x10)
    ack2_ip = ipv4_packet(client_ip, server_ip, ack2_tcp, identification=ident)
    ack2_frame = ethernet_frame(server_mac, client_mac, 0x0800, ack2_ip)
    add_packet(ack2_frame, 0.02)

    # 7) FIN-ACK from server
    fin_tcp = tcp_segment(server_ip, client_ip, server_port, client_port, seq_s_data_end, seq_c_data_end, flags=0x11)
    fin_ip = ipv4_packet(server_ip, client_ip, fin_tcp, identification=ident)
    fin_frame = ethernet_frame(client_mac, server_mac, 0x0800, fin_ip)
    add_packet(fin_frame, 0.02)

    # 8) Final ACK from client
    final_ack_tcp = tcp_segment(client_ip, server_ip, client_port, server_port, seq_c_data_end, seq_s_data_end + 1, flags=0x10)
    final_ack_ip = ipv4_packet(client_ip, server_ip, final_ack_tcp, identification=ident)
    final_ack_frame = ethernet_frame(server_mac, client_mac, 0x0800, final_ack_ip)
    add_packet(final_ack_frame, 0.02)

    return pcap_global_header() + b"".join(packets)


def main():
    output_path = Path(__file__).with_name("http_trace.pcap")
    capture_bytes = build_capture()
    output_path.write_bytes(capture_bytes)
    print(f"Wrote {output_path} ({len(capture_bytes)} bytes)")


if __name__ == "__main__":
    main()
