// 数据集数组
var datasets = [
    { nodes: [{name:"😎"},{name:"☀️"},{name:"🕶"},{name:"👀"},{name:"💯"},{name:"🤓"},{name:"📍"},{name:"😊"},{name:"🔥"},{name:"👓"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [ {name: "👍"},{name: "✌"},{name: "👌"},{name: "👎"},{name: "👏"},{name: "🖕"},{name: "🆗"},{name: "🙌"},{name: "🙆"},{name: "🤙"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [{name: "😳"},{name: "😍"},{name: "👀"}, {name: "😥"},{name: "😲"},{name: "😧"},{name: "😶"},{name: "😰"},{name: "👄"},{name: "😮"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [ {name: "😂"},{name: "🤣"},{name: "👀"},{name: "🤩"},{name: "❤"},{name: "⚠"},{name: "🦷"},{name: "😃"},{name: "👄"},{name: "😹"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [ {name: "🤣"},{name: "😂"},{name: "😄"},{name: "😝"},{name: "😿"},{name: "🤪"},{name: "😅"},{name: "😃"},{name: "😆"},{name: "😹"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [ {name: "💔"},{name: "❤"},{name: "💞"},{name: "🧡"},{name: "💖"},{name: "💓"},{name: "🖤"},{name: "💑"},{name: "💌"},{name: "🥀"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] },
    { nodes: [{name: "😭"},{name: "❤"},{name: "😲"},{name: "😢"},{name: "😫"},{name: "🚰"},{name: "😣"},{name: "👀"},{name: "💧"},{name: "😩"}],
      edges: [{source:0,target:1,value:1.3},{source:0,target:2,value:1},{source:0,target:3,value:1},{source:0,target:4,value:1},{source:0,target:5,value:2},{source:0,target:6,value:0.9},{source:0,target:7,value:1},{source:0,target:8,value:1.6},{source:0,target:9,value:0.7}] }
];

var currentDatasetIndex = 0;

function renderGraph(dataset) {
    var margin2 = 30; // 边距
    var svg2 = d3.select('#forceStimulation');
    var width2 = +svg2.attr('width');
    var height2 = +svg2.attr('height');

    // 清除之前的图形
    svg2.selectAll('*').remove();

    // 创建一个分组 并设置偏移
    var g = svg2.append('g').attr('transform', 'translate(' + margin2 + ',' + margin2 + ')');

    // 新建一个颜色比例尺
    var scaleColor = d3.scaleOrdinal()
        .domain(d3.range(dataset.nodes.length))
        .range(d3.schemeCategory10);

    // 新建一个力导向图
    var forceSimulation = d3.forceSimulation()
        .force("link", d3.forceLink())
        .force("charge", d3.forceManyBody().strength(-400))
        .force("center", d3.forceCenter());

    // 生成节点数据
    forceSimulation.nodes(dataset.nodes)
        .on('tick', ticked); // 这个函数下面会讲解

    // 生成边数据
    forceSimulation.force('link')
        .links(dataset.edges)
        .distance(function (d) {
            return d.value * 150; // 设置边长
        });

    // 设置图形 中心点
    forceSimulation.force('center')
        .x(width2 / 2) // 设置x坐标
        .y(height2 / 2); // 设置y坐标

    // 绘制边  这里注意一下绘制顺序  在d3中  各元素是有层级关系的，先绘制的在下面
    var links = g.append('g')
        .selectAll('line')
        .data(dataset.edges)
        .enter()
        .append('line')
        .attr('stroke-width', '2') // 设置边线宽度
        .attr('stroke', function (d, i) {
            return scaleColor(i); // 设置边线颜色
        });

    // 绘制边上的文字
    var linksText = g.append('g')
        .selectAll('text')
        .data(dataset.edges)
        .enter()
        .append('text')
        .text(function (d) {
            return d.relation || ''; // 假设relation属性存在
        })
        .attr('dy', -3)
        .attr('font-size', '10px');

    // 创建节点分组
    var gs = g.selectAll('.circle')
        .data(dataset.nodes)
        .enter()
        .append('g')
        .attr('class', 'circle')
        .call(
            d3.drag() // 相当于移动端的拖拽手势  分以下三个阶段
                .on('start', start)
                .on('drag', drag)
                .on('end', end)
        );

    // 绘制节点
    gs.append('circle')
        .attr('r', function (d, i) {
            return i == 0 ? 15 : 10;
        })
        .attr('fill', function (d, i) {
            return scaleColor(i);
        });

    // 绘制文字
    gs.append('text')
        .style('font-size', '50px')
        .attr('dx', 15)
        .attr('dy', '.35em')
        .text(function (d) {
            return d.name;
        });

    function ticked() {
        links
            .attr("x1", d => d.source.x)
            .attr("y1", d => d.source.y)
            .attr("x2", d => d.target.x)
            .attr("y2", d => d.target.y);

        linksText
            .attr("x", d => (d.source.x + d.target.x) / 2)
            .attr("y", d => (d.source.y + d.target.y) / 2);

        gs.attr('transform', d => `translate(${d.x},${d.y})`);
    }

    function start(event, d) {
        if (!event.active) forceSimulation.alphaTarget(0.8).restart();
        d.fx = d.x;
        d.fy = d.y;
    }

    function drag(event, d) {
        d.fx = event.x;
        d.fy = event.y;
    }

    function end(event, d) {
        if (!event.active) forceSimulation.alphaTarget(0);
        d.fx = null;
        d.fy = null;
    }
}

// 渲染初始图表
renderGraph(datasets[currentDatasetIndex]);

// 创建按钮
var buttonContainer = d3.select('#buttons');
datasets.forEach((dataset, index) => {
    buttonContainer.append('button')
        .attr('class', 'button')
        .text(dataset.nodes[0].name) // 显示中心节点的emoji
        .on('click', () => {
            currentDatasetIndex = index;
            renderGraph(datasets[index]);
        });
});