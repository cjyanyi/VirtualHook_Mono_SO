//
// VirtualApp Native Project
//
#include <MSHook/Hooker.h>
#include "IOUniformer.h"

static std::list<std::string> ReadOnlyPathMap;
static std::list<std::pair<std::string, std::string> > IORedirectMap;
int apiLevel;

static inline void
hook_template(void *handle, const char *symbol, void *new_func, void **old_func) {
    void *addr = dlsym(handle, symbol);
    if (addr == NULL) {
        LOGW("Error: unable to find the Symbol : %s.", symbol);
        return;
    }
    Cydia::MSHookFunction(addr, new_func, NULL);
}

static const char **patchArgv(char * const *argv) {
    int i=0,j;
    while(argv[i] != NULL) {
        i++;
    }
    const char **res = (const char **)malloc((i+3)*sizeof(char *));
    for(j=0; j<i; j++) {
        res[j] = argv[j];
    }
    if(apiLevel > ANDROID_L) {
        res[j] = "--compile-pic";
        j++;
    }
    if(apiLevel >= ANDROID_M) {
        res[j] = "--debuggable";
        j++;
    }
    res[j] = NULL;
    return res;
}


static inline bool startWith(const std::string &str, const std::string &prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

static void add_pair(const char *_orig_path, const char *_new_path) {
    std::string origPath = std::string(_orig_path);
    std::string newPath = std::string(_new_path);

    IORedirectMap.push_back(std::pair<std::string, std::string>(origPath, newPath));
}

// define as macro since alloca is used
#define match_redirected_path(_path) ({\
    const char *res = _path;\
    size_t _pathLen = strlen(_path);\
    if(_pathLen > 1) {\
        std::list<std::pair<std::string, std::string> >::iterator iterator;\
        for (iterator = IORedirectMap.begin(); iterator != IORedirectMap.end(); iterator++) {\
            const char *originPath = iterator->first.c_str();\
            size_t originPathLen = iterator->first.size();\
            const char *newPath = iterator->second.c_str();\
            size_t newPathLen = iterator->second.size();\
            if(!strncmp(_path, originPath, originPathLen)) {\
                if (originPathLen == _pathLen) {\
                    res = newPath;\
                    break;\
                } else {\
                    unsigned int resSize = (newPathLen+_pathLen-originPathLen+1);\
                    res = (char *)alloca(sizeof(char) * resSize);\
                    strcpy((char *)res, newPath);\
                    strcpy((char *)res + newPathLen, _path + originPathLen);\
                    break;\
                }\
            }\
        }\
    }\
    res;\
})


void IOUniformer::redirect(const char *orig_path, const char *new_path) {
    LOGI("Start Java_nativeRedirect : from %s to %s", orig_path, new_path);
    add_pair(orig_path, new_path);
}

const char *IOUniformer::query(const char *orig_path) {
    return strdup(match_redirected_path(orig_path));
}

void IOUniformer::readOnly(const char *_path) {
    std::string path(_path);
    ReadOnlyPathMap.push_back(path);
}

static bool isReadOnlyPath(const char *_path) {
//    std::string path(_path);
    std::list<std::string>::iterator it;
    for (it = ReadOnlyPathMap.begin(); it != ReadOnlyPathMap.end(); ++it) {
        if (!strncmp(_path, it->c_str(), it->size())) {
            return true;
        }
    }
    return false;
}


const char *IOUniformer::restore(const char *_path) {

    if (_path == NULL) {
        return NULL;
    }
    std::string path(_path);
    if (path.length() <= 1) {
        return _path;
    }
    std::list<std::pair<std::string, std::string> >::iterator iterator;
    for (iterator = IORedirectMap.begin(); iterator != IORedirectMap.end(); iterator++) {
        const std::string &prefix = iterator->first;
        const std::string &new_prefix = iterator->second;
        if (startWith(path, new_prefix)) {
            std::string origin_path = prefix + path.substr(new_prefix.length(), path.length());
            return strdup(origin_path.c_str());
        }
    }
    return _path;
}


__BEGIN_DECLS



// int faccessat(int dirfd, const char *pathname, int mode, int flags);
HOOK_DEF(int, faccessat, int dirfd, const char *pathname, int mode, int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_faccessat, dirfd, redirect_path, mode, flags);
    return ret;
}


// int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags);
HOOK_DEF(int, fchmodat, int dirfd, const char *pathname, mode_t mode, int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    if (isReadOnlyPath(redirect_path)) {
        return -1;
    }
    int ret = syscall(__NR_fchmodat, dirfd, redirect_path, mode, flags);
    return ret;
}

// int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);
HOOK_DEF(int, fstatat, int dirfd, const char *pathname, struct stat *buf, int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_fstatat64, dirfd, redirect_path, buf, flags);
    return ret;
}

// int mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev);
HOOK_DEF(int, mknodat, int dirfd, const char *pathname, mode_t mode, dev_t dev) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_mknodat, dirfd, redirect_path, mode, dev);
    return ret;
}

// int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags);
HOOK_DEF(int, utimensat, int dirfd, const char *pathname, const struct timespec times[2],
         int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_utimensat, dirfd, redirect_path, times, flags);
    return ret;
}

// int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
HOOK_DEF(int, fchownat, int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    if (isReadOnlyPath(redirect_path)) {
        return -1;
    }
    int ret = syscall(__NR_fchownat, dirfd, redirect_path, owner, group, flags);
    return ret;
}

// int chroot(const char *pathname);
HOOK_DEF(int, chroot, const char *pathname) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_chroot, redirect_path);
    return ret;
}


// int renameat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath);
HOOK_DEF(int, renameat, int olddirfd, const char *oldpath, int newdirfd, const char *newpath) {
    const char *redirect_path_old = match_redirected_path(oldpath);
    const char *redirect_path_new = match_redirected_path(newpath);
    if (isReadOnlyPath(redirect_path_old) || isReadOnlyPath(redirect_path_new)) {
        return -1;
    }
    int ret = syscall(__NR_renameat, olddirfd, redirect_path_old, newdirfd, redirect_path_new);
    return ret;
}

// int unlinkat(int dirfd, const char *pathname, int flags);
HOOK_DEF(int, unlinkat, int dirfd, const char *pathname, int flags) {
    const char *redirect_path = match_redirected_path(pathname);
    if (isReadOnlyPath(redirect_path)) {
        return -1;
    }
    int ret = syscall(__NR_unlinkat, dirfd, redirect_path, flags);
    return ret;
}

// int symlinkat(const char *oldpath, int newdirfd, const char *newpath);
HOOK_DEF(int, symlinkat, const char *oldpath, int newdirfd, const char *newpath) {
    const char *redirect_path_old = match_redirected_path(oldpath);
    const char *redirect_path_new = match_redirected_path(newpath);
    int ret = syscall(__NR_symlinkat, redirect_path_old, newdirfd, redirect_path_new);
    return ret;
}

// int linkat(int olddirfd, const char *oldpath, int newdirfd, const char *newpath, int flags);
HOOK_DEF(int, linkat, int olddirfd, const char *oldpath, int newdirfd, const char *newpath,
         int flags) {
    const char *redirect_path_old = match_redirected_path(oldpath);
    const char *redirect_path_new = match_redirected_path(newpath);
    if (isReadOnlyPath(redirect_path_old) || isReadOnlyPath(newpath)) {
        return -1;
    }
    int ret = syscall(__NR_linkat, olddirfd, redirect_path_old, newdirfd, redirect_path_new, flags);
    return ret;
}

// int utimes(const char *filename, const struct timeval *tvp);
HOOK_DEF(int, utimes, const char *pathname, const struct timeval *tvp) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_utimes, redirect_path, tvp);
    return ret;
}

// int mkdirat(int dirfd, const char *pathname, mode_t mode);
HOOK_DEF(int, mkdirat, int dirfd, const char *pathname, mode_t mode) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_mkdirat, dirfd, redirect_path, mode);
    return ret;
}

// int readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz);
HOOK_DEF(int, readlinkat, int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_readlinkat, dirfd, redirect_path, buf, bufsiz);
    return ret;
}

// int __statfs64(const char *path, size_t size, struct statfs *stat);
HOOK_DEF(int, __statfs64, const char *pathname, size_t size, struct statfs *stat) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_statfs64, redirect_path, size, stat);
    return ret;
}


// int truncate(const char *path, off_t length);
HOOK_DEF(int, truncate, const char *pathname, off_t length) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_truncate, redirect_path, length);
    return ret;
}

// int truncate64(const char *pathname, off_t length);
HOOK_DEF(int, truncate64, const char *pathname, off_t length) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_truncate64, redirect_path, length);
    return ret;
}


// int chdir(const char *path);
HOOK_DEF(int, chdir, const char *pathname) {
    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_chdir, redirect_path);
    return ret;
}

// int __openat(int fd, const char *pathname, int flags, int mode);
HOOK_DEF(int, __openat, int fd, const char *pathname, int flags, int mode) {
    const char *redirect_path = match_redirected_path(pathname);
//    LOGE("opening file %s, redirected to %s", pathname, redirect_path);
    int ret = syscall(__NR_openat, fd, redirect_path, flags, mode);
    return ret;
}

// int (*origin_execve)(const char *pathname, char *const argv[], char *const envp[]);
HOOK_DEF(int, execve, const char *pathname, char *const argv[], char *const envp[]) {
    const char **newArgv;
    if (!strcmp(pathname, "/system/bin/dex2oat")) {
        newArgv = patchArgv(argv);
    }
    else {
        newArgv = (const char **)argv;
    }

#ifdef DEBUG
    LOGD("execve: %s, LD_PRELOAD: %s.", pathname, getenv("LD_PRELOAD"));
    for (int i = 0; argv[i] != NULL; ++i) {
        LOGD("argv[%i] : %s", i, newArgv[i]);
    }
    // That string array is NULL terminated
    new_env[i] = NULL;
    for (int n = 0; new_env[n] != NULL; ++n) {
        LOGE("new_env[%i] = %s", n, new_env[n]);
    }
#endif

    const char *redirect_path = match_redirected_path(pathname);
    int ret = syscall(__NR_execve, redirect_path, newArgv, envp);
    FREE(newArgv, argv);
    return ret;
}

// int kill(pid_t pid, int sig);
HOOK_DEF(int, kill, pid_t pid, int sig) {
    LOGD(">>>>> kill >>> pid: %d, sig: %d.", pid, sig);
    extern JavaVM *gVm;
    extern jclass gClass;
    JNIEnv *env = NULL;
    gVm->GetEnv((void **) &env, JNI_VERSION_1_4);
    gVm->AttachCurrentThread(&env, NULL);
    jmethodID method = env->GetStaticMethodID(gClass, "onKillProcess", "(II)V");
    env->CallStaticVoidMethod(gClass, method, pid, sig);
    int ret = syscall(__NR_kill, pid, sig);
    return ret;
}

HOOK_DEF(pid_t, vfork) {
    return fork();
}

__END_DECLS
// end IO DEF

void IOUniformer::hookExec(int api_level) {
    apiLevel = api_level;
    HOOK_SYMBOL(RTLD_DEFAULT, execve);
//    LOGD("hook exec done");
}

void IOUniformer::startUniformer() {
    HOOK_SYMBOL(RTLD_DEFAULT, chroot);
    HOOK_SYMBOL(RTLD_DEFAULT, vfork);
    HOOK_SYMBOL(RTLD_DEFAULT, kill);
    HOOK_SYMBOL(RTLD_DEFAULT, chdir);
    HOOK_SYMBOL(RTLD_DEFAULT, truncate);

    HOOK_SYMBOL(RTLD_DEFAULT, utimes);
    HOOK_SYMBOL(RTLD_DEFAULT, fstatat);
    HOOK_SYMBOL(RTLD_DEFAULT, fchmodat);
    HOOK_SYMBOL(RTLD_DEFAULT, symlinkat);
    HOOK_SYMBOL(RTLD_DEFAULT, readlinkat);
    HOOK_SYMBOL(RTLD_DEFAULT, unlinkat);
    HOOK_SYMBOL(RTLD_DEFAULT, linkat);
    HOOK_SYMBOL(RTLD_DEFAULT, utimensat);
    HOOK_SYMBOL(RTLD_DEFAULT, __openat);
    HOOK_SYMBOL(RTLD_DEFAULT, faccessat);
    HOOK_SYMBOL(RTLD_DEFAULT, mkdirat);
    HOOK_SYMBOL(RTLD_DEFAULT, renameat);
    HOOK_SYMBOL(RTLD_DEFAULT, fchownat);
    HOOK_SYMBOL(RTLD_DEFAULT, mknodat);
}
