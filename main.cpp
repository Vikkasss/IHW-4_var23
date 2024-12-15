#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <random>
#include <unistd.h>
#include <fstream>
#include <cstring>


int N;                                   // number of talkers
sem_t phone_line_sem;                    // semaphore for telephone line
bool* phone_busy;                        // array for tracking the status of phones
std::ofstream outputFile;                // output file

const int MAX_CALLS = 40;                // maximum number of calls
int call_counter = 0;                    // counter of calls
pthread_mutex_t call_counter_mutex;      // mutex for call counter synchronization

// random number generation
int getRandomInt(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

// Function that models the behavior of a talker
void* talk(void* param) {
    int id = *(int*)param; // ID talker
    while (true) {
        // talker decide that what to do: wait or call
        if (getRandomInt(0, 1) == 0) {
            // waiting call
            sem_wait(&phone_line_sem); // capturing semaphore
            std::cout << "B" << id << " waiting call.\n";
            outputFile << "B" << id << " waiting call.\n";
            sem_post(&phone_line_sem); // freeing semaphore
            sleep(getRandomInt(1, 3)); // random time of waiting
        } else {
            // talker calls another subscriber
            int target = getRandomInt(0, N - 1); // random subscriber
            while (target == id) {
                target = getRandomInt(0, N - 1); // Talker doesn`t call himself
            }

            sem_wait(&phone_line_sem); // capturing semaphore
            if (!phone_busy[target]) {
                // the phone is free
                phone_busy[id] = true;
                phone_busy[target] = true;
                std::cout << "B" << id << " calling to B" << target << ".\n";
                outputFile << "B" << id << " calling to B" << target << ".\n";
                sem_post(&phone_line_sem); // freeing semaphore

                sleep(getRandomInt(1, 5)); // talk

                sem_wait(&phone_line_sem); // capturing semaphore
                phone_busy[id] = false;
                phone_busy[target] = false;
                std::cout << "B" << id << " stopped talking with B" << target << ".\n";
                outputFile << "B" << id << " stopped talking with B" << target << ".\n";
                sem_post(&phone_line_sem); // freeing semaphore
            } else {
                // Телефон занят
                std::cout << "B" << id << " see that B" << target << " is busy.\n";
                outputFile << "B" << id << " see that B" << target << " is busy.\n";
                sem_post(&phone_line_sem); // freeing semaphore
            }
        }

        // synchtonize access to call counter Синхронизируем доступ к счетчику звонков
        pthread_mutex_lock(&call_counter_mutex);
        call_counter++;
        pthread_mutex_unlock(&call_counter_mutex);

        // check, has the call limit been reached
        if (call_counter >= MAX_CALLS) {
            break;
        }
    }
    return nullptr;
}

// Function for reading data from config.txt
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
        // entering data from config file
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " --file <config_file>\n";
            return 1;
        }
        if (!readConfigFile(argv[2], N, outputFileName)) {
            return 1;
        }
    } else if (std::strcmp(argv[1], "--params") == 0) {
        // entering data via comman line
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

    // Initialization of semaphore and array of phone states
    sem_init(&phone_line_sem, 0, 1); // semaphore with default value 1
    phone_busy = new bool[N];
    for (int i = 0; i < N; ++i) {
        phone_busy[i] = false;
    }

    // Initialization of mutex for calls counter
    pthread_mutex_init(&call_counter_mutex, nullptr);

    // Creation threads for talkers
    pthread_t threads[N];
    int ids[N];
    for (int i = 0; i < N; ++i) {
        ids[i] = i;
        pthread_create(&threads[i], nullptr, talk, &ids[i]);
    }

    // Wainting for threads to complete
    for (int i = 0; i < N; ++i) {
        pthread_join(threads[i], nullptr);
    }

    // freeing resources
    delete[] phone_busy;
    sem_destroy(&phone_line_sem);
    pthread_mutex_destroy(&call_counter_mutex);
    outputFile.close();

    return 0;
}