import numpy as np
data = np.loadtxt('iris.csv', delimiter=',', encoding='utf-8',skiprows=2)

print(data.shape)
data2 = data[:, :4]
print(data2)

data3 = data2[::2, :]
data4 = data2[1::2, :]
print(data3)
print(data4)

data45 = data4[data4[:,0]>3]
data5 = data45[data45[:, -1]>1.3]
print(data5)

data22 = np.vstack((data3, data4))
print(data22)

data33 = data3 + [0.1,0.2,0.1,0.2]
print(data33)

data6 = data[data[:, -1]==0]
def process6(arr,col):
    min = np.min(arr[:, col-1])
    print(f"第{col}列最小值：{min}")
    max = np.max(arr[:, col-1])
    print(f"第{col}列最大值：{max}")
    mean = np.mean(arr[:, col-1])
    print(f"第{col}列均值：{mean}")
    median = np.median(arr[:, col-1])
    print(f"第{col}列中位数：{median}")
    var = np.var(arr[:, col-1])
    print(f"第{col}列方差：{var}")
    std = np.std(arr[:, col-1])
    print(f"第{col}列标准差：{std}")
for i in range(4):
    process6(data6,i+1)
