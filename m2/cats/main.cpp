#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <curl/curl.h>
#include <opencv2/opencv.hpp>
#include <thread>
#include <future>
#include <mutex>
#include <fstream>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::vector<unsigned char>*)userp)->insert(((std::vector<unsigned char>*)userp)->end(), (unsigned char*)contents, (unsigned char*)contents + size * nmemb);
    return size * nmemb;
}

std::vector<unsigned char> getCatImage(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::vector<unsigned char> image_data;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &image_data);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return image_data;
}

bool sendCollage(const std::string& url, const std::vector<unsigned char>& image_data) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Ошибка инициализации CURL для POST-запроса" << std::endl;
        return false;
    }

    struct curl_httppost* form = nullptr;
    struct curl_httppost* last = nullptr;

    std::string temp_file = "tmp.jpeg";
    std::ofstream ofs(temp_file, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(image_data.data()), image_data.size());
    ofs.close();

    curl_formadd(&form, &last,
                 CURLFORM_COPYNAME, "file",
                 CURLFORM_FILE, temp_file.c_str(),
                 CURLFORM_CONTENTTYPE, "image/jpeg",
                 CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

    CURLcode res = curl_easy_perform(curl);

    std::remove(temp_file.c_str());

    curl_easy_cleanup(curl);
    curl_formfree(form);

    if (res != CURLE_OK) {
        std::cerr << "Ошибка отправки POST-запроса: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}

cv::Mat resizeImageWithAspectRatio(const cv::Mat& img, int target_width, int target_height) {
    float aspect_ratio = static_cast<float>(img.cols) / img.rows;
    int new_width, new_height;

    if (aspect_ratio > 1) {
        new_width = target_width;
        new_height = static_cast<int>(target_height / aspect_ratio);
    } else {
        new_height = target_height;
        new_width = static_cast<int>(target_width * aspect_ratio);
    }

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(new_width, new_height));
    return resized;
}

cv::Mat createCollage(const std::vector<cv::Mat>& images, int rows, int cols, int target_width, int target_height) {
    int cell_width = target_width / cols;
    int cell_height = target_height / rows;
    std::cout << cell_height << "\n";
    std::cout << cell_width << "\n";

    cv::Mat collage(target_height, target_width, CV_8UC3, cv::Scalar(255, 255, 255));

    int image_index = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (image_index < images.size()) {
                cv::Mat resized_image = resizeImageWithAspectRatio(images[image_index], cell_width, cell_height);
                int width = resized_image.cols;
                int height = resized_image.rows;

                if (i * cell_height + height <= target_height && j * cell_width + width <= target_width) {
                    resized_image.copyTo(collage(cv::Rect(j * cell_width, i * cell_height, width, height)));
                } else {
                    std::cout << i * cell_height + height << ", " << height << ", " << target_height << "\n";
                    std::cout << i * cell_width + width << ", " << width << ", " << target_width << "\n";
                    std::cerr << "Ошибка: изображение не помещается в указанную ячейку мозаики" << std::endl;
                }
                image_index++;
            }
        }
    }
    return collage;
}

int main() {
    std::string url = "http://algisothal.ru:8889/cat";
    std::string post_url = "http://algisothal.ru:8889/cat";

    std::set<std::string> unique_images;
    std::vector<cv::Mat> cat_images;
    std::mutex mutex;

    auto fetchImage = [&](int index) {
        while (true) {
            std::cout << index << std::endl;
            std::vector<unsigned char> image_data = getCatImage(url);
            if (image_data.empty()) continue;

            std::string hash = std::to_string(std::hash<std::string>{}(std::string(image_data.begin(), image_data.end())));
            std::lock_guard<std::mutex> lock(mutex);
            if (unique_images.find(hash) == unique_images.end()) {
                unique_images.insert(hash);
                cv::Mat img = cv::imdecode(image_data, cv::IMREAD_COLOR);
                if (!img.empty()) {
                    cat_images.push_back(img);
                }
                break;
            }
        }
    };

    const int required_images = 12;
    std::vector<std::thread> threads;
    for (int i = 0; i < required_images; ++i) {
        threads.emplace_back(fetchImage, i);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int collage_width = 1200;
    int collage_height = 800;
    cv::Mat collage = createCollage(cat_images, 3, 4, collage_width, collage_height);

    std::vector<unsigned char> image_data;
    cv::imencode(".jpeg", collage, image_data);
    cv::imwrite("cats_collage.jpeg", collage);

    if (sendCollage(post_url, image_data)) {
        std::cout << "\nМозаика успешно отправлена на сервер" << std::endl;
    } else {
        std::cerr << "\nОшибка при отправке мозаики на сервер" << std::endl;
    }

    std::cout << "Коллаж создан и сохранен как 'cats_collage.jpeg'" << std::endl;
    return 0;
}