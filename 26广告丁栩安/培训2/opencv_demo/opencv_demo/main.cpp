#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    // 默认读取 images/sample.jpg；也可以从命令行传入图片路径。
    const std::string imagePath = (argc > 1) ? argv[1] : "images/sample.jpg";

    try {
        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cerr << "Failed to read image: " << imagePath << '\n';
            std::cerr << "Please check that the file exists and is a valid image.\n";
            return 1;
        }

        std::cout << "OpenCV version: " << CV_VERSION << '\n';
        std::cout << "Image path: " << imagePath << '\n';
        std::cout << "Image size: " << image.cols << " x " << image.rows
                  << ", channels: " << image.channels() << '\n';
        std::cout << "Press any key in the image window to exit...\n";

        const std::string windowName = "OpenCV Image Viewer";
        cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
        cv::imshow(windowName, image);
        cv::waitKey(0);
        cv::destroyAllWindows();
    } catch (const cv::Exception& error) {
        std::cerr << "OpenCV error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
