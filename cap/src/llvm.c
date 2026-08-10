#include "llvm.h"

#include "cap.h"
#include "ssa.h"
#include "util/arbitrary_int.h"

utf8 llvm_unique_name() {
    static u64 number_counter = 0;
    char buffer[128] = {0};
    snprintf(buffer, 128, "%cU%lld", '%', number_counter++);
    utf8 name_utf8 = {0};
    name_utf8.data = alloc(strlen(buffer) + 1);
    memcpy(name_utf8.data, buffer, strlen(buffer) + 1);
    name_utf8.count = strlen(buffer);
    return name_utf8;
}

utf8 llvm_gernerate_ir_exe(Function* function) {
    utf8_builder builder = {0};
    utf8 global_scope = llvm_global_scope(&builder);
    utf8_builder_append(&builder, utf8_str("define internal i32 @__cap_main__() {\n"));
    SSA_Block* body = &function->data.internal.body;
    llvm_function_setup(body, &builder);
    llvm_function_body(body, &builder);
    utf8_builder_append(&builder, utf8_str("}\n"));
    llvm_build_required_functions(body, &builder);
    llvm_build_required_functions(&context.global_block, &builder);
    return builder.str;
}

void llvm_ssa_set_name(SSA* ssa, utf8 name) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            per_function_context_values->codegen_already_has_name = true;
            per_function_context_values->codegen_name = name;
            return;
        }
    }
}

bool llvm_type_exists_at_runtime(Type* type) {
    if (type->kind == Type_Kind_Void) return false;
    i64 type_size = ssa_type_size(type);
    if (type_size == 0) return false;
    return true;
}

bool llvm_ssa_exists_at_runtime(SSA* ssa) {
    Type* type = ssa_type(ssa);
    return llvm_type_exists_at_runtime(type);
}

utf8 llvm_ssa_set_global_name(SSA* ssa) {
    char buffer[128] = {0};
    if (llvm_ssa_exists_at_runtime(ssa)) {
        snprintf(buffer, 128, "%cG%p", '@', ssa);
    }
    utf8 name_utf8 = {0};
    u32 name_length = strlen(buffer);
    name_utf8.data = alloc(name_length + 1);
    memcpy(name_utf8.data, buffer, name_length + 1);
    name_utf8.count = name_length;
    llvm_ssa_set_name(ssa, name_utf8);
    return name_utf8;
}

utf8 llvm_function_name(Function_Context* function_context) {
    char buffer[128] = {0};
    snprintf(buffer, 128, "%cF%p", '@', function_context);
    utf8 name_utf8 = {0};
    u32 name_length = strlen(buffer);
    name_utf8.data = alloc(name_length + 1);
    memcpy(name_utf8.data, buffer, name_length + 1);
    name_utf8.count = name_length;
    return name_utf8;
}

void llvm_ssa_reset_name(SSA* ssa) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            per_function_context_values->codegen_already_has_name = false;
        }
    }
}

utf8 llvm_ssa_name(SSA* ssa) {
    // return ssa_get_ssa_name(ssa);
    char buffer[128] = {0};
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            bool codegen_already_has_name = per_function_context_values->codegen_already_has_name;
            if (codegen_already_has_name) return per_function_context_values->codegen_name;
            sprintf(buffer, "%cS%p%p", '%', ssa, function_context);
            u32 name_length = strlen(buffer);
            char* name_mem = alloc(name_length + 1);
            memcpy(name_mem, buffer, name_length + 1);
            utf8 name = {0};
            name.data = name_mem;
            name.count = name_length;
            per_function_context_values->codegen_name = name;
            per_function_context_values->codegen_already_has_name = true;
            return name;
        }
    }
    internal_compiler_error();
}

utf8 llvm_block_name(SSA_Block* block) {
    char buffer[128] = {0};
    Function_Context* function_context = ssa_get_function_context().function_context;
    sprintf(buffer, "%c%p%p", 'B', function_context, block);
    utf8 name_utf8 = {0};
    u32 name_length = strlen(buffer);
    name_utf8.data = alloc(name_length + 1);
    memcpy(name_utf8.data, buffer, name_length + 1);
    name_utf8.count = name_length;
    return name_utf8;
}

utf8 llvm_type(Type* type) {
    switch (type->kind) {
        case Type_Kind_Invalid: {
            internal_compiler_error();
        }
        case Type_Kind_Call_Setup:
        case Type_Kind_Function:
        case Type_Kind_Int_Literal:
        case Type_Kind_Float_Literal:
        case Type_Kind_Type: {
            return utf8_str("{}");
        }
        case Type_Kind_Int: {
            char buffer[128] = {0};
            u32 bits = type->data.int_.bits;
            snprintf(buffer, arr_len(buffer), "i%u", bits);
            u32 name_length = strlen(buffer);
            char* name_mem = alloc(name_length + 1);
            memcpy(name_mem, buffer, name_length + 1);
            utf8 name = {0};
            name.data = name_mem;
            name.count = name_length;
            return name;
        }
        case Type_Kind_Uint: {
            char buffer[128] = {0};
            u32 bits = type->data.uint.bits;
            snprintf(buffer, arr_len(buffer), "i%u", bits);
            u32 name_length = strlen(buffer);
            char* name_mem = alloc(name_length + 1);
            memcpy(name_mem, buffer, name_length + 1);
            utf8 name = {0};
            name.data = name_mem;
            name.count = name_length;
            return name;
        }
        case Type_Kind_Float: {
            u32 bits = type->data.float_.bits;
            switch (bits) {
                case 16: {
                    return utf8_str("half");
                }
                case 32: {
                    return utf8_str("float");
                }
                case 64: {
                    return utf8_str("double");
                }
                case 128: {
                    return utf8_str("fp128");
                }
                default: {
                    internal_compiler_error();
                }
            }
        }
        case Type_Kind_Void: {
            return utf8_str("void");
        }
        case Type_Kind_Ptr: {
            return utf8_str("ptr");
        }
        case Type_Kind_Return_Value: {
            utf8_builder builder = {0};
            utf8_builder_append(&builder, utf8_str("{ "));
            bool first = true;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* value_type = type->data.return_value.types[i];
                if (llvm_type_exists_at_runtime(value_type) == false) {
                    continue;
                }
                if (first == false) utf8_builder_append(&builder, utf8_str(", "));
                utf8_builder_append(&builder, llvm_type(value_type));
                first = false;
            }
            utf8_builder_append(&builder, utf8_str(" }"));
            return builder.str;
        }
    }
}

utf8 llvm_function(Function* function, Function_Context* function_context, utf8_builder* builder) {
    Function_Instance_Data* instance_data = ssa_get_function_instance_data(function, function_context);
    assert(instance_data != NULL);
    if (instance_data->codegen_function.count != 0) {
        return instance_data->codegen_function;
    }

    char* start = builder->str.data + builder->str.count;

    All_Function_Context all = {0};
    all.function_context = function_context;
    ssa_push_function_context(all);

    utf8 function_name = llvm_function_name(function_context);
    SSA* return_type_ssa = function_context->return_type;
    Type* return_type = ssa_evaluate_type(return_type_ssa);
    assert(return_type != NULL);
    utf8 return_type_name = llvm_type(return_type);

    utf8_builder_append(builder, utf8_str("define internal "));
    utf8_builder_append(builder, return_type_name);
    utf8_builder_append(builder, utf8_str(" "));
    utf8_builder_append(builder, function_name);
    utf8_builder_append(builder, utf8_str("("));
    bool first = true;
    for (u32 i = 0; i < function_context->parameters_count; i++) {
        SSA* parameter = function_context->parameters[i];
        Type* parameter_type = ssa_type(parameter);
        assert(parameter_type != NULL);
        if (llvm_ssa_exists_at_runtime(parameter) == false) {
            continue;
        }
        llvm_ssa_reset_name(parameter);
        utf8 parameter_type_name = llvm_type(parameter_type);
        utf8 parameter_name = llvm_ssa_name(parameter);

        if (first == false) utf8_builder_append(builder, utf8_str(", "));
        utf8_builder_append(builder, parameter_type_name);
        utf8_builder_append(builder, utf8_str(" "));
        utf8_builder_append(builder, parameter_name);
        first = false;
    }
    utf8_builder_append(builder, utf8_str(") {\n"));
    assert(function->kind == Function_Kind_Internal);
    SSA_Block* body = &function->data.internal.body;
    llvm_function_setup(body, builder);
    llvm_function_body(body, builder);
    utf8_builder_append(builder, utf8_str("}\n"));

    char* end = builder->str.data + builder->str.count;
    utf8 function_str = utf8_slice(start, end);
    instance_data->codegen_function = function_str;

    llvm_build_required_functions(body, builder);

    ssa_pop_function_context();
    return function_str;
}

utf8 llvm_global_setup(SSA_Block* block, utf8_builder* builder) {
    return llvm_function_setup(block, builder);
}

utf8 llvm_function_setup(SSA_Block* block, utf8_builder* builder) {
    char* start = builder->str.data + builder->str.count;
    bool global = block->kind == SSA_Block_Kind_Global;
    u32 ssa_count = 0;
    SSA** ssa_of_kind = ssa_get_all_ssa_of_kind(block, SSA_Kind_Stack_Alloc, &ssa_count);
    for (u32 i = 0; i < ssa_count; i++) {
        SSA* ssa = ssa_of_kind[i];
        assert(ssa->kind == SSA_Kind_Stack_Alloc);
        Type* type = ssa_type(ssa)->data.ptr.type;
        utf8 type_name = llvm_type(type);
        if (global) {
            utf8 name = llvm_ssa_set_global_name(ssa);
            utf8_builder_append(builder, name);
            utf8_builder_append(builder, utf8_str(" = internal global "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(" zeroinitializer\n"));
        } else {
            utf8 name = llvm_ssa_name(ssa);
            utf8_builder_append(builder, name);
            utf8_builder_append(builder, utf8_str(" = "));
            utf8_builder_append(builder, utf8_str("alloca "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str("\n"));
        }
    }
    if (!global) {
        llvm_branch_to_block(block, builder);
        utf8_builder_append(builder, utf8_str("\n"));
    }
    char* end = builder->str.data + builder->str.count;
    return utf8_slice(start, end);
}

void llvm_generate_exe(utf8 llvm_ir, utf8 exe_path) {
    printf("--------------------------------\n");
    printf("------    Running Clang   ------\n");
    printf("--------------------------------\n");
    f64 start = get_time_in_seconds();
    char exe_path_buffer[1024] = {0};
    snprintf(exe_path_buffer, 1024, "clang -Wno-override-module -x ir - -o %.*s", utf8_fmt(exe_path));
    FILE* clang_proc = popen(exe_path_buffer, "w");
    fprintf(clang_proc, "%.*s", utf8_fmt(llvm_ir));
    pclose(clang_proc);
    f64 end = get_time_in_seconds();
    f64 time = end - start;
    printf("Clang Took %f seconds to compile\n", time);
    printf("--------------------------------\n");
    printf("------ Done Running Clang ------\n");
    printf("--------------------------------\n");
}

utf8 llvm_function_body(SSA_Block* block, utf8_builder* builder) {
    char* start = builder->str.data + builder->str.count;
    utf8 block_name = llvm_block_name(block);
    utf8_builder_append(builder, block_name);
    utf8_builder_append(builder, utf8_str(":\n"));

    typedef struct SSA_Block_Default_Branch_Pair {
        SSA_Block* block_to_build;
        SSA_Block* default_branch;
    } SSA_Block_Default_Branch_Pair;
    u32 blocks_to_build_count = 1;
    u32 blocks_to_build_capacity = 1;
    SSA_Block_Default_Branch_Pair* blocks_to_build = alloc(sizeof(SSA_Block_Default_Branch_Pair) * blocks_to_build_capacity);
    blocks_to_build[0].block_to_build = block;
    blocks_to_build[0].default_branch = NULL;

    while (blocks_to_build_count != 0) {
        SSA_Block_Default_Branch_Pair* pair = blocks_to_build + blocks_to_build_count - 1;
        SSA_Block* block = pair->block_to_build;
        SSA_Block* default_branch = pair->default_branch;
        blocks_to_build_count--;
        bool exit = false;
        for (u32 i = 0; i < block->statement_lists_count; i++) {
            SSA_List* list = &block->statement_lists[i];
            for (u32 j = 0; j < list->statements_count; j++) {
                SSA* ssa = &list->statements[j];
                utf8 str = llvm_statement(ssa, builder);
                switch (ssa->kind) {
                    case SSA_Kind_Terminate_Global_Scope:
                    case SSA_Kind_Return: {
                        exit = true;
                        break;
                    }
                    default: {
                        break;
                    }
                }
                if (exit) break;
            }
            if (exit) break;
        }
        if (exit == false) {
            if (default_branch != NULL) {
                llvm_branch_to_block(default_branch, builder);
                utf8_builder_append(builder, utf8_str("\n"));
            } else {
                utf8_builder_append(builder, utf8_str("ret void\n"));
            }
        }
    }

    char* end = builder->str.data + builder->str.count;
    return utf8_slice(start, end);
}

utf8 llvm_build_required_functions(SSA_Block* block, utf8_builder* builder) {
    char* start = builder->str.data + builder->str.count;

    u32 ssa_count = 0;
    SSA** ssa_of_kind = ssa_get_all_ssa_of_kind(block, SSA_Kind_Call, &ssa_count);
    for (u32 i = 0; i < ssa_count; i++) {
        SSA* ssa = ssa_of_kind[i];
        assert(ssa->kind == SSA_Kind_Call);
        SSA* setup = ssa->data.call.setup;
        Function_Context* function_context = ssa_evaluate_function_context(setup);
        assert(function_context != NULL);
        SSA* callee = setup->data.call_setup.callee;
        Function* function = ssa_evaluate_function(callee);
        assert(function != NULL);
        switch (function->kind) {
            case Function_Kind_Internal: {
                utf8 function_str = llvm_function(function, function_context, builder);
                break;
            }
            case Function_Kind_Invalid:
            case Function_Kind_Intrinsic:
                break;
        }
    }

    char* end = builder->str.data + builder->str.count;
    return utf8_slice(start, end);
}

utf8 llvm_global_scope(utf8_builder* builder) {
    char* start = builder->str.data + builder->str.count;

    utf8_builder_append(builder, utf8_str("; ModuleID = 'cap_module'\n"));
    utf8_builder_append(builder, utf8_str("@empty = internal global {} zeroinitializer\n"));

    SSA_Block* global_block = &context.global_block;

    llvm_global_setup(global_block, builder);

    utf8_builder_append(builder, utf8_str("define i32 @main() {\n"));
    llvm_function_body(global_block, builder);
    utf8_builder_append(builder, utf8_str("%ret = call i32 @__cap_main__()\n"));
    utf8_builder_append(builder, utf8_str("ret i32 %ret\n"));
    utf8_builder_append(builder, utf8_str("}\n"));

    char* end = builder->str.data + builder->str.count;
    return utf8_slice(start, end);
}

static void llvm_append_name_equals(utf8_builder* builder, utf8 name, bool exists_at_runtime) {
    if (exists_at_runtime) {
        utf8_builder_append(builder, name);
        utf8_builder_append(builder, utf8_str(" = "));
    }
}

utf8 llvm_statement(SSA* ssa, utf8_builder* builder) {
    char* start = builder->str.data + builder->str.count;
    utf8 our_name = llvm_ssa_name(ssa);
    Type* type = ssa_type(ssa);
    bool exists_at_runtime = llvm_ssa_exists_at_runtime(ssa);
    switch (ssa->kind) {
        case SSA_Kind_Store: {
            SSA* address = ssa->data.store.address;
            utf8 address_name = llvm_ssa_name(address);
            SSA* value = ssa->data.store.value;
            if (llvm_ssa_exists_at_runtime(value) == false) break;
            Type* value_type = ssa_type(value);
            utf8 type_name = llvm_type(value_type);
            utf8 value_name = llvm_ssa_name(ssa->data.store.value);
            utf8_builder_append(builder, utf8_str("store "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(" "));
            utf8_builder_append(builder, value_name);
            utf8_builder_append(builder, utf8_str(", ptr "));
            utf8_builder_append(builder, address_name);
            utf8_builder_append(builder, utf8_str("\n"));
            break;
        }
        case SSA_Kind_Load: {
            if (!exists_at_runtime) break;
            SSA* address = ssa->data.load.address;
            utf8 address_name = llvm_ssa_name(address);
            Type* type = ssa_type(ssa);
            utf8 type_name = llvm_type(type);
            llvm_append_name_equals(builder, our_name, exists_at_runtime);
            utf8_builder_append(builder, utf8_str("load "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(", ptr "));
            utf8_builder_append(builder, address_name);
            break;
        }
        case SSA_Kind_Stack_Alloc: {
            Type* type = ssa_type(ssa->data.stack_alloc.initial_value);
            utf8 type_name = llvm_type(type);
            SSA* initial_value = ssa->data.stack_alloc.initial_value;
            if (llvm_ssa_exists_at_runtime(initial_value) == false) break;
            utf8 initial_value_name = llvm_ssa_name(initial_value);
            utf8_builder_append(builder, utf8_str("store "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(" "));
            utf8_builder_append(builder, initial_value_name);
            utf8_builder_append(builder, utf8_str(", ptr "));
            utf8_builder_append(builder, our_name);
            break;
        }
        case SSA_Kind_Implicit_Cast:
        case SSA_Kind_Explicit_Cast: {
            if (!exists_at_runtime) break;
            SSA* value = ssa->data.explicit_cast.value;
            Type* value_type = ssa_type(value);
            Type* type = ssa_type(ssa);
            utf8 value_type_name = llvm_type(value_type);
            utf8 type_name = llvm_type(type);
            utf8 value_name = llvm_ssa_name(value);
            if (ssa_type_equal(value_type, type)) {
                llvm_ssa_set_name(ssa, value_name);
            } else if (value_type->kind == Type_Kind_Int_Literal) {
                void* comp_value = ssa_evaluate(value);
                Big_Int* big_int = comp_value;
                i64 v = big_int->data;
                bool is_signed = type->kind == Type_Kind_Int;
                utf8 i64_utf8 = arbitrary_int_to_string(comp_value, type->data.int_.bits, is_signed);
                if (type->kind == Type_Kind_Float) {
                    u32 cap = i64_utf8.count;
                    i64_utf8 = utf8_append(i64_utf8, utf8_str(".0"));
                }
                llvm_ssa_set_name(ssa, i64_utf8);
            } else if (value_type->kind == Type_Kind_Float_Literal) {
                f64* comp_value = ssa_evaluate(value);
                f64 v = *comp_value;
                utf8 f64_hex = utf8_memory_as_hex(&v, sizeof(f64));
                llvm_ssa_set_name(ssa, f64_hex);
            } else if (type->kind == Type_Kind_Int && value_type->kind == Type_Kind_Int) {
                i64 bits = type->data.int_.bits;
                i64 value_bits = value_type->data.int_.bits;
                if (bits == value_bits) {
                    llvm_ssa_set_name(ssa, value_name);
                } else if (bits > value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("sext "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else if (bits < value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("trunc "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else {
                    internal_compiler_error();
                }
            } else if (type->kind == Type_Kind_Uint && value_type->kind == Type_Kind_Uint) {
                i64 bits = type->data.uint.bits;
                i64 value_bits = value_type->data.uint.bits;
                if (bits == value_bits) {
                    llvm_ssa_set_name(ssa, value_name);
                } else if (bits > value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("zext "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else if (bits < value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("trunc "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else {
                    internal_compiler_error();
                }
            } else if (type->kind == Type_Kind_Int && value_type->kind == Type_Kind_Uint) {
                i64 bits = type->data.int_.bits;
                i64 value_bits = value_type->data.uint.bits;
                if (bits == value_bits) {
                    llvm_ssa_set_name(ssa, value_name);
                } else if (bits > value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("sext "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else if (bits < value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("trunc "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else {
                    internal_compiler_error();
                }
            } else if (type->kind == Type_Kind_Uint && value_type->kind == Type_Kind_Int) {
                i64 bits = type->data.uint.bits;
                i64 value_bits = value_type->data.int_.bits;
                if (bits == value_bits) {
                    llvm_ssa_set_name(ssa, value_name);
                } else if (bits > value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("zext "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else if (bits < value_bits) {
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("trunc "));
                    utf8_builder_append(builder, value_type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, value_name);
                    utf8_builder_append(builder, utf8_str(" to "));
                    utf8_builder_append(builder, type_name);
                } else {
                    internal_compiler_error();
                }
            } else if (type->kind == Type_Kind_Int && value_type->kind == Type_Kind_Float) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("fptosi "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else if (type->kind == Type_Kind_Uint && value_type->kind == Type_Kind_Float) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("fptoui "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else if (type->kind == Type_Kind_Float && value_type->kind == Type_Kind_Int) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("sitofp "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else if (type->kind == Type_Kind_Float && value_type->kind == Type_Kind_Uint) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("uitofp "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else if (type->kind == Type_Kind_Ptr && value_type->kind == Type_Kind_Ptr) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                llvm_ssa_set_name(ssa, value_name);
            } else if (type->kind == Type_Kind_Ptr && (value_type->kind == Type_Kind_Int || value_type->kind == Type_Kind_Uint)) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("inttoptr "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else if ((type->kind == Type_Kind_Int || type->kind == Type_Kind_Uint) && value_type->kind == Type_Kind_Ptr) {
                llvm_append_name_equals(builder, our_name, exists_at_runtime);
                utf8_builder_append(builder, utf8_str("ptrtoint "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(" to "));
                utf8_builder_append(builder, type_name);
            } else {
                internal_compiler_error();
            }
            break;
        }
        case SSA_Kind_Return: {
            SSA* return_value = ssa->data.return_.return_value;
            Type* return_type = ssa_type(return_value);
            if (return_type->kind == Type_Kind_Void) {
                utf8_builder_append(builder, utf8_str("ret void"));
                break;
            }
            utf8 type_name = llvm_type(return_type);
            utf8 value_name = llvm_ssa_name(return_value);
            utf8_builder_append(builder, utf8_str("ret "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(" "));
            utf8_builder_append(builder, value_name);
            break;
        }
        case SSA_Kind_Call_Setup: {
            Function_Context* function_context = ssa_evaluate_function_context(ssa);
            SSA* callee = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);
            All_Function_Context all = {0};
            all.function_context = function_context;
            ssa_push_function_context(all);
            SSA_Block* function_setup_block = &function->setup_block;
            for (u32 i = 0; i < function_setup_block->statement_lists_count; i++) {
                SSA_List* list = &function_setup_block->statement_lists[i];
                for (u32 j = 0; j < list->statements_count; j++) {
                    SSA* ssa = &list->statements[j];
                    utf8 str = llvm_statement(ssa, builder);
                }
            }
            ssa_pop_function_context();
            break;
        }
        case SSA_Kind_Call: {
            SSA* setup = ssa->data.call.setup;
            Function_Context* function_context = ssa_evaluate_function_context(setup);
            SSA* callee = setup->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);

            All_Function_Context all = {0};
            all.function_context = function_context;
            ssa_push_function_context(all);

            switch (function->kind) {
                case Function_Kind_Invalid: {
                    internal_compiler_error();
                }
                case Function_Kind_Internal: {
                    utf8 type_name = llvm_type(type);
                    utf8 function_name = llvm_function_name(function_context);
                    llvm_append_name_equals(builder, our_name, exists_at_runtime);
                    utf8_builder_append(builder, utf8_str("call "));
                    utf8_builder_append(builder, type_name);
                    utf8_builder_append(builder, utf8_str(" "));
                    utf8_builder_append(builder, function_name);
                    utf8_builder_append(builder, utf8_str("("));
                    bool first = true;
                    for (u32 i = 0; i < function_context->parameters_count; i++) {
                        SSA* parameter = function_context->parameters[i];
                        if (llvm_ssa_exists_at_runtime(parameter) == false) {
                            continue;
                        }
                        Type* parameter_type = ssa_type(parameter);
                        utf8 type_name = llvm_type(parameter_type);
                        utf8 value_name = llvm_ssa_name(parameter);
                        if (first == false) utf8_builder_append(builder, utf8_str(", "));
                        utf8_builder_append(builder, type_name);
                        utf8_builder_append(builder, utf8_str(" "));
                        utf8_builder_append(builder, value_name);
                        first = false;
                    }
                    utf8_builder_append(builder, utf8_str(")"));
                    break;
                }
                case Function_Kind_Intrinsic: {
                    Intrinsic_Function_Kind intrinsic_kind = function->data.intrinsic.kind;
                    switch (intrinsic_kind) {
                        case Intrinsic_Function_Kind_Invalid: {
                            internal_compiler_error();
                        }
                        case Intrinsic_Function_Uint_Type:
                        case Intrinsic_Function_Float_Type:
                        case Intrinsic_Function_Int_Type: {
                            break;
                        }
                        case Intrinsic_Function_Kind_Compile_To_LLVM_IR: {
                            assert(false);
                        }
                    }
                }
            }
            ssa_pop_function_context();
            break;
        }
        case SSA_Kind_Parameter: {
            if (!exists_at_runtime) break;
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            u32 index = ssa->data.parameter.index;
            SSA* parameter = function_context->parameters[index];
            utf8 value_name = llvm_ssa_name(parameter);
            llvm_ssa_set_name(ssa, value_name);
            break;
        }
        case SSA_Kind_Argument: {
            if (!exists_at_runtime) break;
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            u32 index = ssa->data.argument.index;
            SSA* argument = function_context->arguments[index];
            ssa_pop_function_context();
            utf8 value_name = llvm_ssa_name(argument);
            ssa_push_function_context(all);
            llvm_ssa_set_name(ssa, value_name);
            break;
        }
        case SSA_Kind_Return_Value: {
            if (!exists_at_runtime) break;
            utf8 type_name = llvm_type(type);
            u32 runtime_values = 0;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                SSA* value = ssa->data.return_value.values[i];
                if (llvm_ssa_exists_at_runtime(value) == false) {
                    continue;
                }
                runtime_values++;
            }
            u32 current_runtime_value = 0;
            utf8 last_name = utf8_str("undef");
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                SSA* value = ssa->data.return_value.values[i];
                if (llvm_ssa_exists_at_runtime(value) == false) {
                    continue;
                }
                Type* value_type = type->data.return_value.types[i];
                utf8 value_name = llvm_ssa_name(value);
                utf8 value_type_name = llvm_type(value_type);
                utf8 assign_name;
                if (++current_runtime_value == runtime_values) {
                    assign_name = our_name;
                } else {
                    assign_name = llvm_unique_name();
                }
                utf8_builder_append(builder, assign_name);
                utf8_builder_append(builder, utf8_str(" = insertvalue "));
                utf8_builder_append(builder, type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, last_name);
                utf8_builder_append(builder, utf8_str(", "));
                utf8_builder_append(builder, value_type_name);
                utf8_builder_append(builder, utf8_str(" "));
                utf8_builder_append(builder, value_name);
                utf8_builder_append(builder, utf8_str(", "));
                char index_buf[32];
                snprintf(index_buf, 32, "%u", i);
                utf8 index_utf8 = {index_buf, strlen(index_buf)};
                utf8_builder_append(builder, index_utf8);
                if (current_runtime_value != runtime_values) {
                    utf8_builder_append(builder, utf8_str("\n"));
                }
                last_name = assign_name;
            }
            break;
        }
        case SSA_Kind_Return_Value_Index: {
            if (!exists_at_runtime) break;
            SSA* return_value = ssa->data.return_value_index.return_value;
            Type* return_type = ssa_type(return_value);
            assert(return_type->kind == Type_Kind_Return_Value);
            utf8 type_name = llvm_type(return_type);
            utf8 value_name = llvm_ssa_name(return_value);

            llvm_append_name_equals(builder, our_name, exists_at_runtime);
            utf8_builder_append(builder, utf8_str("extractvalue "));
            utf8_builder_append(builder, type_name);
            utf8_builder_append(builder, utf8_str(" "));
            utf8_builder_append(builder, value_name);
            utf8_builder_append(builder, utf8_str(", "));
            char index_buf[32];
            snprintf(index_buf, 32, "%u", ssa->data.return_value_index.index);
            utf8 index_utf8 = {index_buf, strlen(index_buf)};
            utf8_builder_append(builder, index_utf8);
            break;
        }
        case SSA_Kind_Default_Value: {
            if (!exists_at_runtime) break;
            assert(false);
            // Type* type = ssa_type(ssa);
            // utf8 type_name = llvm_type(type);
            // llvm_ssa_set_name(ssa, utf8_str("zeroinitializer"));
            // break;
        }
        case SSA_Kind_Return_Value_Type_Index:
        case SSA_Kind_Terminate_Global_Scope:
        case SSA_Kind_Invalid:
        case SSA_Kind_Int_Type:
        case SSA_Kind_Uint_Type:
        case SSA_Kind_Float_Type:
        case SSA_Kind_Void_Type:
        case SSA_Kind_Compile_To_LLVM_IR:
        case SSA_Kind_Int_Literal_Type:
        case SSA_Kind_Float_Literal_Type:
        case SSA_Kind_Call_Setup_Type:
        case SSA_Kind_Type_Type:
        case SSA_Kind_Function_Type:
        case SSA_Kind_Build:
        case SSA_Kind_Int_Literal:
        case SSA_Kind_Float_Literal:
        case SSA_Kind_Function_Declaration:
        case SSA_Kind_Argument_Type:
        case SSA_Kind_Parameter_Type:
        case SSA_Kind_Underlying_Type:
        case SSA_Kind_Return_Type:
        case SSA_Kind_Call_Return_Type:
        case SSA_Kind_Return_Value_Type:
        case SSA_Kind_Pointer_Type: {
            break;
        }
    }
    char* end = builder->str.data + builder->str.count;
    if (start != end) {
        utf8_builder_append(builder, utf8_str("\n"));
    }
    return utf8_slice(start, end);
}

void llvm_branch_to_block(SSA_Block* block, utf8_builder* builder) {
    utf8 block_name = llvm_block_name(block);
    utf8_builder_append(builder, utf8_str("br label %"));
    utf8_builder_append(builder, block_name);
}
