//1——生源分布
const data = [
    { location: "基点", count: 0 ,Y:22},
    { location: "广东", count: 3 ,Y:23},
    { location: "云南", count: 1 ,Y:25},
    { location: "福建", count: 5 ,Y:26},
    { location: "贵州", count: 1 ,Y:27}, 
    { location: "湖南", count: 1 ,Y:28},
    { location: "江西", count: 5 ,Y:28.5},
    { location: "重庆", count: 3 ,Y:29.5},
    { location: "四川", count: 6 ,Y:30.6},
    { location: "湖北", count: 6 ,Y:31}, 
    { location: "安徽", count: 3 ,Y:31.5},
    { location: "江苏", count: 5 ,Y:32},
    { location: "河南", count: 13 ,Y:34.3},
    { location: "山东", count: 9 ,Y:36.3},
    { location: "山西", count: 2 ,Y:37.5},
    { location: "河北", count: 6 ,Y:38},
    { location: "天津", count: 15,Y:39}, 
    { location: "北京", count: 1 ,Y:39.5},
    { location: "内蒙古", count: 6 ,Y:40.8},
    { location: "辽宁", count: 4 ,Y:41.5},
    { location: "吉林", count: 1 ,Y:43.4},
    { location: "黑龙江", count: 1 ,Y:45},
    { location: "海外", count: 3 ,Y:46}
];

// 聚类处理
function clusterDataF(data, clusterStep) {
    const clusteredData = [];
    let currentCluster = null;
  
    data.forEach(d => {
      // 排除“海外”点
      if (d.location === "海外") {
        clusteredData.push({ ...d, key: d.Y, range: [d.Y, d.Y],cy: d.Y });
        return;
      }
      if (d.location === "黑龙江") {
        clusteredData.push({ ...d, key: d.Y, range: [d.Y, d.Y+1],cy: d.Y });
        return;
      }
      if (d.location === "基点") {
        clusteredData.push({ ...d, key: d.Y, range: [d.Y, d.Y],cy: d.Y });
        return;
      }
  
      const clusterKey = Math.floor(d.Y / clusterStep) * clusterStep;
      const cy=(clusterKey+clusterKey + clusterStep)/2;
      if (!currentCluster || currentCluster.key !== clusterKey) {
        currentCluster = { key: clusterKey, count: 0, range: [clusterKey, clusterKey + clusterStep] ,cy:cy};
        clusteredData.push(currentCluster);
      }
      currentCluster.count += d.count;
    });
  
    return clusteredData;
  }
  
  // 使用2.5作为聚类步长
  const clusterStep = 2.5;
  const clusteredData = clusterDataF(data, clusterStep);
  

const margin = { top: 20, right: 30, bottom: 30, left: 40 };
const width = 500 - margin.left - margin.right;
const height = 700 - margin.top - margin.bottom;

const svg = d3.select("#chart").append("svg")
    .attr("width", width + margin.left + margin.right+400)
    .attr("height", height + margin.top + margin.bottom)
  .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

const x = d3.scaleLinear()
    .range([width,0])
    .domain([0, d3.max(clusteredData, d => d.count) + 5]);

const y = d3.scaleLinear()
    .range([height, 0])
    .domain([d3.min(data,d => d.Y),d3.max(data,d => d.Y)]);

const xAxis = d3.axisTop(x);
const yAxis = d3.axisRight(y).tickFormat("").tickSize(0);

svg.append("g")
    .attr("class", "x axis")
    .attr("transform", "translate(0,0)")
    .call(xAxis);

svg.append("g")
    .attr("class","y axis" )
    .attr("transform", "translate("+width+",0)")
    .call(yAxis);

// 颜色梯度
const gradient = svg.append("defs")
    .append("linearGradient")
    .attr("id", "gradient")
    .attr("x1", "0%")
    .attr("y1", "0%")
    .attr("x2", "0%")
    .attr("y2", "100%");

gradient.append("stop")
    .attr("offset", "0%")
    .attr("stop-color", "blue")
    .attr("stop-opacity", 0.5);

gradient.append("stop")
    .attr("offset", "100%")
    .attr("stop-color", "red")
    .attr("stop-opacity", 0.8);

// 面积图
const area = d3.area()
    .x1(d => x(d.count))
    .x0(width)
    .y(d => y(d.cy))
    .curve(d3.curveMonotoneY);

svg.append("path")
    .datum(clusteredData)
    .attr("class", "area")
    .attr("d", area);

 svg.selectAll(".cluster-point")
    .data(clusteredData)
    .enter()
    .append("circle")
    .attr("class", "cluster-point")
    .attr("cx", d => x(d.count))
    .attr("cy", d => y(d.cy))
    .attr("r", 3) // 设置聚类点的半径
    .style("fill", "black") // 设置聚类点的填充颜色
    .style("stroke", "white") // 设置聚类点的边框颜色
    .style("stroke-width", "1px"); // 设置聚类点的边框宽度

svg.selectAll(".label")
    .data(clusteredData)
    .enter()
    .append("text")
    .attr("class", "label")
    .attr("x", d => x(d.count)+15)
    .attr("y", d => y(d.cy))
    .attr("dy", "-0.5em")
    .attr("text-anchor", "end")
    .style("font-size", "12px") 
    .text(d =>{
        if (d.location === "海外"||d.location === "基点") {
        return ""; // 如果位置是“海外”“基点”，则不显示文字
      }
      return `纬度: [${d.range[0]},${d.range[1]}),人数: ${d.count}`
    } )
   
//将聚类点与各地点连接起来
clusteredData.forEach(d => {
    const range1=d.range[0];
    const range2=d.range[1];
    const cdataX = x(d.count);
    const cdataY = y(d.cy);
    data.forEach(d=>{
        if(d.Y>=range1&&d.Y<range2&&d.location!="海外")
        {
      // 计算数据点在图表中的位置
      const dataX = width;
      const dataY = y(d.Y);
      // 创建一条贝塞尔曲线连接当前数据点和聚类点
      const pathData = `
        M${dataX},${dataY}
        Q${(dataX + cdataX) / 2},${dataY}
        ${cdataX},${cdataY}
      `;
      // 将路径添加到 SVG 中
      svg.append("path")
        .attr("d", pathData)
        .attr("fill", "none")
        .attr("stroke", "white")
        .attr("opacity", 0.5); 
        }
    })
  });



// 右侧南开点
const tianjinDatum = data.find(d => d.location === "天津");
const tianjinX = 750; 
const tianjinY = y(tianjinDatum.Y)-50;

//线宽的比例尺，将人数映射到线宽
const strokeWidthScale = d3.scaleLinear()
  .domain([d3.min(data, d => d.count), d3.max(data, d => d.count)])
  .range([3, 10]); 

// 遍历数据点，对于每个非零计数的数据点，绘制一条曲线连接到南开，并标注地名
data.forEach(d => {
    if (d.count > 0) {
      // 计算数据点在图表中的位置
      const dataX = width;
      const dataY = y(d.Y);
  
      // 创建一条贝塞尔曲线连接当前数据点和天津圆圈
      const pathData = `
        M${dataX},${dataY}
        Q${(dataX + tianjinX) / 2},${dataY}
        ${tianjinX},${tianjinY}
      `;
  
      // 将路径添加到 SVG 中
      svg.append("path")
        .attr("d", pathData)
        .attr("fill", "none")
        .attr("stroke", "url(#gradient)")
        .attr("stroke-width", strokeWidthScale(d.count))
        .attr("opacity", 0.4); 

    //标注地名
    svg.append("text")
    .attr("x", dataX) 
    .attr("y", dataY+2) 
    .text(d.location) 
    .attr("font-family", "sans-serif") 
    .attr("font-size", "11px") 
    .attr("fill", "black"); 
    }
  });

// 后画右侧南开点
svg.append("circle")
    .attr("cx", tianjinX)
    .attr("cy", tianjinY)
    .attr("r", 5)
    .attr("fill", "blue");
//添加“南开大学”字样
svg.append("text")
    .attr("x", tianjinX + 10) 
    .attr("y", tianjinY + 5) 
    .text("南开大学") 
    .attr("font-family", "sans-serif") 
    .attr("font-size", "15px") 
    .attr("fill", "black"); 





//2——优化
// 数据
const data2 = [
    { year: "2019", urban_population: 8.48, rural_population: 5.516 },
    { year: "2020", urban_population: 9.01, rural_population: 5.098 },
    { year: "2021", urban_population: 9.14, rural_population: 4.98 },
    { year: "2022", urban_population: 9.207, rural_population: 4.91 },
    { year: "2023", urban_population: 9.327, rural_population: 4.77 }
];

// 设置画布尺寸
const margin2 = { top: 20, right: 30, bottom: 40, left: 90 };
const width2 = 1000 - margin2.left - margin2.right;
const height2 = 500 - margin2.top - margin2.bottom;

// 创建 SVG 元素
const svg2 = d3.select('#chart-container')
    .append("svg")
    .attr("width", width2 + margin2.left + margin2.right+200)
    .attr("height", height2 + margin2.top + margin2.bottom)
    .append("g")
    .attr("transform", `translate(${margin2.left},${margin2.top})`);

// x 轴比例尺
const xScale = d3.scaleLinear()
    .domain([0, 16])
    .range([0, width2]);

// y 轴比例尺
const yScale = d3.scaleBand()
    .domain(data2.map(d => d.year))
    .range([0, height2])
    .padding(0.1);

// 添加 x 轴
svg2.append("g")
    .attr("transform", `translate(0,${height2})`)
    .call(d3.axisBottom(xScale));

// 添加 y 轴
svg2.append("g")
    .call(d3.axisLeft(yScale));

// 绘制城市人口条形图
svg2.selectAll(".bar")
    .data(data2)
    .enter()
    .append("rect")
    .attr("class", "bar")
    .attr("x", 0.5)
    .attr("y", d => yScale(d.year))
    .attr("width", d => xScale(d.urban_population))
    .attr("height", yScale.bandwidth())
    .on('mouseover', function(event, d){
        // 显示详细信息
    const [x, y] = d3.pointer(event);
    d3.select(this).style('fill', 'orange');
    
    svg2.append('text')
      .attr('id', 'tooltip')
      .attr('x', x)
      .attr('y', y)
      .text(`Urban Population: ${d.urban_population}亿`);
  })
  .on('mouseout', function() {
    // 隐藏详细信息
    d3.select(this).style('fill', '');
    d3.select('#tooltip').remove();
    });

// 绘制农村人口条形图
svg2.selectAll(".bar2")
    .data(data2)
    .enter()
    .append("rect")
    .attr("class", "bar2")
    .attr("x", d => xScale(d.urban_population)+0.5)
    .attr("y", d => yScale(d.year))
    .attr("width", d => xScale(d.rural_population))
    .attr("height", yScale.bandwidth())
    .on('mouseover', function(event, d){
        // 显示详细信息
    const [x, y] = d3.pointer(event);
    d3.select(this).style('fill', 'orange');
    
    svg2.append('text')
      .attr('id', 'tooltip')
      .attr('x', x)
      .attr('y', y)
      .text(`Rural Population: ${d.rural_population}亿`);
  })
  .on('mouseout', function() {
    // 隐藏详细信息
    d3.select(this).style('fill', '');
    d3.select('#tooltip').remove();
    });




// 添加图例
const legend = svg2.append("g")
    .attr("class", "legend")
    .attr("transform", `translate(${width2 + margin2.right - 10},${margin2.top})`);

// 添加城市人口图例
legend.append("rect")
    .attr("x", 0)
    .attr("y", 0)
    .attr("width", 18)
    .attr("height", 18)
    .style("fill", "red");

legend.append("text")
    .attr("x", 24)
    .attr("y", 9)
    .attr("dy", ".35em")
    .style("text-anchor", "start")
    .text("Urban Population");

// 添加乡村人口图例
legend.append("rect")
    .attr("x", 0)
    .attr("y", 30)
    .attr("width", 18)
    .attr("height", 18)
    .style("fill", "slategray");

legend.append("text")
    .attr("x", 24)
    .attr("y", 39)
    .attr("dy", ".35em")
    .style("text-anchor", "start")
    .text("Rural Population");
