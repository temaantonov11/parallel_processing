#include <iostream>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/core.hpp>
#include <string>

void processing_image(cv::Mat& image) {
    for (int i = 0; i < image.rows; ++i) {
        for (int j = 0; j < image.cols; ++j) {
            cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);

            u_int8_t& b = pixel[0];
            u_int8_t& g = pixel[1];
            u_int8_t& r = pixel[2];

            b = 255 - b;
            g = 255 - g;
            r = 255 - r;
        }
    }
}

int main() {
    cv::Mat image = cv::imread("../images/simple_2.jpg");
    if (image.empty()) {
        std::cerr << "Ошибка загрузки изображения!" << '\n';
        return 1;
    }
    processing_image(image);
    cv::imwrite("../images/simple_processing.jpg", image);
}


