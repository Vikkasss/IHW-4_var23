#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <random>
#include <unistd.h>
#include <fstream>
#include <cstring> // Для работы с std::strcmp

// Глобальные переменные
int N; // Количество болтунов
sem_t phone_line_sem; // Семафор для телефонной линии
bool* phone_busy; // Массив для отслеживания состояния телефонов
std::ofstream outputFile; // Файл для вывода

const int MAX_CALLS = 40; // Максимальное количество звонков
int call_counter = 0; // Счетчик звонков
pthread_mutex_t call_counter_mutex; // Мьютекс для синхронизации счетчика звонков

// Генерация случайного числа
int getRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// Функция, моделирующая поведение болтуна
void* talk(void* param) {
    int id = *(int*)param; // ID болтуна
    while (true) {
        // Болтун решает, что делать: ждать или звонить
        if (getRandomInt(0, 1) == 0) {
            // Ждет звонка
            sem_wait(&phone_line_sem); // Захватываем семафор
            std::cout << "B" << id << " waiting call.\n";
            outputFile << "B" << id << " waiting call.\n";
            sem_post(&phone_line_sem); // Освобождаем семафор
            sleep(getRandomInt(1, 3)); // Случайное время ожидания
        } else {
            // Звонит другому абоненту
            int target = getRandomInt(0, N - 1); // Случайный абонент
            while (target == id) {
                target = getRandomInt(0, N - 1); // Не звонит сам себе
            }

            sem_wait(&phone_line_sem); // Захватываем семафор
            if (!phone_busy[target]) {
                // Телефон свободен
                phone_busy[id] = true;
                phone_busy[target] = true;
                std::cout << "B" << id << " calling to B" << target << ".\n";
                outputFile << "B" << id << " calling to B" << target << ".\n";
                sem_post(&phone_line_sem); // Освобождаем семафор

                sleep(getRandomInt(1, 5)); // Разговор

                sem_wait(&phone_line_sem); // Захватываем семафор
                phone_busy[id] = false;
                phone_busy[target] = false;
                std::cout << "B" << id << " stopped talking with B" << target << ".\n";
                outputFile << "B" << id << " stopped talking with B" << target << ".\n";
                sem_post(&phone_line_sem); // Освобождаем семафор
            } else {
                // Телефон занят
                std::cout << "B" << id << " see that B" << target << " is busy.\n";
                outputFile << "B" << id << " see that B" << target << " is busy.\n";
                sem_post(&phone_line_sem); // Освобождаем семафор
            }
        }

        // Синхронизируем доступ к счетчику звонков
        pthread_mutex_lock(&call_counter_mutex);
        call_counter++;
        pthread_mutex_unlock(&call_counter_mutex);

        // Проверяем, достигнут ли лимит звонков
        if (call_counter >= MAX_CALLS) {
            break;
        }
    }
    return nullptr;
}

// Функция для чтения данных из конфигурационного файла
bool readConfigFile(const char* filename, int& N, std::string& outputFileName) {
    std::ifstream configFile(filename);
    if (!configFile.is_open()) {
        std::cerr << "Error: Unable to open config file " << filename << "\n";
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        if (line.find("N=") == 0) {
            N = std::stoi(line.substr(2));
        } else if (line.find("outputFile=") == 0) {
            outputFileName = line.substr(11);
        }
    }

    configFile.close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " --file <config_file> OR --params <N> <outputFile>\n";
        return 1;
    }

    std::string outputFileName;

    if (std::strcmp(argv[1], "--file") == 0) {
        // Ввод данных из конфигурационного файла
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " --file <config_file>\n";
            return 1;
        }
        if (!readConfigFile(argv[2], N, outputFileName)) {
            return 1;
        }
    } else if (std::strcmp(argv[1], "--params") == 0) {
        // Ввод данных через командную строку
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " --params <N> <outputFile>\n";
            return 1;
        }
        N = std::stoi(argv[2]);
        outputFileName = argv[3];
    } else {
        std::cerr << "Unknown option: " << argv[1] << "\n";
        return 1;
    }

    outputFile.open(outputFileName);

    // Инициализация семафора и массива состояний телефонов
    sem_init(&phone_line_sem, 0, 1); // Семафор с начальным значением 1 (аналог мьютекса)
    phone_busy = new bool[N];
    for (int i = 0; i < N; ++i) {
        phone_busy[i] = false;
    }

    // Инициализация мьютекса для счетчика звонков
    pthread_mutex_init(&call_counter_mutex, nullptr);

    // Создание потоков для болтунов
    pthread_t threads[N];
    int ids[N];
    for (int i = 0; i < N; ++i) {
        ids[i] = i;
        pthread_create(&threads[i], nullptr, talk, &ids[i]);
    }

    // Ожидание завершения потоков
    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // Освобождение ресурсов
    delete[] phone_busy;
    sem_destroy(&phone_line_sem);
    pthread_mutex_destroy(&call_counter_mutex);
    outputFile.close();

    return 0;
}