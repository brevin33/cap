#pragma once

#include "cap/base.h"
#include "cap/string.h"

String filesystem_read_file(String path);

String* filesystem_read_files_in_folder(String path, u64* out_count);

String* filesystem_read_folders_in_folder(String path, u64* out_count);
