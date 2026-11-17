#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "utilities.h"
#include "shared.h"

int main(int argc, char* argv[]) {
    if (argc != 4) return 1;

    const std::string filename = argv[1];
    int threadIdx = std::stoi(argv[2]);
    int nReports  = std::stoi(argv[3]);

    sem_t* semFull  = sem_open(SEM_FULL,  0);
    sem_t* semEmpty = sem_open(SEM_EMPTY, 0);
    sem_t* semMutex = sem_open(SEM_MUTEX, 0);
    if (semFull==SEM_FAILED || semEmpty==SEM_FAILED || semMutex==SEM_FAILED) return 1;

    int shmFd = shm_open(SHM_IDX, O_RDWR, 0644);
    if (shmFd < 0) return 1;
    QueueIdx* idx = (QueueIdx*)mmap(nullptr, sizeof(QueueIdx),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (idx == MAP_FAILED) return 1;

    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file.is_open()) return 1;

    bool isActive = true;
    do {
        std::cout << "\nSender #" << threadIdx << "\n0: write message\n1: exit\n> ";
        std::string choiceStr;
        std::getline(std::cin, choiceStr);
        char choice = choiceStr.empty() ? '?' : choiceStr[0];

        switch (choice) {
        case '0': {
            std::string message;
            bool cancel = false;
            while (true) {
                std::cout << "Enter message (от 1 до 20 символов): ";
                std::getline(std::cin, message);
                if (!message.empty() && message.size() <= 20) break;
                std::cout << "Invalid length. 0 - cancel, other - retry: ";
                std::string s; std::getline(std::cin, s);
                if (!s.empty() && s[0] == '0') { cancel = true; break; }
            }
            if (cancel) break;

            char buffer[20] = {0};
            std::strncpy(buffer, message.data(), 20);

            std::cout << "Sending...\n";
            sem_wait(semEmpty);
            sem_wait(semMutex);

            std::streamoff off = (std::streamoff)(idx->tail % nReports) * 20;
            writefileAt(file, buffer, sizeof(buffer), off);
            idx->tail = (idx->tail + 1) % nReports;

            sem_post(semMutex);
            sem_post(semFull);

            std::cout << "Message sent: " << message << std::endl;
            break;
        }
        case '1':
            isActive = false;
            break;
        default:
            std::cout << "Invalid option\n";
            break;
        }
    } while (isActive);

    file.close();
    munmap(idx, sizeof(QueueIdx));
    close(shmFd);
    sem_close(semFull); sem_close(semEmpty); sem_close(semMutex);
    return 0;
}