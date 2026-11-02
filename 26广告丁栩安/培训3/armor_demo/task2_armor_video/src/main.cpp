#include "armor_detector.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {
bool hasFlag(int argc, char** argv, const std::string& flag) {
    for (int i = 1; i < argc; ++i) {
        if (flag == argv[i]) {
            return true;
        }
    }
    return false;
}

void drawTarget(cv::Mat& frame, const ArmorTarget& target) {
    const cv::Scalar yellow(0, 255, 255);
    const cv::Scalar green(0, 255, 0);
    const cv::Scalar blue(255, 0, 0);

    // 两个灯条的外接矩形
    cv::rectangle(frame, target.left.box, yellow, 2);
    cv::rectangle(frame, target.right.box, yellow, 2);

    // 每个灯条的四个角点
    for (const cv::Point& point : target.left.corners) {
        cv::circle(frame, point, 4, green, cv::FILLED);
    }
    for (const cv::Point& point : target.right.corners) {
        cv::circle(frame, point, 4, green, cv::FILLED);
    }

    // 连接装甲板四个角点，形成最大的外接矩形
    for (int i = 0; i < 4; ++i) {
        cv::line(frame, target.armorCorners[i],
                 target.armorCorners[(i + 1) % 4], blue, 2);
        cv::circle(frame, target.armorCorners[i], 5, blue, cv::FILLED);
    }

    cv::putText(frame, "Armor", cv::Point(target.armorBox.x, target.armorBox.y - 8),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, blue, 2);
}
}  // namespace

int main(int argc, char** argv) {
    const std::string videoPath =
        (argc > 1) ? argv[1] : "assets/armor_video_5s.mp4";
    const std::string outputPath =
        (argc > 2) ? argv[2] : "output/armor_detected.mp4";
    const bool showVideo = !hasFlag(argc, argv, "--no-display");

    cv::VideoCapture capture(videoPath);
    if (!capture.isOpened()) {
        std::cerr << "Failed to open video: " << videoPath << '\n';
        return 1;
    }

    const double fps = capture.get(cv::CAP_PROP_FPS) > 0
                           ? capture.get(cv::CAP_PROP_FPS)
                           : 30.0;
    const int width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    const int height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    const int totalFrames = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT));

    std::filesystem::path outputFile(outputPath);
    if (outputFile.has_parent_path()) {
        std::filesystem::create_directories(outputFile.parent_path());
    }

    cv::VideoWriter writer(outputPath, cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                           fps, cv::Size(width, height));
    if (!writer.isOpened()) {
        std::cerr << "Failed to create output video: " << outputPath << '\n';
        return 1;
    }

    std::cout << "Input: " << videoPath << " (" << width << "x" << height
              << ", " << totalFrames << " frames, " << fps << " fps)\n";

    ArmorDetector detector;
    cv::Mat frame;
    int frameIndex = 0;
    int detectedFrames = 0;
    bool savedResultImage = false;

    while (capture.read(frame)) {
        cv::Mat canvas = frame.clone();
        const auto target = detector.detect(frame);

        if (target.has_value()) {
            drawTarget(canvas, *target);
            ++detectedFrames;

            if (!savedResultImage) {
                std::filesystem::create_directories("output");
                cv::imwrite("output/armor_result.jpg", canvas);
                savedResultImage = true;
            }
        } else {
            cv::putText(canvas, "No armor detected", cv::Point(20, 40),
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
        }

        writer.write(canvas);

        if (showVideo) {
            cv::imshow("Armor Detection", canvas);
            const int key = cv::waitKey(1);
            if (key == 27) {
                break;
            }
        }

        ++frameIndex;
        if (frameIndex % 60 == 0) {
            std::cout << "processed " << frameIndex << " frames\n";
        }
    }

    capture.release();
    writer.release();
    if (showVideo) {
        cv::destroyAllWindows();
    }

    std::cout << "Processed frames: " << frameIndex << '\n';
    std::cout << "Detected frames: " << detectedFrames << '\n';
    std::cout << "Output video: " << outputPath << '\n';
    if (savedResultImage) {
        std::cout << "Result image: output/armor_result.jpg\n";
    }

    return detectedFrames > 0 ? 0 : 2;
}
