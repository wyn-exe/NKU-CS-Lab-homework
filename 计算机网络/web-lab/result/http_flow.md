
# 实验3：HTTP 交互全过程说明（基于 `result/lab3.pcapng`）

本说明以 Wireshark 打开 `result/lab3.pcapng` 为依据，按时间顺序梳理 `http://localhost:8000/index.html` 的 HTTP 交互。
显示过滤器：可用 `http` 聚焦 HTTP 报文，或 `tcp.port == 8000` 查看握手与挥手。
报文走本机回环，链路为 Loopback。
IP 层使用 IPv6 回环地址 `::1` <-> `::1`，传输层为 TCP，应用层为明文 HTTP。

## 交互过程（按时间顺序）
1) 浏览器先对主页连接发起三次握手（idx0–2），SYN、SYN/ACK、ACK 依次完成，未携带应用数据。
2) 客户端发送 HTTP GET /index.html（idx3），TCP 标志 PSH+ACK，Seq=1792845185，Ack=1737695184，负载 772 字节请求头。
3) 服务器回纯 ACK（idx4），确认收到全部请求字节，准备发送响应。
4) 服务器发送 200 OK 首部（idx8），PSH+ACK，Seq=1737695184，Ack=1792845957。
    首部声明 Content-Type=text/html、Content-Length=4012。
    Last-Modified 及 Server=SimpleHTTP/0.6 Python/3.12.7 一并给出。
5) 客户端 ACK 首部（idx9），确认后续等待正文。
6) 服务器分段发送 HTML 正文（idx10、idx11、idx12），分别 1440、1440、1132 字节。
    idx10、idx11 为 ACK 置位，包含 HTML 头部与 CSS。
    idx12 为 PSH+ACK，收尾段含个人信息区、6 张图片标签和页脚提示。
7) 客户端累积 ACK（idx13），Ack=1737699383，确认正文收齐。
    服务器窗口保持开放，为后续关闭做准备。
8) 客户端发起关闭（idx14），FIN/ACK 表示主页内容收完。
    服务器回 ACK 再发 FIN/ACK（idx15、idx16），客户端最终 ACK（idx17）。
    四次挥手完成，主页连接释放。
9) 浏览器为图标建立第二条连接（idx5–7），重复 SYN、SYN/ACK、ACK。
10) 客户端发送 GET /favicon.ico（idx18），PSH+ACK，负载 648 字节。
    请求头包含 Host=localhost:8000，Referer=/index.html，说明为同站资源。
11) 服务器回纯 ACK（idx19），确认收到图标请求。
12) 服务器返回图标响应头（idx20），PSH+ACK，Content-Type=image/x-icon，Content-Length=104。
    发送 104 字节图标主体（idx22），客户端在 idx21 与 idx23 ACK。
13) 客户端主动关闭图标连接（idx24），服务器先 ACK。
    服务器再发 FIN/ACK（idx25、idx26），客户端最终 ACK（idx27），四次挥手完成。

## 过程特征与说明
- 多连接并行：主页与图标各占一条 TCP 连接，符合浏览器并行取资源行为。
- TCP 可靠性：每段数据都有 ACK，序列号与负载长度匹配，四次挥手完整呈现连接释放。
- HTTP 明文：使用 http，未加密，内容直接可见。
- 显示过滤器：用 `http` 快速定位 idx3、idx8、idx10、idx11、idx12、idx18、idx20、idx22 等关键报文。
- 链路与网络层简化：Loopback 伪头，无以太网；IPv6 回环 `::1`，无路由跳转。
- 应用层元数据：Content-Length、Content-Type 指定正文长度与类型；User-Agent、Accept、Referer 说明客户端能力和来源。
