#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""滑动窗口最大值算法。

"""

from abc import ABC, abstractmethod
from collections import deque
from typing import Deque, List, Optional, Sequence

# pylint: disable=too-few-public-methods


class WindowMaximumAlgorithm(ABC):
    """滑动窗口最大值算法的抽象基类。"""

    @abstractmethod
    def solve(self, nums: Sequence[int], window_size: int) -> List[int]:
        """返回所有大小为 ``window_size`` 的窗口最大值。"""


class DequeWindowMaximumAlgorithm(WindowMaximumAlgorithm):
    """使用单调队列计算滑动窗口最大值。"""

    def solve(self, nums: Sequence[int], window_size: int) -> List[int]:
        """返回所有滑动窗口的最大值。

        参数：
            nums: 待扫描的整数序列。
            window_size: 正整数窗口大小，不能大于 nums 的长度。

        返回：
            一个列表，包含每个滑动窗口中的最大值。
        """
        window_indices: Deque[int] = deque()
        maximum_values: List[int] = []

        for index, value in enumerate(nums):
            expired_index = index - window_size
            if window_indices and window_indices[0] <= expired_index:
                window_indices.popleft()

            while window_indices and nums[window_indices[-1]] <= value:
                window_indices.pop()
            window_indices.append(index)

            if index >= window_size - 1:
                maximum_values.append(nums[window_indices[0]])

        return maximum_values


def validate_input(nums: Sequence[int], window_size: int) -> None:
    """校验滑动窗口最大值问题的输入参数。

    参数：
        nums: 待扫描的整数序列。
        window_size: 正整数窗口大小。

    异常：
        TypeError: 参数类型不符合题目定义时抛出。
        ValueError: ``window_size`` 超出合法范围时抛出。
    """
    if isinstance(nums, (str, bytes)) or not isinstance(nums, Sequence):
        raise TypeError("nums must be a sequence of integers")

    if isinstance(window_size, bool) or not isinstance(window_size, int):
        raise TypeError("window_size must be an integer")

    if window_size <= 0:
        raise ValueError("window_size must be positive")

    if window_size > len(nums):
        raise ValueError("window_size must not be greater than nums length")

    for value in nums:
        if isinstance(value, bool) or not isinstance(value, int):
            raise TypeError("nums must contain only integers")


def sliding_window_maximum(
    nums: Sequence[int],
    window_size: int,
    algorithm: Optional[WindowMaximumAlgorithm] = None,
) -> List[int]:
    """返回每个滑动窗口的最大值。

    参数：
        nums: 待扫描的整数序列。
        window_size: 正整数窗口大小。
        algorithm: 可选算法策略。省略时使用 O(n) 的单调队列实现。

    返回：
        从左到右所有窗口的最大值。
    """
    validate_input(nums, window_size)
    solver = algorithm or DequeWindowMaximumAlgorithm()
    return solver.solve(nums, window_size)


def main() -> int:
    """从命令行运行一个小示例。"""
    sample_nums = [1, 3, -1, -3, 5, 3, 6, 7]
    sample_window_size = 3
    print(sliding_window_maximum(sample_nums, sample_window_size))
    return 0


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
