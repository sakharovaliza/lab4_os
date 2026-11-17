#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include "utilities.h"
#include "shared.h"

extern char** environ;

static std::string getFilenameFromEnv() {
    const char* env = std::getenv("saharova");
    if (!env) return "";
    return std::string(env);
}

int main() {
    std::string filename;
    int numberOfReports;

    std::cout << "1. Enter file name from console\n";
    std::cout << "2. Get file name from env variable 'saharova'\n";
    std::cout << "Choice: ";
    int srcChoice = 1;
    std::cin >> srcChoice;
    std::cin.ignore(10000, '\n');

    if (srcChoice == 2) {
        filename = getFilenameFromEnv();
        if (filename.empty()) {
            std::cout << "Env variable 'saharova' not found. Enter file name manually: ";
            std::getline(std::cin, filename);
        }
    } else {
        std::cout << "Enter file name: ";
        std::getline(std::cin, filename);
    }
    if (filename.empty()) return 1;

    {
        std::ofstream fout(filename, std::ios::binary | std::ios::trunc);
        if (!fout.is_open()) return 1;
    }

    std::cout << "Enter number of reports (>=1): ";
    while (!(std::cin >> numberOfReports) || numberOfReports < 1) {
        std::cin.clear(); std::cin.ignore(10000, '\n');
        std::cout << "Invalid. Enter number >=1: ";
    }
    std::cin.ignore(10000, '\n');

    {
        std::fstream f(filename, std::ios::binary | std::ios::in | std::ios::out);
        std::vector<char> zeros(20 * numberOfReports, 0);
        f.seekp(0);
        f.write(zeros.data(), zeros.size());
        f.flush();
    }

    cleanupIpc();

    sem_t* semFull  = sem_open(SEM_FULL,  O_CREAT, 0644, 0);
    sem_t* semEmpty = sem_open(SEM_EMPTY, O_CREAT, 0644, numberOfReports);
    sem_t* semMutex = sem_open(SEM_MUTEX, O_CREAT, 0644, 1);
    if (semFull==SEM_FAILED || semEmpty==SEM_FAILED || semMutex==SEM_FAILED) return 1;

    int shmFd = shm_open(SHM_IDX, O_CREAT | O_RDWR, 0644); // создание обьекта
    if (shmFd < 0) return 1;
    if (ftruncate(shmFd, sizeof(QueueIdx)) != 0) return 1;
    QueueIdx* idx = (QueueIdx*)mmap(nullptr, sizeof(QueueIdx),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (idx == MAP_FAILED) return 1;
    idx->head = 0; idx->tail = 0;



    int sendersNum;
    std::cout << "Enter number of senders (1..20): ";
    while (!(std::cin >> sendersNum) || sendersNum < 1 || sendersNum > 20) {
        std::cin.clear(); std::cin.ignore(10000, '\n');
        std::cout << "Invalid. Enter 1..20: ";
    }
    std::cin.ignore(10000, '\n');

    std::vector<pid_t> pids(sendersNum);
    for (int i = 0; i < sendersNum; ++i) {
        std::string argIdx = std::to_string(i);
        std::string argN   = std::to_string(numberOfReports);
        char* argv[] = {
            (char*)"./Sender",
            (char*)filename.c_str(),
            (char*)argIdx.c_str(),
            (char*)argN.c_str(),
            nullptr
        };
        if (posix_spawn(&pids[i], "./Sender", nullptr, nullptr, argv, environ) != 0) return 1;
    }

    usleep(200000);

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return 1;

    int messageCount = 0;
    bool isActive = true;
    while (isActive) {
        std::cout << "\n0: read message\n1: exit\n> ";
        std::string choiceStr;
        std::getline(std::cin, choiceStr);
        char choice = choiceStr.empty() ? '?' : choiceStr[0];

        switch (choice) {
        case '0': {
            std::cout << "Waiting for message...\n";
            sem_wait(semFull);
            sem_wait(semMutex);

            std::streamoff off = (std::streamoff)(idx->head % numberOfReports) * 20;
            char buffer[21] = {0};
            int bytes = readfileAt(file, buffer, sizeof(buffer), off);
            idx->head = (idx->head + 1) % numberOfReports;

            sem_post(semMutex);
            sem_post(semEmpty);

            if (bytes == 20) {
                buffer[20] = '\0';
                std::cout << "Message: " << buffer << std::endl;
                ++messageCount;
            } else {
                std::cout << "No new messages\n";
            }
            break;
        }
        case '1':
            isActive = false;
            break;
        default:
            std::cout << "Invalid option\n";
            break;
        }
    }

    for (pid_t p : pids) kill(p, SIGTERM);
    for (pid_t p : pids) waitpid(p, nullptr, 0);

    std::cout << "Receiver read " << messageCount << " messages\n";

    munmap(idx, sizeof(QueueIdx));
    close(shmFd);
    sem_close(semFull); sem_close(semEmpty); sem_close(semMutex);
    cleanupIpc();

    file.close();
    return 0;
}