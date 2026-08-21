# 实验三要求1：Web 服务器与页面内容分析

本次实验在 `site` 目录下通过 `python -m http.server 8000` 启动了纯 HTTP 的简易 Web 服务器，监听在 `localhost:8000`，未启用 HTTPS 或任何反向代理，从浏览器访问 `http://localhost:8000/index.html` 即可直接获取页面内容，符合实验对“自行搭建 Web 服务器并使用 HTTP” 的限定。服务器无额外路由和压缩配置，所有资源均为本地静态文件，这种极简架构方便 Wireshark 抓取到完整的明文请求行、响应行以及头部和实体，对后续报文分析提供了清晰数据。

网页主体存放于 `site/index.html`，HTML 结构扁平清晰：`<header>` 仅呈现标题“Networking Lab 3 Page”，`<main>` 内的 `.card` 区块承载实验要求的基本文本信息，使用 `<ul>` 列表列出专业、学号、姓名等字段。其中专业标识为“Computer Science”，学号为“2312331”，姓名为“王一诺”，并补充了课程说明“Computer Networks Experiment”，对应实验要求（1）中“包含简单文本信息（包含专业、学号、姓名）”的全部要素。页面使用内联样式保证背景、卡片、文字和分隔线的基础视觉效果，但未引入复杂脚本或外链资源，确保页面保持“不要太复杂”的原则，同时减少额外 HTTP 请求。

页面下半部分的 `<div class="gallery">` 构成六宫格图像区域，逐一引用 `site/images/pic1.svg` 至 `site/images/pic6.svg` 六个本地 SVG 文件，每个 `<figure>` 内包含 `<img>` 与 `<figcaption>`，带有描述性 `alt` 文本（如“Color block 1”）和文字说明（如“Image 1: teal gradient”），保证可访问性与语义清晰度。六幅图像均为静态本地资源，无外部依赖，浏览器在加载 `index.html` 后会顺序发起六个独立的 HTTP GET 请求，便于在抓包时观察多对象的并行获取过程和缓存行为。至此，页面已完整满足实验要求（1）中“包含六幅图像”的量化条件。

综上，当前 Web 服务器与页面设计在保持内容简单明了的同时，提供了专业、学号、姓名三项核心文本信息，并附带六幅本地图像，所有资源经由 `python -m http.server` 提供的 HTTP 通道分发，符合实验三要求（1）的所有检查点，也为后续 Wireshark 报文捕获与层次分析奠定了清晰、可复现的基础。
