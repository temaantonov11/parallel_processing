#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <opencv4/opencv2/opencv.hpp>


class BlockingQueue {
private:
    std::queue<std::tuple<int, int, cv::Mat>> queue; // Очередь задач
    std::mutex mtx; // Мьютекс для синхронизации
    std::condition_variable cond_var; // Условная переменная
    bool finished = false; // Флаг завершения работы

public:
    void push(int startRow, int endRow, cv::Mat& image) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push({startRow, endRow, image}); 
        cond_var.notify_one();
    }

    std::tuple<int, int, cv::Mat> pop() {
        std::unique_lock<std::mutex> lock(mtx); // Захватываем мьютекс

        // Ждём появления задачи или завершения работы
        cond_var.wait(lock, [&]() {return !queue.empty() || finished;});

        if (queue.empty()) {
            return std::make_tuple(0, 0, cv::Mat());
        }

        // Забираем первую задачу из очереди
        auto task = queue.front();
        queue.pop();
        return task;
    }

    void finish() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        cond_var.notify_all();
    }
};

// Функция производителя (Producer)
void producer(BlockingQueue& queue, cv::Mat& image, int numBlocks) {
    int rowsPerBlock = image.rows / numBlocks; // Считаем количество строк на один блок
    int startRow = 0;

    for (int i = 0; i < numBlocks; ++i) {
        int endRow;

        // Если это последний блок, он включает все оставшиеся строки
        if (i == numBlocks - 1) {
            endRow = image.rows;
        } else {
            endRow = startRow + rowsPerBlock;
        }

        // Кладём задачу в очередь
        queue.push(startRow, endRow, image);
        startRow = endRow; // Обновляем начало для следующего блока
    }

    // Уведомляем, что задач больше не будет
    queue.finish();
}

// Функция потребителя (Consumer)
void consumer(BlockingQueue& queue) {
    while (true) {
        // Забираем задачу из очереди
        auto task = queue.pop();
        int startRow = std::get<0>(task);
        int endRow = std::get<1>(task);
        cv::Mat image = std::get<2>(task);

        // Если задача пустая, завершаем работу
        if (startRow == 0 && endRow == 0 && image.empty()) {
            break;
        }

        // Обрабатываем строки изображения
        for (int i = startRow; i < endRow; ++i) {
            for (int j = 0; j < image.cols; ++j) {
                cv::Vec3b& pixel = image.at<cv::Vec3b>(i, j);

                u_int8_t& b = pixel[0];
                u_int8_t& g = pixel[1];
                u_int8_t& r = pixel[2];

                // Инверсия цветов
                b = 255 - b;
                g = 255 - g;
                r = 255 - r;
            }
        }
    }
}

int main() {
    cv::Mat image = cv::imread("image.jpg"); // Загрузка изображения
    if (image.empty()) {
        std::cerr << "Ошибка загрузки изображения!" << std::endl;
        return -1;
    }

    BlockingQueue queue;
    int numBlocks = 6;

    std::thread producerThread(producer, std::ref(queue), std::ref(image), numBlocks);

    std::vector<std::thread> consumerThreads;
    for (int i = 0; i < 4; ++i) {
        consumerThreads.emplace_back(consumer, std::ref(queue));
    }

    producerThread.join();

    for (auto& thread : consumerThreads) {
        thread.join();
    }

    cv::imwrite("output.jpg", image); // Сохраняем обработанное изображение
    return 0;
}
