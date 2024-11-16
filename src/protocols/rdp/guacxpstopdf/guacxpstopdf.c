/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "guacxpstopdf.h"

#include <io.h>
#include <fcntl.h>
#include <processthreadsapi.h>
#include <stdio.h>
#include <string.h>
#include <fileapi.h>
#include <shlwapi.h>
#include <windows.h>

/**
 * The name of the libgxps-provided executable that translates XPS to PDF. This
 * can be in the PATH, or a full path to the executable.
 */
#define TRANSLATION_EXECUTABLE_PATH "xpstopdf.exe"

/**
 * Create a new temporary file, in the configured temporary directory, returning
 * 0 if the file was succesfully created, or a non-zero value otherwise. If any
 * errors occur during the creation of the file, the error will be logged to
 * stderr. The file must be manually deleted after by the caller it's done being used.
 * The full path to the file will be stored in the provided file_path buffer,
 * which must be large enough to hold any file path (MAX_PATH characters long).
 *
 * @param file_path
 *     A pointer to a buffer into which the full path to the newly-created
 *     temporary file will be written.
 *
 * @return
 *     Zero if the file was succesfully created, or a non-zero value otherwise.
 */
static int create_temp_file(char* file_path) {

    /*
     * First, the entire contents of the input stream must be written to a temp
     * file before it can be read by libgxps.
     *
     * NOTE: Technically, according to the docs for GetTempPath, the maximum
     * length of path that can be set is MAX_PATH + 1, since a trailing slash
     * is always added. A path this long would cause GetTempFileName to fail.
     */
    char temp_dir_buffer[MAX_PATH + 1];
    if (!GetTempPath(sizeof(temp_dir_buffer), temp_dir_buffer)) {
        fprintf(stderr,
                "Could not determine temporary directory : %lu\n", GetLastError());
        return 1;
    }

    if (!GetTempFileName(temp_dir_buffer, "GUA", 0, file_path)) {
        fprintf(stderr,
                "Could not create temporary file : %lu\n", GetLastError());
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {

    /* Use the default path unless overriden on the command line */
    char* exe_path = TRANSLATION_EXECUTABLE_PATH;
    if (argc >= 2)
        exe_path = argv[1];

    /* The return code for this program - only set to 0 on success */
    int retcode = 1;

    /* Make sure stdin and stdout are operating in binary mode */
    _setmode(_fileno(stdin),  _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    /* Data structures to extract info from the subprocess */
    STARTUPINFOA startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);
    PROCESS_INFORMATION process_info = { 0 };

    /*
     * Create stdin / stdout handles.
     * These handles do NOT need to be closed, per
     * https://learn.microsoft.com/en-us/windows/console/getstdhandle#handle-disposal
     */
    HANDLE* stdin_handle  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE* stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    /* A temporary file to hold the XPS data from stdin */
    char temp_xps_path[MAX_PATH];
    if(create_temp_file(temp_xps_path))
        return 1;

    /* A temporary file to hold the PDF data to be written to stdout */
    char temp_pdf_path[MAX_PATH];
    if (create_temp_file(temp_pdf_path)) {
        DeleteFile(temp_xps_path);
        return 1;
    }

    HANDLE xps_file_handle = NULL;
    HANDLE pdf_file_handle = NULL;

    /* Open the XPS temp file for writing */
    xps_file_handle = CreateFile(
            temp_xps_path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (xps_file_handle == INVALID_HANDLE_VALUE)
        goto cleanup;

    /* Buffer to hold data being read from / written to files or streams */
    char buffer[16384];
    DWORD bytes_read;
    DWORD bytes_written;

    while (1) {

        BOOL read_success = ReadFile(
                stdin_handle, buffer, sizeof(buffer), &bytes_read, NULL);

        if (!read_success)
            goto cleanup;

        /* Stop reading if there's nothing more in the stream */
        if (bytes_read == 0)
            break;

        WriteFile(xps_file_handle, buffer, bytes_read, &bytes_written, NULL);

        if (bytes_written < bytes_read)
            goto cleanup;
    }

    /* Close this handle now that the file is done being written */
    FlushFileBuffers(xps_file_handle);
    CloseHandle(xps_file_handle);
    xps_file_handle = NULL;

    /*
     * The temp file is now fully written with XPS data, so it's ready to be
     * translated to PDF using the libgxps-provided program.
     */

    /* Add quotes to each of the temp file paths as needed */
    PathQuoteSpaces(temp_xps_path);
    PathQuoteSpaces(temp_pdf_path);

    /* The arguments to xpstopdf.exe - the quoted input and output files */
    size_t xps_path_length = strlen(temp_xps_path);
    size_t pdf_path_length = strlen(temp_pdf_path);

    /* Create a single space-seperated string with the exe and arguments */
    char argument_string[(MAX_PATH * 3) + 2];
    char* next_argument;

    /* First, the executable itself */
    size_t exe_path_length = strlen(exe_path);
    strncpy(argument_string, exe_path, exe_path_length);
    argument_string[exe_path_length] = ' ';
    next_argument = argument_string + exe_path_length + 1;

    /* Second, the input file name */
    strncpy(next_argument, temp_xps_path, xps_path_length);
    next_argument[xps_path_length] = ' ';
    next_argument = next_argument + xps_path_length + 1;

    /* Finally, the output file name */
    strncpy(next_argument, temp_pdf_path, pdf_path_length);
    next_argument[pdf_path_length] = '\0';

    /* Start up the translation program */
    if (!CreateProcess(

        /* Set to NULL to use the first argument as the exe */
        NULL,

        /* The arguments - both temporary files, each quoted */
        argument_string,

        /* Default arguments that we don't need to change */
        NULL, NULL, FALSE, 0, NULL, NULL,

        /* Structures to capture info about the process */
        &startup_info, &process_info
    ))
    {
        fprintf(stderr, "Failed to run translation program: %lu\n", GetLastError() );
        goto cleanup;
    }

    /* Wait for the process to complete */
    WaitForSingleObject(process_info.hProcess, INFINITE);

    /* Get the exit code for the completed process */
    DWORD exit_code;
    if (!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        fprintf(stderr, "Failed to get translation program exit code: %lu\n", GetLastError() );
        goto cleanup;
    }

    if (exit_code != 0) {
        fprintf(stderr, "Translation program failed: %lu\n", exit_code);
        goto cleanup;
    }

    /* Open the new PDF temp file for writing */
    pdf_file_handle = CreateFile(
            temp_pdf_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (pdf_file_handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr,
                "Could not open temporary PDF file: %lu\n", GetLastError());
        goto cleanup;
    }

    while (ReadFile(pdf_file_handle, buffer, sizeof(buffer), &bytes_read, NULL)) {

        if (bytes_read == 0)
            break;

        WriteFile(stdout_handle, buffer, bytes_read, &bytes_written, NULL);

        if (bytes_written < bytes_read) {
            fprintf(stderr,
                    "Error while writing PDF to stdout: %lu\n", GetLastError());
            goto cleanup;
        }
    }

    /* The entire file was successfully written to stdout, so we're done */
    retcode = 0;

cleanup:

    if (process_info.hProcess != NULL)
        CloseHandle(process_info.hProcess);
    if (process_info.hThread != NULL)
        CloseHandle(process_info.hThread);

    if (xps_file_handle != NULL)
        CloseHandle(xps_file_handle);

    if (pdf_file_handle != NULL)
        CloseHandle(pdf_file_handle);

    DeleteFile(temp_xps_path);
    DeleteFile(temp_pdf_path);

    return retcode;

}

