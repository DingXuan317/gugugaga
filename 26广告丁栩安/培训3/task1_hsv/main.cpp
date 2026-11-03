#include <opencv2/opencv.hpp>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {
bool hasFlag(int argc, char** argv, const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (flag == argv[i]) {
            return true;
        }
    }
    return false;
}
}  // namespace

int main(int argc, char** argv) {
    const std::string imagePath = (argc > 1) ? argv[1] : "assets/armor.jpg";
    const std::string outputDir = (argc > 2) ? argv[2] : ".";
    const bool showWindows = !hasFlag(argc, argv, "--no-display");

    cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::cerr << "Failed to read image: " << imagePath << '\n';
        return 1;
    }

    std::filesystem::create_directories(outputDir);

    cv::Mat hsv;
    cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);

    cv::imwrite(outputDir + "/h_channel.png", channels[0]);
    cv::imwrite(outputDir + "/s_channel.png", channels[1]);
    cv::imwrite(outputDir + "/v_channel.png", channels[2]);

    // 红色在 HSV 中跨越 0 度，需要两段阈值
    cv::Mat lowerRed;
    cv::Mat upperRed;
    cv::inRange(hsv, cv::Scalar(0, 90, 70), cv::Scalar(12, 255, 255), lowerRed);
    cv::inRange(hsv, cv::Scalar(168, 90, 70), cv::Scalar(180, 255, 255), upperRed);

    cv::Mat redMask;
    cv::bitwise_or(lowerRed, upperRed, redMask);

    const cv::Mat closeKernel =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    const cv::Mat openKernel =
        cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(redMask, redMask, cv::MORPH_CLOSE, closeKernel);
    cv::morphologyEx(redMask, redMask, cv::MORPH_OPEN, openKernel);

    const std::string maskPath = outputDir + "/red_mask.png";
    cv::imwrite(maskPath, redMask);

    std::cout << "Image: " << imagePath << " (" << image.cols << "x" << image.rows << ")\n";
    std::cout << "H/S/V channels saved to: " << outputDir << '\n';
    std::cout << "Red mask saved: " << maskPath << '\n';
    std::cout << "Threshold: H 0-12 or 168-180, S 90-255, V 70-255\n";

    if (showWindows) {
        cv::imshow("Original", image);
        cv::imshow("H Channel", channels[0]);
        cv::imshow("S Channel", channels[1]);
        cv::imshow("V Channel", channels[2]);
        cv::imshow("Red Mask", redMask);
        std::cout << "Press any key in an image window to exit...\n";
        cv::waitKey(0);
        cv::destroyAllWindows();
    }

    return 0;
}
