#pragma once

#include <opencv2/opencv.hpp>

#include <array>
#include <optional>

struct LightBar {
    cv::Rect box;
    std::array<cv::Point, 4> corners{};
};

struct ArmorTarget {
    LightBar left;
    LightBar right;
    cv::Rect armorBox;
    std::array<cv::Point, 4> armorCorners{};
};

class ArmorDetector {
public:
    ArmorDetector() = default;

    // 提取红色灯条的二值掩膜
    cv::Mat createRedMask(const cv::Mat& bgr) const;

    // 检测一对灯条，并计算装甲板外接矩形
    std::optional<ArmorTarget> detect(const cv::Mat& bgr) const;
};
