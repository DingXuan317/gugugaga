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

void drawCornerBracket(cv::Mat& frame, const cv::Point& point, int dx, int dy,
                       const cv::Scalar& color) {
    constexpr int length = 15;
    cv::line(frame, point, point + cv::Point(dx * length, 0), color, 2, cv::LINE_AA);
    cv::line(frame, point, point + cv::Point(0, dy * length), color, 2, cv::LINE_AA);
}

void drawTarget(cv::Mat& frame, const ArmorTarget& target) {
    const cv::Scalar yellow(0, 255, 255);
    const cv::Scalar green(0, 255, 0);

    // 榛勮壊妗嗭細杞村榻愮殑澶栨帴鐭╁舰锛堝拰鍙傝€冭棰戜竴鑷达級
    cv::rectangle(frame, target.left.box, yellow, 2, cv::LINE_AA);
    cv::rectangle(frame, target.right.box, yellow, 2, cv::LINE_AA);

    // 鍥涗釜瑙掔偣鐩存帴浣跨敤鐏潯鐨勭湡瀹炲瑙掞紝璺熼殢绾㈣壊閮ㄥ垎
    const std::array<cv::Point, 4>& points = target.armorCorners;

    // 杩炴帴 P1-P4锛屽舰鎴愬€炬枩鐨勮鐢叉澘鍥涜竟褰?    for (int i = 0; i < 4; ++i) {
        cv::line(frame, points[i], points[(i + 1) % 4], green, 1, cv::LINE_AA);
    }

    const char* labels[4] = {"P1", "P2", "P3", "P4"};
    const cv::Point labelOffsets[4] = {
        cv::Point(6, -10), cv::Point(6, -10),
        cv::Point(8, 20), cv::Point(-28, 20)
    };
    // 瑙掓爣鍚戝鐢伙紝瑙掔偣鏈韩浠嶇劧钀藉湪鐏潯澶栬涓?    const int dx[4] = {-1, 1, 1, -1};
    const int dy[4] = {-1, -1, 1, 1};

    // 鍥涗釜瑙掔偣锛氱豢鑹插噯鏄熴€丳1-P4 鏍囩銆侀粍鑹?L 褰㈣鏍?    for (int i = 0; i < 4; ++i) {
        const cv::Point& point = points[i];
        cv::circle(frame, point, 5, green, 1, cv::LINE_AA);
        cv::circle(frame, point, 2, green, cv::FILLED, cv::LINE_AA);
        cv::drawMarker(frame, point, green, cv::MARKER_CROSS, 12, 1, cv::LINE_AA);
        cv::putText(frame, labels[i], point + labelOffsets[i],
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, green, 1, cv::LINE_AA);
        drawCornerBracket(frame, point, dx[i], dy[i], yellow);
    }

    // 瑁呯敳鏉夸腑蹇冨崄瀛?    cv::Point center(0, 0);
    for (const cv::Point& point : points) {
        center += point;
    }
    center.x /= 4;
    center.y /= 4;
    cv::drawMarker(frame, center, green, cv::MARKER_CROSS, 18, 1, cv::LINE_AA);
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

        if (target.has_value() && frameIndex == 188) {
            std::cout << "DEBUG L box=" << target->left.box
                      << " corners=";
            for (const auto& p : target->left.corners) std::cout << "(" << p.x << "," << p.y << ") ";
            std::cout << " R box=" << target->right.box << " corners=";
            for (const auto& p : target->right.corners) std::cout << "(" << p.x << "," << p.y << ") ";
            std::cout << " armor=";
            for (const auto& p : target->armorCorners) std::cout << "(" << p.x << "," << p.y << ") ";
            std::cout << "\n";
        }
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

