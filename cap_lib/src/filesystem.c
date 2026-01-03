#include "cap.h"

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

#ifdef _WIN32
#include <windows.h>
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
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
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
#endif
