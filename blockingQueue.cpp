#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <opencv4/opencv2/opencv.hpp>


// Блокирующая очередь для хранения задач
class BlockingQueue {
private:
    std::queue<std::tuple<int, int, cv::Mat>> queue; // Очередь задач
    std::mutex mtx; // Мьютекс для синхронизации
    std::condition_variable cv; // Условная переменная
    bool finished = false; // Флаг завершения работы

public:
    // Метод для добавления задачи в очередь
    void push(int startRow, int endRow, cv::Mat& image) {
        std::lock_guard<std::mutex> lock(mtx); // Захватываем мьютекс
        queue.push({startRow, endRow, image}); // Добавляем задачу в очередь
        cv.notify_one(); // Уведомляем один из ожидающих потоков
    }

    // Метод для извлечения задачи из очереди
    std::tuple<int, int, cv::Mat> pop() {
        std::unique_lock<std::mutex> lock(mtx); // Захватываем мьютекс

        // Ждём появления задачи или завершения работы
        cv.wait(lock, [&]() {
            // Условие выхода из ожидания: либо очередь не пуста, либо работа завершена
            return !queue.empty() || finished;
        });

        // Если очередь пуста и работа завершена, возвращаем пустую задачу
        if (queue.empty()) {
            return std::make_tuple(0, 0, cv::Mat());
        }

        // Забираем первую задачу из очереди
        auto task = queue.front();
        queue.pop(); // Удаляем задачу из очереди
        return task;
    }

    // Метод для установки флага завершения работы
    void finish() {
        std::lock_guard<std::mutex> lock(mtx); // Захватываем мьютекс
        finished = true; // Устанавливаем флаг завершения работы
        cv.notify_all(); // Уведомляем все ожидающие потоки
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

                u_int8_t& b = pixel[0]; // Синий канал
                u_int8_t& g = pixel[1]; // Зелёный канал
                u_int8_t& r = pixel[2]; // Красный канал

                // Инверсия цветов
                b = 255 - b;
                g = 255 - g;
                r = 255 - r;
            }
        }
    }
}