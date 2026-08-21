# 软件工程第四次个人作业代码说明

## 文件

- `sliding_window.py`: 滑动窗口最大值算法实现。
- `test_sliding_window.py`: 基于 `unittest` 的单元测试和测试数据。
- `profile_sliding_window.py`: 基于标准库 `profile` 的性能分析脚本。

## 单元测试设计

`test_sliding_window.py` 使用 `unittest.TestCase` 组织 42 个测试用例，覆盖正常功能、边界条件、特殊序列模式、非法输入、扩展接口和命令行入口。测试断言包括 `assertEqual`、`assertRaises` 和 `mock.patch` 调用验证。

## 运行

```bash
python sliding_window.py
python -m unittest -v test_sliding_window.py
python profile_sliding_window.py
pylint sliding_window.py test_sliding_window.py profile_sliding_window.py
coverage run -m unittest test_sliding_window.py
coverage report
```

算法使用单调队列，每个元素最多入队和出队一次，时间复杂度为 O(n)，额外空间复杂度为 O(k)。
