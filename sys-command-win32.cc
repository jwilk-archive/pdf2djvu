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

#if WIN32

#include "system.hh"

#include <cassert>
#include <sstream>

#include <windows.h>

#include "i18n.hh"
#include "string-printf.hh"

Command::Command(const std::string& command) : command(command)
{
    // Convert path separators:
    std::ostringstream stream;
    for (char c : command) {
        if (c == '/')
            stream << '\\';
        else
            stream << c;
    }
    this->command = stream.str();
    this->argv.push_back(this->command);
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
// Translate a sequence of arguments into a command line string.
{
    std::ostringstream buffer;
    // Using the same rules as the MS C runtime:
    //
    // 1) Arguments are delimited by white space, which is either a space or a
    //    tab.
    //
    // 2) A string surrounded by double quotation marks is interpreted as a
    //    single argument, regardless of white space contained within. A
    //    quoted string can be embedded in an argument.
    //
    // 3) A double quotation mark preceded by a backslash is interpreted as a
    //    literal double quotation mark.
    //
    // 4) Backslashes are interpreted literally, unless they immediately
    //    precede a double quotation mark.
    //
    // 5) If backslashes immediately precede a double quotation mark, every
    //    pair of backslashes is interpreted as a literal backslash.  If the
    //    number of backslashes is odd, the last backslash escapes the next
    //    double quotation mark as described in rule 3.
    //
    // See <https://docs.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=vs-2017>.
    for (const std::string &arg : argv) {
        int backslashed = 0;
        bool need_quote = arg.find_first_of(" \t") != std::string::npos;
        if (need_quote)
            buffer << '"';
        for (char c : arg) {
            if (c == '\\')
                backslashed++;
            else if (c == '"') {
                for (int i = 0; i < backslashed; i++)
                    buffer << "\\\\";
                backslashed = 0;
                buffer << "\\\"";
            } else {
                for (int i = 0; i < backslashed; i++)
                    buffer << '\\';
                backslashed = 0;
                buffer << c;
            }
        }
        for (int i = 0; i < backslashed; i++)
            buffer << '\\';
        if (need_quote)
            buffer << '"';
        buffer << ' ';
    }
    return buffer.str();
}

std::string Command::repr()
{
    return string_printf(
        // L10N: "<command> ..."
        _("%s ..."),
        this->command.c_str()
    );
}

void Command::call(std::istream *stdin_, std::ostream *stdout_, bool stderr_)
{
    assert(stdin_ == nullptr); // stdin support not implemented yet
    int status = 0;
    unsigned long rc;
    PROCESS_INFORMATION process_info;
    HANDLE read_end, write_end, error_handle;
    SECURITY_ATTRIBUTES security_attributes;
    memset(&process_info, 0, sizeof process_info);
    security_attributes.nLength = sizeof (SECURITY_ATTRIBUTES);
    security_attributes.lpSecurityDescriptor = nullptr;
    security_attributes.bInheritHandle = true;
    if (CreatePipe(&read_end, &write_end, &security_attributes, 0) == 0)
        throw_win32_error("CreatePipe");
    rc = SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);
    if (rc == 0) {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
            // Presumably it's Windows 9x, so the call is not supported.
            // Punt on security and let the pipe end be inherited.
        } else
            throw_win32_error("SetHandleInformation");
    }
    if (stderr_) {
        error_handle = GetStdHandle(STD_ERROR_HANDLE);
        if (error_handle != INVALID_HANDLE_VALUE) {
            rc = DuplicateHandle(
                GetCurrentProcess(), error_handle,
                GetCurrentProcess(), &error_handle,
                0, true, DUPLICATE_SAME_ACCESS
            );
            if (rc == 0)
                throw_win32_error("DuplicateHandle");
        }
    } else {
        error_handle = CreateFile("nul",
            GENERIC_WRITE, FILE_SHARE_WRITE,
            &security_attributes, OPEN_EXISTING, 0, nullptr);
        // Errors can be safely ignored:
        // - For the Windows NT family, INVALID_HANDLE_VALUE does actually the right thing.
        // - For Windows 9x, spurious debug messages could be generated, tough luck!
    }
    {
        const std::string &command_line = argv_to_command_line(this->argv);
        STARTUPINFO startup_info;
        memset(&startup_info, 0, sizeof startup_info);
        startup_info.cb = sizeof startup_info;
        startup_info.hStdOutput = write_end;
        startup_info.hStdError = error_handle;
        startup_info.dwFlags = STARTF_USESTDHANDLES;
        char *c_command_line = strdup(command_line.c_str());
        if (c_command_line == nullptr)
            throw_posix_error("strdup");
        rc = CreateProcess(
            nullptr, c_command_line,
            nullptr, nullptr,
            true, 0,
            nullptr, nullptr,
            &startup_info,
            &process_info
        );
        free(c_command_line);
        if (rc == 0)
            status = -1;
        else {
            if (error_handle != INVALID_HANDLE_VALUE)
                CloseHandle(error_handle); // ignore errors
        }
    }
    if (status == 0) {
        unsigned long exit_code;
        CloseHandle(write_end); // ignore errors
        while (true) {
            char buffer[BUFSIZ];
            unsigned long nbytes;
            bool success = ReadFile(read_end, buffer, sizeof buffer, &nbytes, nullptr);
            if (!success) {
                status = -(GetLastError() != ERROR_BROKEN_PIPE);
                break;
            }
            if (stdout_ != nullptr)
                stdout_->write(buffer, nbytes);
        }
        CloseHandle(read_end); // ignore errors
        rc = WaitForSingleObject(process_info.hProcess, INFINITE);
        if (rc == WAIT_FAILED)
            throw_win32_error("WaitForSingleObject");
        rc = GetExitCodeProcess(process_info.hProcess, &exit_code);
        CloseHandle(process_info.hProcess); // ignore errors
        CloseHandle(process_info.hThread); // ignore errors
        if (rc == 0)
            status = -1;
        else if (exit_code != 0) {
            std::string message = string_printf(
                _("External command \"%s\" failed with exit status %lu"),
                this->repr().c_str(),
                exit_code
            );
            throw CommandFailed(message);
        }
    }
    if (status < 0) {
        std::string message = string_printf(
            _("External command \"%s\" failed"),
            this->repr().c_str()
        );
        throw_win32_error(message);
    }
}

class FilterWriterData
{
public:
    HANDLE handle;
    const std::string &string;
    FilterWriterData(HANDLE handle, const std::string &string)
    : handle(handle),
      string(string)
    { }
};

unsigned long WINAPI filter_writer(void *data_)
{
    bool success;
    FilterWriterData *data = reinterpret_cast<FilterWriterData*>(data_);
    success = WriteFile(data->handle, data->string.c_str(), data->string.length(), nullptr, nullptr);
    if (!success)
        throw_win32_error("WriteFile");
    success = CloseHandle(data->handle);
    if (!success)
        throw_win32_error("CloseHandle");
    return 0;
}

std::string Command::filter(const std::string &command_line, const std::string &string)
{
    int status = 0;
    unsigned long rc;
    HANDLE stdin_read, stdin_write, stdout_read, stdout_write, error_handle;
    PROCESS_INFORMATION process_info;
    SECURITY_ATTRIBUTES security_attributes;
    memset(&process_info, 0, sizeof process_info);
    security_attributes.nLength = sizeof (SECURITY_ATTRIBUTES);
    security_attributes.lpSecurityDescriptor = nullptr;
    security_attributes.bInheritHandle = true;
    if (CreatePipe(&stdin_read, &stdin_write, &security_attributes, 0) == 0)
        throw_win32_error("CreatePipe");
    if (CreatePipe(&stdout_read, &stdout_write, &security_attributes, 0) == 0)
        throw_win32_error("CreatePipe");
    rc = (
        SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0) &&
        SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0)
    );
    if (rc == 0) {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
            // Presumably it's Windows 9x, so the call is not supported.
            // Punt on security and let the pipe end be inherited.
        } else
            throw_win32_error("SetHandleInformation");
    }
    error_handle = GetStdHandle(STD_ERROR_HANDLE);
    if (error_handle != INVALID_HANDLE_VALUE) {
        rc = DuplicateHandle(
            GetCurrentProcess(), error_handle,
            GetCurrentProcess(), &error_handle,
            0, true, DUPLICATE_SAME_ACCESS
        );
        if (rc == 0)
            throw_win32_error("DuplicateHandle");
    }
    {
        STARTUPINFO startup_info;
        memset(&startup_info, 0, sizeof startup_info);
        startup_info.cb = sizeof startup_info;
        startup_info.hStdInput = stdin_read;
        startup_info.hStdOutput = stdout_write;
        startup_info.hStdError = error_handle;
        startup_info.dwFlags = STARTF_USESTDHANDLES;
        char *c_command_line = strdup(command_line.c_str());
        if (c_command_line == nullptr)
            throw_posix_error("strdup");
        rc = CreateProcess(
            nullptr, c_command_line,
            nullptr, nullptr,
            true, 0,
            nullptr, nullptr,
            &startup_info,
            &process_info
        );
        free(c_command_line);
        if (rc == 0)
            status = -1;
        else {
            if (error_handle != INVALID_HANDLE_VALUE)
                CloseHandle(error_handle); // ignore errors
        }
    }
    if (status == 0) {
        unsigned long exit_code;
        std::ostringstream stream;
        CloseHandle(stdin_read); // ignore errors
        CloseHandle(stdout_write); // ignore errors
        FilterWriterData writer_data(stdin_write, string);
        HANDLE thread_handle = CreateThread(nullptr, 0, filter_writer, &writer_data, 0, nullptr);
        if (thread_handle == nullptr)
            throw_win32_error("CreateThread");
        while (true) {
            char buffer[BUFSIZ];
            unsigned long nbytes;
            bool success = ReadFile(stdout_read, buffer, sizeof buffer, &nbytes, nullptr);
            if (!success) {
                status = -(GetLastError() != ERROR_BROKEN_PIPE);
                break;
            }
            stream.write(buffer, nbytes);
        }
        CloseHandle(stdout_read); // ignore errors
        rc = WaitForSingleObject(thread_handle, INFINITE);
        if (rc == WAIT_FAILED)
            throw_win32_error("WaitForSingleObject");
        CloseHandle(thread_handle); // ignore errors
        rc = WaitForSingleObject(process_info.hProcess, INFINITE);
        if (rc == WAIT_FAILED)
            throw_win32_error("WaitForSingleObject");
        rc = GetExitCodeProcess(process_info.hProcess, &exit_code);
        CloseHandle(process_info.hProcess); // ignore errors
        CloseHandle(process_info.hThread); // ignore errors
        if (rc == 0)
            status = -1;
        else if (exit_code != 0) {
            std::string message = string_printf(
                _("External command \"%s\" failed with exit status %lu"),
                command_line.c_str(),
                exit_code
            );
            throw CommandFailed(message);
        }
        return stream.str();
    }
    if (status < 0) {
        std::string message = string_printf(
            _("External command \"%s\" failed"),
            command_line.c_str()
        );
        throw_win32_error(message);
    }
    return string; // should not really happen
}

#endif

// vim:ts=4 sts=4 sw=4 et
