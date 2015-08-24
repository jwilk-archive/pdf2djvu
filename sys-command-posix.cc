/* Copyright © 2007-2015 Jakub Wilk <jwilk@jwilk.net>
 * Copyright © 2009 Mateusz Turcza
 *
 * This file is part of pdfdjvu.
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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#if _OPENMP
#include <omp.h>
#endif

#include "array.hh"
#include "i18n.hh"
#include "system.hh"

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
    int max_fd_per_thread = 32; // rough estimate
#if _OPENMP
    return max_fd_per_thread * omp_get_num_threads();
#else
    return max_fd_per_thread;
#endif
}

static void fd_close(int fd)
{
    int rc = close(fd);
    if (rc < 0)
        throw_posix_error("close()");
}

static void mkfifo(int fd[2])
{
    int rc = pipe(fd);
    if (rc < 0)
        throw_posix_error("pipe()");
}

static int fd_set_cloexec(int fd_from, int fd_to)
{
    for (int fd = fd_from; fd <= fd_to; fd++) {
        int fdflags = fcntl(fd, F_GETFD);
        if (fdflags < 0) {
            if (errno == EBADF)
                continue;
            return -1;
        }
        fdflags |= FD_CLOEXEC;
        int rc = fcntl(fd, F_SETFD, fdflags);
        if (rc < 0)
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
    write(fd, context, strlen(context));
}

const std::string signame(int sig)
{
    switch (sig) {
#define s(n) case n: return "" # n "";
    /* POSIX.1-1990: */
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
    /* SUSv2 and POSIX.1-2001: */
    s(SIGBUS);
    s(SIGPOLL);
    s(SIGPROF);
    s(SIGSYS);
    s(SIGTRAP);
    s(SIGURG);
    s(SIGVTALRM);
    s(SIGXCPU);
    s(SIGXFSZ);
#undef s
    default:
        return string_printf(_("signal %d"), sig);
    }
}

void Command::call(std::ostream *my_stdout, bool quiet)
{
    int rc;
    int max_fd = get_max_fd();
    int stdio_pipe[2];
    int error_pipe[2];
    size_t argc = this->argv.size();
    Array<const char *> c_argv(argc + 1);
    for (size_t i = 0; i < argc; i++)
        c_argv[i] = argv[i].c_str();
    c_argv[argc] = NULL;
    mkfifo(stdio_pipe);
    mkfifo(error_pipe);
    pid_t pid = fork();
    if (pid < 0)
        throw_posix_error("fork()");
    if (pid == 0) {
        /* The child:
         * At this point, only async-singal-safe functions can be used.
         * See the signal(7) manpage for the full list.
         */
        rc = dup2(stdio_pipe[1], STDOUT_FILENO);
        if (rc < 0) {
            report_posix_error(error_pipe[1], "dup2()");
            abort();
        }
        if (quiet) {
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
        int rc = fd_set_cloexec(STDERR_FILENO + 1, max_fd);
        if (rc < 0) {
            report_posix_error(error_pipe[1], "fcntl()");
            abort();
        }
        execvp(c_argv[0],
            const_cast<char * const *>(
                static_cast<const char **>(c_argv)
            )
        );
        report_posix_error(error_pipe[1], "\xff");
        abort();
    }
    /* The parent: */
    fd_close(stdio_pipe[1]);
    fd_close(error_pipe[1]);
    char buffer[BUFSIZ];
    while (1) {
        ssize_t nbytes = read(stdio_pipe[0], buffer, sizeof buffer);
        if (nbytes < 0)
            throw_posix_error("read()");
        if (nbytes == 0)
            break;
        my_stdout->write(buffer, nbytes);
    }
    fd_close(stdio_pipe[0]);
    int wait_status;
    pid = waitpid(pid, &wait_status, 0);
    if (pid < 0)
        throw_posix_error("waitpid()");
    int child_errno = 0;
    ssize_t nbytes = read(error_pipe[0], &child_errno, sizeof child_errno);
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
        if (child_error_reason[0] != '\xff')
            throw_posix_error(child_error_reason);
        std::string child_error = POSIXError::error_message("");
        std::string message = string_printf(
            _("External command \"%s ...\" failed: %s"),
            this->command.c_str(),
            child_error.c_str()
        );
        throw CommandFailed(message);
    }
    fd_close(error_pipe[0]);
    if (WIFEXITED(wait_status)) {
        unsigned long exit_status = WEXITSTATUS(wait_status);
        if (exit_status != 0) {
            std::string message = string_printf(
                _("External command \"%s ...\" failed with exit status %lu"),
                this->command.c_str(),
                exit_status
            );
            throw CommandFailed(message);
        }
    } else if (WIFSIGNALED(wait_status)) {
        int sig = WTERMSIG(wait_status);
        std::string message = string_printf(
            _("External command \"%s ...\" was terminated by %s"),
            this->command.c_str(),
            signame(sig).c_str()
        );
        throw CommandFailed(message);
    } else {
        // should not happen
        errno = EINVAL;
        throw_posix_error("waitpid()");
    }
}

std::string Command::filter(const std::string &command_line, const std::string string)
{
    int rc;
    int pipe_fds[2];
    rc = pipe(pipe_fds);
    if (rc != 0)
        throw_posix_error("pipe");
    pid_t writer_pid = fork();
    if (writer_pid == -1)
        throw_posix_error("fork");
    else if (writer_pid == 0) {
        /* Writer: */
        close(pipe_fds[0]); /* Deliberately ignore errors. */
        rc = dup2(pipe_fds[1], STDOUT_FILENO);
        if (rc == -1)
            throw_posix_error("dup2");
        FILE *fp = popen(command_line.c_str(), "w");
        if (fp == NULL)
            throw_posix_error("popen");
        if (fputs(string.c_str(), fp) == EOF)
            throw_posix_error("fputs");
        rc = pclose(fp);
        if (rc == -1)
            throw_posix_error("pclose");
        else if (WIFEXITED(rc))
            exit(WEXITSTATUS(rc));
        else
            exit(-1);
        exit(rc);
    } else {
        /* Main process: */
        close(pipe_fds[1]); /* Deliberately ignore errors. */
        std::ostringstream stream;
        while (1) {
            char buffer[BUFSIZ];
            ssize_t n = read(pipe_fds[0], buffer, sizeof buffer);
            if (n < 0)
                throw_posix_error("read");
            else if (n == 0)
                break;
            else
                stream.write(buffer, n);
        }
        int status;
        writer_pid = wait(&status);
        if (writer_pid == static_cast<pid_t>(-1))
            throw_posix_error("wait");
        if (status != 0) {
            std::string message;
            if (WIFEXITED(status)) {
                unsigned long exit_status = WEXITSTATUS(status);
                message = string_printf(
                    _("External command \"%s\" failed with exit status %lu"),
                    command_line.c_str(),
                    exit_status
                );
            } else if (WIFSIGNALED(status)) {
                int sig = WTERMSIG(status);
                message = string_printf(
                    _("External command \"%s\" was terminated by %s"),
                    command_line.c_str(),
                    signame(sig)
                );
            } else {
                // should not happen
                errno = EINVAL;
                throw_posix_error("wait()");
            }
            throw CommandFailed(message);
        }
        return stream.str();
    }
    return string; /* Should not really happen. */
}

#endif

// vim:ts=4 sts=4 sw=4 et
