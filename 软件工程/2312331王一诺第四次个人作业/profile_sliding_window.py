#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""分析滑动窗口最大值实现的性能"""

from profile import Profile
from pstats import Stats

from sliding_window import sliding_window_maximum


def build_test_data(size):
    """构造包含正负值的确定性基准测试数据"""
    return [(index * 37) % 1000 - 500 for index in range(size)]


def run_benchmark():
    """多次运行算法，以收集稳定的性能分析数据"""
    nums = build_test_data(10000)
    window_size = 128
    for _ in range(100):
        sliding_window_maximum(nums, window_size)


def main():
    """按累计耗时排序并打印性能分析统计信息。"""
    profiler = Profile()
    profiler.runcall(run_benchmark)
    Stats(profiler).strip_dirs().sort_stats("cumtime").print_stats(15)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
