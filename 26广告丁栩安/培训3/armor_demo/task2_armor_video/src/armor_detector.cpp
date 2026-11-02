#include "armor_detector.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

std::array<cv::Point, 4> rectCorners(const cv::Rect& rect) {
    return {
        rect.tl(),
        cv::Point(rect.x + rect.width - 1, rect.y),
        cv::Point(rect.x + rect.width - 1, rect.y + rect.height - 1),
        cv::Point(rect.x, rect.y + rect.height - 1),
    };
}

cv::Rect boundingRectOf(const std::vector<cv::Point>& points) {
    int minX = points.front().x;
    int minY = points.front().y;
    int maxX = minX;
    int maxY = minY;

    for (const cv::Point& point : points) {
        minX = std::min(minX, point.x);
        minY = std::min(minY, point.y);
        maxX = std::max(maxX, point.x);
        maxY = std::max(maxY, point.y);
    }

    return cv::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

cv::Rect clampRect(const cv::Rect& rect, const cv::Size& size) {
    const int x1 = std::clamp(rect.x, 0, size.width);
    const int y1 = std::clamp(rect.y, 0, size.height);
    const int x2 = std::clamp(rect.x + rect.width, 0, size.width);
    const int y2 = std::clamp(rect.y + rect.height, 0, size.height);
    return cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
}

}  // namespace

cv::Mat ArmorDetector::createRedMask(const cv::Mat& bgr) const {
    cv::Mat hsv;
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);

    // 红色在 HSV 中跨过 0 度，所以需要两段范围
    cv::Mat lowerRed;
    cv::Mat upperRed;
    cv::inRange(hsv, cv::Scalar(0, 90, 70), cv::Scalar(12, 255, 255), lowerRed);
    cv::inRange(hsv, cv::Scalar(168, 90, 70), cv::Scalar(180, 255, 255), upperRed);

    cv::Mat mask;
    cv::bitwise_or(lowerRed, upperRed, mask);

    const cv::Mat closeKernel =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    const cv::Mat openKernel =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, closeKernel);
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, openKernel);
    return mask;
}

std::optional<ArmorTarget> ArmorDetector::detect(const cv::Mat& bgr) const {
    if (bgr.empty()) {
        return std::nullopt;
    }

    const cv::Mat mask = createRedMask(bgr);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    struct Candidate {
        cv::Rect box;
    };
    std::vector<Candidate> bars;
    bars.reserve(contours.size());

    for (const auto& contour : contours) {
        const cv::Rect box = boundingRectOf(contour);
        if (box.width < 4 || box.height < 20) {
            continue;
        }
        if (box.area() < 80) {
            continue;
        }
        // 灯条是竖直的：高度至少是宽度的 1.4 倍
        if (box.height < static_cast<int>(1.4 * box.width)) {
            continue;
        }
        bars.push_back({box});
    }

    if (bars.size() < 2) {
        return std::nullopt;
    }

    std::sort(bars.begin(), bars.end(), [](const Candidate& a, const Candidate& b) {
        return a.box.x < b.box.x;
    });

    std::optional<ArmorTarget> best;
    double bestScore = 1e18;

    for (std::size_t i = 0; i < bars.size(); ++i) {
        for (std::size_t j = i + 1; j < bars.size(); ++j) {
            const cv::Rect& left = bars[i].box;
            const cv::Rect& right = bars[j].box;

            const double h1 = static_cast<double>(left.height);
            const double h2 = static_cast<double>(right.height);
            const double hRatio = std::max(h1, h2) / std::min(h1, h2);
            const double avgHeight = (h1 + h2) * 0.5;
            const double centerY1 = left.y + left.height * 0.5;
            const double centerY2 = right.y + right.height * 0.5;
            const double gap = right.x - (left.x + left.width);

            if (hRatio > 2.0) {
                continue;
            }
            if (std::abs(centerY1 - centerY2) > avgHeight * 0.65) {
                continue;
            }
            if (gap < 15.0 || gap > bgr.cols * 0.55) {
                continue;
            }

            // 两灯条间距大约等于灯条高度的 2~3 倍时最可信
            const double score =
                std::abs(gap - avgHeight * 2.4) +
                std::abs(centerY1 - centerY2) * 0.8;

            if (score < bestScore) {
                bestScore = score;

                const int x1 = std::min(left.x, right.x);
                const int y1 = std::min(left.y, right.y);
                const int x2 = std::max(left.x + left.width, right.x + right.width);
                const int y2 = std::max(left.y + left.height, right.y + right.height);

                cv::Rect armorBox(x1 - 6, y1 - 5, (x2 - x1) + 12, (y2 - y1) + 10);
                armorBox = clampRect(armorBox, bgr.size());

                best = ArmorTarget{
                    LightBar{left, rectCorners(left)},
                    LightBar{right, rectCorners(right)},
                    armorBox,
                    rectCorners(armorBox),
                };
            }
        }
    }

    return best;
}
