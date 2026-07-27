#include "llvm.h"

#include "cap.h"
#include "ssa.h"

void llvm_ssa_to_name_set_name(SSA* ssa, utf8 name) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa);
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            per_function_context_values->codegen_already_has_name = true;
            per_function_context_values->codegen_name = name;
            assert(per_function_context_values->codegen_already_has_name || utf8_equal(name, per_function_context_values->codegen_name));
            return;
        }
    }
}

utf8 llvm_ssa_to_name_set_local_name(SSA* ssa) {
    char buffer[128] = {0};
    Function_Context* function_context = ssa_get_cache_function_context(ssa);
    snprintf(buffer, 128, "%cS%p%p", '%', ssa, function_context);
    utf8 name_utf8 = {0};
    u32 name_length = strlen(buffer);
    name_utf8.data = alloc(name_length + 1);
    memcpy(name_utf8.data, buffer, name_length + 1);
    name_utf8.count = name_length;
    llvm_ssa_to_name_set_name(ssa, name_utf8);
    return name_utf8;
}

utf8 llvm_ssa_to_name_set_global_name(SSA* ssa) {
    char buffer[128] = {0};
    snprintf(buffer, 128, "%cG%p%p", '@', ssa, ssa_get_cache_function_context(ssa));
    utf8 name_utf8 = {0};
    u32 name_length = strlen(buffer);
    name_utf8.data = alloc(name_length + 1);
    memcpy(name_utf8.data, buffer, name_length + 1);
    name_utf8.count = name_length;
    llvm_ssa_to_name_set_name(ssa, name_utf8);
    return name_utf8;
}

utf8 llvm_ssa_to_name(SSA* ssa) {
    // return ssa_get_ssa_name(ssa);
    char buffer[128] = {0};
    Function_Context* function_context = ssa_get_cache_function_context(ssa);
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            bool codegen_already_has_name = per_function_context_values->codegen_already_has_name;
            if (codegen_already_has_name) return per_function_context_values->codegen_name;
            switch (ssa->kind) {
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
                case SSA_Kind_Function_Type: {
                    buffer[0] = '{';
                    buffer[1] = '}';
                    buffer[2] = 0;
                    break;
                }
                case SSA_Kind_Stack_Alloc: {
                    if (ssa->block->kind == SSA_Block_Kind_Global) {
                        snprintf(buffer, 128, "%cG%p%p", '@', ssa, function_context);
                        break;
                    }
                }
                case SSA_Kind_Store:
                case SSA_Kind_Load:
                case SSA_Kind_Function_Declaration:
                case SSA_Kind_Parameter:
                case SSA_Kind_Parameter_Type:
                case SSA_Kind_Argument:
                case SSA_Kind_Argument_Type:
                case SSA_Kind_Int_Literal:
                case SSA_Kind_Float_Literal:
                case SSA_Kind_Return:
                case SSA_Kind_Return_Type:
                case SSA_Kind_Implicit_Cast:
                case SSA_Kind_Explicit_Cast:
                case SSA_Kind_Build:
                case SSA_Kind_Call_Setup:
                case SSA_Kind_Call:
                case SSA_Kind_Call_Return_Type:
                case SSA_Kind_Pointer_Type:
                case SSA_Kind_Underlying_Type:
                case SSA_Kind_Default_Value:
                case SSA_Kind_Struct_Type:
                case SSA_Kind_Struct_Value:
                case SSA_Kind_Struct_Index_Number:
                case SSA_Kind_Struct_Type_Index_Number:
                case SSA_Kind_Struct_Index_Name:
                case SSA_Kind_Struct_Type_Index_Name: {
                    sprintf(buffer, "%cS%p%p", '%', ssa, function_context);
                    break;
                }
            }
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

utf8 llvm_block_to_name(SSA_Block* block, char* buffer) {
    Function_Context* function_context = ssa_get_function_context().function_context;
    sprintf(buffer, "%c%p%p", 'B', function_context, block);
    return utf8_str(buffer);
}

utf8 llvm_type(Type* type, char* buffer, u32 buffer_capacity) {
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
            u32 bits = type->data.int_.bits;
            snprintf(buffer, buffer_capacity, "i%u", bits);
            return utf8_str(buffer);
        }
        case Type_Kind_Uint: {
            u32 bits = type->data.uint.bits;
            snprintf(buffer, buffer_capacity, "u%u", bits);
            return utf8_str(buffer);
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
        case Type_Kind_Struct: {
            assert(false);
        }
        case Type_Kind_Optional: {
            assert(false);
        }
    }
    assert(false);
    return utf8_str("invalid");
}

utf8 llvm_block_to_llvm(SSA_Block* block, utf8* buffer, u32* buffer_capacity) {
    char* start = buffer->data + buffer->count;

    llvm_block_setup(block, buffer, buffer_capacity);
    llvm_block_body(block, buffer, buffer_capacity);
    char* end = buffer->data + buffer->count;
    return utf8_slice(start, end);
}

utf8 llvm_block_setup(SSA_Block* block, utf8* buffer, u32* buffer_capacity) {
    char* start = buffer->data + buffer->count;

    bool global = block->kind == SSA_Block_Kind_Global;
    bool exit = false;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = &block->statement_lists[i];
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = &list->statements[j];
            switch (ssa->kind) {
                case SSA_Kind_Stack_Alloc: {
                    Type* type = ssa_type(ssa)->data.ptr.type;
                    char type_buffer[512] = {0};
                    utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
                    utf8 name = llvm_ssa_to_name(ssa);
                    if (global) {
                        utf8_append_with_capacity(buffer, buffer_capacity, name);
                        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = internal global "));
                        utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" zeroinitializer\n"));
                    } else {
                        utf8_append_with_capacity(buffer, buffer_capacity, name);
                        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = "));
                        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("alloca "));
                        utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("\n"));
                    }
                    break;
                }
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

    if (!global) {
        char block_buffer[512] = {0};
        utf8 block_name = llvm_block_to_name(block, block_buffer);
        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("br label %"));
        utf8_append_with_capacity(buffer, buffer_capacity, block_name);
        utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("\n"));
    }

    char* end = buffer->data + buffer->count;
    return utf8_slice(start, end);
}

utf8 llvm_block_body(SSA_Block* block, utf8* buffer, u32* buffer_capacity) {
    char* start = buffer->data + buffer->count;

    char block_buffer[512] = {0};
    utf8 block_name = llvm_block_to_name(block, block_buffer);
    utf8_append_with_capacity(buffer, buffer_capacity, block_name);
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(":\n"));

    bool exit = false;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = &block->statement_lists[i];
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = &list->statements[j];
            utf8 str = llvm_ssa_to_statement(ssa, buffer, buffer_capacity);
            switch (ssa->kind) {
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
    char* end = buffer->data + buffer->count;
    return utf8_slice(start, end);
}

utf8 llvm_global_scope_to_llvm(utf8* buffer, u32* buffer_capacity) {
    char* start = buffer->data + buffer->count;

    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("; ModuleID = 'cap_module'\n"));
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("@empty = internal global {} zeroinitializer\n"));

    SSA_Block* global_block = &context.global_block;

    llvm_block_setup(global_block, buffer, buffer_capacity);

    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("define i32 @main() {\n"));
    llvm_block_body(global_block, buffer, buffer_capacity);
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("%ret = call i32 @__cap_main__()\n"));
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("ret i32 %ret\n"));
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("}\n"));

    char* end = buffer->data + buffer->count;
    return utf8_slice(start, end);
}

void llvm_append_assign_empty_value(utf8 name, utf8* buffer, u32* buffer_capacity) {
    utf8_append_with_capacity(buffer, buffer_capacity, name);
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = load {}, ptr @empty"));
}

utf8 llvm_ssa_to_statement(SSA* ssa, utf8* buffer, u32* buffer_capacity) {
    char* start = buffer->data + buffer->count;
    char name_buffer[512] = {0};
    utf8 our_name = llvm_ssa_to_name(ssa);
    Type* type = ssa_type(ssa);
    switch (ssa->kind) {
        case SSA_Kind_Store: {
            char adress_buffer[512] = {0};
            SSA* address = ssa->data.store.address;
            utf8 address_name = llvm_ssa_to_name(address);
            Type* value_type = ssa_type(ssa->data.store.value);
            char type_buffer[512] = {0};
            utf8 type_name = llvm_type(value_type, type_buffer, arr_len(type_buffer));
            char value_buffer[512] = {0};
            utf8 value_name = llvm_ssa_to_name(ssa->data.store.value);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("store "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
            utf8_append_with_capacity(buffer, buffer_capacity, value_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", ptr "));
            utf8_append_with_capacity(buffer, buffer_capacity, address_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("\n"));
            llvm_append_assign_empty_value(our_name, buffer, buffer_capacity);
            break;
        }
        case SSA_Kind_Load: {
            char address_buffer[512] = {0};
            SSA* address = ssa->data.load.address;
            utf8 address_name = llvm_ssa_to_name(address);
            char type_buffer[512] = {0};
            Type* type = ssa_type(ssa);
            utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
            utf8_append_with_capacity(buffer, buffer_capacity, our_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = load "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", ptr "));
            utf8_append_with_capacity(buffer, buffer_capacity, address_name);
            break;
        }
        case SSA_Kind_Stack_Alloc: {
            char type_buffer[512] = {0};
            Type* type = ssa_type(ssa->data.stack_alloc.initial_value);

            utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
            char initial_value_buffer[512] = {0};
            utf8 initial_value_name = llvm_ssa_to_name(ssa->data.stack_alloc.initial_value);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("store "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
            utf8_append_with_capacity(buffer, buffer_capacity, initial_value_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", ptr "));
            utf8_append_with_capacity(buffer, buffer_capacity, our_name);
            break;
        }
        case SSA_Kind_Call_Setup: {
            Function_Context* function_context = ssa_evaluate_function_context(ssa);
            All_Function_Context all = {0};
            all.function_context = function_context;
            ssa_push_function_context(all);
            SSA* callee = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);
            SSA_Block* function_setup_block = &function->setup_block;
            for (u32 i = 0; i < function_setup_block->statement_lists_count; i++) {
                SSA_List* list = &function_setup_block->statement_lists[i];
                for (u32 j = 0; j < list->statements_count; j++) {
                    SSA* ssa = &list->statements[j];
                    utf8 str = llvm_ssa_to_statement(ssa, buffer, buffer_capacity);
                }
            }
            ssa_pop_function_context();
            break;
        }
        case SSA_Kind_Implicit_Cast:
        case SSA_Kind_Explicit_Cast: {
            SSA* value = ssa->data.explicit_cast.value;
            Type* value_type = ssa_type(value);
            Type* type = ssa_type(ssa);
            if (ssa_type_equal(value_type, type)) {
                char type_buffer[512] = {0};
                utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
                char value_buffer[512] = {0};
                utf8 value_name = llvm_ssa_to_name(value);
                utf8_append_with_capacity(buffer, buffer_capacity, our_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = select i1 true, "));
                utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
                utf8_append_with_capacity(buffer, buffer_capacity, value_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", "));
                utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
                utf8_append_with_capacity(buffer, buffer_capacity, value_name);
            } else if (value_type->kind == Type_Kind_Int_Literal) {
                void* comp_value = ssa_evaluate(value);
                Big_Int* big_int = comp_value;
                i64 v = big_int->data;

                char i64_buffer[512] = {0};
                snprintf(i64_buffer, 512, "%lld", v);
                char type_buffer[512] = {0};
                utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));

                utf8_append_with_capacity(buffer, buffer_capacity, our_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = add "));
                utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" 0, "));
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(i64_buffer));
            } else if (value_type->kind == Type_Kind_Float_Literal) {
                f64* comp_value = ssa_evaluate(value);
                f64 v = *comp_value;

                utf8 f64_hex = utf8_memory_as_hex(&v, sizeof(f64));
                char type_buffer[512] = {0};
                utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));

                utf8_append_with_capacity(buffer, buffer_capacity, our_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = fadd "));
                utf8_append_with_capacity(buffer, buffer_capacity, type_name);
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
                utf8_append_with_capacity(buffer, buffer_capacity, f64_hex);
            } else {
                assert(false);
            }
            break;
        }
        case SSA_Kind_Return: {
            SSA* return_value = ssa->data.return_.return_value;
            Type* return_type = ssa_type(return_value);
            if (return_type->kind == Type_Kind_Void) {
                utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("ret void"));
                break;
            }
            char type_buffer[512] = {0};
            utf8 type_name = llvm_type(return_type, type_buffer, arr_len(type_buffer));
            char value_buffer[512] = {0};
            utf8 value_name = llvm_ssa_to_name(return_value);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("ret "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
            utf8_append_with_capacity(buffer, buffer_capacity, value_name);
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
                    assert(false);
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
                            llvm_append_assign_empty_value(our_name, buffer, buffer_capacity);
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
        case SSA_Kind_Argument: {
            Type* type = ssa_type(ssa);

            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            u32 index = ssa->data.argument.index;
            SSA* argument = function_context->arguments[index];

            char type_buffer[512] = {0};
            utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
            char value_buffer[512] = {0};

            ssa_pop_function_context();
            utf8 value_name = llvm_ssa_to_name(argument);
            ssa_push_function_context(all);

            utf8_append_with_capacity(buffer, buffer_capacity, our_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = select i1 true, "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
            utf8_append_with_capacity(buffer, buffer_capacity, value_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" "));
            utf8_append_with_capacity(buffer, buffer_capacity, value_name);
            break;
        }
        case SSA_Kind_Default_Value: {
            Type* type = ssa_type(ssa);
            char type_buffer[512] = {0};
            utf8 type_name = llvm_type(type, type_buffer, arr_len(type_buffer));
            utf8_append_with_capacity(buffer, buffer_capacity, our_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" = select i1 true, "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" zeroinitializer"));
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(", "));
            utf8_append_with_capacity(buffer, buffer_capacity, type_name);
            utf8_append_with_capacity(buffer, buffer_capacity, utf8_str(" zeroinitializer"));
            break;
        }
        case SSA_Kind_Struct_Value:
        case SSA_Kind_Struct_Index_Number:
        case SSA_Kind_Struct_Index_Name: {
            assert(false);
        }
        case SSA_Kind_Build:
        case SSA_Kind_Int_Literal:
        case SSA_Kind_Float_Literal:
        case SSA_Kind_Function_Declaration:
        case SSA_Kind_Argument_Type:
        case SSA_Kind_Parameter_Type:
        case SSA_Kind_Underlying_Type:
        case SSA_Kind_Return_Type:
        case SSA_Kind_Call_Return_Type:
        case SSA_Kind_Pointer_Type:
        case SSA_Kind_Struct_Type:
        case SSA_Kind_Struct_Type_Index_Number:
        case SSA_Kind_Struct_Type_Index_Name: {
            llvm_append_assign_empty_value(our_name, buffer, buffer_capacity);
            break;
        }
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
        case SSA_Kind_Parameter: {
            return utf8_slice(start, start);
        }
    }
    utf8_append_with_capacity(buffer, buffer_capacity, utf8_str("\n"));
    char* end = buffer->data + buffer->count;
    return utf8_slice(start, end);
}
