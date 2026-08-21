
// 定义SVG画布的大小
const margin = { top: 50, right: 50, bottom: 80, left: 100 }; // 增加了margin以确保轴和标签不会被裁剪掉
const width = 1600 - margin.left - margin.right; // 增加宽度
const height = 700 - margin.top - margin.bottom; // 增加高度

// 创建SVG画布
const svg = d3.select("#sidebar").append('svg')
    .attr('width', width + margin.left + margin.right)
    .attr('height', height + margin.top + margin.bottom)
    .append('g')
    .attr('transform', `translate(${margin.left}, ${margin.top})`)
    .style('z-index','100');

// 定义颜色比例尺
const color = d3.scaleOrdinal()
    .domain(['Male', 'Female'])
    .range(['skyblue', 'pink']);

// 定义年龄分段函数，以十年为一段
function getAgeGroup(age) {
    return Math.floor(age / 10) * 10 + '-' + (Math.floor(age / 10) * 10 + 9);
}

// 读取数据并创建气泡图
d3.csv('emoji_usage_dataset.csv').then(function (data) {
    console.log(data); // 打印数据以确认加载成功

    // 数据转换，将UserAge转换为实际的数字，并映射到年龄段
    data.forEach(d => {
        d.UserAge = +d.UserAge; // 转换为数字
        d.AgeGroup = getAgeGroup(d.UserAge); // 映射到年龄段
        d.UsageFrequency = +d.UsageFrequency; // 假设UsageFrequency是数字
    });

    // 输出数据的最大值和最小值以进行调试
    console.log('Min UserAge:', d3.min(data, d => d.UserAge));
    console.log('Max UserAge:', d3.max(data, d => d.UserAge));
    console.log('Min UsageFrequency:', d3.min(data, d => d.UsageFrequency));
    console.log('Max UsageFrequency:', d3.max(data, d => d.UsageFrequency));

    // 打印所有的AgeGroup值
    const allAgeGroups = new Set(data.map(d => d.AgeGroup));
    console.log('All AgeGroups in data:', [...allAgeGroups]);

    // 创建比例尺
    const xScale = d3.scaleBand()
        .domain([...new Set(data.map(d => d.Emoji))])
        .range([0, width])
        .padding(0.1);

    const yScale = d3.scaleBand()
        .domain([...new Set(data.map(d => d.AgeGroup))].sort((a, b) => a.split('-')[0] - b.split('-')[0]))
        .range([0, height])
        .padding(0.1);

    console.log('yScale domain:', yScale.domain());

    // 创建轴
    const xAxis = d3.axisBottom(xScale);
    const yAxis = d3.axisLeft(yScale);

    // 添加X轴
    svg.append('g')
        .attr('transform', `translate(0, ${height})`)
        .call(xAxis)
        .style('font-size', '36px')
        .append('text')
        .attr('class', 'label')
        .attr('x', width)
        .attr('y', -6)
        .style('text-anchor', 'end')
        .text('Emoji')
        .selectAll('.tick text') // 选择x轴上的刻度文本

    // 添加Y轴
    svg.append('g')
        .call(yAxis)
        .style('font-size', '20px')
        .selectAll('.tick text') // 选择Y轴上的刻度文本
        .on('click', function(event, ageGroup) {
    // 获取当前点击的年龄段
        const selectedAgeGroup = ageGroup;

    // 过滤数据以获取特定年龄段的数据
        const filteredData = bubbleData.filter(d => d.AgeGroup === selectedAgeGroup);

    // 聚合数据以计算每个表情符号在特定年龄段内不同性别的使用人数
        const emojiCounts = {};
        filteredData.forEach(d => {
        if (!emojiCounts[d.Emoji]) {
            emojiCounts[d.Emoji] = { Male: 0, Female: 0 };
        }
            emojiCounts[d.Emoji][d.UserGender] += d.count;
        });

    // 将聚合数据转换为数组
    const aggregatedData = Object.keys(emojiCounts).map(emoji => ({
        Emoji: emoji,
        Male: emojiCounts[emoji].Male,
        Female: emojiCounts[emoji].Female
    }));

    // 创建浮窗
    const tooltipWidth = 1200;
    const tooltipHeight = 600;
    const tooltipMargin = 30;

    // 检查浮窗是否存在
    let tooltip = d3.select('.tooltip');
    if (tooltip.empty()) {
        tooltip = d3.select('#sidebar').append('div')
            .attr('class', 'tooltip')
            .style('position', 'absolute')
            .style('background-color', 'white')
            .style('border', '1px solid black')
            .style('padding', '10px')
            .style('pointer-events', 'none');

            const pageWidth = window.innerWidth;
            const pageHeight = window.innerHeight;
            const tooltipX = (pageWidth - tooltipWidth) / 2;
            const tooltipY = (pageHeight - tooltipHeight) / 2;

            tooltip.style('left', `${tooltipX}px`)
                .style('top', tooltipY+'px')
    } 
    // else {
    //     // 如果浮窗存在，则切换显示状态
    //     if (tooltip.style('display') === 'none') {
    //         tooltip.style('display', 'block');
    //     } else {
    //         tooltip.style('display', 'none');
    //     }
    //     return;
    // }

    tooltip.html('');

    // 创建浮窗中的SVG
    const tooltipSvg = tooltip.append('svg')
        .attr('width', tooltipWidth)
        .attr('height', tooltipHeight);

    // 创建比例尺
    const xTooltipScale = d3.scaleBand()
        .domain(aggregatedData.map(d => d.Emoji))
        .range([0, tooltipWidth - 2 * tooltipMargin])
        .paddingInner(0.1)
        .paddingOuter(0.5);

    // 子比例尺用于处理每个年龄段内的条形图位置
    const subXTooltipScale = d3.scaleBand()
        .domain(['Male', 'Female'])
        .range([0, xTooltipScale.bandwidth()])
        .paddingInner(0.1)
        .paddingOuter(0.3);

    const yTooltipScale = d3.scaleLinear()
        .domain([0, d3.max(aggregatedData, d => Math.max(d.Male, d.Female))])
        .nice()
        .range([tooltipHeight - 2 * tooltipMargin, 0]);

    // 创建X轴和Y轴
    const xAxisTooltip = d3.axisBottom(xTooltipScale);
    const yAxisTooltip = d3.axisLeft(yTooltipScale);

    tooltipSvg.append('g')
        .attr('transform', `translate(${tooltipMargin}, ${tooltipHeight - tooltipMargin})`)
        .call(xAxisTooltip)
        .style('font-size', '16px')
        .style('color','black');

    tooltipSvg.append('g')
        .attr('transform', `translate(${tooltipMargin}, ${tooltipMargin})`)
        .call(yAxisTooltip)
        .style('font-size', '16px')
        .style('color','black');

    // 绘制条形图
    tooltipSvg.selectAll('.bar-group')
    .data(aggregatedData)
    .enter()
    .append('g')
    .attr('class', 'bar-group')
    .attr('transform', d => `translate(${xTooltipScale(d.Emoji) + tooltipMargin}, 0)`) // 移动到正确的X位置
    .selectAll('.bar')
    .data(d => [
        { gender: 'Male', value: d.Male },
        { gender: 'Female', value: d.Female }
    ])
    .enter()
    .append('rect')
    .attr('class', d => `bar ${d.gender}`)
    .attr('x', d => subXTooltipScale(d.gender)) // 使用子比例尺确定X位置
    .attr('y', tooltipHeight - tooltipMargin)
    .attr('width', subXTooltipScale.bandwidth())
    .attr('height', 0)
    .attr('fill', d => color(d.gender))
    .on('mouseover', function(event, d) {
        d3.select(this)
            .transition()
            .duration(200)
            .attr('fill', 'orange'); // 鼠标移上去时变为橙色
    })
    .on('mouseout', function(event, d) {
        d3.select(this)
            .transition()
            .duration(200)
            .attr('fill', color(d.UserGender)); // 鼠标移开后恢复原色
    })
    .transition()
    .delay((d,i) =>i * 50)
    .duration(500)
    .attr('y', d => yTooltipScale(d.value) + tooltipMargin) // 将Y轴位置重置为0，然后动画到正确的位置
    .attr('height', d => tooltipHeight - 2 * tooltipMargin - yTooltipScale(d.value))
    .append('title') // 添加悬停提示
    .text(d => `Gender: ${d.gender}\nCount: ${d.value}`);

        const legendData = ['Male', 'Female'];
        const legend = tooltipSvg.selectAll('.legend')
            .data(legendData).enter()
            .append('g')
            .attr('class', 'legend')
            .attr('transform', function(d, i) { return `translate(${tooltipWidth - 120},${i * 20 + 10})`; });

        legend.append('rect')
            .attr('x', 0)
            .attr('y', 0)
            .attr('width', 18)
            .attr('height', 18)
            .style('fill', color);

        legend.append('text')
            .attr('x', 20)
            .attr('y', 15)
            .text(d => d);

    // 显示浮窗
    tooltip.style('left', event.pageX + tooltipMargin + 'px')
           .style('top', event.pageY + tooltipMargin + 'px');
})
.append('text')
.attr('class', 'label')
.attr('transform', 'rotate(-90)')
.attr('y', 6)
.attr('dy', '.71em')
.style('text-anchor', 'end')
.text('Age Group');

    // 统计相同表情、相同年龄段和相同性别的数据点的个数
    const countMap = {};
    data.forEach(d => {
        const key = `${d.Emoji}+${d.AgeGroup}+${d.UserGender}`;
        if (!countMap[key]) {
            countMap[key] = 0;
        }
        countMap[key]++;
    });

    // 打印统计结果
    console.log('Count of each Emoji, Age Group, and Gender combination:', countMap);

    // 创建新的比例尺，根据 countMap 中的最大计数值来设置半径
    const maxCount = d3.max(Object.values(countMap));
    const rScale = d3.scaleSqrt()
        .domain([0, maxCount])
        .range([0, 17]); // 半径范围从0到17

    // 创建气泡数据并按count降序排序
    const bubbleData = Object.keys(countMap).map(key => {
        const [Emoji, AgeGroup, UserGender] = key.split('+');
        return { Emoji, AgeGroup, UserGender, count: countMap[key] };
    }).sort((a, b) => a.count - b.count); // 按count降序排序
    // 创建一个 D3 饼图生成器
const pie = d3.pie()
.value(d => d.value);  // 使用 value 属性来计算每一块的大小

// 创建一个弧生成器（arc），用于绘制饼图
const arc = d3.arc()
.innerRadius(0)  // 内半径为 0，表示绘制实心饼图
.outerRadius(d => rScale(d.data.total));  // 外半径根据数据的 count 来设置

    // 创建气泡
    svg.selectAll('.arc')
    .data(bubbleData)
    .enter()
    .append('g')
    .attr('class', 'arc')
    .attr('transform', d => {
        // 计算每个饼图的位置，根据 AgeGroup 和 Emoji 的位置来决定
        const x = xScale(d.Emoji) + xScale.bandwidth() / 2;
        const y = yScale(d.AgeGroup) + yScale.bandwidth() / 2;
        return `translate(${x}, ${y})`;  // 使每个饼图位于对应的 (x, y) 坐标
    })
    .each(function(d) {
        const g = d3.select(this);

        // 创建每个 emoji 和年龄段的性别数据
        const genderData = [
            { gender: 'Male', value: bubbleData.filter(b => b.Emoji === d.Emoji && b.AgeGroup === d.AgeGroup && b.UserGender === 'Male').reduce((sum, b) => sum + b.count, 0) },
            { gender: 'Female', value: bubbleData.filter(b => b.Emoji === d.Emoji && b.AgeGroup === d.AgeGroup && b.UserGender === 'Female').reduce((sum, b) => sum + b.count, 0) }
        ];
        // 计算该 emoji 和年龄段的总人数（男生和女生的总和）
        const totalCount = bubbleData.filter(b => b.Emoji === d.Emoji && b.AgeGroup === d.AgeGroup)
            .reduce((sum, b) => sum + b.count, 0);
        // 将性别数据添加 total 字段，用于设置饼图的总人数（半径）
        genderData.forEach(d => d.total = totalCount);

        // 生成饼图的数据
        const pieData = pie(genderData);

        // 绘制每个饼图的路径
        g.selectAll('path')
            .data(pieData)
            .enter()
            .append('path')
            .attr('d', arc)  // 绘制饼图路径
            .attr('fill', d => color(d.data.gender))  // 根据性别选择颜色
            .style('stroke', 'white')
            .style('stroke-width', 1)
            .on('mouseover', function(event, d) {
                d3.select(this)
                    .transition()
                    .duration(200)
                    .attr('fill', 'orange');  // 鼠标悬停时变色
            })
            .on('mouseout', function(event, d) {
                d3.select(this)
                    .transition()
                    .duration(200)
                    .attr('fill', color(d.data.gender));  // 鼠标移开时恢复颜色
            })
            .append('title') // 添加悬停提示
            .text(d => `Emoji: ${d.Emoji}\nAge Group: ${d.AgeGroup}\nGender: ${d.data.gender}\nCount: ${d.data.value}`);
    });
        
        
    // 添加图例
    const legendData = ['Male', 'Female'];
    const legend = svg.selectAll('.legend')
        .data(legendData)
        .enter().append('g')
        .attr('class', 'legend')
        .attr('transform', function(d, i) { return `translate(0,${i * 20})`; });

    legend.append('rect')
        .attr('x', width - 18)
        .attr('width', 18)
        .attr('height', 18)
        .style('fill', color)
        .style('stroke', 'white');

    legend.append('text')
        .attr('x', width - 24)
        .attr('y', 9)
        .attr('dy', '.35em')
        .style('text-anchor', 'end')
        .text(d => d)
        .style('fill','white')

}).catch(function (error) {
    console.log(error); // 打印错误信息
});