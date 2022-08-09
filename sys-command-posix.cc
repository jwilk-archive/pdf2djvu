/* Copyright © 2007-2022 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
 *
 * This file is part of pdf2djvu.
 *
 * pdf2djvu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * pdf2djvu is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#if !WIN32

#include "autoconf.hh"
#include "system.hh"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

#if _OPENMP
#include <omp.h>
#endif

#include "string-printf.hh"
#include "i18n.hh"

Command::Command(const std::string& command) : command(command)
{
    this->argv.push_back(command);
}

Command &Command::operator <<(const std::string& arg)
{
    this->argv.push_back(arg);
    return *this;
}

Command &Command::operator <<(const File& arg)
{
    this->argv.push_back(arg);
    return *this;
}

Command &Command::operator <<(int i)
{
    std::ostringstream stream;
    stream << i;
    return *this << stream.str();
}

static int get_max_fd()
{
    int max_fd_per_thread = 16; // rough estimate
    int max_fd = 16;
#if _OPENMP
    max_fd += max_fd_per_thread * omp_get_num_threads();
#else
    max_fd += max_fd_per_thread;
#endif
    return max_fd;
}

static void fd_close(int fd)
{
    int rc = close(fd);
    if (rc < 0)
        throw_posix_error("close()");
}

static void mkfifo(int fd[2], int add_flags=0)
{
    int rc = pipe(fd);
    if (rc < 0)
        throw_posix_error("pipe()");
    for (int i = 0; i < 2; i++) {
        // file descriptor flags:
        int rc = fcntl(fd[i], F_SETFD, FD_CLOEXEC);
        if (rc < 0)
            throw_posix_error("fcntl(fd, F_SETFD, FD_CLOEXEC)");
        // file status flags:
        int fd_add_flags = add_flags;
        if (i == 0)
            // non-blocking reads are not useful:
            fd_add_flags &= ~O_NONBLOCK;
        if (fd_add_flags) {
            int status_flags = fcntl(fd[i], F_GETFL);
            if (status_flags < 0)
                throw_posix_error("fcntl(fd, F_GETFL)");
            rc = fcntl(fd[i], F_SETFL, status_flags | fd_add_flags);
            if (rc < 0)
                throw_posix_error("fcntl(fd, F_SETFL, ...)");
        }
    }
}

static int fd_close_range(int fd_from, int fd_to, int fd_except=-1)
{
    for (int fd = fd_from; fd <= fd_to; fd++) {
        if (fd == fd_except)
            continue;
        int rc = close(fd);
        if (rc == 0 || errno == EBADF)
            continue;
        else
            return rc;
    }
    return 0;
}

static void report_posix_error(int fd, const char *context)
{
    int errno_copy = errno;
    ssize_t n = write(fd, &errno_copy, sizeof errno_copy);
    if (n != sizeof errno_copy)
        return;
    n = write(fd, context, strlen(context));
    // There isn't much we can do here about write() failing,
    // so let's silence the compiler warning.
    (void) n;
}

static const char * get_signal_name(int sig)
{
    switch (sig) {
#define s(n) case n: return #n;
    // POSIX.1-1990:
    s(SIGHUP);
    s(SIGINT);
    s(SIGQUIT);
    s(SIGILL);
    s(SIGABRT);
    s(SIGFPE);
    s(SIGKILL);
    s(SIGSEGV);
    s(SIGPIPE);
    s(SIGALRM);
    s(SIGTERM);
    s(SIGUSR1);
    s(SIGUSR2);
    s(SIGCHLD);
    s(SIGCONT);
    s(SIGSTOP);
    s(SIGTSTP);
    s(SIGTTIN);
    s(SIGTTOU);
    // SUSv2 and POSIX.1-2001:
    s(SIGBUS);
#ifdef SIGPOLL
    // not supported on OpenBSD
    s(SIGPOLL);
#endif
    s(SIGPROF);
    s(SIGSYS);
    s(SIGTRAP);
    s(SIGURG);
    s(SIGVTALRM);
    s(SIGXCPU);
    s(SIGXFSZ);
#undef s
    default:
        return nullptr;
    }
}

std::string Command::repr()
{
    bool sh = (
        this->argv.size() == 3 &&
        this->argv[0] == std::string("sh") &&
        this->argv[1] == std::string("-c")
    );
    if (sh)
        return this->argv[2];
    else
        return string_printf(
            // L10N: "<command> ..."
            _("%s ..."),
            this->command.c_str()
        );
}

void Command::call(std::istream *stdin_, std::ostream *stdout_, bool stderr_)
{
    int rc;
    int max_fd = get_max_fd();
    int stdout_pipe[2];
    int stdin_pipe[2];
    int error_pipe[2];
    size_t argc = this->argv.size();
    std::vector<const char *> c_argv(argc + 1);
    for (size_t i = 0; i < argc; i++)
        c_argv[i] = argv[i].c_str();
    c_argv[argc] = nullptr;
    assert(c_argv[0] != nullptr);
    mkfifo(stdout_pipe);
    mkfifo(stdin_pipe, O_NONBLOCK);
    mkfifo(error_pipe);
    pid_t pid = fork();
    if (pid < 0)
        throw_posix_error("fork()");
    if (pid == 0) {
        // The child:
        // At this point, only async-signal-safe functions can be used.
        // See the signal(7) manpage for the full list.
        rc = dup2(stdin_pipe[0], STDIN_FILENO);
        if (rc < 0) {
            report_posix_error(error_pipe[1], "dup2()");
            abort();
        }
        rc = dup2(stdout_pipe[1], STDOUT_FILENO);
        if (rc < 0) {
            report_posix_error(error_pipe[1], "dup2()");
            abort();
        }
        if (!stderr_) {
            int fd = open("/dev/null", O_WRONLY);
            if (fd < 0) {
                report_posix_error(error_pipe[1], "open()");
                abort();
            }
            rc = dup2(fd, STDERR_FILENO);
            if (rc < 0) {
                report_posix_error(error_pipe[1], "dup2()");
                abort();
            }
        }
        int rc = fd_close_range(STDERR_FILENO + 1, max_fd, error_pipe[1]);
        if (rc < 0) {
            report_posix_error(error_pipe[1], "close()");
            abort();
        }
        execvp(c_argv[0],
            const_cast<char * const *>(c_argv.data())
        );
        report_posix_error(error_pipe[1], "\xFF");
        abort();
    }
    // The parent:
    fd_close(stdin_pipe[0]);
    fd_close(stdout_pipe[1]);
    fd_close(error_pipe[1]);
    char buffer[BUFSIZ];
    struct pollfd fds[2];
    if (stdin_)
        fds[0].fd = stdin_pipe[1];
    else {
        fds[0].fd = -1;
        fd_close(stdin_pipe[1]);
    }
    fds[0].events = POLLOUT;
    fds[1].fd = stdout_pipe[0];
    fds[1].events = POLLIN;
    while (1) {
        rc = poll(fds, 2, -1);
        if (rc < 0)
            throw_posix_error("poll()");
        if (fds[0].revents) {
            assert(stdin_);
            std::streamsize rbytes = stdin_->readsome(buffer, sizeof buffer);
            if (rbytes == 0) {
                fd_close(stdin_pipe[1]);
                fds[0].fd = -1;
            } else {
                ssize_t wbytes = write(stdin_pipe[1], buffer, rbytes);
                if (wbytes < 0)
                    throw_posix_error("write()");
                stdin_->seekg(wbytes - rbytes, std::ios_base::cur);
            }
        }
        if (fds[1].revents) {
            ssize_t nbytes = read(stdout_pipe[0], buffer, sizeof buffer);
            if (nbytes < 0)
                throw_posix_error("read()");
            if (nbytes == 0)
                break;
            if (stdout_)
                stdout_->write(buffer, nbytes);
        }
    }
    if (stdin_) {
        std::streamsize rbytes = stdin_->readsome(buffer, 1);
        if (rbytes > 0) {
            // The child process terminated,
            // even though it didn't receive the complete input.
            errno = EPIPE;
            throw_posix_error("write()");
        }
    }
    fd_close(stdout_pipe[0]);
    int wait_status;
    pid = waitpid(pid, &wait_status, 0);
    if (pid < 0)
        throw_posix_error("waitpid()");
    int child_errno = 0;
    ssize_t nbytes = read(error_pipe[0], &child_errno, sizeof child_errno);
    if (nbytes < 0)
        throw_posix_error("read()");
    if (nbytes > 0 && static_cast<size_t>(nbytes) < sizeof child_errno) {
        errno = EIO;
        throw_posix_error("read()");
    }
    if (child_errno > 0) {
        char child_error_reason[BUFSIZ];
        ssize_t nbytes = read(
            error_pipe[0],
            child_error_reason,
            (sizeof child_error_reason) - 1
        );
        if (nbytes < 0)
            throw_posix_error("read()");
        fd_close(error_pipe[0]);
        child_error_reason[nbytes] = '\0';
        errno = child_errno;
        if (child_error_reason[0] != '\xFF')
            throw_posix_error(child_error_reason);
        std::string child_error = POSIXError::error_message("");
        std::string message = string_printf(
            _("External command \"%s\" failed: %s"),
            this->repr().c_str(),
            child_error.c_str()
        );
        throw CommandFailed(message);
    }
    fd_close(error_pipe[0]);
    if (WIFEXITED(wait_status)) {
        unsigned long exit_status = WEXITSTATUS(wait_status);
        if (exit_status != 0) {
            std::string message = string_printf(
                _("External command \"%s\" failed with exit status %lu"),
                this->repr().c_str(),
                exit_status
            );
            throw CommandFailed(message);
        }
    } else if (WIFSIGNALED(wait_status)) {
        int sig = WTERMSIG(wait_status);
        const char * signame = get_signal_name(sig);
        std::string message;
        if (signame)
            message = string_printf(
                // L10N: the latter argument is an untranslated signal name
                // (such as "SIGSEGV")
                _("External command \"%s\" was terminated by %s"),
                this->repr().c_str(),
                signame
            );
        else
            message = string_printf(
                _("External command \"%s\" was terminated by signal %d"),
                this->repr().c_str(),
                sig
            );
        throw CommandFailed(message);
    } else {
        // should not happen
        errno = EINVAL;
        throw_posix_error("waitpid()");
    }
}

std::string Command::filter(const std::string &command_line, const std::string &string)
{
    std::istringstream stdin_(string);
    std::ostringstream stdout_;
    Command cmd("sh");
    cmd << "-c" << command_line;
    cmd.call(&stdin_, &stdout_, true);
    return stdout_.str();
}

#endif

// vim:ts=4 sts=4 sw=4 et
