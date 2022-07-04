#include <thread>
#include <random>

void f() {
    std::mt19937 gen(rand());
    std::uniform_int_distribution<> distr(0, 65536);

    for (size_t i = 0; i < 10000; ++i) {
        volatile auto v_ = malloc(distr(gen));
        free(v_);
    }
}

int main() {
    srand(time(NULL));
    
    auto thread_0 = std::thread(f);
    
    auto thread_1 = std::thread(f);
    
    auto thread_2 = std::thread(f);
    
    auto thread_3 = std::thread(f);
    
    auto thread_4 = std::thread(f);
    
    auto thread_5 = std::thread(f);
    
    auto thread_6 = std::thread(f);
    
    auto thread_7 = std::thread(f);
    
    auto thread_8 = std::thread(f);
    
    auto thread_9 = std::thread(f);
    
    auto thread_10 = std::thread(f);
    
    auto thread_11 = std::thread(f);
    
    auto thread_12 = std::thread(f);
    
    auto thread_13 = std::thread(f);
    
    auto thread_14 = std::thread(f);
    
    auto thread_15 = std::thread(f);
     
    
    thread_0.join();
    
    thread_1.join();
    
    thread_2.join();
    
    thread_3.join();
    
    thread_4.join();
    
    thread_5.join();
    
    thread_6.join();
    
    thread_7.join();
    
    thread_8.join();
    
    thread_9.join();
    
    thread_10.join();
    
    thread_11.join();
    
    thread_12.join();
    
    thread_13.join();
    
    thread_14.join();
    
    thread_15.join();
     
}