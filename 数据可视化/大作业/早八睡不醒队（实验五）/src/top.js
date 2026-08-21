let dataset = {
    '😂': [1, 1, 1, 1, 1, 1, 1, 1, 1],
    '❤️': [2, 2, 2, 2, 2, 2, 2, 2, 2],
    '😍': [3, 3, 3, 3, 3, 3, 3, 3, 3],
    '😘': [4, 5, null, null, null, null, null,null, null],
    '😁': [5, null, 4, null, null, null, null, null, null],
    '👍': [null, null, 5, null,null, null, null, null, null],
    '😎': [null, null, null, 4, 4, null, null,null, null],
    '🙏': [null, null, null, 4, null, null, null, null, null],
    '🤣': [null, null, null, null, 5,5, null, null, null],
    '😭': [null, null, null, null,null, 4, 4, 4, 5],
    '🥺': [null, null, null, null, null, null, 5, 5, 4]
};

let years = ['2015', '2016', '2017', '2018', '2019', '2020', '2021', '2022', '2023'];
let w = 1000;
let h = 600;
let padding = 80;

let xScale = d3.scaleBand()
    .domain(years)
    .range([padding, w - padding])
    .paddingInner(0.05);

let yScale = d3.scaleLinear()
    .domain([6, 1])
    .rangeRound([h - padding, padding]);

let svg3 = d3.select('#top')
    .append('svg')
    .attr('width', w)
    .attr('height', h)
    .attr('overflow', 'hidden')
    .style('z-index','100');

// 绘制x轴
let xAxis = d3.axisBottom(xScale);
svg3.append("g")
    .attr("class", "axis x-axis")
    .attr("transform", "translate(0," + (h - padding) + ")")
    .call(xAxis);

// 绘制y轴
let yAxis = d3.axisLeft(yScale).ticks(5);
svg3.append("g")
    .attr("class", "axis y-axis")
    .attr("transform", "translate(" + padding + ",0)")
    .call(yAxis);

let colorScale = d3.scaleOrdinal(d3.schemeCategory10);

let line = d3.line()
    .defined(d => d !== null)
    .x((d, i) => xScale(years[i]) + xScale.bandwidth() / 2)
    .y(d => yScale(d));

let polyLines = svg3.selectAll(".line")
    .data(Object.keys(dataset))
    .enter().append("path")
    .attr("fill", "none")
    .attr("stroke-width", 3.5)
    .attr("class", "line")
    .style("opacity", 0)
    .attr("stroke", d => colorScale(d))
    .attr("d", d => line(dataset[d]));

// 添加图例
let legend = svg3.append("g")
    .attr("class", "legend")
    .attr("transform", `translate(${w - 60}, ${padding})`);

Object.keys(dataset).forEach((emoji, i) => {
    legend.append("rect")
        .attr("x", 0)
        .attr("y", i * 20)
        .attr("fill", colorScale(emoji));

    legend.append("text")
        .attr("x", 25)
        .attr("y", i * 20 + 9)
        .text(emoji);
});

// 添加数据点
let emojiPoints = svg3.selectAll('.emoji-points')
    .data(Object.entries(dataset))
    .enter().selectAll('.emoji-point')
    .data(([key, values]) => values.map((v, i) => ({ key, value: v, index: i })))
    .enter().append('text')
    .attr('x', d => xScale(years[d.index]) + xScale.bandwidth() / 2)
    .attr('y', d => d.value === null ? 1000 : yScale(d.value)) // 设置null值为NaN以忽略这些点
    .attr('class', 'emoji-point')
    .text(d => d.key)
    .style('fill', d => colorScale(d.key))
    .style('opacity', 0);

// 动态更新图表
function updateChart(emoji) {
    if (emoji === 'all') {
        polyLines.each(function(d) {
            let totalLength = this.getTotalLength();
            d3.select(this)
                .attr("stroke-dasharray", totalLength + " " + totalLength)
                .attr("stroke-dashoffset", totalLength)
                .transition()
                .duration(1000)
                .ease(d3.easeLinear)
                .attr("stroke-dashoffset", 0)
                .style("opacity", 1);
        });
        emojiPoints.transition()
            .duration(1000)
            .style("opacity", 1);
    } else {
        polyLines.each(function(d) {
            let totalLength = this.getTotalLength();
            d3.select(this)
                .attr("stroke-dasharray", totalLength + " " + totalLength)
                .attr("stroke-dashoffset", totalLength)
                .transition()
                .duration(1000)
                .ease(d3.easeLinear)
                .attr("stroke-dashoffset", 0)
                .style("opacity", d === emoji ? 1 : 0);
        });
        emojiPoints.transition()
            .duration(1000)
            .style("opacity", d => d.key === emoji ? 1 : 0);
    }
}

// 初始化显示第一个表情的数据
updateChart('😂');