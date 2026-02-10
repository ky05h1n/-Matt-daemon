#include "../header/Md_header.hpp"

volatile sig_atomic_t g_sig = 0;

void signalHandler(int sig) {
    g_sig = sig;
}


Atr::Atr()
{
    std::cout << "Starting Daemon..." << std::endl;
    logpath = "/var/log/";
    logfile = "matt_daemon.log";
    lockFilePath = "/var/lock/matt_daemon.lock";
    root = "/";
    lockFd = -1;
    tempfd = 0;
    Obj.fd = 0;
}



uid_t GetEffectiveUserId() {

    uid_t euid = geteuid();
    return euid;
}


Atr::~Atr()
{
   
}


bool Atr::CheckFiles_Dirs(){
    
        struct stat st;
        if (stat(logpath.c_str(), &st) != 0) {
            if (mkdir(logpath.c_str(), 0755) != 0) {
                perror("mkdir logpath");
                return false;
            }
        }


        if (chdir(this->root.c_str()) == 0)
        {
            std::string fullpath = logpath + logfile;
            this->Obj.setLogFilePath(fullpath);
            this->Obj.fd = open(fullpath.c_str(), O_RDWR | O_CREAT | O_APPEND, 0644);
            if (this->Obj.fd == -1)
            {
                std::cout << "daemon : can't create log file" << std::endl;
                return false;
            }
            return true;
        }
        else
        {
            std::cout << "daemon : can't access to 'root' folder" << std::endl;
            return false;
        }
}

bool Atr::CreateLockFile(){

    this->lockFd = open(this->lockFilePath.c_str(), O_CREAT | O_RDWR, 0644);
    if (this->lockFd < 0)
        return false;

    if (flock(lockFd, LOCK_EX | LOCK_NB) < 0)
    {
        
        close(lockFd);
        return false;
    }
    else
    {
        ftruncate(lockFd, 0);
        pid_t pid = getpid();
        std::string pid_string = std::to_string(pid);
        ssize_t n = write(lockFd, pid_string.c_str(), pid_string.size());
        if (n != static_cast<ssize_t>(pid_string.size())) {
            perror("write lockfile");
            close(lockFd);
            return false;
        }      

    }                   
    return true;
}
        

void Atr::RemoveLockfile(){
    if (this->lockFd >= 0) {
        flock(lockFd, LOCK_UN);
        close(lockFd);
        unlink(lockFilePath.c_str());
        lockFd = -1;
    }
}


void Atr::Daemon(){

    
    pid_t pid = fork();
 
    if (pid == 0){
        pid_t s_id = setsid();
        umask(0);
        if (s_id < 0)
        {
            std::cout << "Failed To Create New Session !" << std::endl;
            exit(1);
        }
        std::cout << "Started ! Daemon Running With PID : " << getpid() << std::endl;
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        this->tempfd = open("/dev/null", O_RDWR);
        if (this->tempfd < 0) { 
            perror("open /dev/null"); 
            exit(1); 
        }
        dup2(this->tempfd, STDIN_FILENO);
        dup2(this->tempfd, STDOUT_FILENO);
        dup2(this->tempfd, STDERR_FILENO);

        int signals[] = {
            SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT,
            SIGBUS, SIGFPE, SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE,
            SIGALRM, SIGTERM, SIGCHLD, SIGCONT, SIGTSTP, SIGTTIN,
            SIGTTOU, SIGURG, SIGXCPU, SIGXFSZ, SIGVTALRM, SIGPROF,
            SIGWINCH, SIGIO, SIGSYS
        };
        for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++)
            signal(signals[i], signalHandler);

        if (!this->CheckFiles_Dirs())
            exit(1);

        this->Run();
        exit(0);
        
    }
    else if (pid == -1){
    
        std::cout << "Fork Failed To Run !" << std::endl;
        return;
    }
    if (pid > 0)
        return;
}

