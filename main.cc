
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <iostream>
#include <exception>

class a_semaphore {
private:
    const char *name_;
    sem_t *id_;
public:
    a_semaphore(const char *name)
        : name_(name), id_(sem_open(name, O_CREAT, S_IRWXU, 1)) {
        if(SEM_FAILED != id_) {
            int v = -1;
            if(!sem_getvalue(id_, &v) && !v) {
                sem_post(id_);
            }
        }
    }
    ~a_semaphore() {
        if(SEM_FAILED != id_) {
            std::cout << "Closing sem(" << name_ << ")\n" << std::flush;
            sem_close(id_);
        }
    }
    
    bool is_open() const {
        return SEM_FAILED != id_;
    }
    
    class failure : public std::exception {
    public:
        virtual ~failure() throw() { }
        virtual const char* what() const throw() {
            return "a_semaphore not initialized";
        }
    };
    
    void acquire() {
        if(!is_open()) throw failure();
        sem_wait(id_);
    }
    void release() {
        if(!is_open()) throw failure();
        sem_post(id_);
    }
};

static a_semaphore &
sem1() {
    static a_semaphore s1("sem1");
    return s1;
}

static a_semaphore &
sem2() {
    static a_semaphore s2("sem2");
    return s2;
}

void *
thread_func_processor(void *var) {
    std::cout << "thread_func_processor()\n" << std::flush;
    int *counter = static_cast<int *>(var);
    sem1().acquire();
    while(true) {
        sem2().acquire();
        (*counter)++;
        sem1().release();
        if(*counter > 100) break;
    }
    return 0;
}

void *
thread_func_reader(void *var) {
    std::cout << "thread_func_reader()\n" << std::flush;
    int *counter = static_cast<int *>(var);
    while(true) {
        sem1().acquire();
        std::cout << "Reader: " << *counter << '\n' << std::flush;
        sem2().release();
        if(*counter > 100) break;
    }
    return 0;
}

int
main(int argc, char** argv) {
    int shared_var = 0;
    pthread_t processor_tid;
    pthread_t reader_tid;

    if(pthread_create(&processor_tid, 0, &thread_func_processor, &shared_var) ||
       pthread_create(&reader_tid, 0, &thread_func_reader, &shared_var)) {
        std::cerr << "Error creating threads...\n";
    }
    
    pthread_join(reader_tid, 0);
    pthread_join(processor_tid, 0);
    
    std::cout << "Finally, shared_var = " << shared_var << '\n';
    
    return 0;
}

