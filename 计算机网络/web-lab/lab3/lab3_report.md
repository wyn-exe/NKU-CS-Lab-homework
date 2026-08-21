# 实验3：配置 Web 服务器，捕获 HTTP 报文并分析

## 实验环境
- 系统：Windows（可在任意主机复现）
- Web 服务：Python 内置 `http.server`（HTTP 明文，无 HTTPS）
- 浏览器：任意现代浏览器（Chrome/Edge/Firefox 等）
- 抓包：Wireshark（显示过滤器使用 `http`）

## 实验内容与资源
- 网页文件：`site/index.html`
- 6 幅图像：`site/images/pic1.svg` ~ `pic6.svg`
- 抓包文件：`captures/http_trace.pcap`（示例 HTTP 交互，便于直接分析）
- 报文生成脚本：`captures/make_pcap.py`（可重现生成示例抓包）

## Web 服务器搭建步骤（HTTP）
1) 终端进入网页目录：`cd site`
2) 启动 HTTP 服务：`python -m http.server 8000`
3) 浏览器访问：`http://localhost:8000`（确保是 HTTP 而非 HTTPS）
4) 页面内含必填信息占位符（专业、学号、姓名），提交前请替换为真实信息。

## Wireshark 抓包步骤
1) 打开 Wireshark，选择与浏览器流量相同的网卡。
2) 可选：在“捕获过滤器”中使用 `tcp port 8000`（若使用 8000 端口）或 `tcp port 80` 只捕获 HTTP 会话。
3) 开始捕获后，在浏览器访问 `http://localhost:8000/index.html`。
4) 停止捕获，将结果保存为 `.pcapng`/`.pcap`。在“显示过滤器”输入 `http` 即可只显示 HTTP 报文。

## 报文封装层次分析（示例基于 `captures/http_trace.pcap`）
选取其中的两个关键报文：HTTP 请求（客户端 → 服务器）与 HTTP 响应（服务器 → 客户端）。

### 报文 A：HTTP GET 请求（客户端→服务器）
- 数据链路层：Ethernet II，Src MAC `0a:00:27:00:00:0a`，Dst MAC `0a:00:27:00:00:14`，Type `0x0800`。
- 网络层：IPv4，Src `192.168.1.10`，Dst `192.168.1.20`，首部长度 20 字节，总长度 134 字节，协议号 6 (TCP)。
- 传输层：TCP，Src Port 54832，Dst Port 80，Seq=1001，Ack=5001，Flags=PSH,ACK，数据长度 94 字节，窗口 64240。
- 应用层：HTTP/1.1 请求行 `GET /index.html HTTP/1.1`，首部包含 `Host: 192.168.1.20`、`User-Agent: LabClient/1.0`、`Accept: text/html` 等。

### 报文 B：HTTP 200 OK 响应（服务器→客户端）
- 数据链路层：Ethernet II，Src MAC `0a:00:27:00:00:14`，Dst MAC `0a:00:27:00:00:0a`，Type `0x0800`。
- 网络层：IPv4，Src `192.168.1.20`，Dst `192.168.1.10`，首部长度 20 字节，总长度 220 字节，协议号 6 (TCP)。
- 传输层：TCP，Src Port 80，Dst Port 54832，Seq=5001，Ack=1095（1001+94），Flags=PSH,ACK，数据长度 180 字节。
- 应用层：HTTP/1.1 200 OK；首部示例 `Content-Type: text/html`，`Content-Length: 97`，`Connection: close`；正文包含简单 HTML（标题与段落）。

## HTTP 交互过程说明（对应示例抓包）
1) 三次握手：客户端 SYN（Seq=1000）→ 服务器 SYN+ACK（Seq=5000，Ack=1001）→ 客户端 ACK（Ack=5001）。
2) 客户端发送 HTTP GET（PSH+ACK，Seq=1001，长度 94）。
3) 服务器返回 HTTP/1.1 200 OK（PSH+ACK，Seq=5001，长度 180，Ack=1095）。
4) 客户端 ACK 应答（Ack=5181）。
5) 服务器发送 FIN+ACK 关闭连接，客户端最终 ACK，连接释放。
6) 在 Wireshark 使用显示过滤器 `http` 即可只看到请求与响应两条 HTTP 报文，便于分析头部与负载。

## 提交材料清单
- `site/index.html`（包含专业、学号、姓名占位及 6 幅图像）
- `site/images/pic1.svg` ~ `pic6.svg`
- `captures/http_trace.pcap`（示例 HTTP 抓包，可直接打开分析）
- `captures/make_pcap.py`（需要时可重新生成示例抓包）
- `lab3_report.md`（本报告）

## 备注与建议
- 提交前将 `index.html` 中的占位信息替换为真实的专业/学号/姓名。
- 若课堂要求使用真实抓包：启动本地 HTTP 服务器后按“Wireshark 抓包步骤”自行抓取，并替换/补充提交文件。
- 确保浏览器访问使用 `http://`，否则抓包会出现 TLS 加密的 HTTPS，无法直接查看 HTTP 首部与正文。
