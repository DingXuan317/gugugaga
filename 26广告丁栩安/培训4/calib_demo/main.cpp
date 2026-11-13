// calib_demo —— 相机内参标定与畸变矫正示例
//
// 用法：
//   calib_demo [图像目录] [内角点列数] [内角点行数] [方格边长mm]
//   例：calib_demo ../cam_mix_img 10 7 25
//
// 流程：
//   1) 读入目录下所有标定图像，用 findChessboardCorners 找棋盘格内角点，
//      再用 cornerSubPix 做亚像素细化；
//   2) calibrateCamera 求解相机内参 K、畸变系数 dist（k1,k2,p1,p2,k3）与重投影误差；
//   3) 稳健处理：把单幅重投影平均误差 > 1px 的视图剔除后重新标定（最少保留 20 幅），
//      避免个别模糊/倾斜过大的图像把整体结果带偏；
//   4) 用最终参数对一幅图像做去畸变，结果写入 undistort_result.jpg，
//      并在 shots/ 下保存对比图、角点检测图与运行日志。

#include <opencv2/opencv.hpp>
// OpenCV 5 把 projectPoints / boundingRect 归到了 geometry 模块，
// opencv.hpp 这个伞头文件里没有包含它，需要显式引入。
#include <opencv2/geometry.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX          // 防止 windows.h 把 min/max 定义成宏，和 std::min/std::max 冲突
#endif
#include <windows.h>
#endif

namespace {

constexpr int    kMinViewsForCalib = 20;     // 最少视图数，低于此值不再剔除
constexpr double kOutlierThreshold = 1.0;    // 单幅平均重投影误差阈值（像素）

// 一幅有效视图：图像名 + 亚像素角点 + 标定后算出的重投影误差
struct View {
    std::string name;
    std::string path;
    cv::Mat     gray;
    std::vector<cv::Point2f> corners;
    double      error = 0.0;
};

// 同时往控制台和日志文件写一行
struct Logger {
    std::ofstream file;
    explicit Logger(const std::string& path) { file.open(path, std::ios::binary); }
    void line(const std::string& text) {
        std::cout << text << '\n';
        if (file.is_open()) file << text << '\n';
    }
};

std::string fmt(double value, int prec = 4) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(prec) << value;
    return os.str();
}

// 收集目录下的图像文件
std::vector<std::string> collectImages(const std::string& dir) {
    std::vector<std::string> files;
    for (const char* ext : {"/*.jpg", "/*.jpeg", "/*.png", "/*.bmp"}) {
        std::vector<cv::String> part;
        cv::glob(dir + ext, part, false);
        for (const auto& p : part) files.push_back(p);
    }
    std::sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        const auto numOf = [](const std::string& s) {
            std::string digits;
            for (char c : s) if (std::isdigit(static_cast<unsigned char>(c))) digits.push_back(c);
            return digits.empty() ? 0LL : std::stoll(digits);
        };
        return numOf(a) < numOf(b);
    });
    return files;
}

std::string baseName(const std::string& path) {
    const auto pos = path.find_last_of("/\\");
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
}

// 用给定视图子集标定，并回填每幅视图的重投影误差
bool calibrate(const std::vector<std::vector<cv::Point3f>>& objectPoints,
               std::vector<View>& views,
               const std::vector<int>& subset,
               const cv::Size& imageSize,
               cv::Mat& K, cv::Mat& dist, double& rms,
               std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs) {
    std::vector<std::vector<cv::Point3f>> obj;
    std::vector<std::vector<cv::Point2f>> img;
    for (int i : subset) {
        obj.push_back(objectPoints[i]);
        img.push_back(views[i].corners);
    }

    const int flags = 0;                        // 自由求解 k1,k2,p1,p2,k3
    const cv::TermCriteria criteria(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 100, 1e-9);
    rms = cv::calibrateCamera(obj, img, imageSize, K, dist, rvecs, tvecs, flags, criteria);
    if (!std::isfinite(rms)) return false;

    for (size_t k = 0; k < subset.size(); ++k) {
        std::vector<cv::Point2f> projected;
        cv::projectPoints(objectPoints[subset[k]], rvecs[k], tvecs[k], K, dist, projected);
        double sum = 0.0;
        for (size_t j = 0; j < projected.size(); ++j) {
            sum += cv::norm(img[k][j] - projected[j]);
        }
        views[subset[k]].error = sum / static_cast<double>(projected.size());
    }
    return true;
}

void printMatrix(Logger& log, const std::string& title, const cv::Mat& m) {
    log.line(title);
    for (int r = 0; r < m.rows; ++r) {
        std::string row = "  [";
        for (int c = 0; c < m.cols; ++c) {
            row += (c ? ", " : "") + fmt(m.at<double>(r, c), 4);
        }
        log.line(row + "]");
    }
}

// 角点可视化：每个内角点画圆点并编号，按行上色
// 编号规则与示例一致：左下角为 1，向右递增，向上换行
cv::Mat renderNumberedCorners(const cv::Mat& gray, const std::vector<cv::Point2f>& corners,
                              const cv::Size& pattern) {
    cv::Mat canvas;
    cv::cvtColor(gray, canvas, cv::COLOR_GRAY2BGR);

    // 底行红 -> 顶行紫：彩虹顺序（BGR）
    const cv::Scalar palette[] = {
        cv::Scalar(0, 0, 255),     // 红
        cv::Scalar(0, 128, 255),   // 橙
        cv::Scalar(0, 255, 255),   // 黄
        cv::Scalar(0, 200, 0),     // 绿
        cv::Scalar(255, 255, 0),   // 青
        cv::Scalar(255, 0, 0),     // 蓝
        cv::Scalar(255, 0, 255),   // 紫
    };
    const int paletteSize = static_cast<int>(sizeof(palette) / sizeof(palette[0]));

    const int cols = pattern.width;
    const int rows = pattern.height;
    for (int r = 0; r < rows; ++r) {
        // findChessboardCorners 返回行优先顺序；同一行内按 x 排序得到"从左到右"
        std::vector<std::pair<float, cv::Point2f>> rowPoints;
        rowPoints.reserve(static_cast<size_t>(cols));
        for (int c = 0; c < cols; ++c) {
            const cv::Point2f& p = corners[static_cast<size_t>(r) * cols + c];
            rowPoints.emplace_back(p.x, p);
        }
        std::sort(rowPoints.begin(), rowPoints.end(),
                  [](const std::pair<float, cv::Point2f>& a, const std::pair<float, cv::Point2f>& b) {
                      return a.first < b.first;
                  });

        const int colorIdx = (rows <= 1) ? 0 : r * (paletteSize - 1) / (rows - 1);
        const cv::Scalar color = palette[colorIdx];
        for (int c = 0; c < cols; ++c) {
            const cv::Point2f& p = rowPoints[static_cast<size_t>(c)].second;
            const int number = (rows - 1 - r) * cols + c + 1;   // 左下角为 1
            cv::circle(canvas, p, 3, color, -1, cv::LINE_AA);
            const std::string text = std::to_string(number);
            int baseline = 0;
            const cv::Size ts = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseline);
            // 默认标在角点右上方；贴边时翻到相反一侧，避免被裁掉
            cv::Point org(static_cast<int>(p.x) + 4, static_cast<int>(p.y) - 4);
            if (org.x + ts.width > canvas.cols - 2) org.x = static_cast<int>(p.x) - 4 - ts.width;
            if (org.y - ts.height < 2) org.y = static_cast<int>(p.y) + 4 + ts.height;
            cv::putText(canvas, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.4,
                        cv::Scalar(0, 0, 0), 3, cv::LINE_AA);   // 先描黑底，保证可读
            cv::putText(canvas, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1, cv::LINE_AA);
        }
    }
    return canvas;
}

// 单幅图像的棋盘格包围盒面积，用来挑一幅"板子占画面最大"的图做矫正展示
double boardArea(const std::vector<cv::Point2f>& corners) {
    return cv::boundingRect(corners).area();
}

}  // namespace

// 真正的流程入口（供 main 调用；也便于在测试工程里以 DLL 方式加载）
int runCalib(int argc, char** argv) {
    const std::string imageDir = (argc > 1) ? argv[1] : "images";
    const int patternCols = (argc > 2) ? std::atoi(argv[2]) : 10;   // 内角点列数
    const int patternRows = (argc > 3) ? std::atoi(argv[3]) : 7;    // 内角点行数
    const double squareMm = (argc > 4) ? std::atof(argv[4]) : 25.0; // 方格边长（只影响平移量单位）

    const std::string shotsDir = "shots";
    std::filesystem::create_directories(shotsDir);   // 不存在时创建
    Logger log(shotsDir + "/calib_log.txt");

    log.line("================ calib_demo ================");
    log.line("OpenCV 版本 : " + std::string(CV_VERSION));
    log.line("图像目录    : " + imageDir);
    log.line("棋盘内角点  : " + std::to_string(patternCols) + " x " + std::to_string(patternRows));
    log.line("方格边长    : " + fmt(squareMm, 2) + " mm（仅影响平移量单位，不影响内参与畸变系数）");
    log.line("");

    // ---------- 1. 读图 + 角点检测 ----------
    const std::vector<std::string> files = collectImages(imageDir);
    if (files.empty()) {
        log.line("[错误] 目录下没有找到图像：" + imageDir);
        return 1;
    }

    const cv::Size pattern(patternCols, patternRows);
    std::vector<cv::Point3f> grid;
    grid.reserve(static_cast<size_t>(patternCols) * patternRows);
    for (int y = 0; y < patternRows; ++y) {
        for (int x = 0; x < patternCols; ++x) {
            grid.emplace_back(static_cast<float>(x) * static_cast<float>(squareMm),
                              static_cast<float>(y) * static_cast<float>(squareMm), 0.0f);
        }
    }

    std::vector<View> views;
    std::vector<std::vector<cv::Point3f>> objectPoints;
    cv::Size imageSize;

    const cv::TermCriteria subpix(cv::TermCriteria::COUNT | cv::TermCriteria::EPS, 30, 0.001);
    int failed = 0;
    for (const auto& path : files) {
        cv::Mat gray = cv::imread(path, cv::IMREAD_GRAYSCALE);
        if (gray.empty()) {
            log.line("[跳过] 无法读取：" + baseName(path));
            ++failed;
            continue;
        }
        if (imageSize.area() == 0) imageSize = gray.size();

        std::vector<cv::Point2f> corners;
        const int flags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;
        if (!cv::findChessboardCorners(gray, pattern, corners, flags)) {
            ++failed;
            continue;
        }
        cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1), subpix);

        View v;
        v.name = baseName(path);
        v.path = path;
        v.gray = gray.clone();
        v.corners = corners;
        views.push_back(std::move(v));
        objectPoints.push_back(grid);
    }

    log.line("[1/4] 角点检测：" + std::to_string(views.size()) + " 幅成功 / " +
             std::to_string(files.size()) + " 幅总计（" + std::to_string(failed) + " 幅无角点或读取失败）");
    if (views.size() < static_cast<size_t>(kMinViewsForCalib)) {
        log.line("[错误] 有效视图太少，至少需要 " + std::to_string(kMinViewsForCalib) + " 幅");
        return 1;
    }
    log.line("");

    // ---------- 2. 全量标定 ----------
    std::vector<int> used(views.size());
    for (size_t i = 0; i < used.size(); ++i) used[i] = static_cast<int>(i);

    cv::Mat K, dist;
    double rms = 0.0;
    std::vector<cv::Vec3d> rvecs, tvecs;
    if (!calibrate(objectPoints, views, used, imageSize, K, dist, rms, rvecs, tvecs)) {
        log.line("[错误] calibrateCamera 求解失败");
        return 1;
    }
    log.line("[2/4] 全量标定：" + std::to_string(used.size()) + " 幅，重投影 RMS = " + fmt(rms, 4) + " px");
    log.line("");

    // ---------- 3. 剔除高误差视图后重标定 ----------
    int round = 1;
    while (true) {
        std::vector<int> keep;
        for (int i : used) {
            if (views[i].error <= kOutlierThreshold) keep.push_back(i);
        }
        if (keep.size() == used.size() || keep.size() < static_cast<size_t>(kMinViewsForCalib)) break;

        used = keep;
        if (!calibrate(objectPoints, views, used, imageSize, K, dist, rms, rvecs, tvecs)) {
            log.line("[错误] 剔除异常视图后重标定失败");
            return 1;
        }
        log.line("[3/4] 第 " + std::to_string(round) + " 轮剔除（误差 > " + fmt(kOutlierThreshold, 1) +
                 " px）：保留 " + std::to_string(used.size()) + " 幅，RMS = " + fmt(rms, 4) + " px");
        ++round;
    }
    if (round == 1) {
        log.line("[3/4] 所有视图误差均 <= " + fmt(kOutlierThreshold, 1) + " px，无需剔除");
    }

    log.line("");
    printMatrix(log, "相机内参矩阵 K =", K);
    const cv::Mat distRow = dist.reshape(1, 1);
    printMatrix(log, "畸变系数 [k1, k2, p1, p2, k3] =", distRow);
    log.line("最终重投影 RMS = " + fmt(rms, 4) + " px（参与标定 " + std::to_string(used.size()) + " 幅）");

    double best = 1e9, worst = 0.0, sumErr = 0.0;
    int bestIdx = used.front(), worstIdx = used.front();
    for (int i : used) {
        if (views[i].error < best) { best = views[i].error; bestIdx = i; }
        if (views[i].error > worst) { worst = views[i].error; worstIdx = i; }
        sumErr += views[i].error;
    }
    log.line("单幅误差：最小 " + fmt(best, 3) + " px，平均 " + fmt(sumErr / used.size(), 3) +
             " px，最大 " + fmt(worst, 3) + " px");
    log.line("");

    // ---------- 4. 去畸变 ----------
    int target = used.front();
    double maxArea = -1.0;
    for (int i : used) {
        const double area = boardArea(views[i].corners);
        if (area > maxArea) { maxArea = area; target = i; }
    }

    const cv::Mat newK = cv::getOptimalNewCameraMatrix(K, dist, imageSize, 0.0, imageSize);

    // 检测角点用的是灰度图，展示效果用彩色原图
    cv::Mat colorSrc = cv::imread(views[target].path, cv::IMREAD_COLOR);
    if (colorSrc.empty()) colorSrc = views[target].gray;

    cv::Mat undistorted;
    cv::undistort(colorSrc, undistorted, K, dist, newK);
    cv::imwrite("undistort_result.jpg", undistorted);

    // 原图与矫正图的并排对比
    cv::Mat before = (colorSrc.channels() == 3) ? colorSrc : cv::Mat();
    if (before.empty()) cv::cvtColor(colorSrc, before, cv::COLOR_GRAY2BGR);
    cv::Mat after = (undistorted.channels() == 3) ? undistorted : cv::Mat();
    if (after.empty()) cv::cvtColor(undistorted, after, cv::COLOR_GRAY2BGR);
    cv::Mat compare(before.rows, before.cols * 2 + 8, CV_8UC3, cv::Scalar(255, 255, 255));
    before.copyTo(compare(cv::Rect(0, 0, before.cols, before.rows)));
    after.copyTo(compare(cv::Rect(before.cols + 8, 0, after.cols, after.rows)));
    cv::putText(compare, "before", cv::Point(12, 28), cv::FONT_HERSHEY_SIMPLEX, 0.8,
                cv::Scalar(0, 0, 255), 2);
    cv::putText(compare, "after", cv::Point(before.cols + 20, 28), cv::FONT_HERSHEY_SIMPLEX, 0.8,
                cv::Scalar(0, 160, 0), 2);
    cv::imwrite(shotsDir + "/compare_before_after.jpg", compare);

    // 角点检测可视化：编号 1~N 并按行上色（左下角为 1，向右递增，向上换行）
    // 展示用 target（棋盘格占画面最大、畸变最明显，与 undistort_result.jpg 同源）
    for (const auto& [label, idx] : {std::pair<std::string, int>{"corners_numbered", target},
                                     std::pair<std::string, int>{"corners_worst", worstIdx}}) {
        cv::Mat shown = renderNumberedCorners(views[idx].gray, views[idx].corners, pattern);
        cv::putText(shown, baseName(views[idx].name) + "  err=" + fmt(views[idx].error, 2) + "px",
                    cv::Point(10, 24), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 255), 2);
        cv::imwrite(shotsDir + "/" + label + ".jpg", shown);
    }

    log.line("[4/4] 去畸变：使用 " + baseName(views[target].name) +
             "（棋盘格占画面最大，畸变最明显）");
    log.line("  输出 undistort_result.jpg");
    log.line("  输出 " + shotsDir + "/compare_before_after.jpg");
    log.line("  输出 " + shotsDir + "/corners_numbered.jpg、corners_worst.jpg（角点编号 + 按行上色）");
    log.line("  输出 " + shotsDir + "/calib_log.txt");
    log.line("============================================");
    return 0;
}

int main(int argc, char** argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   // 让中文日志在 Windows 控制台正常显示
#endif
    try {
        return runCalib(argc, argv);
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV 异常: " << e.what() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "异常: " << e.what() << '\n';
    }
    return 1;
}
