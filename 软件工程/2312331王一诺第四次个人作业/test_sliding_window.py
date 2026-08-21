#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""滑动窗口最大值实现的单元测试"""

import unittest
from unittest.mock import patch

from sliding_window import (
    DequeWindowMaximumAlgorithm,
    WindowMaximumAlgorithm,
    main,
    sliding_window_maximum,
    validate_input,
)


class FixedAlgorithm(WindowMaximumAlgorithm):  # pylint: disable=too-few-public-methods
    """用于验证算法扩展点的测试替身。"""

    def __init__(self):
        """初始化捕获到的参数"""
        self.received_nums = None
        self.received_window_size = None

    def solve(self, nums, window_size):
        """返回固定结果，同时记录传入的原始参数"""
        self.received_nums = nums
        self.received_window_size = window_size
        return [42]


class SlidingWindowBaseTestCase(unittest.TestCase):
    """测试基类"""

    def setUp(self):
        """为每个测试方法创建新的算法对象"""
        self.algorithm = DequeWindowMaximumAlgorithm()

    def assert_window_result(self, expected, nums, window_size):
        """同时断言公开函数和单调队列策略的结果"""
        self.assertEqual(expected, sliding_window_maximum(nums, window_size))
        self.assertEqual(expected, self.algorithm.solve(nums, window_size))


class NormalCaseTestCase(SlidingWindowBaseTestCase):
    """正常输入的功能测试"""

    def test_leetcode_sample_case(self):
        """典型混合数值"""
        nums = [1, 3, -1, -3, 5, 3, 6, 7]
        self.assert_window_result([3, 3, 5, 5, 6, 7], nums, 3)

    def test_mixed_values_with_window_size_two(self):
        """正负混合数值+较小窗口"""
        self.assert_window_result(
            [10, 10, 9, -4, -4, 2, 2],
            [9, 10, 9, -7, -4, -8, 2, -6],
            2,
        )

    def test_mixed_values_with_window_size_five(self):
        """混合数值+较大窗口"""
        self.assert_window_result([10, 10, 9, 2], [9, 10, 9, -7, -4, -8, 2, -6], 5)

    def test_late_large_value_updates_all_following_windows(self):
        """末尾较大值"""
        nums = [2, 1, 3, 4, 6, 3, 8, 9, 10, 12, 56]
        self.assert_window_result([4, 6, 6, 8, 9, 10, 12, 56], nums, 4)

    def test_alternating_peak_values(self):
        """交替峰值"""
        self.assert_window_result([5, 5, 5, 5, 5], [1, 5, 1, 5, 1, 5], 2)

    def test_zero_values_mixed_with_positive_values(self):
        """零值混合"""
        self.assert_window_result([0, 2, 2, 1], [0, -1, 2, 0, 1], 2)

    def test_repeated_late_maximum_values(self):
        """重复最大值进入后续窗口"""
        self.assert_window_result([4, 5, 5, 5, 5], [4, 1, 3, 5, 5, 2, 1], 3)


class BoundaryCaseTestCase(SlidingWindowBaseTestCase):
    """合法输入边界的测试"""

    def test_window_size_one_returns_original_values(self):
        """窗口大小为一时应原样返回每个值"""
        nums = [4, -2, 8, 8]
        self.assert_window_result(nums, nums, 1)

    def test_window_size_one_with_single_value(self):
        """单元素序列搭配窗口大小一是合法输入"""
        self.assert_window_result([7], [7], 1)

    def test_window_size_equals_length_returns_single_maximum(self):
        """窗口大小等于序列长度时只返回整体最大值"""
        self.assert_window_result([9], [2, 9, 1, 7], 4)

    def test_window_size_equals_length_with_all_negative_values(self):
        """全负数输入+全长窗口"""
        self.assert_window_result([-2], [-5, -9, -2, -7], 4)

    def test_window_size_is_length_minus_one(self):
        """接近全长的窗口"""
        self.assert_window_result([8, 8], [1, 8, 3, 2], 3)

    def test_two_values_with_full_window(self):
        """最小的多元素全长窗口"""
        self.assert_window_result([2], [2, 1], 2)

    def test_tuple_input_is_accepted(self):
        """接受任意整数序列"""
        self.assert_window_result([6, 3], (6, 1, 3), 2)

    def test_range_input_is_accepted(self):
        """范围对象也是合法的整数序列输入"""
        self.assert_window_result([2, 3, 4], range(5), 3)


class SequencePatternTestCase(SlidingWindowBaseTestCase):
    """特殊数值序列模式的测试。"""

    def test_increasing_sequence_with_window_size_three(self):
        """递增输入应得到每个窗口最右侧的值。"""
        self.assert_window_result([3, 4, 5], [1, 2, 3, 4, 5], 3)

    def test_increasing_sequence_with_window_size_two(self):
        """递增输入+较小窗口"""
        self.assert_window_result([2, 3, 4, 5], [1, 2, 3, 4, 5], 2)

    def test_decreasing_sequence_with_window_size_three(self):
        """递减输入应得到每个窗口最左侧的值"""
        self.assert_window_result([5, 4, 3], [5, 4, 3, 2, 1], 3)

    def test_decreasing_sequence_with_window_size_two(self):
        """递减输入+较小窗口"""
        self.assert_window_result([5, 4, 3, 2], [5, 4, 3, 2, 1], 2)

    def test_all_equal_values(self):
        """所有值相等时每个窗口最大值应相同"""
        self.assert_window_result([5, 5, 5], [5, 5, 5, 5], 2)

    def test_duplicate_values_are_supported(self):
        """重复最大值"""
        self.assert_window_result([5, 5, 5], [5, 5, 5, 1, 5], 3)

    def test_plateau_then_drop(self):
        """平台最大值"""
        self.assert_window_result([7, 7, 7, 6], [7, 7, 7, 6, 5, 4], 3)

    def test_negative_numbers_are_supported(self):
        """负整数"""
        self.assert_window_result([-1, -1, -2], [-4, -1, -3, -2], 2)

    def test_all_negative_decreasing_values(self):
        """递减负数序列"""
        self.assert_window_result([-1, -2, -3], [-1, -2, -3, -4], 2)

    def test_all_negative_with_repeated_maximum(self):
        """重复的负数最大值"""
        self.assert_window_result([-2, -2, -2], [-3, -2, -2, -5, -4], 3)


class InvalidInputTestCase(unittest.TestCase):
    """类型和值校验路径的测试"""

    def assert_invalid_input(self, error_type, nums, window_size):
        """断言非法参数会抛出预期异常类型。"""
        with self.assertRaises(error_type):
            validate_input(nums, window_size)
        with self.assertRaises(error_type):
            sliding_window_maximum(nums, window_size)

    def test_non_sequence_integer_nums_raises_type_error(self):
        """整数输入不是合法序列"""
        self.assert_invalid_input(TypeError, 123, 1)

    def test_none_nums_raises_type_error(self):
        """空值输入不是合法序列"""
        self.assert_invalid_input(TypeError, None, 1)

    def test_string_nums_raises_type_error(self):
        """字符串输入虽然是序列，但不是整数数组"""
        self.assert_invalid_input(TypeError, "123", 1)

    def test_bytes_nums_raises_type_error(self):
        """字节串输入"""
        self.assert_invalid_input(TypeError, b"123", 1)

    def test_non_integer_float_element_raises_type_error(self):
        """每个输入值都必须是整数"""
        self.assert_invalid_input(TypeError, [1, 2.5, 3], 2)

    def test_non_integer_string_element_raises_type_error(self):
        """字符串元素"""
        self.assert_invalid_input(TypeError, [1, "2", 3], 2)

    def test_bool_element_raises_type_error(self):
        """布尔值不应作为整数被接受"""
        self.assert_invalid_input(TypeError, [1, True, 3], 2)

    def test_non_integer_float_window_size_raises_type_error(self):
        """窗口大小必须是整数"""
        self.assert_invalid_input(TypeError, [1, 2, 3], 2.0)

    def test_string_window_size_raises_type_error(self):
        """字符串窗口大小"""
        self.assert_invalid_input(TypeError, [1, 2, 3], "2")

    def test_bool_window_size_raises_type_error(self):
        """布尔类型窗口大小"""
        self.assert_invalid_input(TypeError, [1, 2, 3], True)

    def test_zero_window_size_raises_value_error(self):
        """零不在合法窗口大小范围内"""
        self.assert_invalid_input(ValueError, [1, 2, 3], 0)

    def test_negative_window_size_raises_value_error(self):
        """负数窗口大小"""
        self.assert_invalid_input(ValueError, [1, 2, 3], -1)

    def test_window_larger_than_nums_raises_value_error(self):
        """窗口大小不能大于输入序列长度"""
        self.assert_invalid_input(ValueError, [1, 2, 3], 4)

    def test_empty_nums_with_positive_window_raises_value_error(self):
        """空输入无法满足正数窗口大小"""
        self.assert_invalid_input(ValueError, [], 1)


class ExtensionAndEntryPointTestCase(unittest.TestCase):
    """扩展接口和命令行入口的测试"""

    def test_custom_algorithm_is_supported(self):
        """公开函数允许接入新的算法策略"""
        algorithm = FixedAlgorithm()
        result = sliding_window_maximum([1, 2, 3], 2, algorithm)
        self.assertEqual([42], result)
        self.assertEqual([1, 2, 3], algorithm.received_nums)
        self.assertEqual(2, algorithm.received_window_size)

    def test_custom_algorithm_receives_tuple_input(self):
        """自定义算法应收到校验后的原始序列"""
        algorithm = FixedAlgorithm()
        result = sliding_window_maximum((3, 1, 2), 2, algorithm)
        self.assertEqual([42], result)
        self.assertEqual((3, 1, 2), algorithm.received_nums)
        self.assertEqual(2, algorithm.received_window_size)

    @patch("builtins.print")
    def test_main_prints_sample_result(self, mock_print):
        """命令行示例应打印样例结果"""
        self.assertEqual(0, main())
        mock_print.assert_called_once_with([3, 3, 5, 5, 6, 7])


if __name__ == "__main__":  # pragma: no cover
    unittest.main()
