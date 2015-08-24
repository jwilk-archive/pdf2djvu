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

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "system.hh"
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

static const std::string argv_to_command_line(const std::vector<std::string> &argv)
/* Translate a sequence of arguments into a command line string. */
{
    std::ostringstream buffer;
    for (std::vector<std::string>::const_iterator parg = argv.begin(); parg != argv.end(); parg++) {
        buffer << "'";
        if (parg->find("\'") == std::string::npos)
            buffer << *parg;
        else
            for (std::string::const_iterator pch = parg->begin(); pch != parg->end(); pch++) {
                if (*pch == '\'')
                    buffer << "'\\''";
                else
                    buffer << *pch;
            }
        buffer << "' ";
    }
    return buffer.str();
}

void Command::call(std::ostream *my_stdout, bool quiet)
{
    int status;
    FILE *file;
    {
        std::string command_line = argv_to_command_line(this->argv);
        if (quiet)
            command_line += " 2>/dev/null";
        file = ::popen(command_line.c_str(), "r");
    }
    if (file != NULL) {
        char buffer[BUFSIZ];
        size_t nbytes;
        status = 0;
        while (!feof(file)) {
            nbytes = fread(buffer, 1, sizeof buffer, file);
            if (ferror(file)) {
                status = -1;
                break;
            }
            if (my_stdout != NULL)
                my_stdout->write(buffer, nbytes);
        }
        int wait_status = pclose(file);
        if (wait_status == -1)
            throw_posix_error("pclose");
        else if (status == 0) {
            if (WIFEXITED(wait_status)) {
                unsigned int exit_code = WEXITSTATUS(wait_status);
                if (exit_code != 0) {
                    std::string message = string_printf(
                        _("External command \"%s ...\" failed with exit code %u"),
                        this->command.c_str(),
                        exit_code
                    );
                    throw CommandFailed(message);
                }
            }
            else
                status = -1;
        }
    }
    else
        status = -1;
    if (status < 0) {
        std::string message = string_printf(
            _("External command \"%s ...\" failed"),
            this->command.c_str()
        );
        throw_posix_error(message);
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
                message = string_printf(
                    _("External command \"%s\" failed with exit code %u"),
                    command_line.c_str(),
                    static_cast<unsigned int>(WEXITSTATUS(status))
                );
            } else {
                message = string_printf(
                    _("External command \"%s\" failed"),
                    command_line.c_str()
                );
            }
            throw CommandFailed(message);
        }
        return stream.str();
    }
    return string; /* Should not really happen. */
}

#endif

// vim:ts=4 sts=4 sw=4 et
