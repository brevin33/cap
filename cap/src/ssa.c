#include "ssa.h"

#include <math.h>

#include "ast.h"
#include "cap.h"
#include "log.h"
#include "util/big_int.h"

SSA* ssa_add_to_block(SSA ssa, SSA_Block* block) {
    ssa.block = block;
    if (block->statement_lists_count == 0) {
        SSA_List list = {0};
        list.statements_capacity = 8;
        list.statements = alloc(sizeof(SSA) * list.statements_capacity);
        ptr_append(block->statement_lists, block->statement_lists_count, block->statement_lists_capacity, list);
    }
    SSA_List* list = block->statement_lists + block->statement_lists_count - 1;
    if (list->statements_count == list->statements_capacity) {
        SSA_List new_list = {0};
        new_list.statements_capacity = list->statements_capacity * 2;
        new_list.statements = alloc(sizeof(SSA) * new_list.statements_capacity);
        ptr_append(block->statement_lists, block->statement_lists_count, block->statement_lists_capacity, new_list);
        list = block->statement_lists + block->statement_lists_count - 1;
    }
    list->statements[list->statements_count++] = ssa;
    SSA* ssa_ptr = list->statements + list->statements_count - 1;
    return ssa_ptr;
}

SSA* ssa_top_level_ast_to_ssa(Ast* ast, SSA_Block* block) {
    switch (ast->kind) {
        case Ast_Kind_Build: {
            return ssa_build_ast_prototype(ast, block);
        }
        case Ast_Kind_Function_Declaration: {
            return ssa_function_declaration_ast_prototype(ast, block);
        }
        default: {
            return ssa_ast_to_ssa(ast, block);
        }
    }
}

void ssa_top_level_post_parse(SSA* ssa, SSA_Block* block) {
    switch (ssa->kind) {
        case SSA_Kind_Function_Declaration: {
            ssa_function_declaration_ast_implement(ssa, block);
            break;
        }
        case SSA_Kind_Build: {
            ssa_build_ast_implement(ssa, block);
            break;
        }
        default:
            break;
    }
}

SSA* ssa_ast_to_ssa(Ast* ast, SSA_Block* block) {
    switch (ast->kind) {
        case Ast_Kind_Assign: {
            u32 lhs_count = ast->data.assign.lhs_count;
            u32 rhs_count = ast->data.assign.rhs_count;
            if (lhs_count == rhs_count) {
                for (u32 i = 0; i < rhs_count; i++) {
                    Ast* lhs_ast = &ast->data.assign.lhs[i];
                    Ast* rhs_ast = &ast->data.assign.rhs[i];
                    if (lhs_ast->kind == Ast_Kind_Variable_Declaration) {
                        SSA* type = ssa_ast_to_ssa(lhs_ast->data.variable_declaration.type, block);
                        SSA* rhs_ssa = ssa_ast_to_ssa(rhs_ast, block);
                        SSA* rhs_cast = ssa_implicit_cast(rhs_ssa, type, block, rhs_ssa->ast);
                        SSA* variable = ssa_stack_alloc(type, rhs_cast, block, lhs_ast);
                        lhs_ast->data.variable_declaration.value = variable;
                    } else {
                        SSA* lhs_ssa = ssa_ast_to_ssa(lhs_ast, block);
                        SSA* rhs_ssa = ssa_ast_to_ssa(rhs_ast, block);
                        SSA* cast_type = ssa_underlying_type(lhs_ssa->type, block, lhs_ssa->type->ast);
                        SSA* rhs_cast = ssa_implicit_cast(rhs_ssa, cast_type, block, rhs_ssa->ast);
                        SSA* store = ssa_store(rhs_cast, lhs_ssa, block, lhs_ssa->ast);
                    }
                }
            } else {
                assert(rhs_count == 1);
                Ast* rhs_ast = &ast->data.assign.rhs[0];
                SSA** lhs_ssas = alloc(lhs_count * sizeof(SSA*));
                for (u32 i = 0; i < lhs_count; i++) {
                    Ast* lhs_ast = &ast->data.assign.lhs[i];
                    if (lhs_ast->kind == Ast_Kind_Variable_Declaration) {
                        SSA* type = ssa_ast_to_ssa(lhs_ast->data.variable_declaration.type, block);
                        lhs_ssas[i] = type;
                    } else {
                        SSA* lhs_ssa = ssa_ast_to_ssa(lhs_ast, block);
                        lhs_ssas[i] = lhs_ssa;
                    }
                }
                SSA* rhs_ssa_multi_value = ssa_ast_to_ssa(rhs_ast, block);
                for (u32 i = 0; i < lhs_count; i++) {
                    Ast* lhs_ast = &ast->data.assign.lhs[i];
                    SSA* rhs_ssa = ssa_return_value_index(rhs_ssa_multi_value, i, block, rhs_ssa_multi_value->ast);
                    if (lhs_ast->kind == Ast_Kind_Variable_Declaration) {
                        SSA* type = lhs_ssas[i];
                        SSA* rhs_cast = ssa_implicit_cast(rhs_ssa, type, block, rhs_ssa->ast);
                        SSA* variable = ssa_stack_alloc(type, rhs_cast, block, lhs_ast);
                        lhs_ast->data.variable_declaration.value = variable;
                    } else {
                        SSA* lhs_ssa = lhs_ssas[i];
                        SSA* cast_type = ssa_underlying_type(lhs_ssa->type, block, lhs_ssa->type->ast);
                        SSA* rhs_cast = ssa_implicit_cast(rhs_ssa, cast_type, block, rhs_ssa->ast);
                        SSA* store = ssa_store(rhs_cast, lhs_ssa, block, lhs_ssa->ast);
                    }
                }
            }
            return NULL;
        }
        case Ast_Kind_Function_Declaration: {
            SSA* function_declaration_ast_prototype = ssa_function_declaration_ast_prototype(ast, block);
            ssa_function_declaration_ast_implement(function_declaration_ast_prototype, block);
            return function_declaration_ast_prototype;
        }
        case Ast_Kind_Int: {
            return ssa_int_literal(ast->data.int_.value, block, ast);
        }
        case Ast_Kind_Float: {
            return ssa_float_literal(ast->data.float_.value, block, ast);
        }
        case Ast_Kind_Variable: {
            Ast* declaration = ast->data.variable.variable_declaration;
            SSA* declaration_ssa = NULL;
            switch (declaration->kind) {
                case Ast_Kind_Variable_Declaration: {
                    declaration_ssa = declaration->data.variable_declaration.value;
                    break;
                }
                case Ast_Kind_Function_Declaration: {
                    declaration_ssa = declaration->data.function_declaration.value;
                    break;
                }
                case Ast_Kind_Parameter: {
                    declaration_ssa = declaration->data.parameter.value;
                    break;
                }
                default: {
                    internal_compiler_error();
                }
            }
            assert(declaration_ssa != NULL);
            return declaration_ssa;
        }
        case Ast_Kind_Return: {
            u32 return_values_count = ast->data.return_.values_count;
            SSA** return_values = alloc(sizeof(SSA*) * return_values_count);
            SSA* return_type = ssa_return_type(block, ast);
            for (u32 i = 0; i < return_values_count; i++) {
                Ast* return_value = ast->data.return_.values + i;
                SSA* value = ssa_ast_to_ssa(return_value, block);
                SSA* value_return_type;
                if (return_values_count == 1) {
                    value_return_type = return_type;
                } else {
                    value_return_type = ssa_return_value_type_index(return_type, i, block, return_value);
                }
                SSA* implicit_cast = ssa_implicit_cast(value, value_return_type, block, return_value);
                return_values[i] = implicit_cast;
            }
            if (return_values_count == 1) {
                return ssa_return(return_values[0], block, ast);
            }
            SSA* return_value = ssa_return_value(return_values, return_values_count, block, ast);
            return ssa_return(return_value, block, ast);
        }
        case Ast_Kind_Variable_Declaration: {
            SSA* type = ssa_ast_to_ssa(ast->data.variable_declaration.type, block);
            SSA* default_value = ssa_default_value(type, block, ast);
            SSA* variable = ssa_stack_alloc(type, default_value, block, ast);
            ast->data.variable_declaration.value = variable;
            return variable;
        }
        case Ast_Kind_Call: {
            Ast* callee = ast->data.call.callee;
            SSA* callee_ssa = ssa_ast_to_ssa(callee, block);

            Ast* argument_list = ast->data.call.argument_list;
            u32 arguments_count = argument_list->data.argument_list.arguments_count;

            SSA** arguments = alloc(sizeof(SSA*) * arguments_count);
            for (u32 i = 0; i < arguments_count; i++) {
                Ast* argument = argument_list->data.argument_list.arguments + i;
                SSA* argument_ssa = ssa_ast_to_ssa(argument, block);
                arguments[i] = argument_ssa;
            }

            SSA* call_setup = ssa_call_setup(callee_ssa, arguments, arguments_count, block, ast);
            SSA* call = ssa_call(call_setup, block, ast);
            return call;
        }
        case Ast_Kind_Build: {
            SSA* build_ast_prototype = ssa_build_ast_prototype(ast, block);
            ssa_build_ast_implement(build_ast_prototype, block);
            return build_ast_prototype;
        }
        case Ast_Kind_Load: {
            Ast* address = ast->data.load.address;
            SSA* address_ssa = ssa_ast_to_ssa(address, block);
            SSA* load = ssa_load(address_ssa, block, ast);
            return load;
        }
        case Ast_Kind_Intrinsic_Int_Type: {
            return ssa_int_type();
        }
        case Ast_Kind_Intrinsic_Uint_Type: {
            return ssa_uint_type();
        }
        case Ast_Kind_Intrinsic_Float_Type: {
            return ssa_float_type();
        }
        case Ast_Kind_Intrinsic_Compile_To_LLVM_IR: {
            return ssa_compile_to_llvm_ir();
        }
        case Ast_Kind_Intrinsic_Type: {
            return ssa_type_type();
        }
        case Ast_Kind_Intrinsic_Function: {
            return ssa_function_type();
        }
        case Ast_Kind_Intrinsic_Void: {
            return ssa_void_type();
        }
        case Ast_Kind_Binary_Operation:
        case Ast_Kind_Argument_List:
        case Ast_Kind_Parameter_List:
        case Ast_Kind_Parameter:
        case Ast_Kind_Scope:
        case Ast_Kind_File:
        case Ast_Kind_Invalid: {
            internal_compiler_error();
            return NULL;
        }
    }
}

static bool _ssa_type_check(SSA* ssa, Type* type) {
    switch (ssa->kind) {
        case SSA_Kind_Invalid: {
            internal_compiler_error();
        }
        case SSA_Kind_Store: {
            SSA* address = ssa->data.store.address;
            Type* address_type = ssa_type(address);
            if (address_type == NULL) return false;
            if (address_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("address type is not ptr", log_error, address);
                return false;
            }
            SSA* value = ssa->data.store.value;
            Type* value_type = ssa_type(value);
            if (value_type == NULL) return false;
            Type* address_underlying_type = address_type->data.ptr.type;
            if (!ssa_type_equal(value_type, address_underlying_type)) {
                log_msg_ssa("value type does not match address underlying type", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Load: {
            SSA* address = ssa->data.load.address;
            Type* address_type = ssa_type(address);
            if (address_type == NULL) return false;
            if (address_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("address type is not ptr", log_error, address);
                return false;
            }
            return true;
        }
        case SSA_Kind_Stack_Alloc: {
            SSA* alloc_type_ssa = ssa->data.stack_alloc.type;
            Type* alloc_type = ssa_evaluate_type(alloc_type_ssa);
            if (alloc_type == NULL) return false;
            assert(type->kind == Type_Kind_Ptr);
            return true;
        }
        case SSA_Kind_Implicit_Cast: {
            Type* from_type = ssa_type(ssa->data.implicit_cast.value);
            if (from_type == NULL) return false;
            assert(ssa_type_equal(ssa_evaluate_type(ssa->data.implicit_cast.type), type));
            bool res = ssa_can_implicit_cast(from_type, type);
            if (!res) {
                log_msg_ssa("Can't implicit cast from type", log_error, ssa);
                log_msg_ssa("From type", log_info, ssa->data.implicit_cast.value->type);
                log_msg_ssa("To type", log_info, ssa->type);
                return false;
            }
            return true;
        }
        case SSA_Kind_Explicit_Cast: {
            Type* from_type = ssa_type(ssa->data.explicit_cast.value);
            if (from_type == NULL) return false;
            Type* to_type = ssa_evaluate_type(ssa->data.explicit_cast.type);
            if (to_type == NULL) return false;
            bool res = ssa_can_explicit_cast(from_type, to_type);
            if (!res) {
                log_msg_ssa("Can't explicit cast cast from type", log_error, ssa);
                log_msg_ssa("From type", log_info, ssa->data.implicit_cast.value->type);
                log_msg_ssa("To type", log_info, ssa->type);
                return false;
            }
            return res;
        }
        case SSA_Kind_Call_Setup: {
            SSA* callee = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);
            if (function == NULL) return false;
            if (ssa->data.call_setup.arguments_count > function->parameter_count) {
                log_msg_ssa("Too many arguments to call function", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Call_Return_Type: {
            SSA* call_setup = ssa->data.call_return_type.setup;
            Function_Context* function_context = ssa_evaluate_function_context(call_setup);
            if (function_context == NULL) return false;
            return true;
        }
        case SSA_Kind_Call: {
            return ssa_type_check_call(ssa, type);
        }
        case SSA_Kind_Pointer_Type: {
            SSA* pointer_of_type_ssa = ssa->data.pointer_type.type;
            Type* pointer_of_type = ssa_evaluate_type(pointer_of_type_ssa);
            if (pointer_of_type == NULL) return false;
            return true;
        }
        case SSA_Kind_Underlying_Type: {
            SSA* ptr_type_ssa = ssa->data.underlying_type.type;
            Type* ptr_type = ssa_evaluate_type(ptr_type_ssa);
            if (ptr_type == NULL) return false;
            if (ptr_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can't get underlying type of non-ptr type", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Return_Value_Type: {
            for (u32 i = 0; i < ssa->data.return_value_type.types_count; i++) {
                SSA* type_inside = ssa->data.return_value_type.types[i];
                Type* t = ssa_evaluate_type(type_inside);
                if (t == NULL) return false;
            }
            return true;
        }
        case SSA_Kind_Return_Value_Index: {
            SSA* return_value_ssa = ssa->data.return_value_index.return_value;
            Type* return_value_type = ssa_type(return_value_ssa);
            if (return_value_type == NULL) return false;
            if (return_value_type->kind != Type_Kind_Return_Value) {
                log_msg_ssa("Can't index return value when type is not a return value type", log_error, ssa);
                return false;
            }
            u32 index = ssa->data.return_value_index.index;
            if (index >= return_value_type->data.return_value.types_count) {
                utf8_builder builder = {0};
                utf8_builder_append(&builder, utf8_str("Trying to get return value with index "));
                char num_buf[64];
                sprintf(num_buf, "%u", index);
                utf8 num_utf8 = {num_buf, strlen(num_buf)};
                utf8_builder_append(&builder, num_utf8);
                utf8_builder_append(&builder, utf8_str(" but return values only have "));
                char num_buf2[64];
                sprintf(num_buf2, "%u", return_value_type->data.return_value.types_count);
                utf8 num_utf82 = {num_buf2, strlen(num_buf2)};
                utf8_builder_append(&builder, num_utf82);
                utf8_builder_append(&builder, utf8_str(" values"));
                log_utf8_ssa(builder.str, log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Return_Value_Type_Index: {
            SSA* return_value_type_ssa = ssa->data.return_value_type_index.return_value_type;
            Type* return_value_type = ssa_evaluate_type(return_value_type_ssa);
            if (return_value_type == NULL) return false;
            if (return_value_type->kind != Type_Kind_Return_Value) {
                log_msg_ssa("Can't index return value type when type is not a return value type", log_error, ssa);
                return false;
            }
            u32 index = ssa->data.return_value_type_index.index;
            if (index >= return_value_type->data.return_value.types_count) {
                utf8_builder builder = {0};
                utf8_builder_append(&builder, utf8_str("Trying to get return value with index "));
                char num_buf[64];
                sprintf(num_buf, "%u", index);
                utf8 num_utf8 = {num_buf, strlen(num_buf)};
                utf8_builder_append(&builder, num_utf8);
                utf8_builder_append(&builder, utf8_str(" but return values only have "));
                char num_buf2[64];
                sprintf(num_buf2, "%u", return_value_type->data.return_value.types_count);
                utf8 num_utf82 = {num_buf2, strlen(num_buf2)};
                utf8_builder_append(&builder, num_utf82);
                utf8_builder_append(&builder, utf8_str(" values"));
                log_utf8_ssa(builder.str, log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Terminate_Global_Scope:
        case SSA_Kind_Return_Value:
        case SSA_Kind_Parameter:
        case SSA_Kind_Default_Value:
        case SSA_Kind_Return:
        case SSA_Kind_Argument:
        case SSA_Kind_Argument_Type:
        case SSA_Kind_Parameter_Type:
        case SSA_Kind_Build:
        case SSA_Kind_Return_Type:
        case SSA_Kind_Function_Declaration:
        case SSA_Kind_Function_Type:
        case SSA_Kind_Type_Type:
        case SSA_Kind_Int_Literal_Type:
        case SSA_Kind_Int_Literal:
        case SSA_Kind_Float_Literal_Type:
        case SSA_Kind_Float_Literal:
        case SSA_Kind_Int_Type:
        case SSA_Kind_Uint_Type:
        case SSA_Kind_Float_Type:
        case SSA_Kind_Void_Type:
        case SSA_Kind_Call_Setup_Type:
        case SSA_Kind_Compile_To_LLVM_IR: {
            return true;
        }
    }
}

bool ssa_type_check(SSA* ssa) {
    bool res = false;
    if (ssa_already_type_checked(ssa, &res)) return res;
    Type* type = ssa_evaluate_type(ssa->type);
    if (type != NULL) res = _ssa_type_check(ssa, type);
    if (res) {
        log_msg_ssa("Type check succeeded", log_debug, ssa);
    } else {
        if (type == NULL) {
            log_msg_ssa("Type check failed becuase could not evaluate type", log_info, ssa);
        } else {
            log_msg_ssa("Type check failed", log_info, ssa);
        }
    }
    ssa_cache_type_check(ssa, res, type);
    return res;
}

Type* ssa_type(SSA* ssa) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            Type* type = &per_function_context_values->type;
            if (type->kind == Type_Kind_Invalid) return NULL;
            return type;
        }
    }
    return NULL;
}

bool ssa_already_type_checked(SSA* ssa, bool* out_res) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            *out_res = per_function_context_values->type_check_result;
            return per_function_context_values->type_checked;
        }
    }
    *out_res = false;
    return false;
}

void ssa_cache_type_check(SSA* ssa, bool res, Type* type) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;

    SSA_Per_Function_Context_Values per_function_context_values = {0};
    per_function_context_values.function_context = function_context;
    per_function_context_values.type_check_result = res;
    if (type != NULL) per_function_context_values.type = *type;
    per_function_context_values.type_checked = true;
    ptr_append(ssa->ssa_per_function_context_values, ssa->ssa_per_function_context_values_count, ssa->ssa_per_function_context_values_capacity,
               per_function_context_values);
}

bool ssa_type_check_call(SSA* ssa, Type* type) {
    SSA* call_setup = ssa->data.call.setup;
    Function_Context* function_context = ssa_evaluate_function_context(call_setup);
    if (function_context->dont_need_to_build == true) return true;
    if (function_context == NULL) return false;
    SSA* callee = call_setup->data.call_setup.callee;
    Function* function = ssa_evaluate_function(callee);
    if (function == NULL) return false;
    All_Function_Context all = {0};
    all.function_context = function_context;
    ssa_push_function_context(all);
    function_context->dont_need_to_build = true;
    bool res = true;
    switch (function->kind) {
        case Function_Kind_Invalid: {
            internal_compiler_error();
        }
        case Function_Kind_Internal: {
            SSA_Block* function_body = &function->data.internal.body;
            res = ssa_type_check_block(function_body);
            break;
        }
        case Function_Kind_Intrinsic: {
            switch (function->data.intrinsic.kind) {
                case Intrinsic_Function_Kind_Invalid: {
                    internal_compiler_error();
                }
                case Intrinsic_Function_Int_Type: {
                    res = ssa_type_check_call_int_type(ssa, function_context, function);
                    break;
                }
                case Intrinsic_Function_Uint_Type: {
                    res = ssa_type_check_call_uint_type(ssa, function_context, function);
                    break;
                }
                case Intrinsic_Function_Float_Type: {
                    res = ssa_type_check_call_float_type(ssa, function_context, function);
                    break;
                }
                case Intrinsic_Function_Kind_Compile_To_LLVM_IR: {
                    res = ssa_type_check_call_compile_to_llvm_ir(ssa, function_context, function);
                    break;
                }
            }
            break;
        }
    }
    ssa_pop_function_context();
    if (res) {
        function_context->already_built = true;
    }
    return res;
}

bool ssa_type_check_call_int_type(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* bits_parameter = function_context->parameters[0];
    Type* bits_parameter_type = ssa_type(bits_parameter);
    if (bits_parameter_type == NULL) return false;
    if (bits_parameter_type->kind != Type_Kind_Int && bits_parameter_type->kind != Type_Kind_Uint && bits_parameter_type->kind != Type_Kind_Int_Literal) {
        log_msg_ssa("Parameter to int type function is not int or uint", log_error, function_context->arguments[0]);
        return false;
    }

    void* bits_value = ssa_evaluate(bits_parameter);
    if (bits_value == NULL) return false;
    Type i64_type = {0};
    i64_type.kind = Type_Kind_Int;
    i64_type.data.int_.bits = 64;
    i64* bits_i64 = ssa_cast_value(bits_value, bits_parameter_type, &i64_type);
    if (bits_i64 == NULL) internal_compiler_error();
    i64 bits = *bits_i64;

    if (bits <= 0) {
        log_msg_ssa("Parameter to int type function is not positive", log_error, function_context->arguments[0]);
        return false;
    }

    return true;
}

bool ssa_type_check_call_uint_type(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* bits_parameter = function_context->parameters[0];
    Type* bits_parameter_type = ssa_type(bits_parameter);
    if (bits_parameter_type == NULL) return false;
    if (bits_parameter_type->kind != Type_Kind_Int && bits_parameter_type->kind != Type_Kind_Uint && bits_parameter_type->kind != Type_Kind_Int_Literal) {
        log_msg_ssa("Parameter to int type function is not int or uint", log_error, function_context->arguments[0]);
        return false;
    }

    void* bits_value = ssa_evaluate(bits_parameter);
    if (bits_value == NULL) return false;
    Type i64_type = {0};
    i64_type.kind = Type_Kind_Int;
    i64_type.data.int_.bits = 64;
    i64* bits_i64 = ssa_cast_value(bits_value, bits_parameter_type, &i64_type);
    if (bits_i64 == NULL) internal_compiler_error();
    i64 bits = *bits_i64;

    if (bits <= 0) {
        log_msg_ssa("Parameter to int type function is not positive", log_error, function_context->arguments[0]);
        return false;
    }

    return true;
}

bool ssa_type_check_call_float_type(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* bits_parameter = function_context->parameters[0];
    Type* bits_parameter_type = ssa_type(bits_parameter);
    if (bits_parameter_type == NULL) return false;
    if (bits_parameter_type->kind != Type_Kind_Int && bits_parameter_type->kind != Type_Kind_Uint && bits_parameter_type->kind != Type_Kind_Int_Literal) {
        log_msg_ssa("Parameter to int type function is not int or uint", log_error, function_context->arguments[0]);
        return false;
    }

    void* bits_value = ssa_evaluate(bits_parameter);
    if (bits_value == NULL) return false;
    Type i64_type = {0};
    i64_type.kind = Type_Kind_Int;
    i64_type.data.int_.bits = 64;
    i64* bits_i64 = ssa_cast_value(bits_value, bits_parameter_type, &i64_type);
    if (bits_i64 == NULL) internal_compiler_error();
    i64 bits = *bits_i64;

    if (bits <= 0) {
        log_msg_ssa("Parameter to int type function is not positive", log_error, function_context->arguments[0]);
        return false;
    }

    if (bits != 64 && bits != 32) {
        log_msg_ssa("Parameter to float type function can only be 32 or 64", log_error, function_context->arguments[0]);
        return false;
    }

    return true;
}

bool ssa_type_check_call_compile_to_llvm_ir(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* parameter_0_ssa = function_context->parameters[0];
    Function* compile_function = ssa_evaluate_function(parameter_0_ssa);
    if (compile_function == NULL) return false;

    SSA_Block* function_setup_block = &compile_function->setup_block;
    if (!ssa_type_check_block(function_setup_block)) return false;

    SSA* compile_function_return_type = compile_function->return_type;
    Type* compile_function_return_type_type = ssa_evaluate_type(compile_function_return_type);
    if (compile_function_return_type_type == NULL) return false;
    if (compile_function_return_type_type->kind == Type_Kind_Void) {
    } else if (compile_function_return_type_type->kind == Type_Kind_Int) {
        if (compile_function_return_type_type->data.int_.bits != 32) {
            log_msg_ssa("Return type of compile to llvm ir function must be 32 bit int or void", log_error, function_context->arguments[0]);
            return false;
        }
    } else {
        log_msg_ssa("Return type of compile to llvm ir function must be 32 bit int or void", log_error, function_context->arguments[0]);
        return false;
    }

    if (compile_function->parameter_count != 0) {
        assert(false);
    }

    return true;
}

bool ssa_type_check_block(SSA_Block* block) {
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            if (!ssa_type_check(ssa)) return false;
        }
    }
    return true;
}

bool ssa_type_check_builds() {
    SSA_Block* block = &context.global_block;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            if (ssa->kind == SSA_Kind_Build) {
                SSA_Build* build = &ssa->data.build;
                Function_Context* function_context = build->function_context;
                All_Function_Context all = {0};
                all.function_context = function_context;
                ssa_push_function_context(all);
                bool res = ssa_type_check_block(&build->block);
                ssa_pop_function_context();
                if (!res) return false;
            }
        }
    }
    return true;
}

Function_Context* ssa_evaluate_function_context(SSA* ssa) {
    Type* type = ssa_type(ssa);
    if (type == NULL) return NULL;
    if (type->kind != Type_Kind_Call_Setup) {
        log_msg_ssa("Can't evaluate value to function", log_error, ssa);
        return NULL;
    }
    Function_Context** function_context = ssa_evaluate(ssa);
    if (function_context == NULL) return NULL;
    return *function_context;
}

Function* ssa_evaluate_function(SSA* ssa) {
    Type* type = ssa_type(ssa);
    if (type == NULL) return NULL;
    if (type->kind != Type_Kind_Function) {
        log_msg_ssa("Can't evaluate value to function", log_error, ssa);
        return NULL;
    }
    return *(Function**)ssa_evaluate(ssa);
}

Type* ssa_evaluate_type(SSA* ssa) {
    if (ssa->kind == SSA_Kind_Type_Type) return context.intrinsic_ssa_block.type_type_value;
    Type* type = ssa_type(ssa);
    if (type == NULL) return NULL;
    if (type->kind != Type_Kind_Type) {
        log_msg_ssa("Can't evaluate value to type", log_error, ssa);
        return NULL;
    }
    return ssa_evaluate(ssa);
}

static bool ssa_infer_trace_argument(Infer_Context* infer_context, SSA* argument_side, SSA* declaration_side, bool* out_conflict) {
    *out_conflict = false;
    if (declaration_side->kind == SSA_Kind_Implicit_Cast) {
        SSA* value = declaration_side->data.implicit_cast.value;
        return ssa_infer_trace_argument(infer_context, argument_side, value, out_conflict);
    }
    if (declaration_side->kind == SSA_Kind_Explicit_Cast) {
        SSA* value = declaration_side->data.explicit_cast.value;
        return ssa_infer_trace_argument(infer_context, argument_side, value, out_conflict);
    }
    if (declaration_side->kind == SSA_Kind_Call && argument_side->kind == SSA_Kind_Call) {
        SSA* decl_setup = declaration_side->data.call.setup;
        SSA* argument_setup = argument_side->data.call.setup;

        bool type_check_res = false;

        context.evaluate_context = infer_context->decl_side_evaluate_context;
        SSA* decl_callee = decl_setup->data.call_setup.callee;
        if (!ssa_already_type_checked(decl_callee, &type_check_res)) return false;
        if (!type_check_res) return false;
        Function* decl_function = ssa_evaluate_function(decl_callee);
        if (decl_function == NULL) return false;

        context.evaluate_context = infer_context->arg_side_evaluate_context;
        SSA* argument_callee = argument_setup->data.call_setup.callee;
        Function* argument_function = ssa_evaluate_function(argument_callee);
        if (argument_function == NULL) return false;

        if (decl_function != argument_function) return false;

        u32 min_args = min(decl_setup->data.call_setup.arguments_count, argument_setup->data.call_setup.arguments_count);
        for (u32 i = 0; i < min_args; i++) {
            SSA* decl_argument = decl_setup->data.call_setup.arguments[i];
            SSA* argument_argument = argument_setup->data.call_setup.arguments[i];
            if (ssa_infer_trace_argument(infer_context, argument_argument, decl_argument, out_conflict)) return true;
        }
        return false;
    }
    if (declaration_side->kind == SSA_Kind_Argument) {
        u32 index = declaration_side->data.argument.index;
        SSA* prev_argument = infer_context->arguments[index];
        if (prev_argument == NULL) {
            infer_context->arguments[index] = argument_side;
            return true;
        }

        context.evaluate_context = infer_context->arg_side_evaluate_context;

        Type* argument_type = ssa_type(argument_side);
        if (argument_type == NULL) return false;
        Type* prev_argument_type = ssa_type(prev_argument);
        if (prev_argument_type == NULL) return false;
        if (!ssa_type_equal(argument_type, prev_argument_type)) {
            log_msg_ssa("Conflict between inferred argument types", log_error, argument_side);
            *out_conflict = true;
            return false;
        }

        void* argument_value = ssa_evaluate(argument_side);
        if (argument_value == NULL) return false;
        void* prev_argument_value = ssa_evaluate(prev_argument);
        if (prev_argument_value == NULL) return false;
        if (!ssa_compile_time_value_equal(argument_value, prev_argument_value, argument_type)) {
            log_msg_ssa("Conflict between inferred argument values", log_error, argument_side);
            *out_conflict = true;
            return false;
        }
        return false;
    }
    return false;
}

static bool* _ssa_infer_get_argument_dependencies_found_for_ssa(SSA* ssa, SSA_Block* block, Infer_Context* infer_context) {
    u32 argument_dependencies_index = 0;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* statement = list->statements + j;
            if (statement == ssa) {
                return infer_context->argument_dependencies[argument_dependencies_index];
            }
            argument_dependencies_index++;
        }
    }
    internal_compiler_error();
}

static void _ssa_infer_take_dependencies(bool* argument_dependencies, SSA* other_ssa, SSA_Block* block, Infer_Context* infer_context) {
    if (other_ssa->block->kind == SSA_Block_Kind_Global) return;
    bool* other_argument_dependencies = _ssa_infer_get_argument_dependencies_found_for_ssa(other_ssa, block, infer_context);
    for (u32 i = 0; i < infer_context->arguments_count; i++) {
        bool other_argument_dependency = other_argument_dependencies[i];
        if (other_argument_dependency) argument_dependencies[i] = true;
    }
}

Function_Instance_Data* ssa_can_use_previous_function_context(Function_Instance_Data* instance_data, Function* function) {
    Function_Context* function_context = instance_data->function_context;
    u32 parameter_count = instance_data->function_context->parameters_count;
    for (u32 i = 0; i < function->instance_data_count; i++) {
        Function_Instance_Data* other_instance_data = &function->instance_data[i];
        Function_Context* other_function_context = other_instance_data->function_context;
        bool all_type_equal = true;
        for (u32 i = 0; i < parameter_count; i++) {
            SSA* argument = function_context->arguments[i];
            Type* argument_type = ssa_type(argument);
            assert(argument_type != NULL);
            SSA* other_argument = other_function_context->arguments[i];
            Type* other_argument_type = ssa_type(other_argument);
            assert(other_argument_type != NULL);
            if (!ssa_type_equal(argument_type, other_argument_type)) {
                all_type_equal = false;
                break;
            }
        }
        if (all_type_equal) {
            bool perfect_match = true;
            for (u32 i = 0; i < parameter_count; i++) {
                SSA* argument = function_context->arguments[i];
                SSA* other_argument = other_function_context->arguments[i];
                Type* argument_type = ssa_type(argument);

                void* argument_value = ssa_evaluate_speculative(argument);

                void* other_argument_value = NULL;
                All_Function_Context other_evaluation_context = {};
                other_evaluation_context.function_context = other_function_context;
                ssa_push_function_context(other_evaluation_context);
                bool res = ssa_already_evaluated(other_argument, &other_argument_value);
                ssa_pop_function_context();

                if (argument_value == NULL || other_argument_value == NULL) {
                    if (argument_value != NULL || other_argument_value != NULL) {
                        perfect_match = false;
                        break;
                    }
                } else if (!ssa_compile_time_value_equal(argument_value, other_argument_value, argument_type)) {
                    perfect_match = false;
                    break;
                }
            }
            if (perfect_match) return other_instance_data;
        }
    }
    return NULL;
}

bool ssa_infer_arguments(SSA** arguments, SSA** parameters, u32 arguments_count, SSA_Block* setup_block, All_Function_Context function_context,
                         SSA* setup_ssa) {
    Evaluate_Context* argument_side_context = context.evaluate_context;

    Evaluate_Context parameters_side_context_s_mem = {0};
    Evaluate_Context* parameters_side_context = &parameters_side_context_s_mem;
    parameters_side_context->function_context_stack_count = argument_side_context->function_context_stack_count;
    parameters_side_context->function_context_stack_capacity = parameters_side_context->function_context_stack_count + 1;
    parameters_side_context->function_context_stack = alloc(sizeof(All_Function_Context) * parameters_side_context->function_context_stack_capacity);
    memcpy(parameters_side_context->function_context_stack, argument_side_context->function_context_stack,
           sizeof(All_Function_Context) * parameters_side_context->function_context_stack_count);

    context.evaluate_context = parameters_side_context;
    ssa_push_function_context(function_context);
    context.evaluate_context = argument_side_context;

    Infer_Context infer_context_s_mem = {0};
    Infer_Context* infer_context = &infer_context_s_mem;
    infer_context->arguments = arguments;
    infer_context->arguments_count = arguments_count;
    infer_context->arg_side_evaluate_context = argument_side_context;
    infer_context->decl_side_evaluate_context = parameters_side_context;

    for (u32 i = 0; i < setup_block->statement_lists_count; i++) {
        SSA_List* list = setup_block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            bool* argument_dependencies = alloc(sizeof(bool) * arguments_count);
            SSA* ssa = list->statements + j;
            _ssa_infer_take_dependencies(argument_dependencies, ssa->type, setup_block, infer_context);
            switch (ssa->kind) {
                case SSA_Kind_Argument: {
                    u32 arg_index = ssa->data.argument.index;
                    assert(arg_index < arguments_count);
                    argument_dependencies[arg_index] = true;
                    break;
                }
                case SSA_Kind_Argument_Type: {
                    u32 arg_index = ssa->data.argument_type.index;
                    assert(arg_index < arguments_count);
                    argument_dependencies[arg_index] = true;
                    break;
                }
                case SSA_Kind_Store: {
                    SSA* address = ssa->data.store.address;
                    SSA* value = ssa->data.store.value;
                    _ssa_infer_take_dependencies(argument_dependencies, address, setup_block, infer_context);
                    _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Load: {
                    SSA* address = ssa->data.load.address;
                    _ssa_infer_take_dependencies(argument_dependencies, address, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Stack_Alloc: {
                    SSA* type = ssa->data.stack_alloc.type;
                    SSA* default_value = ssa->data.stack_alloc.initial_value;
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Return: {
                    SSA* value = ssa->data.return_.return_value;
                    _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Implicit_Cast: {
                    SSA* value = ssa->data.implicit_cast.value;
                    SSA* type = ssa->data.implicit_cast.type;
                    _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Explicit_Cast: {
                    SSA* value = ssa->data.explicit_cast.value;
                    SSA* type = ssa->data.explicit_cast.type;
                    _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Call_Setup: {
                    SSA* callee = ssa->data.call_setup.callee;
                    _ssa_infer_take_dependencies(argument_dependencies, callee, setup_block, infer_context);
                    for (u32 i = 0; i < ssa->data.call_setup.arguments_count; i++) {
                        SSA* argument = ssa->data.call_setup.arguments[i];
                        _ssa_infer_take_dependencies(argument_dependencies, argument, setup_block, infer_context);
                    }
                    break;
                }
                case SSA_Kind_Call: {
                    SSA* setup = ssa->data.call.setup;
                    _ssa_infer_take_dependencies(argument_dependencies, setup, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Call_Return_Type: {
                    SSA* setup = ssa->data.call_return_type.setup;
                    _ssa_infer_take_dependencies(argument_dependencies, setup, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Pointer_Type: {
                    SSA* type = ssa->data.pointer_type.type;
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Underlying_Type: {
                    SSA* type = ssa->data.underlying_type.type;
                    _ssa_infer_take_dependencies(argument_dependencies, type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Return_Value: {
                    for (u32 i = 0; i < ssa->data.return_value.values_count; i++) {
                        SSA* value = ssa->data.return_value.values[i];
                        _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    }
                    break;
                }
                case SSA_Kind_Return_Value_Type: {
                    for (u32 i = 0; i < ssa->data.return_value_type.types_count; i++) {
                        SSA* value = ssa->data.return_value_type.types[i];
                        _ssa_infer_take_dependencies(argument_dependencies, value, setup_block, infer_context);
                    }
                    break;
                }
                case SSA_Kind_Return_Value_Index: {
                    SSA* return_value = ssa->data.return_value_index.return_value;
                    _ssa_infer_take_dependencies(argument_dependencies, return_value, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Return_Value_Type_Index: {
                    SSA* return_value_type = ssa->data.return_value_type_index.return_value_type;
                    _ssa_infer_take_dependencies(argument_dependencies, return_value_type, setup_block, infer_context);
                    break;
                }
                case SSA_Kind_Default_Value:
                case SSA_Kind_Call_Setup_Type:
                case SSA_Kind_Int_Type:
                case SSA_Kind_Uint_Type:
                case SSA_Kind_Float_Type:
                case SSA_Kind_Void_Type:
                case SSA_Kind_Compile_To_LLVM_IR:
                case SSA_Kind_Float_Literal:
                case SSA_Kind_Int_Literal:
                case SSA_Kind_Type_Type:
                case SSA_Kind_Float_Literal_Type:
                case SSA_Kind_Int_Literal_Type:
                case SSA_Kind_Function_Type: {
                    break;
                }
                case SSA_Kind_Terminate_Global_Scope:
                case SSA_Kind_Build:
                case SSA_Kind_Return_Type:
                case SSA_Kind_Function_Declaration:
                case SSA_Kind_Parameter:
                case SSA_Kind_Parameter_Type:
                case SSA_Kind_Invalid: {
                    internal_compiler_error();
                }
            }

            ptr_append(infer_context->argument_dependencies, infer_context->argument_dependencies_count, infer_context->argument_dependencies_capacity,
                       argument_dependencies);
        }
    }

    bool type_check_res = true;
    bool changed = true;
    while (changed) {
        changed = false;
        u32 argument_dependencies_index = 0;
        context.evaluate_context = infer_context->decl_side_evaluate_context;
        for (u32 i = 0; i < setup_block->statement_lists_count; i++) {
            SSA_List* list = setup_block->statement_lists + i;
            for (u32 j = 0; j < list->statements_count; j++) {
                SSA* ssa = list->statements + j;
                bool* argument_dependencies = infer_context->argument_dependencies[argument_dependencies_index++];
                bool can_type_check = true;
                for (u32 k = 0; k < arguments_count; k++) {
                    bool dependent_on_argument = argument_dependencies[k];
                    SSA* argument = arguments[k];
                    if (dependent_on_argument && argument == NULL) {
                        can_type_check = false;
                        break;
                    }
                }
                if (!can_type_check) {
                    continue;
                }
                bool res = ssa_type_check(ssa);
                type_check_res = type_check_res && res;
            }
        }

        for (u32 i = 0; i < arguments_count; i++) {
            SSA* argument = arguments[i];
            if (argument == NULL) continue;
            SSA* argument_type = argument->type;
            SSA* parameter = parameters[i];
            SSA* parameter_type = parameter->type;
            bool conflict = false;
            bool infer_res = ssa_infer_trace_argument(infer_context, argument_type, parameter_type, &conflict);
            if (conflict) {
                context.evaluate_context = argument_side_context;
                return false;
            }
            if (infer_res) {
                changed = true;
                break;
            }
        }
    }

    bool infered_all_arguments = true;
    for (u32 i = 0; i < arguments_count; i++) {
        SSA* argument = arguments[i];
        if (argument == NULL) {
            log_msg_ssa("Could not infer all argument", log_error, setup_ssa);
            infered_all_arguments = false;
            break;
        }
    }

    context.evaluate_context = argument_side_context;
    return infered_all_arguments && type_check_res;
}

static void* _ssa_evaluate(SSA* ssa) {
    switch (ssa->kind) {
        case SSA_Kind_Invalid: {
            internal_compiler_error();
        }
        case SSA_Kind_Terminate_Global_Scope:
        case SSA_Kind_Build:
        case SSA_Kind_Return:
        case SSA_Kind_Store: {
            return SSA_SUCCESS_VOID_VALUE;
        }
        case SSA_Kind_Load: {
            SSA* address = ssa->data.load.address;
            if (ssa_running_interpreter()) {
                void* memory = ssa_evaluate(address);
                if (memory == NULL) return NULL;
                return *(void**)memory;
            }
            switch (address->kind) {
                case SSA_Kind_Stack_Alloc: {
                    SSA* lost_const_at = address->data.stack_alloc.lost_const_at;
                    if (lost_const_at != NULL) {
                        log_msg_ssa("Can't determine load at compile time", log_error, ssa);
                        log_msg_ssa("Cause was variable lost const at", log_info, lost_const_at);
                        return NULL;
                    }
                    SSA* initial_value = address->data.stack_alloc.initial_value;
                    return ssa_evaluate(initial_value);
                }
                default: {
                    break;
                }
            }
            log_msg_ssa("Can't determine load at compile time", log_error, ssa);
            return NULL;
        }
        case SSA_Kind_Stack_Alloc: {
            log_msg_ssa("Can't evaluate stack alloc at compile time", log_error, ssa);
            return NULL;
        }
        case SSA_Kind_Function_Type: {
            return context.intrinsic_ssa_block.function_type_value;
        }
        case SSA_Kind_Type_Type: {
            return context.intrinsic_ssa_block.type_type_value;
        }
        case SSA_Kind_Int_Literal_Type: {
            return context.intrinsic_ssa_block.int_literal_type_value;
        }
        case SSA_Kind_Float_Literal_Type: {
            return context.intrinsic_ssa_block.float_literal_type_value;
        }
        case SSA_Kind_Function_Declaration: {
            Function** func = alloc(sizeof(Function*));
            *func = &ssa->data.function_declaration.function;
            return func;
        }
        case SSA_Kind_Int_Type: {
            return context.intrinsic_ssa_block.int_type_value;
        }
        case SSA_Kind_Uint_Type: {
            return context.intrinsic_ssa_block.uint_type_value;
        }
        case SSA_Kind_Float_Type: {
            return context.intrinsic_ssa_block.float_type_value;
        }
        case SSA_Kind_Void_Type: {
            return context.intrinsic_ssa_block.void_type_value;
        }
        case SSA_Kind_Compile_To_LLVM_IR: {
            return context.intrinsic_ssa_block.compile_to_llvm_ir_value;
        }
        case SSA_Kind_Call_Setup_Type: {
            return context.intrinsic_ssa_block.call_setup_type_value;
        }
        case SSA_Kind_Int_Literal: {
            Big_Int* big_int = &ssa->data.int_literal.value;
            return big_int;
        }
        case SSA_Kind_Float_Literal: {
            f64* float_ = &ssa->data.float_literal.value;
            return float_;
        }
        case SSA_Kind_Parameter: {
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            u32 index = ssa->data.parameter.index;
            assert(index < function_context->parameters_count);
            SSA* parameter = function_context->parameters[index];
            void* parameter_value = ssa_evaluate(parameter);
            return parameter_value;
        }
        case SSA_Kind_Parameter_Type: {
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            u32 index = ssa->data.parameter_type.index;
            assert(index < function_context->parameters_count);
            SSA* parameter = function_context->parameters[index];
            Type* parameter_type = ssa_type(parameter);
            return parameter_type;
        }
        case SSA_Kind_Argument: {
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            ssa_pop_function_context();
            u32 index = ssa->data.argument.index;
            assert(index < function_context->parameters_count);
            SSA* argument = function_context->arguments[index];
            void* argument_value = ssa_evaluate(argument);
            ssa_push_function_context(all);
            return argument_value;
        }
        case SSA_Kind_Argument_Type: {
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            ssa_pop_function_context();
            u32 index = ssa->data.argument_type.index;
            assert(index < function_context->parameters_count);
            SSA* argument = function_context->arguments[index];
            Type* argument_type = ssa_type(argument);
            ssa_push_function_context(all);
            return argument_type;
        }
        case SSA_Kind_Return_Type: {
            All_Function_Context all = ssa_get_function_context();
            Function_Context* function_context = all.function_context;
            SSA* return_type = function_context->return_type;
            if (return_type == NULL) {
                log_msg_ssa("Can't evaluate return type for this scope", log_error, ssa);
                return NULL;
            }
            void* return_type_value = ssa_evaluate(return_type);
            return return_type_value;
        }
        case SSA_Kind_Implicit_Cast: {
            Type* cast_type = ssa_evaluate_type(ssa->data.implicit_cast.type);
            assert(cast_type != NULL);
            void* value = ssa_evaluate(ssa->data.implicit_cast.value);
            if (value == NULL) return NULL;
            Type* value_type = ssa_type(ssa->data.implicit_cast.value);
            assert(value_type != NULL);
            return ssa_cast_value(value, value_type, cast_type);
        }
        case SSA_Kind_Explicit_Cast: {
            Type* cast_type = ssa_evaluate_type(ssa->data.explicit_cast.type);
            assert(cast_type != NULL);
            void* value = ssa_evaluate(ssa->data.explicit_cast.value);
            if (value == NULL) return NULL;
            Type* value_type = ssa_type(ssa->data.explicit_cast.value);
            assert(value_type != NULL);
            return ssa_cast_value(value, value_type, cast_type);
        }
        case SSA_Kind_Call_Setup: {
            SSA* callee = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);
            assert(function != NULL);

            u32 parameters_count = function->parameter_count;
            SSA** arguments = alloc(sizeof(SSA*) * parameters_count);
            for (u32 i = 0; i < ssa->data.call_setup.arguments_count; i++) {
                SSA* argument = ssa->data.call_setup.arguments[i];
                arguments[i] = argument;
            }

            Function_Context* function_context = alloc(sizeof(Function_Context));
            function_context->arguments = arguments;
            function_context->parameters = function->parameters;
            function_context->parameters_count = parameters_count;
            function_context->return_type = function->return_type;

            All_Function_Context all = {0};
            all.function_context = function_context;

            bool res = ssa_infer_arguments(arguments, function->parameters, parameters_count, &function->setup_block, all, ssa);
            if (!res) return NULL;

            Function_Instance_Data instance_data = {0};
            instance_data.function_context = function_context;

            Function_Instance_Data* previous_instance_data = ssa_can_use_previous_function_context(&instance_data, function);
            if (previous_instance_data != NULL) return &previous_instance_data->function_context;

            ptr_append(function->instance_data, function->instance_data_count, function->instance_data_capacity, instance_data);
            Function_Instance_Data* instance_data_ptr = function->instance_data + function->instance_data_count - 1;
            return &instance_data_ptr->function_context;
        }
        case SSA_Kind_Call: {
            SSA* setup = ssa->data.call.setup;
            SSA* callee = setup->data.call.setup;
            Function_Context* function_context = ssa_evaluate_function_context(setup);
            if (!function_context->already_built) {
                log_msg_ssa("Can't call function before it is built(probably a circular dependency in what is being evaluated at compile time)", log_error,
                            ssa);
                return NULL;
            }
            assert(function_context != NULL);
            Function* function = ssa_evaluate_function(callee);
            assert(function != NULL);
            void* res = NULL;
            All_Function_Context all = {0};
            all.function_context = function_context;
            ssa_push_function_context(all);

            void** parameters = alloc(sizeof(void*) * function_context->parameters_count);
            for (u32 i = 0; i < function_context->parameters_count; i++) {
                SSA* parameter = function_context->parameters[i];
                void* parameter_value = ssa_evaluate(parameter);
                if (parameter_value == NULL) return NULL;
                parameters[i] = parameter_value;
            }

            switch (function->kind) {
                case Function_Kind_Invalid: {
                    internal_compiler_error();
                }
                case Function_Kind_Internal: {
                    SSA_Block* body = &function->data.internal.body;
                    res = ssa_run_block(body, parameters, ssa);
                    break;
                }
                case Function_Kind_Intrinsic: {
                    switch (function->data.intrinsic.kind) {
                        case Intrinsic_Function_Kind_Invalid: {
                            internal_compiler_error();
                        }
                        case Intrinsic_Function_Int_Type: {
                            SSA* bits_parameter = function_context->parameters[0];
                            void* bits_value = parameters[0];
                            Type* bits_parameter_type = ssa_type(bits_parameter);
                            assert(bits_parameter_type != NULL);

                            Type i64_type = {0};
                            i64_type.kind = Type_Kind_Int;
                            i64_type.data.int_.bits = 64;
                            i64 bits = *(i64*)ssa_cast_value(bits_value, bits_parameter_type, &i64_type);

                            Type* int_type = alloc(sizeof(Type));
                            int_type->kind = Type_Kind_Int;
                            int_type->data.int_.bits = bits;
                            res = int_type;
                            break;
                        }
                        case Intrinsic_Function_Uint_Type: {
                            SSA* bits_parameter = function_context->parameters[0];
                            void* bits_value = parameters[0];
                            assert(bits_value != NULL);
                            Type* bits_parameter_type = ssa_type(bits_parameter);
                            assert(bits_parameter_type != NULL);

                            Type i64_type = {0};
                            i64_type.kind = Type_Kind_Int;
                            i64_type.data.int_.bits = 64;
                            i64 bits = *(i64*)ssa_cast_value(bits_value, bits_parameter_type, &i64_type);

                            Type* uint_type = alloc(sizeof(Type));
                            uint_type->kind = Type_Kind_Uint;
                            uint_type->data.uint.bits = bits;
                            res = uint_type;
                            break;
                        }
                        case Intrinsic_Function_Float_Type: {
                            SSA* bits_parameter = function_context->parameters[0];
                            void* bits_value = parameters[0];
                            assert(bits_value != NULL);
                            Type* bits_parameter_type = ssa_type(bits_parameter);
                            assert(bits_parameter_type != NULL);

                            Type i64_type = {0};
                            i64_type.kind = Type_Kind_Int;
                            i64_type.data.int_.bits = 64;
                            i64 bits = *(i64*)ssa_cast_value(bits_value, bits_parameter_type, &i64_type);

                            Type* float_type = alloc(sizeof(Type));
                            float_type->kind = Type_Kind_Float;
                            float_type->data.float_.bits = bits;
                            res = float_type;
                            break;
                        }
                        case Intrinsic_Function_Kind_Compile_To_LLVM_IR: {
                            SSA* main_function_ssa = function_context->parameters[0];

                            Function* main_function = ssa_evaluate_function(main_function_ssa);

                            All_Function_Context all = {0};
                            Function_Context main_function_context = {0};
                            main_function_context.return_type = main_function->return_type;
                            assert(main_function->parameter_count == 0);
                            all.function_context = &main_function_context;
                            ssa_push_function_context(all);
                            SSA_Block* function_setup_block = &main_function->setup_block;

                            if (ssa_type_check_block(function_setup_block)) {
                                SSA_Block* body_block = &main_function->data.internal.body;
                                if (ssa_type_check_block(body_block)) {
                                    utf8 ir = llvm_gernerate_ir_exe(main_function);
                                    printf("%.*s\n", utf8_fmt(ir));
                                    llvm_generate_exe(ir, utf8_str("test.exe"));
                                } else {
                                    log_msg_ssa("Failed to type check body block", log_info, function_context->arguments[0]);
                                }
                            } else {
                                log_msg_ssa("Failed to type check function setup block", log_info, function_context->arguments[0]);
                            }
                            ssa_pop_function_context();
                            res = SSA_SUCCESS_VOID_VALUE;
                            break;
                        }
                    }
                }
            }
            ssa_pop_function_context();
            return res;
        }
        case SSA_Kind_Call_Return_Type: {
            SSA* call_setup = ssa->data.call_return_type.setup;
            Function_Context* function_context = ssa_evaluate_function_context(call_setup);
            assert(function_context != NULL);
            All_Function_Context all = {0};
            all.function_context = function_context;
            ssa_push_function_context(all);
            Type* return_type = ssa_evaluate_type(function_context->return_type);
            ssa_pop_function_context();
            return return_type;
        }
        case SSA_Kind_Pointer_Type: {
            Type* pointer_of_type = ssa_evaluate_type(ssa->data.pointer_type.type);
            assert(pointer_of_type != NULL);
            Type* type = alloc(sizeof(Type));
            type->kind = Type_Kind_Ptr;
            type->data.ptr.type = pointer_of_type;
            return type;
        }
        case SSA_Kind_Underlying_Type: {
            Type* ptr_type = ssa_evaluate_type(ssa->data.underlying_type.type);
            assert(ptr_type != NULL);
            assert(ptr_type->kind == Type_Kind_Ptr);
            return ptr_type->data.ptr.type;
        }
        case SSA_Kind_Default_Value: {
            assert(false);
        }
        case SSA_Kind_Return_Value_Type: {
            Type* return_value_type = alloc(sizeof(Type));
            return_value_type->kind = Type_Kind_Return_Value;
            u32 types_count = ssa->data.return_value_type.types_count;
            return_value_type->data.return_value.types_count = types_count;
            Type** return_types = alloc(sizeof(Type*) * types_count);
            return_value_type->data.return_value.types = return_types;
            for (u32 i = 0; i < types_count; i++) {
                SSA* return_type = ssa->data.return_value_type.types[i];
                Type* return_type_type = ssa_evaluate_type(return_type);
                if (return_type_type == NULL) return NULL;
                return_types[i] = return_type_type;
            }
            return return_value_type;
        }
        case SSA_Kind_Return_Value_Type_Index: {
            SSA* return_value_type = ssa->data.return_value_type_index.return_value_type;
            Type* return_value_type_type = ssa_evaluate_type(return_value_type);
            assert(return_value_type_type != NULL);
            assert(return_value_type_type->kind == Type_Kind_Return_Value);
            u32 index = ssa->data.return_value_type_index.index;
            assert(index <= return_value_type_type->data.return_value.types_count);
            return return_value_type_type->data.return_value.types[index];
        }
        case SSA_Kind_Return_Value: {
            Type* type = ssa_type(ssa);
            if (type == NULL) return NULL;
            i64 type_size = ssa_type_size(type);
            void* value = alloc(type_size);
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                SSA* return_value_ssa = ssa->data.return_value.values[i];
                Type* return_type = type->data.return_value.types[i];
                void* return_value = ssa_evaluate(return_value_ssa);
                if (return_value == NULL) return NULL;
                i64 value_type_size = ssa_type_size(return_type);
                i64 offset_of_value = ssa_return_value_type_index_to_offset_compile_time(type, i);
                memcpy(((char*)value) + offset_of_value, return_value, value_type_size);
            }
            return value;
        }
        case SSA_Kind_Return_Value_Index: {
            Type* type = ssa_type(ssa->data.return_value_index.return_value);
            if (type == NULL) return NULL;
            u32 index = ssa->data.return_value_index.index;
            i64 offset_of_value = ssa_return_value_type_index_to_offset_compile_time(type, index);
            SSA* return_value_ssa = ssa->data.return_value_index.return_value;
            void* return_value = ssa_evaluate(return_value_ssa);
            if (return_value == NULL) return NULL;
            void* value_index = ((char*)return_value) + offset_of_value;
            return value_index;
        }
    }
    return NULL;
}

void* ssa_evaluate_interpreter_run(SSA* ssa) {
    bool type_check_res;
    assert(ssa_already_type_checked(ssa, &type_check_res));
    assert(type_check_res);
    void* res = NULL;
    if (ssa_already_evaluated(ssa, &res)) return res;
    res = _ssa_evaluate(ssa);
    if (res != NULL) {
        log_msg_ssa("Evaluation succeeded", log_debug, ssa);
    } else {
        log_msg_ssa("Evaluation failed", log_info, ssa);
    }
    ssa_set_interpreter_paired_value(ssa, res);
    return res;
}

void* ssa_evaluate_speculative(SSA* ssa) {
    bool prev_speculative = context.evaluate_context->speculative;
    context.evaluate_context->speculative = true;
    void* res = ssa_evaluate(ssa);
    context.evaluate_context->speculative = prev_speculative;
    return res;
}

void* ssa_evaluate(SSA* ssa) {
    bool speculative = context.evaluate_context->speculative;
    u32 start_log_location = log_end_location();

    bool type_check_res;
    if (!ssa_already_type_checked(ssa, &type_check_res)) internal_compiler_error();
    if (!type_check_res) return NULL;

    void* res = NULL;
    if (ssa_already_evaluated(ssa, &res)) return res;

    if (ssa_running_interpreter()) {
        return ssa_evaluate_interpreter_get_paired_value(ssa);
    }

    res = _ssa_evaluate(ssa);
    if (res != NULL) {
        log_msg_ssa("Evaluation succeeded", log_debug, ssa);
    } else {
        log_msg_ssa("Evaluation failed", log_info, ssa);
    }

    bool should_cache = !ssa_running_interpreter();
    should_cache = should_cache && (speculative == false || res != NULL);
    if (should_cache) {
        ssa_cache_evaluated(ssa, res);
    }
    if (speculative && res == NULL) {
        log_clear_after(start_log_location);
    }
    return res;
}

void* ssa_evaluate_interpreter_get_paired_value(SSA* ssa) {
    assert(ssa_running_interpreter());
    Interpreter_Function_Context* inter_function_context = ssa_get_cache_function_context(ssa).interpreter_function_context;
    for (u32 i = 0; i < inter_function_context->ssa_value_pairs_count; i++) {
        Interpreter_SSA_Value_Pair* pair = inter_function_context->ssa_value_pairs + i;
        if (pair->ssa == ssa) {
            return pair->value;
        }
    }
    internal_compiler_error();
}

void ssa_set_interpreter_paired_value(SSA* ssa, void* value) {
    assert(ssa_running_interpreter());
    Interpreter_Function_Context* inter_function_context = ssa_get_cache_function_context(ssa).interpreter_function_context;
    for (u32 i = 0; i < inter_function_context->ssa_value_pairs_count; i++) {
        Interpreter_SSA_Value_Pair* pair = inter_function_context->ssa_value_pairs + i;
        if (pair->ssa == ssa) {
            pair->value = value;
            return;
        }
    }
    Interpreter_SSA_Value_Pair new_pair = {ssa, value};
    ptr_append(inter_function_context->ssa_value_pairs, inter_function_context->ssa_value_pairs_count, inter_function_context->ssa_value_pairs_capacity,
               new_pair);
}

bool ssa_already_evaluated(SSA* ssa, void** out_value) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            *out_value = per_function_context_values->value;
            return per_function_context_values->evaluated_value;
        }
    }
    return false;
}

void ssa_cache_evaluated(SSA* ssa, void* value) {
    Function_Context* function_context = ssa_get_cache_function_context(ssa).function_context;
    for (u32 i = 0; i < ssa->ssa_per_function_context_values_count; i++) {
        SSA_Per_Function_Context_Values* per_function_context_values = ssa->ssa_per_function_context_values + i;
        if (per_function_context_values->function_context == function_context) {
            per_function_context_values->value = value;
            per_function_context_values->evaluated_value = true;
            return;
        }
    }
    internal_compiler_error();  // expect that per function context values are init because of type check
}

void ssa_run_block_setup(SSA_Block* block) {
    u32 stack_alloc_count = 0;
    SSA** ssa_stack_alloc = ssa_get_all_ssa_of_kind(block, SSA_Kind_Stack_Alloc, &stack_alloc_count);
    for (u32 i = 0; i < stack_alloc_count; i++) {
        SSA* ssa = ssa_stack_alloc[i];
        SSA* type_ssa = ssa->data.stack_alloc.type;
        Type* type = ssa_evaluate_type(type_ssa);
        i64 size = ssa_type_size_compile_time(type);
        void* mem = alloc(size);
        void* mem_ptr = alloc(sizeof(void*));
        *(void**)mem_ptr = mem;
        ssa_set_interpreter_paired_value(ssa, mem_ptr);
    }
}

static void* _ssa_run(SSA* ssa) {
    switch (ssa->kind) {
        case SSA_Kind_Invalid: {
            internal_compiler_error();
        }
        case SSA_Kind_Store: {
            SSA* address = ssa->data.store.address;
            void* memory = ssa_evaluate(address);
            if (memory == NULL) return NULL;
            Type* value_type = ssa_type(ssa->data.store.value);
            if (value_type == NULL) return NULL;
            i64 value_size = ssa_type_size_compile_time(value_type);
            void* value = ssa_evaluate(ssa->data.store.value);
            if (value == NULL) return NULL;
            memcpy(*(void**)memory, value, value_size);
            break;
        }
        case SSA_Kind_Stack_Alloc: {
            void* memory = ssa_evaluate(ssa);
            if (memory == NULL) return NULL;
            Type* type = ssa_evaluate_type(ssa->data.stack_alloc.type);
            if (type == NULL) return NULL;
            i64 size = ssa_type_size_compile_time(type);
            void* value = ssa_evaluate(ssa->data.stack_alloc.initial_value);
            if (value == NULL) return NULL;
            memcpy(*(void**)memory, value, size);
            break;
        }
        case SSA_Kind_Return: {
            SSA* return_value = ssa->data.return_.return_value;
            void* value = ssa_evaluate_interpreter_run(return_value);
            return value;
        }
        case SSA_Kind_Return_Value:
        case SSA_Kind_Return_Value_Type:
        case SSA_Kind_Return_Value_Index:
        case SSA_Kind_Return_Value_Type_Index:
        case SSA_Kind_Load:
        case SSA_Kind_Function_Type:
        case SSA_Kind_Type_Type:
        case SSA_Kind_Function_Declaration:
        case SSA_Kind_Parameter:
        case SSA_Kind_Parameter_Type:
        case SSA_Kind_Argument:
        case SSA_Kind_Argument_Type:
        case SSA_Kind_Int_Literal_Type:
        case SSA_Kind_Int_Literal:
        case SSA_Kind_Float_Literal_Type:
        case SSA_Kind_Float_Literal:
        case SSA_Kind_Return_Type:
        case SSA_Kind_Implicit_Cast:
        case SSA_Kind_Explicit_Cast:
        case SSA_Kind_Int_Type:
        case SSA_Kind_Uint_Type:
        case SSA_Kind_Float_Type:
        case SSA_Kind_Void_Type:
        case SSA_Kind_Compile_To_LLVM_IR:
        case SSA_Kind_Build:
        case SSA_Kind_Call_Setup:
        case SSA_Kind_Call_Setup_Type:
        case SSA_Kind_Call:
        case SSA_Kind_Call_Return_Type:
        case SSA_Kind_Pointer_Type:
        case SSA_Kind_Underlying_Type:
        case SSA_Kind_Terminate_Global_Scope:
        case SSA_Kind_Default_Value: {
            void* value = ssa_evaluate_interpreter_run(ssa);
            if (value == NULL) return NULL;
            break;
        }
    }
    return SSA_SUCCESS_VOID_VALUE;
}

void* ssa_run(SSA* ssa) {
    void* res = _ssa_run(ssa);
    if (res == NULL) {
        log_msg_ssa("Failed to run", log_info, ssa);
    } else {
        log_msg_ssa("Successfully ran", log_debug, ssa);
    }
    return res;
}

void* ssa_run_block_body(SSA_Block* block) {
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            void* res = _ssa_run(ssa);
            if (res != SSA_SUCCESS_VOID_VALUE) return res;
        }
    }
    return SSA_SUCCESS_VOID_VALUE;
}

void* ssa_run_block(SSA_Block* block, void** parameters, SSA* log_ssa) {
    ssa_add_interpreter_to_function_context();
    All_Function_Context all_function_context = ssa_get_function_context();
    Function_Context* function_context = all_function_context.function_context;
    Interpreter_Function_Context* inter_function_context = all_function_context.interpreter_function_context;
    for (u32 i = 0; i < function_context->parameters_count; i++) {
        SSA* parameter = function_context->parameters[i];
        void* parameter_value = parameters[i];
        ssa_set_interpreter_paired_value(parameter, parameter_value);
    }
    ssa_run_block_setup(block);
    void* res = ssa_run_block_body(block);
    if (res == NULL) {
        log_msg_ssa("Failed to run block", log_info, log_ssa);
    } else {
        log_msg_ssa("Successfully ran block", log_debug, log_ssa);
    }
    ssa_remove_interpreter_from_function_context();
    return res;
}

bool ssa_run_builds() {
    for (u32 i = 0; i < context.global_block.statement_lists_count; i++) {
        SSA_List* list = context.global_block.statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            if (ssa->kind == SSA_Kind_Build) {
                SSA_Build* build = &ssa->data.build;
                Function_Context* function_context = build->function_context;
                All_Function_Context all = {0};
                all.function_context = function_context;
                ssa_push_function_context(all);
                void* res = ssa_run_block(&build->block, NULL, ssa);
                ssa_pop_function_context();
                if (res == NULL) return false;
            }
        }
    }
    return true;
}

SSA* ssa_function_declaration_ast_prototype(Ast* ast, SSA_Block* block) {
    SSA* function_type = ssa_function_type();
    SSA* function_declaration = ssa_function_declaration(block, ast);
    SSA* function_memory = ssa_stack_alloc(function_type, function_declaration, block, ast);
    ast->data.function_declaration.value = function_memory;

    Function* function = &function_declaration->data.function_declaration.function;

    function->kind = Function_Kind_Internal;
    function->name = ast->data.function_declaration.name;
    function->setup_block.kind = SSA_Block_Kind_Function_Setup;

    Ast* parameter_list = ast->data.function_declaration.parameter_list;
    u32 parameter_count = parameter_list->data.parameter_list.parameters_count;

    SSA** parameters = alloc(sizeof(SSA*) * parameter_count);
    for (u32 i = 0; i < parameter_count; i++) {
        Ast* parameter = parameter_list->data.parameter_list.parameter_semantic_parse_order[i];
        u32 parameter_index = parameter - parameter_list->data.parameter_list.parameters;
        assert(parameter->kind == Ast_Kind_Parameter);
        Ast* type = parameter->data.parameter.type;
        SSA* type_ssa = ssa_ast_to_ssa(type, &function->setup_block);
        SSA* argument = ssa_argument(parameter_index, &function->setup_block, parameter);
        SSA* casted_argument = ssa_implicit_cast(argument, type_ssa, &function->setup_block, parameter);
        parameter->data.parameter.value = casted_argument;
        parameters[parameter_index] = casted_argument;
    }
    function->parameters = parameters;
    function->parameter_count = parameter_count;

    u32 return_types_count = ast->data.function_declaration.return_types_count;
    SSA** return_types = alloc(sizeof(SSA*) * return_types_count);
    for (u32 i = 0; i < return_types_count; i++) {
        Ast* return_type = ast->data.function_declaration.return_types + i;
        SSA* return_type_ssa = ssa_ast_to_ssa(return_type, &function->setup_block);
        return_types[i] = return_type_ssa;
    }
    SSA* return_type = NULL;
    if (return_types_count == 1) {
        return_type = return_types[0];
    } else {
        return_type = ssa_return_value_type(return_types, return_types_count, &function->setup_block, NULL);
    }
    function->return_type = return_type;
    return function_memory;
}

void ssa_function_declaration_ast_implement(SSA* ssa, SSA_Block* block) {
    Ast* ast = ssa->ast;
    assert(ast->kind == Ast_Kind_Function_Declaration);
    Ast* scope = ast->data.function_declaration.body;
    Function* function = &ssa->data.function_declaration.function;
    SSA_Block* body_block = &function->data.internal.body;
    body_block->kind = SSA_Block_Kind_Function;

    Ast* parameter_list = ast->data.function_declaration.parameter_list;
    u32 parameter_types_count = parameter_list->data.parameter_list.parameters_count;
    for (u32 i = 0; i < parameter_types_count; i++) {
        Ast* parameter = parameter_list->data.parameter_list.parameters + i;
        assert(parameter->kind == Ast_Kind_Parameter);
        SSA* parameter_ssa = ssa_parameter(i, body_block, parameter);
        parameter->data.parameter.value = ssa_stack_alloc(parameter_ssa->type, parameter_ssa, body_block, parameter);
    }

    ssa_build_scope(scope, body_block);
}

SSA* ssa_build_ast_prototype(Ast* ast, SSA_Block* block) {
    return ssa_build(block, ast);
}

void ssa_build_ast_implement(SSA* ssa, SSA_Block* block) {
    Ast* ast = ssa->ast;
    assert(ast->kind == Ast_Kind_Build);
    Ast* scope = ast->data.build.scope;
    SSA_Block* build_body = &ssa->data.build.block;
    build_body->kind = SSA_Block_Kind_Function;

    Function_Context* function_context = alloc(sizeof(Function_Context));
    ssa->data.build.function_context = function_context;

    All_Function_Context all = {0};
    all.function_context = function_context;
    ssa_push_function_context(all);
    ssa_build_scope(scope, build_body);
    ssa_pop_function_context();
}

SSA* ssa_stack_alloc(SSA* type, SSA* initial_value, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Stack_Alloc;
    ssa.ast = ast;
    ssa.data.stack_alloc.type = type;
    ssa.data.stack_alloc.initial_value = initial_value;
    ssa.type = ssa_pointer_type(type, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_store(SSA* value, SSA* address, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Store;
    ssa.ast = ast;
    ssa.data.store.value = value;
    ssa.data.store.address = address;
    ssa.type = ssa_void_type();
    SSA* ssa_ptr = ssa_add_to_block(ssa, block);
    if (address->kind == SSA_Kind_Stack_Alloc && address->data.stack_alloc.lost_const_at == NULL) address->data.stack_alloc.lost_const_at = ssa_ptr;
    return ssa_ptr;
}

SSA* ssa_load(SSA* address, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Load;
    ssa.ast = ast;
    ssa.data.load.address = address;
    ssa.type = ssa_underlying_type(address->type, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_argument(u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Argument;
    ssa.ast = ast;
    ssa.data.argument.index = index;
    ssa.type = ssa_argument_type(index, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_parameter(u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Parameter;
    ssa.ast = ast;
    ssa.data.parameter.index = index;
    ssa.type = ssa_parameter_type(index, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_function_declaration(SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Function_Declaration;
    ssa.ast = ast;
    ssa.type = ssa_function_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_int_literal(Big_Int value, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Int_Literal;
    ssa.ast = ast;
    ssa.data.int_literal.value = value;
    ssa.type = ssa_int_literal_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_float_literal(f64 value, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Float_Literal;
    ssa.ast = ast;
    ssa.data.float_literal.value = value;
    ssa.type = ssa_float_literal_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return(SSA* return_value, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return;
    ssa.ast = ast;
    ssa.data.return_.return_value = return_value;
    ssa.type = ssa_void_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return_type(SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return_Type;
    ssa.ast = ast;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_implicit_cast(SSA* value, SSA* type, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Implicit_Cast;
    ssa.ast = ast;
    ssa.data.implicit_cast.value = value;
    ssa.data.implicit_cast.type = type;
    ssa.type = type;
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_explicit_cast(SSA* value, SSA* type, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Explicit_Cast;
    ssa.ast = ast;
    ssa.data.explicit_cast.value = value;
    ssa.data.explicit_cast.type = type;
    ssa.type = type;
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_build(SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Build;
    ssa.ast = ast;
    ssa.type = ssa_void_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_call_setup(SSA* callee, SSA** arguments, u32 arguments_count, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Call_Setup;
    ssa.ast = ast;
    ssa.data.call_setup.callee = callee;
    ssa.data.call_setup.arguments = arguments;
    ssa.data.call_setup.arguments_count = arguments_count;
    ssa.type = ssa_call_setup_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_call(SSA* setup, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Call;
    ssa.ast = ast;
    ssa.data.call.setup = setup;
    ssa.type = ssa_call_return_type(setup, block, ast);

    return ssa_add_to_block(ssa, block);
}

SSA* ssa_call_return_type(SSA* setup, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Call_Return_Type;
    ssa.ast = ast;
    ssa.data.call_return_type.setup = setup;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_pointer_type(SSA* type, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Pointer_Type;
    ssa.ast = ast;
    ssa.data.pointer_type.type = type;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_underlying_type(SSA* type, SSA_Block* block, Ast* ast) {
    if (type->kind == SSA_Kind_Pointer_Type) {
        return type->data.pointer_type.type;
    }
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Underlying_Type;
    ssa.ast = ast;
    ssa.data.underlying_type.type = type;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_argument_type(u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Argument_Type;
    ssa.ast = ast;
    ssa.data.argument_type.index = index;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_parameter_type(u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Parameter_Type;
    ssa.ast = ast;
    ssa.data.parameter_type.index = index;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_default_value(SSA* type, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Default_Value;
    ssa.ast = ast;
    ssa.data.default_value.type = type;
    ssa.type = type;
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return_value(SSA** values, u32 values_count, SSA_Block* block, Ast* ast) {
    assert(values_count > 1);
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return_Value;
    ssa.ast = ast;
    ssa.data.return_value.values = values;
    ssa.data.return_value.values_count = values_count;

    SSA** types = alloc(sizeof(SSA*) * values_count);
    for (u32 i = 0; i < values_count; i++) {
        SSA* value = values[i];
        types[i] = value->type;
    }
    ssa.type = ssa_return_value_type(types, values_count, block, ast);

    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return_value_type(SSA** types, u32 types_count, SSA_Block* block, Ast* ast) {
    assert(types_count > 1);
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return_Value_Type;
    ssa.ast = ast;
    ssa.data.return_value_type.types = types;
    ssa.data.return_value_type.types_count = types_count;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return_value_index(SSA* return_value, u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return_Value_Index;
    ssa.ast = ast;
    ssa.data.return_value_index.return_value = return_value;
    ssa.data.return_value_index.index = index;
    ssa.type = ssa_return_value_type_index(return_value->type, index, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_return_value_type_index(SSA* return_value_type, u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Return_Value_Type_Index;
    ssa.ast = ast;
    ssa.data.return_value_type_index.return_value_type = return_value_type;
    ssa.data.return_value_type_index.index = index;
    ssa.type = ssa_type_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_terminate_global_scope(SSA_Block* block) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Terminate_Global_Scope;
    ssa.type = ssa_void_type();
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_function_type() {
    return context.intrinsic_ssa_block.function_type;
}

SSA* ssa_type_type() {
    return context.intrinsic_ssa_block.type_type;
}

SSA* ssa_int_literal_type() {
    return context.intrinsic_ssa_block.int_literal_type;
}

SSA* ssa_float_literal_type() {
    return context.intrinsic_ssa_block.float_literal_type;
}

SSA* ssa_void_type() {
    return context.intrinsic_ssa_block.void_type;
}

SSA* ssa_int_type() {
    return context.intrinsic_ssa_block.int_type;
}

SSA* ssa_uint_type() {
    return context.intrinsic_ssa_block.uint_type;
}

SSA* ssa_float_type() {
    return context.intrinsic_ssa_block.float_type;
}

SSA* ssa_compile_to_llvm_ir() {
    return context.intrinsic_ssa_block.compile_to_llvm_ir;
}

SSA* ssa_call_setup_type() {
    return context.intrinsic_ssa_block.call_setup_type;
}

void ssa_build_scope(Ast* ast, SSA_Block* block) {
    assert(ast->kind == Ast_Kind_Scope);
    for (u32 i = 0; i < ast->data.scope.ast_count; i++) {
        Ast* statement = ast->data.scope.statements + i;
        ssa_ast_to_ssa(statement, block);
    }
}

utf8 ssa_get_ssa_block_name(SSA_Block* block) {
    typedef struct SSA_Block_Name_Ptr_Pair {
        SSA_Block* block;
        utf8 name;
    } SSA_Block_Name_Ptr_Pair;

    static SSA_Block_Name_Ptr_Pair* block_name_ptr_pairs = NULL;
    static u32 block_name_ptr_pairs_capacity = 0;
    static u32 block_name_ptr_pairs_count = 0;

    for (u32 i = 0; i < block_name_ptr_pairs_count; i++) {
        SSA_Block_Name_Ptr_Pair* pair = block_name_ptr_pairs + i;
        if (pair->block == block) {
            return pair->name;
        }
    }
    static u64 number_counter = 0;

    u64 seed = number_counter + 989687798 * (number_counter + 89897);
    seed = (seed ^ (seed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    seed = (seed ^ (seed >> 27)) * 0x94d049bb133111ebULL;
    seed = seed ^ (seed >> 31);
    uint8_t r = (uint8_t)((seed & 0xFF) % 156) + 100;
    uint8_t g = (uint8_t)(((seed >> 8) & 0xFF) % 156) + 100;
    uint8_t b = (uint8_t)(((seed >> 16) & 0xFF) % 156) + 100;

    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "\x1b[38;2;%d;%d;%dmBlock%llu\033[0m", r, g, b, number_counter++);
    u32 name_length = strlen(buffer);
    char* name = alloc(name_length + 1);
    memcpy(name, buffer, name_length + 1);

    utf8 name_utf8 = {0};
    name_utf8.data = name;
    name_utf8.count = name_length;

    SSA_Block_Name_Ptr_Pair pair = {block, name_utf8};
    ptr_append(block_name_ptr_pairs, block_name_ptr_pairs_count, block_name_ptr_pairs_capacity, pair);
    return name_utf8;
}

utf8 ssa_get_ssa_name(SSA* ssa) {
    // special cases
    switch (ssa->kind) {
        case SSA_Kind_Invalid:
            return utf8_str("\x1b[94mInvalid\033[0m");
        case SSA_Kind_Function_Type:
            return utf8_str("\x1b[94mFunction_Type\033[0m");
        case SSA_Kind_Type_Type:
            return utf8_str("\x1b[94mType_Type\033[0m");
        case SSA_Kind_Int_Literal_Type:
            return utf8_str("\x1b[94mInt_Literal_Type\033[0m");
        case SSA_Kind_Float_Literal_Type:
            return utf8_str("\x1b[94mFloat_Literal_Type\033[0m");
        case SSA_Kind_Int_Type:
            return utf8_str("\x1b[94mInt_Type\033[0m");
        case SSA_Kind_Uint_Type:
            return utf8_str("\x1b[94mUint_Type\033[0m");
        case SSA_Kind_Float_Type:
            return utf8_str("\x1b[94mFloat_Type\033[0m");
        case SSA_Kind_Void_Type:
            return utf8_str("\x1b[94mVoid_Type\033[0m");
        case SSA_Kind_Compile_To_LLVM_IR:
            return utf8_str("\x1b[94mCompile_To_LLVM_IR\033[0m");
        case SSA_Kind_Call_Setup_Type:
            return utf8_str("\x1b[94mCall_Setup_Type\033[0m");
        default:
            break;
    }

    typedef struct SSA_Name_Ptr_Pair {
        SSA* ssa;
        utf8 name;
    } SSA_Name_Ptr_Pair;

    static SSA_Name_Ptr_Pair* name_ptr_pairs = NULL;
    static u32 name_ptr_pairs_capacity = 0;
    static u32 name_ptr_pairs_count = 0;

    for (u32 i = 0; i < name_ptr_pairs_count; i++) {
        SSA_Name_Ptr_Pair* pair = name_ptr_pairs + i;
        if (pair->ssa == ssa) {
            return pair->name;
        }
    }
    static u64 number_counter = 0;

    u64 seed = number_counter + 223218 * (number_counter + 2876779);
    seed = (seed ^ (seed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    seed = (seed ^ (seed >> 27)) * 0x94d049bb133111ebULL;
    seed = seed ^ (seed >> 31);
    uint8_t r = (uint8_t)((seed & 0xFF) % 156) + 100;
    uint8_t g = (uint8_t)(((seed >> 8) & 0xFF) % 156) + 100;
    uint8_t b = (uint8_t)(((seed >> 16) & 0xFF) % 156) + 100;

    char buffer[4096];
    snprintf(buffer, sizeof(buffer), "\x1b[38;2;%d;%d;%dmSSA%llu\033[0m", r, g, b, number_counter++);
    u32 name_length = strlen(buffer);
    char* name = alloc(name_length + 1);
    memcpy(name, buffer, name_length + 1);

    utf8 name_utf8 = {0};
    name_utf8.data = name;
    name_utf8.count = name_length;

    SSA_Name_Ptr_Pair pair = {ssa, name_utf8};
    ptr_append(name_ptr_pairs, name_ptr_pairs_count, name_ptr_pairs_capacity, pair);
    return name_utf8;
}

static void _ssa_add_block_if_not_already_added(SSA_Block** blocks, u32* blocks_count_out, u32* block_capacity_out, SSA_Block* block) {
    u32 block_count = *blocks_count_out;
    u32 block_capacity = *block_capacity_out;
    bool already_added = false;
    for (u32 k = 0; k < *blocks_count_out; k++) {
        SSA_Block* block_in = blocks[k];
        if (block_in == block) {
            already_added = true;
            break;
        }
    }
    if (!already_added) {
        ptr_append(blocks, block_count, block_capacity, block);
    }
    *blocks_count_out = block_count;
    *block_capacity_out = block_capacity;
}

static void _ssa_recursive_get_blocks(SSA_Block** blocks, u32* blocks_count_out, u32* block_capacity_out, SSA_Block* block) {
    u32 block_count = *blocks_count_out;
    u32 block_capacity = *block_capacity_out;

    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            switch (ssa->kind) {
                case SSA_Kind_Build: {
                    SSA_Block* build_block = &ssa->data.build.block;
                    _ssa_add_block_if_not_already_added(blocks, &block_count, &block_capacity, build_block);
                    break;
                }
                case SSA_Kind_Function_Declaration: {
                    SSA_Block* setup_block = &ssa->data.function_declaration.function.setup_block;
                    SSA_Block* function_body_block = &ssa->data.function_declaration.function.data.internal.body;
                    _ssa_add_block_if_not_already_added(blocks, &block_count, &block_capacity, setup_block);
                    _ssa_add_block_if_not_already_added(blocks, &block_count, &block_capacity, function_body_block);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
    *blocks_count_out = block_count;
    *block_capacity_out = block_capacity;
}

utf8 ssa_recursive_get_block_strings(SSA_Block* block) {
    u32 block_count = 1;
    u32 block_capacity = 8;
    SSA_Block** blocks = alloc(sizeof(SSA_Block*) * 8);

    u32 block_strings_count = 0;
    u32 block_strings_capacity = block_capacity;
    utf8* block_strings = alloc(sizeof(utf8) * block_capacity);

    blocks[0] = block;
    for (u32 i = 0; i < block_count; i++) {
        SSA_Block* block = blocks[i];
        utf8 block_string = ssa_block_to_string(block);
        ptr_append(block_strings, block_strings_count, block_strings_capacity, block_string);
        _ssa_recursive_get_blocks(blocks, &block_count, &block_capacity, block);
    }

    u32 new_utf8_count = 0;
    for (u32 i = 0; i < block_strings_count; i++) {
        utf8 block_string = block_strings[i];
        new_utf8_count += block_string.count + 1;
    }

    char* new_utf8_memory = alloc(new_utf8_count);

    utf8_builder builder = {0};
    builder.str.data = new_utf8_memory;
    builder.capacity = new_utf8_count;
    builder.str.count = 0;

    for (u32 i = 0; i < block_strings_count; i++) {
        utf8_builder_append(&builder, block_strings[i]);
        utf8_builder_append(&builder, utf8_str("\n"));
    }
    return builder.str;
}

utf8 ssa_block_to_string(SSA_Block* block) {
    utf8_builder builder = {0};

    utf8 block_name = ssa_get_ssa_block_name(block);
    utf8_builder_append(&builder, block_name);
    utf8_builder_append(&builder, utf8_str(":\n"));

    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            utf8 ssa_name = ssa_get_ssa_name(ssa);
            utf8 ssa_type_name = ssa_get_ssa_name(ssa->type);
            utf8_builder_append(&builder, ssa_type_name);

            u32 type_padding = 18;
            u32 current_padding = utf8_visual_len(ssa_type_name);
            for (u32 k = current_padding; k < type_padding; k++) {
                utf8_builder_append(&builder, utf8_str(" "));
            }

            utf8_builder_append(&builder, ssa_name);
            utf8_builder_append(&builder, utf8_str(": "));
            switch (ssa->kind) {
                case SSA_Kind_Invalid: {
                    utf8_builder_append(&builder, utf8_str("Invalid"));
                    break;
                }
                case SSA_Kind_Store: {
                    SSA* value = ssa->data.store.value;
                    utf8 value_name = ssa_get_ssa_name(value);
                    SSA* address = ssa->data.store.address;
                    utf8 address_name = ssa_get_ssa_name(address);
                    utf8_builder_append(&builder, utf8_str("Store: (Value: "));
                    utf8_builder_append(&builder, value_name);
                    utf8_builder_append(&builder, utf8_str(", Memory: "));
                    utf8_builder_append(&builder, address_name);
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Load: {
                    SSA* address = ssa->data.load.address;
                    utf8 address_name = ssa_get_ssa_name(address);
                    utf8_builder_append(&builder, utf8_str("Load: "));
                    utf8_builder_append(&builder, address_name);
                    break;
                }
                case SSA_Kind_Stack_Alloc: {
                    SSA* type = ssa->data.stack_alloc.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    SSA* initial_value = ssa->data.stack_alloc.initial_value;
                    utf8 initial_value_name = ssa_get_ssa_name(initial_value);
                    utf8_builder_append(&builder, utf8_str("Stack_Alloc(Type: "));
                    utf8_builder_append(&builder, type_name);
                    utf8_builder_append(&builder, utf8_str(", Initial_Value: "));
                    utf8_builder_append(&builder, initial_value_name);
                    if (ssa->data.stack_alloc.lost_const_at != NULL) {
                        SSA* lost_const_at = ssa->data.stack_alloc.lost_const_at;
                        utf8 lost_const_at_name = ssa_get_ssa_name(lost_const_at);
                        utf8_builder_append(&builder, utf8_str(", Lost_Const_At: "));
                        utf8_builder_append(&builder, lost_const_at_name);
                    }
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Function_Type: {
                    utf8_builder_append(&builder, utf8_str("Function_Type"));
                    break;
                }
                case SSA_Kind_Type_Type: {
                    utf8_builder_append(&builder, utf8_str("Type_Type"));
                    break;
                }
                case SSA_Kind_Function_Declaration: {
                    utf8 function_name = ssa->data.function_declaration.function.name;

                    SSA_Block* setup_block = &ssa->data.function_declaration.function.setup_block;
                    utf8 setup_block_name = ssa_get_ssa_block_name(setup_block);

                    SSA_Block* function_body_block = &ssa->data.function_declaration.function.data.internal.body;
                    utf8 function_body_block_name = ssa_get_ssa_block_name(function_body_block);

                    utf8_builder_append(&builder, utf8_str("Function_Declaration "));
                    utf8_builder_append(&builder, function_name);
                    utf8_builder_append(&builder, utf8_str(": (Setup: "));
                    utf8_builder_append(&builder, setup_block_name);
                    utf8_builder_append(&builder, utf8_str(", Body: "));
                    utf8_builder_append(&builder, function_body_block_name);
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Parameter: {
                    u32 index = ssa->data.parameter.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Parameter: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Argument: {
                    u32 index = ssa->data.argument.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Argument: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Int_Literal_Type: {
                    utf8_builder_append(&builder, utf8_str("Int_Literal_Type"));
                    break;
                }
                case SSA_Kind_Int_Literal: {
                    Big_Int value = ssa->data.int_literal.value;
                    char value_buf[32];
                    snprintf(value_buf, 32, "%llu", value.data);
                    utf8 value_utf8 = {value_buf, strlen(value_buf)};
                    utf8_builder_append(&builder, utf8_str("Int_Literal: "));
                    utf8_builder_append(&builder, value_utf8);
                    break;
                }
                case SSA_Kind_Float_Literal_Type: {
                    utf8_builder_append(&builder, utf8_str("Float_Literal_Type"));
                    break;
                }
                case SSA_Kind_Float_Literal: {
                    f64 value = ssa->data.float_literal.value;
                    char value_buf[32];
                    snprintf(value_buf, 32, "%f", value);
                    utf8 value_utf8 = {value_buf, strlen(value_buf)};
                    utf8_builder_append(&builder, utf8_str("Float_Literal: "));
                    utf8_builder_append(&builder, value_utf8);
                    break;
                }
                case SSA_Kind_Return: {
                    SSA* return_value = ssa->data.return_.return_value;
                    utf8 return_value_name = ssa_get_ssa_name(return_value);
                    utf8_builder_append(&builder, utf8_str("Return: "));
                    utf8_builder_append(&builder, return_value_name);
                    break;
                }
                case SSA_Kind_Return_Type: {
                    utf8_builder_append(&builder, utf8_str("Return_Type"));
                    break;
                }
                case SSA_Kind_Explicit_Cast: {
                    SSA* value = ssa->data.explicit_cast.value;
                    SSA* type = ssa->data.explicit_cast.type;
                    utf8 value_name = ssa_get_ssa_name(value);
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_builder_append(&builder, utf8_str("Explicit_Cast: "));
                    utf8_builder_append(&builder, value_name);
                    utf8_builder_append(&builder, utf8_str(" -> "));
                    utf8_builder_append(&builder, type_name);
                    break;
                }
                case SSA_Kind_Implicit_Cast: {
                    SSA* value = ssa->data.implicit_cast.value;
                    SSA* type = ssa->data.implicit_cast.type;
                    utf8 value_name = ssa_get_ssa_name(value);
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_builder_append(&builder, utf8_str("Implicit_Cast: "));
                    utf8_builder_append(&builder, value_name);
                    utf8_builder_append(&builder, utf8_str(" -> "));
                    utf8_builder_append(&builder, type_name);
                    break;
                }
                case SSA_Kind_Int_Type: {
                    utf8_builder_append(&builder, utf8_str("Int_Type"));
                    break;
                }
                case SSA_Kind_Uint_Type: {
                    utf8_builder_append(&builder, utf8_str("Uint_Type"));
                    break;
                }
                case SSA_Kind_Float_Type: {
                    utf8_builder_append(&builder, utf8_str("Float_Type"));
                    break;
                }
                case SSA_Kind_Void_Type: {
                    utf8_builder_append(&builder, utf8_str("Void_Type"));
                    break;
                }
                case SSA_Kind_Compile_To_LLVM_IR: {
                    utf8_builder_append(&builder, utf8_str("Compile_To_LLVM_IR"));
                    break;
                }
                case SSA_Kind_Build: {
                    SSA_Block* build_block = &ssa->data.build.block;
                    utf8 build_block_name = ssa_get_ssa_block_name(build_block);

                    utf8_builder_append(&builder, utf8_str("Build("));
                    utf8_builder_append(&builder, build_block_name);
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Call_Setup: {
                    SSA* callee = ssa->data.call_setup.callee;
                    utf8 callee_name = ssa_get_ssa_name(callee);
                    SSA** arguments = ssa->data.call_setup.arguments;
                    u32 arguments_count = ssa->data.call_setup.arguments_count;
                    utf8_builder_append(&builder, utf8_str("Call_Setup "));
                    utf8_builder_append(&builder, callee_name);
                    utf8_builder_append(&builder, utf8_str("("));
                    for (u32 i = 0; i < arguments_count; i++) {
                        SSA* argument = arguments[i];
                        utf8 argument_name = ssa_get_ssa_name(argument);
                        utf8_builder_append(&builder, argument_name);
                        if (i != arguments_count - 1) {
                            utf8_builder_append(&builder, utf8_str(", "));
                        }
                    }
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Call: {
                    SSA* setup = ssa->data.call.setup;
                    utf8 setup_name = ssa_get_ssa_name(setup);
                    utf8_builder_append(&builder, utf8_str("Call: "));
                    utf8_builder_append(&builder, setup_name);
                    break;
                }
                case SSA_Kind_Parameter_Type: {
                    u32 index = ssa->data.parameter_type.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Parameter_Type: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Argument_Type: {
                    u32 index = ssa->data.argument_type.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Argument_Type: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Call_Setup_Type: {
                    utf8_builder_append(&builder, utf8_str("Call_Setup_Type"));
                    break;
                }
                case SSA_Kind_Call_Return_Type: {
                    SSA* setup = ssa->data.call_return_type.setup;
                    utf8_builder_append(&builder, utf8_str("Call_Return_Type: "));
                    utf8 setup_name = ssa_get_ssa_name(setup);
                    utf8_builder_append(&builder, setup_name);
                    break;
                }
                case SSA_Kind_Pointer_Type: {
                    SSA* type = ssa->data.pointer_type.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_builder_append(&builder, utf8_str("Pointer_Type: "));
                    utf8_builder_append(&builder, type_name);
                    break;
                }
                case SSA_Kind_Underlying_Type: {
                    SSA* type = ssa->data.underlying_type.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_builder_append(&builder, utf8_str("Underlying_Type: "));
                    utf8_builder_append(&builder, type_name);
                    break;
                }
                case SSA_Kind_Default_Value: {
                    SSA* type = ssa->data.default_value.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_builder_append(&builder, utf8_str("Default_Value: "));
                    utf8_builder_append(&builder, type_name);
                    break;
                }
                case SSA_Kind_Return_Value: {
                    SSA** values = ssa->data.return_value.values;
                    u32 values_count = ssa->data.return_value.values_count;
                    utf8_builder_append(&builder, utf8_str("Return_Value("));
                    for (u32 i = 0; i < values_count; i++) {
                        SSA* value = values[i];
                        utf8 value_name = ssa_get_ssa_name(value);
                        utf8_builder_append(&builder, value_name);
                        if (i != values_count - 1) {
                            utf8_builder_append(&builder, utf8_str(", "));
                        }
                    }
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Return_Value_Type: {
                    SSA** types = ssa->data.return_value_type.types;
                    u32 types_count = ssa->data.return_value_type.types_count;
                    utf8_builder_append(&builder, utf8_str("Return_Value_Type("));
                    for (u32 i = 0; i < types_count; i++) {
                        SSA* type = types[i];
                        utf8 type_name = ssa_get_ssa_name(type);
                        utf8_builder_append(&builder, type_name);
                        if (i != types_count - 1) {
                            utf8_builder_append(&builder, utf8_str(", "));
                        }
                    }
                    utf8_builder_append(&builder, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Return_Value_Index: {
                    SSA* return_value = ssa->data.return_value_index.return_value;
                    utf8 return_value_name = ssa_get_ssa_name(return_value);
                    u32 index = ssa->data.return_value_index.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Return_Value_Index: "));
                    utf8_builder_append(&builder, return_value_name);
                    utf8_builder_append(&builder, utf8_str(", Index: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Return_Value_Type_Index: {
                    SSA* return_value_type = ssa->data.return_value_type_index.return_value_type;
                    utf8 return_value_type_name = ssa_get_ssa_name(return_value_type);
                    u32 index = ssa->data.return_value_type_index.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_builder_append(&builder, utf8_str("Return_Value_Type_Index: "));
                    utf8_builder_append(&builder, return_value_type_name);
                    utf8_builder_append(&builder, utf8_str(", Index: "));
                    utf8_builder_append(&builder, index_utf8);
                    break;
                }
                case SSA_Kind_Terminate_Global_Scope: {
                    utf8_builder_append(&builder, utf8_str("Terminate_Global_Scope"));
                    break;
                }
            }

            utf8_builder_append(&builder, utf8_str("\n"));
        }
    }

    return builder.str;
}

void ssa_init_intrinsic_block() {
    SSA_Block* block = &context.global_block;

    SSA type_type = {0};
    type_type.kind = SSA_Kind_Type_Type;
    context.intrinsic_ssa_block.type_type = ssa_add_to_block(type_type, block);
    context.intrinsic_ssa_block.type_type->type = ssa_type_type();  // must be called this to self reference
    Type* type_type_type = alloc(sizeof(Type));
    type_type_type->kind = Type_Kind_Type;
    context.intrinsic_ssa_block.type_type_value = type_type_type;

    SSA function_type = {0};
    function_type.kind = SSA_Kind_Function_Type;
    function_type.type = ssa_type_type();
    context.intrinsic_ssa_block.function_type = ssa_add_to_block(function_type, block);
    Type* function_type_type = alloc(sizeof(Type));
    function_type_type->kind = Type_Kind_Function;
    context.intrinsic_ssa_block.function_type_value = function_type_type;

    SSA int_literal_type = {0};
    int_literal_type.kind = SSA_Kind_Int_Literal_Type;
    int_literal_type.type = ssa_type_type();
    context.intrinsic_ssa_block.int_literal_type = ssa_add_to_block(int_literal_type, block);
    Type* int_literal_type_type = alloc(sizeof(Type));
    int_literal_type_type->kind = Type_Kind_Int_Literal;
    context.intrinsic_ssa_block.int_literal_type_value = int_literal_type_type;

    SSA float_literal_type = {0};
    float_literal_type.kind = SSA_Kind_Float_Literal_Type;
    float_literal_type.type = ssa_type_type();
    context.intrinsic_ssa_block.float_literal_type = ssa_add_to_block(float_literal_type, block);
    Type* float_literal_type_type = alloc(sizeof(Type));
    float_literal_type_type->kind = Type_Kind_Float_Literal;
    context.intrinsic_ssa_block.float_literal_type_value = float_literal_type_type;

    SSA void_type = {0};
    void_type.kind = SSA_Kind_Void_Type;
    void_type.type = ssa_type_type();
    context.intrinsic_ssa_block.void_type = ssa_add_to_block(void_type, block);
    Type* void_type_type = alloc(sizeof(Type));
    void_type_type->kind = Type_Kind_Void;
    context.intrinsic_ssa_block.void_type_value = void_type_type;

    SSA call_setup_type = {0};
    call_setup_type.kind = SSA_Kind_Call_Setup_Type;
    call_setup_type.type = ssa_type_type();
    context.intrinsic_ssa_block.call_setup_type = ssa_add_to_block(call_setup_type, block);
    Type* call_setup_type_type = alloc(sizeof(Type));
    call_setup_type_type->kind = Type_Kind_Call_Setup;
    context.intrinsic_ssa_block.call_setup_type_value = call_setup_type_type;

    {
        SSA int_type = {0};
        int_type.kind = SSA_Kind_Int_Type;
        int_type.type = ssa_function_type();
        context.intrinsic_ssa_block.int_type = ssa_add_to_block(int_type, block);
        Function* int_type_function = alloc(sizeof(Function));
        int_type_function->kind = Function_Kind_Intrinsic;
        int_type_function->data.intrinsic.kind = Intrinsic_Function_Int_Type;
        int_type_function->name = utf8_str("int_type");
        int_type_function->parameter_count = 2;
        int_type_function->parameters = alloc(sizeof(SSA*) * 2);
        SSA* argument_1 = ssa_argument(1, &int_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, ssa_type_type(), &int_type_function->setup_block, NULL);
        int_type_function->parameters[1] = casted_argument_1;
        SSA* argument_0 = ssa_argument(0, &int_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, casted_argument_1, &int_type_function->setup_block, NULL);
        int_type_function->parameters[0] = casted_argument_0;
        int_type_function->return_type = ssa_type_type();

        Function** int_type_function_ptr = alloc(sizeof(Function*));
        *int_type_function_ptr = int_type_function;
        context.intrinsic_ssa_block.int_type_value = int_type_function_ptr;
    }

    {
        SSA uint_type = {0};
        uint_type.kind = SSA_Kind_Uint_Type;
        uint_type.type = ssa_function_type();
        context.intrinsic_ssa_block.uint_type = ssa_add_to_block(uint_type, block);
        Function* uint_type_function = alloc(sizeof(Function));
        uint_type_function->kind = Function_Kind_Intrinsic;
        uint_type_function->data.intrinsic.kind = Intrinsic_Function_Uint_Type;
        uint_type_function->name = utf8_str("uint_type");
        uint_type_function->parameter_count = 2;
        uint_type_function->parameters = alloc(sizeof(SSA*) * 2);
        SSA* argument_1 = ssa_argument(1, &uint_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, ssa_type_type(), &uint_type_function->setup_block, NULL);
        uint_type_function->parameters[1] = casted_argument_1;
        SSA* argument_0 = ssa_argument(0, &uint_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, casted_argument_1, &uint_type_function->setup_block, NULL);
        uint_type_function->parameters[0] = casted_argument_0;
        uint_type_function->return_type = ssa_type_type();

        Function** uint_type_function_ptr = alloc(sizeof(Function*));
        *uint_type_function_ptr = uint_type_function;
        context.intrinsic_ssa_block.uint_type_value = uint_type_function_ptr;
    }

    {
        SSA float_type = {0};
        float_type.kind = SSA_Kind_Float_Type;
        float_type.type = ssa_function_type();
        context.intrinsic_ssa_block.float_type = ssa_add_to_block(float_type, block);
        Function* float_type_function = alloc(sizeof(Function));
        float_type_function->kind = Function_Kind_Intrinsic;
        float_type_function->data.intrinsic.kind = Intrinsic_Function_Float_Type;
        float_type_function->name = utf8_str("float_type");
        float_type_function->parameter_count = 2;
        float_type_function->parameters = alloc(sizeof(SSA*) * 2);
        SSA* argument_1 = ssa_argument(1, &float_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, ssa_type_type(), &float_type_function->setup_block, NULL);
        float_type_function->parameters[1] = casted_argument_1;
        SSA* argument_0 = ssa_argument(0, &float_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, casted_argument_1, &float_type_function->setup_block, NULL);
        float_type_function->parameters[0] = casted_argument_0;
        float_type_function->return_type = ssa_type_type();

        Function** float_type_function_ptr = alloc(sizeof(Function*));
        *float_type_function_ptr = float_type_function;
        context.intrinsic_ssa_block.float_type_value = float_type_function_ptr;
    }

    {
        SSA compile_to_llvm_ir = {0};
        compile_to_llvm_ir.kind = SSA_Kind_Compile_To_LLVM_IR;
        compile_to_llvm_ir.type = ssa_function_type();
        context.intrinsic_ssa_block.compile_to_llvm_ir = ssa_add_to_block(compile_to_llvm_ir, block);
        Function* compile_to_llvm_ir_function = alloc(sizeof(Function));
        compile_to_llvm_ir_function->kind = Function_Kind_Intrinsic;
        compile_to_llvm_ir_function->data.intrinsic.kind = Intrinsic_Function_Kind_Compile_To_LLVM_IR;
        compile_to_llvm_ir_function->name = utf8_str("compile_to_llvm_ir");
        compile_to_llvm_ir_function->parameter_count = 1;
        compile_to_llvm_ir_function->parameters = alloc(sizeof(SSA*) * 1);
        SSA* argument_0 = ssa_argument(0, &compile_to_llvm_ir_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, ssa_function_type(), &compile_to_llvm_ir_function->setup_block, NULL);
        compile_to_llvm_ir_function->parameters[0] = casted_argument_0;
        compile_to_llvm_ir_function->return_type = ssa_void_type();

        Function** compile_to_llvm_ir_function_ptr = alloc(sizeof(Function*));
        *compile_to_llvm_ir_function_ptr = compile_to_llvm_ir_function;
        context.intrinsic_ssa_block.compile_to_llvm_ir_value = compile_to_llvm_ir_function_ptr;
    }
}

bool ssa_is_math_type(Type* type) {
    return type->kind == Type_Kind_Int || type->kind == Type_Kind_Uint || type->kind == Type_Kind_Float || type->kind == Type_Kind_Float_Literal ||
           type->kind == Type_Kind_Int_Literal;
}

i64 ssa_type_alignment(Type* type) {
    switch (type->kind) {
        case Type_Kind_Ptr: {
            return alignof(u64);
        }
        case Type_Kind_Int: {
            u32 bits = type->data.int_.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Float: {
            u32 bits = type->data.float_.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Uint: {
            u32 bits = type->data.uint.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Call_Setup:
        case Type_Kind_Type:
        case Type_Kind_Int_Literal:
        case Type_Kind_Float_Literal:
        case Type_Kind_Void:
        case Type_Kind_Function: {
            return 0;
        }
        case Type_Kind_Invalid: {
            internal_compiler_error();
        }
        case Type_Kind_Return_Value: {
            i64 max_alignment = 0;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* return_type = type->data.return_value.types[i];
                i64 alignment = ssa_type_alignment(return_type);
                if (alignment > max_alignment) {
                    max_alignment = alignment;
                }
            }
            return max_alignment;
        }
    }
}

i64 ssa_type_alignment_compile_time(Type* type) {
    switch (type->kind) {
        case Type_Kind_Int_Literal: {
            return alignof(Big_Int);
        }
        case Type_Kind_Float_Literal: {
            return alignof(f64);
        }
        case Type_Kind_Type: {
            return alignof(Type);
        }
        case Type_Kind_Function: {
            return alignof(Function*);
        }
        case Type_Kind_Call_Setup: {
            return alignof(Function_Context*);
        }
        case Type_Kind_Invalid:
        case Type_Kind_Int:
        case Type_Kind_Uint:
        case Type_Kind_Float:
        case Type_Kind_Void:
        case Type_Kind_Ptr: {
            return ssa_type_alignment(type);
        }
        case Type_Kind_Return_Value: {
            i64 max_alignment = 0;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* return_type = type->data.return_value.types[i];
                i64 alignment = ssa_type_alignment_compile_time(return_type);
                if (alignment > max_alignment) {
                    max_alignment = alignment;
                }
            }
            return max_alignment;
        }
    }
}

i64 ssa_type_size_compile_time(Type* type) {
    switch (type->kind) {
        case Type_Kind_Int_Literal: {
            return sizeof(Big_Int);
        }
        case Type_Kind_Float_Literal: {
            return sizeof(f64);
        }
        case Type_Kind_Type: {
            return sizeof(Type);
        }
        case Type_Kind_Function: {
            return sizeof(Function*);
        }
        case Type_Kind_Call_Setup: {
            return sizeof(Function_Context*);
        }
        case Type_Kind_Return_Value: {
            i64 size = 0;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* return_type = type->data.return_value.types[i];
                i64 return_type_size = ssa_type_size_compile_time(return_type);
                i64 alignment = ssa_type_alignment_compile_time(return_type);
                i64 padding = (alignment - (size % alignment)) % alignment;
                size += padding;
                size += return_type_size;
            }
            return size;
        }
        case Type_Kind_Invalid:
        case Type_Kind_Int:
        case Type_Kind_Uint:
        case Type_Kind_Float:
        case Type_Kind_Void:
        case Type_Kind_Ptr: {
            return ssa_type_size(type);
        }
    }
}

i64 ssa_type_size(Type* type) {
    switch (type->kind) {
        case Type_Kind_Ptr: {
            return sizeof(u64);
        }
        case Type_Kind_Int: {
            u32 bits = type->data.int_.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Float: {
            u32 bits = type->data.float_.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Uint: {
            u32 bits = type->data.uint.bits;
            return (bits + 7) / 8;
        }
        case Type_Kind_Return_Value: {
            i64 size = 0;
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* return_type = type->data.return_value.types[i];
                i64 return_type_size = ssa_type_size(return_type);
                i64 alignment = ssa_type_alignment(return_type);
                i64 padding = (alignment - (size % alignment)) % alignment;
                size += padding;
                size += return_type_size;
            }
            return size;
        }
        case Type_Kind_Call_Setup:
        case Type_Kind_Type:
        case Type_Kind_Int_Literal:
        case Type_Kind_Float_Literal:
        case Type_Kind_Void:
        case Type_Kind_Function: {
            return 0;
        }
        case Type_Kind_Invalid: {
            internal_compiler_error();
        }
    }
}

i64 ssa_return_value_type_index_to_offset_compile_time(Type* type, i64 index) {
    i64 offset = 0;
    for (u32 i = 0; i < index; i++) {
        Type* return_type = type->data.return_value.types[i];
        i64 return_type_size = ssa_type_size_compile_time(return_type);
        i64 alignment = ssa_type_alignment_compile_time(return_type);
        i64 padding = (alignment - (offset % alignment)) % alignment;
        offset += padding;
        offset += return_type_size;
    }

    Type* index_type = type->data.return_value.types[index];
    i64 index_type_size = ssa_type_size(index_type);
    i64 alignment = ssa_type_alignment(index_type);
    i64 padding = (alignment - (offset % alignment)) % alignment;
    offset += padding;

    return offset;
}

i64 ssa_return_value_type_index_to_offset(Type* type, i64 index) {
    i64 offset = 0;
    for (u32 i = 0; i < index; i++) {
        Type* return_type = type->data.return_value.types[i];
        i64 return_type_size = ssa_type_size(return_type);
        i64 alignment = ssa_type_alignment(return_type);
        i64 padding = (alignment - (offset % alignment)) % alignment;
        offset += padding;
        offset += return_type_size;
    }

    Type* index_type = type->data.return_value.types[index];
    i64 index_type_size = ssa_type_size(index_type);
    i64 alignment = ssa_type_alignment(index_type);
    i64 padding = (alignment - (offset % alignment)) % alignment;
    offset += padding;

    return offset;
}

bool ssa_can_explicit_cast(Type* type, Type* cast_type) {
    if (ssa_type_equal(type, cast_type)) return true;
    if (type->kind == Type_Kind_Ptr && cast_type->kind == Type_Kind_Ptr) return true;
    if (ssa_is_math_type(type) && ssa_is_math_type(cast_type)) return true;
    return false;
}

bool ssa_can_implicit_cast(Type* type, Type* cast_type) {
    if (ssa_type_equal(type, cast_type)) return true;

    // Int implicit casts
    if (type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Int) {
        return true;
    }
    if (type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Uint) {
        return true;
    }
    if (type->kind == Type_Kind_Int && cast_type->kind == Type_Kind_Int) {
        u32 bits = type->data.int_.bits;
        u32 cast_bits = cast_type->data.int_.bits;
        return bits <= cast_bits;
    }
    if (type->kind == Type_Kind_Uint && cast_type->kind == Type_Kind_Uint) {
        u32 bits = type->data.uint.bits;
        u32 cast_bits = cast_type->data.uint.bits;
        return bits <= cast_bits;
    }

    // Float implicit casts
    if (type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Float) {
        return true;
    }
    if (type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Float_Literal) {
        return true;
    }
    if (type->kind == Type_Kind_Float_Literal && cast_type->kind == Type_Kind_Float) {
        return true;
    }
    if (type->kind == Type_Kind_Float && cast_type->kind == Type_Kind_Float) {
        return true;
    }

    return false;
}

void* ssa_cast_value(void* value, Type* value_type, Type* cast_type) {
    assert(ssa_can_explicit_cast(value_type, cast_type));
    if (ssa_type_equal(value_type, cast_type)) {
        return value;
    }

    if (value_type->kind == Type_Kind_Ptr && cast_type->kind == Type_Kind_Ptr) {
        return value;
    }

    // int casting
    if (value_type->kind == Type_Kind_Int && cast_type->kind == Type_Kind_Int) {
        u32 bits = value_type->data.int_.bits;
        u32 cast_bits = cast_type->data.int_.bits;
        void* new_value = arbitrary_int_cast(value, bits, cast_bits, true);
        return new_value;
    }
    if (value_type->kind == Type_Kind_Uint && cast_type->kind == Type_Kind_Uint) {
        u32 bits = value_type->data.uint.bits;
        u32 cast_bits = cast_type->data.uint.bits;
        void* new_value = arbitrary_int_cast(value, bits, cast_bits, false);
        return new_value;
    }
    if (value_type->kind == Type_Kind_Int && cast_type->kind == Type_Kind_Uint) {
        u32 bits = value_type->data.int_.bits;
        u32 cast_bits = cast_type->data.uint.bits;
        void* new_value = arbitrary_int_cast(value, bits, cast_bits, true);
        return new_value;
    }
    if (value_type->kind == Type_Kind_Uint && cast_type->kind == Type_Kind_Int) {
        u32 bits = value_type->data.uint.bits;
        u32 cast_bits = cast_type->data.int_.bits;
        void* new_value = arbitrary_int_cast(value, bits, cast_bits, false);
        return new_value;
    }
    if (value_type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Int) {
        Big_Int* big_int = value;
        i64 value = big_int->data;
        return arbitrary_int_cast(&value, 64, cast_type->data.int_.bits, true);
    }
    if (value_type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Uint) {
        Big_Int* big_int = value;
        i64 value = big_int->data;
        return arbitrary_int_cast(&value, 64, cast_type->data.uint.bits, true);
    }

    // float casting
    if (value_type->kind == Type_Kind_Float_Literal && cast_type->kind == Type_Kind_Float) {
        f64* float_ = value;
        u64 current_bits = 64;
        u64 new_bits = cast_type->data.float_.bits;
        return all_float_cast(float_, current_bits, new_bits);
    }
    if (value_type->kind == Type_Kind_Float && cast_type->kind == Type_Kind_Float) {
        u64 current_bits = cast_type->data.float_.bits;
        u64 new_bits = 64;
        return all_float_cast(value, current_bits, new_bits);
    }
    if (value_type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Float_Literal) {
        Big_Int* big_int = value;
        i64 value = big_int->data;
        f64* res = alloc(sizeof(f64));
        *res = (f64)value;
        return res;
    }
    if (value_type->kind == Type_Kind_Int && cast_type->kind == Type_Kind_Float) {
        u64 current_bits = value_type->data.int_.bits;
        u64 new_bits = cast_type->data.float_.bits;
        f64 as_float = arbitrary_int_cast_to_float(value, current_bits, true);
        return all_float_cast(&as_float, current_bits, new_bits);
    }
    if (value_type->kind == Type_Kind_Uint && cast_type->kind == Type_Kind_Float) {
        u64 current_bits = value_type->data.uint.bits;
        u64 new_bits = cast_type->data.float_.bits;
        f64 as_float = arbitrary_int_cast_to_float(value, current_bits, false);
        return all_float_cast(&as_float, current_bits, new_bits);
    }
    if (value_type->kind == Type_Kind_Float_Literal && cast_type->kind == Type_Kind_Int) {
        f64* float_ = value;
        i64 value = (i64)*float_;
        return arbitrary_int_cast(&value, 64, cast_type->data.int_.bits, true);
    }
    if (value_type->kind == Type_Kind_Float_Literal && cast_type->kind == Type_Kind_Uint) {
        f64* float_ = value;
        u64 value = (u64)*float_;
        return arbitrary_int_cast(&value, 64, cast_type->data.int_.bits, false);
    }
    if (value_type->kind == Type_Kind_Float && cast_type->kind == Type_Kind_Int) {
        f64* float_ = all_float_cast(value, value_type->data.float_.bits, 64);
        i64 value = (i64)*float_;
        return arbitrary_int_cast(&value, 64, cast_type->data.int_.bits, false);
    }
    if (value_type->kind == Type_Kind_Float && cast_type->kind == Type_Kind_Uint) {
        f64* float_ = all_float_cast(value, value_type->data.float_.bits, 64);
        u64 value = (u64)*float_;
        return arbitrary_int_cast(&value, 64, cast_type->data.int_.bits, false);
    }
    if (value_type->kind == Type_Kind_Int_Literal && cast_type->kind == Type_Kind_Float_Literal) {
        Big_Int* big_int = value;
        i64 value = big_int->data;
        f64 f_value = (f64)value;
        f64* f_mem = alloc(sizeof(f64));
        *f_mem = f_value;
        return f_mem;
    }

    internal_compiler_error();
    return NULL;
}

bool ssa_compile_time_value_equal(void* value1, void* value2, Type* type) {
    assert(value1 != NULL);
    assert(value2 != NULL);
    if (value1 == value2) return true;
    switch (type->kind) {
        case Type_Kind_Invalid: {
            internal_compiler_error();
        }
        case Type_Kind_Type: {
            return ssa_type_equal(value1, value2);
        }
        case Type_Kind_Int_Literal: {
            Big_Int* big_int1 = value1;
            i64 value_i64_1 = big_int1->data;
            Big_Int* big_int2 = value2;
            i64 value_i64_2 = big_int2->data;
            return value_i64_1 == value_i64_2;
        }
        case Type_Kind_Float_Literal: {
            f64* float1 = value1;
            f64 value_f64_1 = *float1;
            f64* float2 = value2;
            f64 value_f64_2 = *float2;
            return value_f64_1 == value_f64_2;
        }
        case Type_Kind_Call_Setup:
        case Type_Kind_Function: {
            return *(void**)value1 == *(void**)value2;
        }
        case Type_Kind_Void: {
            return true;
        }
        case Type_Kind_Ptr:
        case Type_Kind_Int:
        case Type_Kind_Uint:
        case Type_Kind_Float: {
            i64 type_size = ssa_type_size(type);
            if (memcmp(value1, value2, type_size) != 0) return false;
            return true;
        }
        case Type_Kind_Return_Value: {
            for (u32 i = 0; i < type->data.return_value.types_count; i++) {
                Type* new_type = type->data.return_value.types[i];
                i64 offset_of_value = ssa_return_value_type_index_to_offset_compile_time(type, i);
                void* new_value1 = ((char*)value1) + offset_of_value;
                void* new_value2 = ((char*)value2) + offset_of_value;
                if (ssa_compile_time_value_equal(new_value1, new_value2, new_type) == false) {
                    return false;
                }
            }
            return true;
        }
    }
}

bool ssa_type_equal(Type* type1, Type* type2) {
    if (type1->kind != type2->kind) return false;
    switch (type1->kind) {
        case Type_Kind_Ptr: {
            Type* ptr1 = type1->data.ptr.type;
            Type* ptr2 = type2->data.ptr.type;
            return ssa_type_equal(ptr1, ptr2);
        }
        case Type_Kind_Int: {
            u32 bits1 = type1->data.int_.bits;
            u32 bits2 = type2->data.int_.bits;
            return bits1 == bits2;
        }
        case Type_Kind_Float: {
            u32 bits1 = type1->data.float_.bits;
            u32 bits2 = type2->data.float_.bits;
            return bits1 == bits2;
        }
        case Type_Kind_Uint: {
            u32 bits1 = type1->data.uint.bits;
            u32 bits2 = type2->data.uint.bits;
            return bits1 == bits2;
        }
        case Type_Kind_Return_Value: {
            if (type1->data.return_value.types_count != type2->data.return_value.types_count) {
                return false;
            }
            for (u32 i = 0; i < type1->data.return_value.types_count; i++) {
                Type* return_type1 = type1->data.return_value.types[i];
                Type* return_type2 = type2->data.return_value.types[i];
                if (!ssa_type_equal(return_type1, return_type2)) {
                    return false;
                }
            }
            return true;
        }
        case Type_Kind_Invalid:
        case Type_Kind_Type:
        case Type_Kind_Int_Literal:
        case Type_Kind_Float_Literal:
        case Type_Kind_Void:
        case Type_Kind_Function:
        case Type_Kind_Call_Setup:
            return true;
    }
}

void ssa_add_interpreter_to_function_context() {
    if (context.evaluate_context->function_context_stack_count == 0) internal_compiler_error();
    All_Function_Context* ctx = &context.evaluate_context->function_context_stack[context.evaluate_context->function_context_stack_count - 1];
    if (ctx->interpreter_function_context != NULL) internal_compiler_error();
    ctx->interpreter_function_context = alloc(sizeof(Interpreter_Function_Context));
}

void ssa_remove_interpreter_from_function_context() {
    if (context.evaluate_context->function_context_stack_count == 0) internal_compiler_error();
    All_Function_Context* ctx = &context.evaluate_context->function_context_stack[context.evaluate_context->function_context_stack_count - 1];
    if (ctx->interpreter_function_context == NULL) internal_compiler_error();
    ctx->interpreter_function_context = NULL;
}

All_Function_Context ssa_get_cache_function_context(SSA* ssa) {
    if (ssa->block->kind == SSA_Block_Kind_Global) return context.evaluate_context->function_context_stack[0];
    return ssa_get_function_context();
}

All_Function_Context ssa_get_function_context() {
    if (context.evaluate_context->function_context_stack_count == 0) internal_compiler_error();
    return context.evaluate_context->function_context_stack[context.evaluate_context->function_context_stack_count - 1];
}

void ssa_pop_function_context() {
    context.evaluate_context->function_context_stack_count--;
}

void ssa_push_function_context(All_Function_Context all) {
    ptr_append(context.evaluate_context->function_context_stack, context.evaluate_context->function_context_stack_count,
               context.evaluate_context->function_context_stack_capacity, all);
}

bool ssa_running_interpreter() {
    All_Function_Context all = ssa_get_function_context();
    return all.interpreter_function_context != NULL;
}

Function_Instance_Data* ssa_get_function_instance_data(Function* function, Function_Context* function_context) {
    for (u32 i = 0; i < function->instance_data_count; i++) {
        Function_Instance_Data* instance_data = function->instance_data + i;
        if (instance_data->function_context == function_context) {
            return instance_data;
        }
    }
    return NULL;
}

SSA** ssa_get_all_ssa_of_kind(SSA_Block* block, SSA_Kind kind, u32* out_count) {
    SSA** ssa_of_kind = NULL;
    u32 count = 0;
    u32 capacity = 0;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            if (ssa->kind == kind) {
                ptr_append(ssa_of_kind, count, capacity, ssa);
            }
            // TODO: handle branching
            switch (ssa->kind) {
                default: {
                    break;
                }
            }
        }
    }
    *out_count = count;
    return ssa_of_kind;
}

utf8 ssa_kind_to_string(SSA_Kind kind) {
    switch (kind) {
        case SSA_Kind_Invalid:
            return utf8_str("SSA_Kind_Invalid");
        case SSA_Kind_Store:
            return utf8_str("SSA_Kind_Store");
        case SSA_Kind_Load:
            return utf8_str("SSA_Kind_Load");
        case SSA_Kind_Stack_Alloc:
            return utf8_str("SSA_Kind_Stack_Alloc");
        case SSA_Kind_Function_Type:
            return utf8_str("SSA_Kind_Function_Type");
        case SSA_Kind_Type_Type:
            return utf8_str("SSA_Kind_Type_Type");
        case SSA_Kind_Function_Declaration:
            return utf8_str("SSA_Kind_Function_Declaration");
        case SSA_Kind_Parameter:
            return utf8_str("SSA_Kind_Parameter");
        case SSA_Kind_Int_Literal_Type:
            return utf8_str("SSA_Kind_Int_Literal_Type");
        case SSA_Kind_Int_Literal:
            return utf8_str("SSA_Kind_Int_Literal");
        case SSA_Kind_Float_Literal_Type:
            return utf8_str("SSA_Kind_Float_Literal_Type");
        case SSA_Kind_Float_Literal:
            return utf8_str("SSA_Kind_Float_Literal");
        case SSA_Kind_Return:
            return utf8_str("SSA_Kind_Return");
        case SSA_Kind_Return_Type:
            return utf8_str("SSA_Kind_Return_Type");
        case SSA_Kind_Implicit_Cast:
            return utf8_str("SSA_Kind_Implicit_Cast");
        case SSA_Kind_Int_Type:
            return utf8_str("SSA_Kind_Int_Type");
        case SSA_Kind_Uint_Type:
            return utf8_str("SSA_Kind_Uint_Type");
        case SSA_Kind_Float_Type:
            return utf8_str("SSA_Kind_Float_Type");
        case SSA_Kind_Void_Type:
            return utf8_str("SSA_Kind_Void_Type");
        case SSA_Kind_Compile_To_LLVM_IR:
            return utf8_str("SSA_Kind_Compile_To_LLVM_IR");
        case SSA_Kind_Build:
            return utf8_str("SSA_Kind_Build");
        case SSA_Kind_Call_Setup:
            return utf8_str("SSA_Kind_Call_Setup");
        case SSA_Kind_Call:
            return utf8_str("SSA_Kind_Call");
        case SSA_Kind_Argument:
            return utf8_str("SSA_Kind_Argument");
        case SSA_Kind_Parameter_Type:
            return utf8_str("SSA_Kind_Parameter_Type");
        case SSA_Kind_Argument_Type:
            return utf8_str("SSA_Kind_Argument_Type");
        case SSA_Kind_Call_Setup_Type:
            return utf8_str("SSA_Kind_Call_Setup_Type");
        case SSA_Kind_Call_Return_Type:
            return utf8_str("SSA_Kind_Call_Return_Type");
        case SSA_Kind_Pointer_Type:
            return utf8_str("SSA_Kind_Pointer_Type");
        case SSA_Kind_Underlying_Type:
            return utf8_str("SSA_Kind_Underlying_Type");
        case SSA_Kind_Default_Value:
            return utf8_str("SSA_Kind_Default_Value");
        case SSA_Kind_Explicit_Cast:
            return utf8_str("SSA_Kind_Explicit_Cast");
        case SSA_Kind_Return_Value:
            return utf8_str("SSA_Kind_Return_Value");
        case SSA_Kind_Return_Value_Type:
            return utf8_str("SSA_Kind_Return_Value_Type");
        case SSA_Kind_Return_Value_Index:
            return utf8_str("SSA_Kind_Return_Value_Index");
        case SSA_Kind_Return_Value_Type_Index:
            return utf8_str("SSA_Kind_Return_Value_Type_Index");
        case SSA_Kind_Terminate_Global_Scope:
            return utf8_str("SSA_Kind_Terminate_Global_Scope");
    }
}
