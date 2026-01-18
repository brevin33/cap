#include "cap/filesystem.h"

#include "cap.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

String filesystem_read_file(String path) {
    // make sure it is null terminated
    char buffer[4096];
    sprintf(buffer, "%.*s", str_info(path));

    FILE* file = fopen(buffer, "rb");
    if (file == NULL) {
        sprintf(buffer, "Failed to open file %.*s", str_info(path));
        String msg = string_create(buffer, strlen(buffer));
        mabort(msg);
    }
    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* mem = cap_alloc(size);
    fread(mem, 1, size, file);

    fclose(file);

    String str = string_create(mem, size);
    return str;
}

bool filesystem_path_is_absolute(String path) {
#ifdef _WIN32
    if (path.length < 3) return false;
    if (((path.data[0] >= 'A' && path.data[0] <= 'Z') || (path.data[0] >= 'a' && path.data[0] <= 'z')) && path.data[1] == ':' &&
        (path.data[2] == '\\' || path.data[2] == '/')) {
        return true;
    }
    return false;
#else
    if (path.length < 1) return false;
    return path.data[0] == '/';
#endif
}

#ifdef _WIN32
String* filesystem_read_files_in_folder(String path, u64* out_count) {
    WIN32_FIND_DATA find_file_data = {0};
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%.*s/*", str_info(path));
    HANDLE hFind = FindFirstFile(search_path, &find_file_data);

    u64 capacity = 8;
    u64 count = 0;
    String* files = cap_alloc(capacity * sizeof(String));

    if (hFind == INVALID_HANDLE_VALUE) {
        *out_count = 0;
        return NULL;
    }
    while (true) {
        if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            u64 len = strlen(find_file_data.cFileName);
            char* file_name = cap_alloc(len);
            memcpy(file_name, find_file_data.cFileName, len);
            String file = string_create(file_name, len);
            ptr_append(files, count, capacity, file);
        }
        if (!FindNextFile(hFind, &find_file_data)) break;
    }
    FindClose(hFind);
    *out_count = count;
    return files;
}

String* filesystem_read_folders_in_folder(String path, u64* out_count) {
    WIN32_FIND_DATA find_file_data = {0};
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%.*s/*", str_info(path));
    HANDLE hFind = FindFirstFile(search_path, &find_file_data);

    u64 capacity = 8;
    u64 count = 0;
    String* files = cap_alloc(capacity * sizeof(String));

    if (hFind == INVALID_HANDLE_VALUE) {
        *out_count = 0;
        return NULL;
    }
    while (true) {
        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            u64 len = strlen(find_file_data.cFileName);
            String file = string_create(find_file_data.cFileName, len);
            if (string_equal(file, str("."))) continue;
            if (string_equal(file, str(".."))) continue;
            char* file_name = cap_alloc(len);
            memcpy(file_name, find_file_data.cFileName, len);
            file.data = file_name;
            ptr_append(files, count, capacity, file);
        }
        if (!FindNextFile(hFind, &find_file_data)) break;
    }
    FindClose(hFind);
    *out_count = count;
    return files;
}

String filesystem_get_absolute_path(String path) {
    char string_buffer[4096];
    snprintf(string_buffer, sizeof(string_buffer), "%.*s", str_info(path));
    char buffer[4096];
    char* absolute_path = _fullpath(buffer, string_buffer, sizeof(buffer));
    if (absolute_path == NULL) {
        mabort(str("Could not get absolute path"));
    }
    u64 length = strlen(absolute_path);
    char* data = cap_alloc(length);
    memcpy(data, absolute_path, length);
    return string_create(data, length);
}

bool filesystem_make_directory(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%.*s", str_info(path));
    return CreateDirectoryA(cPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

bool filesystem_directory_exists(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%.*s", str_info(path));
    DWORD dwAttrib = GetFileAttributesA(cPath);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool filesystem_delete_directory(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%.*s", str_info(path));
    return RemoveDirectoryA(cPath);
}

#else
String* filesystem_read_files_in_folder(String path, u64* out_count) {
    // make sure it is null terminated
    char buffer[4096];
    sprintf(buffer, "%.*s", str_info(path));

    DIR* dir = opendir(buffer);
    if (dir == NULL) {
        *out_count = 0;
        return NULL;
    }

    u64 capacity = 8;
    u64 count = 0;
    String* files = cap_alloc(capacity * sizeof(String));

    struct dirent* entry;
    while (true) {
        entry = readdir(dir);
        if (entry == NULL) break;
        if (entry->d_type == DT_DIR) continue;
        u64 len = strlen(entry->d_name) + 1;
        String file = string_create(entry->d_name, len);

        char* file_name = cap_alloc(len);
        memcpy(file_name, entry->d_name, len);
        file.data = file_name;
        ptr_append(files, count, capacity, file);
    }
    closedir(dir);
    *out_count = count;
    return files;
}
String* filesystem_read_folders_in_folder(String path, u64* out_count) {
    // make sure it is null terminated
    char buffer[4096];
    sprintf(buffer, "%.*s", str_info(path));

    DIR* dir = opendir(buffer);
    if (dir == NULL) {
        *out_count = 0;
        return NULL;
    }

    u64 capacity = 8;
    u64 count = 0;
    String* files = cap_alloc(capacity * sizeof(String));

    struct dirent* entry;
    while (true) {
        entry = readdir(dir);
        if (entry == NULL) break;
        if (entry->d_type != DT_DIR) continue;
        u64 len = strlen(entry->d_name) + 1;
        String file = string_create(entry->d_name, len);
        if (string_equal(file, str("."))) continue;
        if (string_equal(file, str(".."))) continue;

        char* file_name = cap_alloc(len);
        memcpy(file_name, entry->d_name, len);
        file.data = file_name;
        ptr_append(files, count, capacity, file);
    }
    closedir(dir);
    *out_count = count;
    return files;
}

String filesystem_get_absolute_path(String path) {
    char string_buffer[4096];
    snprintf(string_buffer, sizeof(string_buffer), "%.*s", str_info(path));
    char buffer[4096];
    char* absolute_path = realpath(path.data, buffer);
    if (absolute_path == NULL) {
        mabort(str("Could not get absolute path"));
    }
    u64 length = strlen(absolute_path);
    char* data = cap_alloc(length);
    memcpy(data, absolute_path, length);
    return string_create(data, length);
}

bool filesystem_make_directory(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%s", path);
    bool worked = mkdir(path, 0755) == 0 || errno == EEXIST;
    errno = 0;
    return worked;
}

bool filesystem_directory_exists(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%s", path);
    struct stat info;
    bool worked = stat(path, &info) == 0 && info.st_mode & S_IFDIR;
    errno = 0;
    return worked;
}

bool filesystem_delete_directory(String path) {
    char cPath[MAX_PATH];
    snprintf(cPath, MAX_PATH, "%s", path);
    bool worked = rmdir(path) == 0 || errno == ENOENT;
    errno = 0;
    return worked;
}

#endif

bool filesystem_path_are_equal(String path1, String path2) {
    String absolute_path1 = filesystem_get_absolute_path(path1);
    String absolute_path2 = filesystem_get_absolute_path(path2);
    return string_equal(absolute_path1, absolute_path2);
}
