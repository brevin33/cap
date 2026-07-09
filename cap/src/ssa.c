#include "ssa.h"

#include <math.h>

#include "cap.h"
#include "log.h"

SSA* ssa_add_to_block(SSA ssa, SSA_Block* block) {
    if (block == NULL) {
        SSA* ssa_ptr = alloc(sizeof(SSA));
        *ssa_ptr = ssa;
        return ssa_ptr;
    }
    ssa.block = block;
    if (block->statement_lists_count == 0) {
        SSA_List list = {0};
        list.statements_capacity = 8;
        list.statements = alloc(sizeof(SSA) * list.statements_capacity);
        list.statements_count = 0;
        ptr_append(block->statement_lists, block->statement_lists_count, block->statement_lists_capacity, list);
    }
    SSA_List* list = block->statement_lists + block->statement_lists_count - 1;
    if (list->statements_count == list->statements_capacity) {
        SSA_List new_list = {0};
        new_list.statements = alloc(sizeof(SSA) * list->statements_capacity * 2);
        new_list.statements_count = 0;
        ptr_append(block->statement_lists, block->statement_lists_count, block->statement_lists_capacity, new_list);
        list = block->statement_lists + block->statement_lists_count - 1;
    }
    ptr_append(list->statements, list->statements_count, list->statements_capacity, ssa);
    SSA* ssa_ptr = list->statements + list->statements_count - 1;
    // log_msg_ssa("Added SSA to block", log_debug, ssa_ptr);
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

Function* ssa_evaluate_function(SSA* ssa) {
    Type* type = ssa_evaluate_type(ssa->type);
    if (type == NULL) return NULL;
    if (type->kind != Type_Kind_Function) {
        log_msg_ssa("Can't evaluate to function", log_error, ssa);
        return NULL;
    }
    return ssa_evaluate(ssa);
}

Type* ssa_evaluate_type(SSA* ssa) {
    if (ssa->kind == SSA_Kind_Type_Type) return context.intrinsic_ssa_block.type_type_value;
    Type* type = ssa_evaluate_type(ssa->type);
    if (type == NULL) return NULL;
    if (type->kind != Type_Kind_Type) {
        log_msg_ssa("Can't evaluate to type", log_error, ssa);
        return NULL;
    }
    return ssa_evaluate(ssa);
}

SSA_Possible_Values ssa_evaluate_int_type_function(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* p1 = function_context->parameters[0];
    SSA* p1_type_ssa = function->parameter_types[0];
    Type* p1_type = ssa_evaluate_type(p1_type_ssa);

    void* p1_value = ssa_evaluate(p1);
    if (p1_value == NULL) return (SSA_Possible_Values){0};

    Type i64_type = {0};
    i64_type.kind = Type_Kind_Int;
    i64_type.data.int_.bits = 64;

    i64* value_as_i64 = ssa_cast_value(p1_value, p1_type, &i64_type);

    Type* int_type_type = alloc(sizeof(Type));
    int_type_type->kind = Type_Kind_Int;
    int_type_type->data.int_.bits = *value_as_i64;

    return ssa_possible_single_value(int_type_type);
}

SSA_Possible_Values ssa_evaluate_uint_type_function(SSA* ssa, Function_Context* function_context, Function* function) {
    assert(false);
}

SSA_Possible_Values ssa_evaluate_float_type_function(SSA* ssa, Function_Context* function_context, Function* function) {
    assert(false);
}

SSA_Possible_Values ssa_evaluate_compile_to_llvm_ir_function(SSA* ssa, Function_Context* function_context, Function* function) {
    SSA* p1 = function_context->parameters[0];
    SSA_Block main_run_block = {0};
    main_run_block.kind = SSA_Block_Kind_Scope;
    SSA* main_function_setup = ssa_call_setup(p1, NULL, 0, &main_run_block, p1->ast);
    if (ssa_type_check(main_function_setup) == false) return (SSA_Possible_Values){0};
    SSA* main_function_call = ssa_call(main_function_setup, &main_run_block, p1->ast);
    if (ssa_type_check(main_function_call) == false) return (SSA_Possible_Values){0};
    assert(false);
}

static bool _ssa_figure_out_arguments(SSA** arguments, u32 arguments_count, SSA* argument_side, SSA_Evaluation_Context* argument_evaluation_context,
                                      SSA* parameter_side, SSA_Evaluation_Context* parameter_evaluation_context, bool* out_changed) {
    if (parameter_side->kind == SSA_Kind_Argument) {
        u32 index = parameter_side->data.argument.index;
        SSA* current_argument = arguments[index];
        if (current_argument == NULL) {
            arguments[index] = argument_side;
            *out_changed = true;
            return true;
        }

        context.ssa_evaluation_context = argument_evaluation_context;
        Type* argument_side_type = ssa_evaluate_type(argument_side->type);
        if (argument_side_type == NULL) return false;

        context.ssa_evaluation_context = parameter_evaluation_context;
        Type* current_argument_type = ssa_evaluate_type(current_argument->type);
        if (current_argument_type == NULL) return false;

        if (ssa_type_equal(argument_side_type, current_argument_type) == false) {
            log_msg_ssa("Conflicting inferred argument types", log_error, argument_side);
            return false;
        }

        context.ssa_evaluation_context = argument_evaluation_context;
        void* argument_side_value = ssa_evaluate(argument_side);
        if (argument_side_value == NULL) return false;

        context.ssa_evaluation_context = parameter_evaluation_context;
        void* current_argument_value = ssa_evaluate(current_argument);
        if (current_argument_value == NULL) return false;

        if (ssa_compile_time_value_equal(argument_side_value, current_argument_value, argument_side_type) == false) {
            log_msg_ssa("Conflicting inferred argument values", log_error, argument_side);
            return false;
        }

        return true;
    }
    // TODO: further type inference
    return true;
}

static bool _ssa_find_value_at_providence(SSA_Block* block, SSA_Block_Location location, Pointer_Providence* providence, i64 value_size, Type* type,
                                          SSA_Possible_Providence_Values* possible_values) {
    if (providence == 0) return true;
    bool first = true;
    for (u32 i = location.list_index; i != UINT32_MAX; i--) {
        SSA_List* list = block->statement_lists + i;
        u32 start_index = first ? location.statement_index : list->statements_count - 1;
        first = false;
        for (u32 j = start_index; j != UINT32_MAX; j--) {
            SSA* ssa = list->statements + j;
            switch (ssa->kind) {
                case SSA_Kind_Store: {
                    SSA* address = ssa->data.store.address;
                    SSA* stored = ssa->data.store.value;
                    Type* address_type = ssa_evaluate_type(address->type);
                    if (address_type == NULL) return false;
                    if (address_type->kind != Type_Kind_Ptr) {
                        log_msg_ssa("Found store into address not of type pointer", log_error, ssa);
                        return false;
                    }
                    Type* stored_type = ssa_evaluate_type(stored->type);
                    if (stored_type == NULL) return false;
                    i64 stored_type_size = ssa_type_size(stored_type);

                    SSA_Possible_Values address_possibilities = ssa_general_evaluate_speculative(address);
                    if (address_possibilities.values_count == 0) {
                        assert(false);
                        // TODO: don't know what to do here think nothing but want to see when if the ever happens
                    }
                    SSA_Possible_Values stored_possibilities = ssa_general_evaluate_speculative(stored);

                    SSA_Possible_Providence_Value* values = NULL;
                    u32 values_count = 0;
                    u32 values_capacity = 0;
                    for (u32 i = 0; i < address_possibilities.values_count; i++) {
                        if (ssa_in_providence_and_will_change(providence, value_size, address_possibilities.values[i], stored_type_size, possible_values) ==
                            false)
                            continue;

                        // can't do anything about loading value that doesn't exist
                        if (stored_possibilities.values_count == 0) {
                            possible_values->values_count = 0;
                            return true;
                        }

                        for (u32 j = 0; j < stored_possibilities.values_count; j++) {
                            SSA_Possible_Providence_Value possible_value = ssa_create_possible_providence_value(
                                providence, value_size, address_possibilities.values[i], stored_type_size, stored_possibilities.values[j]);
                            if (possible_value.value == NULL) continue;

                            for (u32 k = 0; k < possible_values->values_count; k++) {
                                SSA_Possible_Providence_Value possible_value_copy = ssa_copy_possible_providence_value(possible_value, value_size);
                                SSA_Possible_Providence_Value* current_value = possible_values->values + k;
                                for (i64 l = 0; l < value_size; l++) {
                                    if (current_value->bytes_filled[l] == 1) {
                                        possible_value_copy.bytes_filled[l] = 1;
                                        ((char*)possible_value_copy.value)[l] = ((char*)current_value->value)[l];
                                    }
                                }
                                ptr_append(values, values_count, values_capacity, possible_value_copy);
                            }
                        }
                    }

                    if (values_count != 0) {
                        possible_values->values_count = 0;
                        for (u32 i = 0; i < values_count; i++) {
                            SSA_Possible_Providence_Value value = values[i];
                            ptr_append(possible_values->values, possible_values->values_count, possible_values->values_capacity, value);
                        }
                        ssa_possible_providence_values_consolidate(possible_values, type, value_size);
                    }
                    bool all_full = true;
                    for (u32 i = 0; i < possible_values->values_count; i++) {
                        SSA_Possible_Providence_Value value = possible_values->values[i];
                        if (ssa_possible_providence_value_full(value, value_size) == false) {
                            all_full = false;
                        }
                    }
                    if (all_full) return true;
                    break;
                }
                case SSA_Kind_Call: {
                    assert(false);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }

    SSA_Possible_Providence_Values* branch_values = NULL;
    u32 branch_values_count = 0;
    u32 branch_values_capacity = 0;

    for (u32 i = 0; i < block->branchs_to_this_block_count; i++) {
        SSA_Block* branch = block->branchs_to_this_block[i];

        SSA_Possible_Providence_Values values = {0};
        for (u32 j = 0; j < possible_values->values_count; j++) {
            SSA_Possible_Providence_Value value = possible_values->values[j];
            SSA_Possible_Providence_Value copy = ssa_copy_possible_providence_value(value, value_size);
            ptr_append(values.values, values.values_count, values.values_capacity, copy);
        }

        SSA_Block_Location branch_end_location = {0};
        branch_end_location.list_index = branch->statement_lists_count - 1;
        SSA_List last_list = branch->statement_lists[branch_end_location.list_index];
        branch_end_location.statement_index = last_list.statements_count - 1;
        bool res = _ssa_find_value_at_providence(branch, branch_end_location, providence, value_size, type, &values);
        ptr_append(branch_values, branch_values_count, branch_values_capacity, values);
        if (res == false) return false;
    }

    if (block->kind == SSA_Block_Kind_Function || block->kind == SSA_Block_Kind_Function_Setup) {
        Function_Context* function_context = ssa_pop_function_context();
        SSA* call_ssa = function_context->call;
        SSA_Block_Location call_location = ssa_get_location(call_ssa);
        SSA_Block* call_block = call_ssa->block;

        SSA_Possible_Providence_Values values = {0};
        for (u32 j = 0; j < possible_values->values_count; j++) {
            SSA_Possible_Providence_Value value = possible_values->values[j];
            SSA_Possible_Providence_Value copy = ssa_copy_possible_providence_value(value, value_size);
            ptr_append(values.values, values.values_count, values.values_capacity, copy);
        }
        bool res = _ssa_find_value_at_providence(call_block, call_location, providence, value_size, type, &values);
        ptr_append(branch_values, branch_values_count, branch_values_capacity, values);

        ssa_push_function_context(function_context);
        if (res == false) return false;
    }

    possible_values->values_count = 0;
    for (u32 i = 0; i < branch_values_count; i++) {
        SSA_Possible_Providence_Values values = branch_values[i];
        for (u32 j = 0; j < values.values_count; j++) {
            SSA_Possible_Providence_Value value = values.values[j];
            ptr_append(possible_values->values, possible_values->values_count, possible_values->values_capacity, value);
        }
    }

    ssa_possible_providence_values_consolidate(possible_values, type, value_size);

    return true;
}

static SSA_Possible_Values _ssa_general_evaluate(SSA* ssa) {
    switch (ssa->kind) {
        case SSA_Kind_Void_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.void_type_value);
        }
        case SSA_Kind_Function_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.function_type_value);
        }
        case SSA_Kind_Type_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.type_type_value);
        }
        case SSA_Kind_Int_Literal_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.int_literal_type_value);
        }
        case SSA_Kind_Float_Literal_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.float_literal_type_value);
        }
        case SSA_Kind_Call_Setup_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.call_setup_type_value);
        }
        case SSA_Kind_Int_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.int_type_value);
        }
        case SSA_Kind_Uint_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.uint_type_value);
        }
        case SSA_Kind_Float_Type: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.float_type_value);
        }
        case SSA_Kind_Compile_To_LLVM_IR: {
            return ssa_possible_single_value(context.intrinsic_ssa_block.compile_to_llvm_ir_value);
        }
        case SSA_Kind_Pointer_Type: {
            Type* underlying_type = ssa_evaluate_type(ssa->data.pointer_type.type);
            if (underlying_type == NULL) return (SSA_Possible_Values){0};
            Type* type = alloc(sizeof(Type));
            type->kind = Type_Kind_Ptr;
            type->data.ptr.type = underlying_type;

            return ssa_possible_single_value(type);
        }
        case SSA_Kind_Underlying_Type: {
            Type* ptr_type = ssa_evaluate_type(ssa->data.underlying_type.type);
            if (ptr_type == NULL) return (SSA_Possible_Values){0};
            if (ptr_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can only evaluate underlying type if type is a pointer", log_error, ssa);
                return (SSA_Possible_Values){0};
            }
            return ssa_possible_single_value(ptr_type->data.ptr.type);
        }
        case SSA_Kind_Function_Declaration: {
            Function* function = &ssa->data.function_declaration.function;
            return ssa_possible_single_value(function);
        }
        case SSA_Kind_Int_Literal: {
            Big_Int* value = &ssa->data.int_literal.value;
            return ssa_possible_single_value(value);
        }
        case SSA_Kind_Float_Literal: {
            f64* value = &ssa->data.float_literal.value;
            return ssa_possible_single_value(value);
        }
        case SSA_Kind_Multi_Value: {
            assert(false);
            return (SSA_Possible_Values){0};
        }
        case SSA_Kind_Index_Multi_Value: {
            assert(false);
            return (SSA_Possible_Values){0};
        }
        case SSA_Kind_Index_Multi_Type: {
            assert(false);
            return (SSA_Possible_Values){0};
        }
        case SSA_Kind_Multi_Type: {
            assert(false);
            return (SSA_Possible_Values){0};
        }
        case SSA_Kind_Parameter: {
            Function_Context* function_context = ssa_pop_function_context();
            u32 index = ssa->data.parameter.index;
            SSA* parameter = function_context->parameters[index];
            SSA_Possible_Values value = ssa_general_evaluate(parameter);
            ssa_push_function_context(function_context);
            return value;
        }
        case SSA_Kind_Parameter_Type: {
            Function_Context* function_context = ssa_pop_function_context();
            u32 index = ssa->data.parameter_type.index;
            SSA* parameter = function_context->parameters[index];
            SSA_Possible_Values value = ssa_general_evaluate(parameter->type);
            ssa_push_function_context(function_context);
            return value;
        }
        case SSA_Kind_Call_Return_Type: {
            SSA* call_setup = ssa->data.call_return_type.setup;
            Function_Context* function_context = ssa_evaluate(call_setup);
            if (function_context == NULL) return (SSA_Possible_Values){0};
            ssa_push_function_context(function_context);
            SSA* return_type = function_context->return_type;
            SSA_Possible_Values value = ssa_general_evaluate(return_type);
            ssa_pop_function_context();
            return value;
        }
        case SSA_Kind_Argument: {
            Function_Context* function_context = ssa_pop_function_context();
            u32 index = ssa->data.argument.index;
            SSA* argument = function_context->arguments[index];
            SSA_Possible_Values value = ssa_general_evaluate(argument);
            ssa_push_function_context(function_context);
            return value;
        }
        case SSA_Kind_Argument_Type: {
            Function_Context* function_context = ssa_pop_function_context();
            u32 index = ssa->data.argument_type.index;
            SSA* argument = function_context->arguments[index];
            SSA_Possible_Values value = ssa_general_evaluate(argument->type);
            ssa_push_function_context(function_context);
            return value;
        }
        case SSA_Kind_Return_Type: {
            Function_Context* function_context = ssa_pop_function_context();
            SSA* return_type = function_context->return_type;
            SSA_Possible_Values value = ssa_general_evaluate(return_type);
            ssa_push_function_context(function_context);
            return value;
        }
        case SSA_Kind_Explicit_Cast: {
            SSA* value_ssa = ssa->data.explicit_cast.value;
            SSA* type_ssa = ssa->data.explicit_cast.type;

            Type* value_type = ssa_evaluate_type(value_ssa->type);
            if (value_type == NULL) return (SSA_Possible_Values){0};
            Type* type = ssa_evaluate_type(type_ssa);
            if (type == NULL) return (SSA_Possible_Values){0};

            void* value = ssa_evaluate(value_ssa);
            if (value == NULL) return (SSA_Possible_Values){0};

            void* casted_value = ssa_cast_value(value, value_type, type);
            if (casted_value == NULL) return (SSA_Possible_Values){0};
            return ssa_possible_single_value(casted_value);
        }
        case SSA_Kind_Implicit_Cast: {
            SSA* value_ssa = ssa->data.implicit_cast.value;
            SSA* type_ssa = ssa->data.implicit_cast.type;

            Type* value_type = ssa_evaluate_type(value_ssa->type);
            if (value_type == NULL) return (SSA_Possible_Values){0};
            Type* type = ssa_evaluate_type(type_ssa);
            if (type == NULL) return (SSA_Possible_Values){0};

            void* value = ssa_evaluate(value_ssa);
            if (value == NULL) return (SSA_Possible_Values){0};

            void* casted_value = ssa_cast_value(value, value_type, type);
            if (casted_value == NULL) return (SSA_Possible_Values){0};
            return ssa_possible_single_value(casted_value);
        }
        case SSA_Kind_Call_Setup: {
            Function_Context* function_context = alloc(sizeof(Function_Context));

            SSA* callee_ssa = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee_ssa);
            if (function == NULL) return (SSA_Possible_Values){0};

            u32 parameters_count = function->parameter_types_count;
            SSA** arguments = alloc(sizeof(SSA*) * parameters_count);
            for (u32 i = 0; i < ssa->data.call_setup.arguments_count; i++) {
                SSA* argument = ssa->data.call_setup.arguments[i];
                arguments[i] = argument;
            }

            SSA_Evaluation_Context* evaluation_context = context.ssa_evaluation_context;
            SSA_Evaluation_Context other_evaluation_context = {0};
            ssa_copy_evaluation_context_into_other_context(evaluation_context, &other_evaluation_context);

            bool changed = true;
            while (changed) {
                changed = false;
                for (u32 i = 0; i < parameters_count; i++) {
                    SSA* argument = arguments[i];
                    if (argument == NULL) continue;
                    SSA* argument_type = argument->type;
                    SSA* parameter_type = function->parameter_types[i];
                    bool res = _ssa_figure_out_arguments(arguments, parameters_count, argument_type, evaluation_context, parameter_type,
                                                         &other_evaluation_context, &changed);
                    if (res == false) return (SSA_Possible_Values){0};
                }
            }

            context.ssa_evaluation_context = evaluation_context;
            for (u32 i = 0; i < parameters_count; i++) {
                SSA* argument = arguments[i];
                if (argument == NULL) {
                    log_msg_ssa("Failed to figure out argument to call function", log_error, ssa);
                    return (SSA_Possible_Values){0};
                }
            }
            function_context->call = ssa;
            function_context->name = function->name;
            function_context->arguments = arguments;
            function_context->arguments_count = parameters_count;
            function_context->return_type = function->return_type;
            function_context->parameters = function->parameters;
            function_context->parameters_count = parameters_count;
            ssa_push_function_context(function_context);

            SSA_Block* setup_block = &function->setup_block;
            bool res = ssa_type_check_block(setup_block);

            for (u32 i = 0; i < function->parameter_types_count; i++) {
                SSA* parameter_type = function->parameter_types[i];
                Type* parameter_type_type = ssa_evaluate_type(parameter_type);
                res = res && parameter_type_type != NULL;
            }
            SSA* return_type = function->return_type;
            Type* return_type_type = ssa_evaluate_type(return_type);
            res = res && return_type_type != NULL;
            ssa_pop_function_context();

            if (!res) {
                log_msg_ssa("Failed to type check function parameter types", log_info, ssa);
                return (SSA_Possible_Values){0};
            }

            return ssa_possible_single_value(function_context);
        }
        case SSA_Kind_Call: {
            SSA* setup = ssa->data.call.setup;
            SSA* callee = setup->data.call_setup.callee;
            Function_Context* function_context = ssa_evaluate(setup);
            Function* function = ssa_evaluate_function(callee);
            if (function == NULL) return (SSA_Possible_Values){0};

            ssa_push_function_context(function_context);
            SSA_Possible_Values res_value = {0};
            switch (function->kind) {
                case Function_Kind_Invalid: {
                    internal_compiler_error();
                }
                case Function_Kind_Internal: {
                    assert(false);
                }
                case Function_Kind_Intrinsic: {
                    Function_Intrinsic* function_intrinsic = &function->data.intrinsic;
                    switch (function_intrinsic->kind) {
                        case Intrinsic_Function_Kind_Invalid: {
                            internal_compiler_error();
                        }
                        case Intrinsic_Function_Int_Type: {
                            res_value = ssa_evaluate_int_type_function(function_context->parameters[0], function_context, function);
                            break;
                        }
                        case Intrinsic_Function_Uint_Type: {
                            res_value = ssa_evaluate_uint_type_function(function_context->parameters[0], function_context, function);
                            break;
                        }

                        case Intrinsic_Function_Float_Type: {
                            res_value = ssa_evaluate_float_type_function(function_context->parameters[0], function_context, function);
                            break;
                        }
                        case Intrinsic_Function_Kind_Compile_To_LLVM_IR: {
                            res_value = ssa_evaluate_compile_to_llvm_ir_function(function_context->parameters[0], function_context, function);
                            break;
                        }
                    }
                }
            }
            ssa_pop_function_context();
            return res_value;
        }
        case SSA_Kind_Stack_Alloc: {
            Type* type = ssa_evaluate_type(ssa->type);
            if (type == NULL) return (SSA_Possible_Values){0};
            assert(type->kind == Type_Kind_Ptr);
            Pointer_Providence* providence = alloc(sizeof(Pointer_Providence));
            i64 type_size = ssa_type_size(type);
            providence->providence = context.pointer_providence_counter;
            context.pointer_providence_counter += type_size + 1;
            return ssa_possible_single_value(providence);
        }
        case SSA_Kind_Load: {
            SSA_Block* block = ssa->block;
            Type* type = ssa_evaluate_type(ssa->type);
            if (type == NULL) return (SSA_Possible_Values){0};
            i64 type_size = ssa_type_size(type);
            SSA_Block_Location location = ssa_get_location(ssa);
            SSA* address = ssa->data.load.address;
            Type* address_type = ssa_evaluate_type(address->type);
            if (address_type == NULL) return (SSA_Possible_Values){0};
            if (address_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can't evaluate pointer provience of non-pointer type", log_error, ssa);
                return (SSA_Possible_Values){0};
            }
            SSA_Possible_Values address_possibilities = ssa_general_evaluate(address);
            if (address_possibilities.values_count == 0) return (SSA_Possible_Values){0};
            SSA_Possible_Values possible_values = {0};
            SSA_Possible_Providence_Values provience_possible_values = {0};
            for (u32 i = 0; i < address_possibilities.values_count; i++) {
                Pointer_Providence* address_provience = address_possibilities.values[i];
                provience_possible_values.values_count = 0;
                SSA_Possible_Providence_Value possible_value = {0};
                if (type_size == 0) {
                    possible_value.value = NULL;
                    possible_value.bytes_filled = NULL;
                } else {
                    possible_value.value = alloc(type_size);
                    possible_value.bytes_filled = alloc(type_size);
                }
                ptr_append(provience_possible_values.values, provience_possible_values.values_count, provience_possible_values.values_capacity, possible_value);
                bool res = _ssa_find_value_at_providence(block, location, address_provience, type_size, type, &provience_possible_values);
                if (res == false) return (SSA_Possible_Values){0};

                for (u32 j = 0; j < provience_possible_values.values_count; j++) {
                    SSA_Possible_Providence_Value possible_value = provience_possible_values.values[j];
                    bool all_bytes_full = true;
                    for (i64 k = 0; k < type_size; k++) {
                        if (possible_value.bytes_filled[k] == 0) all_bytes_full = false;
                    }
                    if (all_bytes_full) {
                        ptr_append(possible_values.values, possible_values.values_count, possible_values.values_capacity, possible_value.value);
                    } else {
                        ptr_append(possible_values.values, possible_values.values_count, possible_values.values_capacity, NULL);
                    }
                }
            }

            bool all_null = true;
            for (u32 i = 0; i < possible_values.values_count; i++) {
                if (possible_values.values[i] != NULL) {
                    all_null = false;
                    break;
                }
            }
            if (all_null) {
                log_msg_ssa("Failed to determine load value at compile time", log_error, ssa);
                return (SSA_Possible_Values){0};
            }

            ssa_possible_values_consolidate(&possible_values, type);
            return possible_values;
        }
        case SSA_Kind_Default_Value: {
            assert(false);
        }
        case SSA_Kind_Return:
        case SSA_Kind_Build:
        case SSA_Kind_Store:
        case SSA_Kind_Invalid: {
            log_msg_ssa("Trying to evaluate unevaluateable ssa", log_error, ssa);
            return (SSA_Possible_Values){0};
        }
    }
}

SSA_Possible_Values ssa_general_evaluate(SSA* ssa) {
    SSA_Possible_Values possible_values = {0};
    if (ssa_has_been_evaluated(ssa, &possible_values)) return possible_values;
    if (ssa_type_check(ssa) == false) return (SSA_Possible_Values){0};
    possible_values = _ssa_general_evaluate(ssa);

    // Cache value
    SSA_Compile_Time_Value value = {0};
    value.possible_values = possible_values;
    value.function_context = ssa_get_function_context();
    ptr_append(ssa->compile_time_value, ssa->compile_time_value_count, ssa->compile_time_value_capacity, value);

    if (value.possible_values.values_count == 0) {
        log_msg_ssa("Failed to evaluate compile time value", log_info, ssa);
    } else {
        log_msg_ssa("Evaluated compile time value", log_debug, ssa);
    }

    return possible_values;
}

SSA_Possible_Values ssa_general_evaluate_speculative(SSA* ssa) {
    u32 log_location = log_end_location();
    SSA_Possible_Values possible_values = ssa_general_evaluate(ssa);
    if (possible_values.values_count == 0 && log_has_msg_after(log_location)) {
        Log* new_logs = context.logs + log_location;
        u32 new_logs_count = context.logs_count - log_location;
        Log* new_logs_memory = alloc(sizeof(Log) * new_logs_count);
        memcpy(new_logs_memory, new_logs, sizeof(Log) * new_logs_count);
        new_logs = new_logs_memory;

        for (u32 i = 0; i < ssa->compile_time_value_count; i++) {
            SSA_Compile_Time_Value* compile_time_value = ssa->compile_time_value + i;
            if (compile_time_value->function_context == ssa_get_function_context()) {
                compile_time_value->displayed_logs = false;
                compile_time_value->log = new_logs;
                compile_time_value->log_count = new_logs_count;
                break;
            }
        }
        log_clear_after(log_location);
    }

    return possible_values;
}

void* ssa_evaluate(SSA* ssa) {
    SSA_Possible_Values value = ssa_general_evaluate(ssa);
    if (value.values_count == 0) return NULL;
    if (value.values_count > 1) {
        log_msg_ssa("Can't evaluate as there are multiple possible values", log_error, ssa);
        return NULL;
    }
    return value.values[0];
}

bool ssa_type_check_block(SSA_Block* block) {
    bool function_res = true;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            bool res = ssa_type_check(ssa);
            function_res = function_res && res;
        }
    }
    return function_res;
}

bool ssa_type_check_call(SSA* ssa) {
    assert(ssa->kind == SSA_Kind_Call);
    SSA* setup = ssa->data.call.setup;
    Function_Context* function_context = ssa_evaluate(setup);
    if (function_context == NULL) return false;
    SSA* callee = setup->data.call_setup.callee;
    Function* function = ssa_evaluate_function(callee);
    if (function == NULL) return false;

    ssa_push_function_context(function_context);

    bool res = true;
    switch (function->kind) {
        case Function_Kind_Invalid: {
            internal_compiler_error();
        }
        case Function_Kind_Internal: {
            SSA_Block* function_block = &function->data.internal.body;
            res = ssa_type_check_block(function_block);
            break;
        }
        case Function_Kind_Intrinsic: {
            Function_Intrinsic* intrinsic = &function->data.intrinsic;
            switch (intrinsic->kind) {
                case Intrinsic_Function_Kind_Invalid: {
                    internal_compiler_error();
                }
                case Intrinsic_Function_Uint_Type:
                case Intrinsic_Function_Float_Type:
                case Intrinsic_Function_Int_Type: {
                    SSA* p1 = function_context->parameters[0];
                    SSA* p1_type_ssa = function->parameter_types[0];
                    Type* p1_type = ssa_evaluate_type(p1_type_ssa);
                    Type i64_type = {0};
                    i64_type.kind = Type_Kind_Int;
                    i64_type.data.int_.bits = 64;
                    // bool can_cast = ssa_can_explicit_cast(p1_type, &i64_type);
                    if (p1_type->kind != Type_Kind_Int && p1_type->kind != Type_Kind_Uint && p1_type->kind != Type_Kind_Int_Literal) {
                        log_msg_ssa("Can't cast to int_type(64)", log_error, function_context->arguments[0]);
                        res = false;
                        break;
                    }
                    void* p1_value = ssa_evaluate(p1);
                    res = p1_value != NULL;
                    if (res == false) break;
                    if (intrinsic->kind == Intrinsic_Function_Float_Type) {
                        i64* p1_cast_i64 = ssa_cast_value(p1_value, p1_type, &i64_type);
                        i64 p1_val = *p1_cast_i64;
                        if (p1_val != 16 && p1_val != 32 && p1_val != 64 && p1_val != 128) {
                            log_msg_ssa("Float type must be 16, 32, 64, or 128 bits", log_error, function_context->arguments[0]);
                            res = false;
                            break;
                        }
                    }

                    res = true;
                    break;
                }
                case Intrinsic_Function_Kind_Compile_To_LLVM_IR: {
                    SSA* p1 = function_context->parameters[0];
                    Function* main_function = ssa_evaluate_function(p1);
                    res = main_function != NULL;
                    if (res == false) break;
                    assert(false);
                }
            }
        }
    }

    ssa_pop_function_context();

    return res;
}

static bool _ssa_type_check(SSA* ssa) {
    Type* type = ssa_evaluate_type(ssa->type);
    if (type == NULL) return false;
    switch (ssa->kind) {
        case SSA_Kind_Call_Setup: {
            SSA* callee = ssa->data.call_setup.callee;
            Function* function = ssa_evaluate_function(callee);
            if (function == NULL) return false;
            return true;
        }
        case SSA_Kind_Call: {
            return ssa_type_check_call(ssa);
        }
        case SSA_Kind_Explicit_Cast: {
            Type* cast_type = ssa_evaluate_type(ssa->data.explicit_cast.type);
            if (cast_type == NULL) return false;
            Type* value_type = ssa_evaluate_type(ssa->data.explicit_cast.value->type);
            if (value_type == NULL) return false;

            if (ssa_can_explicit_cast(value_type, cast_type)) {
                return true;
            }
            log_msg_ssa("Can't explicit cast", log_error, ssa);
            log_msg_ssa("Value being cast", log_info, ssa->data.explicit_cast.value);
            log_msg_ssa("Type casting to", log_info, ssa->data.explicit_cast.type);
            return false;
        }
        case SSA_Kind_Implicit_Cast: {
            Type* cast_type = ssa_evaluate_type(ssa->data.implicit_cast.type);
            if (cast_type == NULL) return false;
            Type* value_type = ssa_evaluate_type(ssa->data.implicit_cast.value->type);
            if (value_type == NULL) return false;

            if (ssa_can_implicit_cast(value_type, cast_type)) {
                return true;
            }
            log_msg_ssa("Can't implicit cast", log_error, ssa);
            log_msg_ssa("Value being cast", log_info, ssa->data.implicit_cast.value);
            log_msg_ssa("Type casting to", log_info, ssa->data.implicit_cast.type);
            return false;
        }
        case SSA_Kind_Stack_Alloc: {
            Type* type = ssa_evaluate_type(ssa->data.stack_alloc.type);
            if (type == NULL) return false;
            return true;
        }
        case SSA_Kind_Multi_Type: {
            assert(false);
        }
        case SSA_Kind_Index_Multi_Type: {
            assert(false);
        }
        case SSA_Kind_Load: {
            SSA* address = ssa->data.load.address;
            Type* address_type = ssa_evaluate_type(address->type);
            if (address_type == NULL) return false;
            if (address_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can't load from non-pointer type", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Store: {
            SSA* address = ssa->data.store.address;
            Type* address_type = ssa_evaluate_type(address->type);
            if (address_type == NULL) return false;
            if (address_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can't store into non-pointer type", log_error, ssa);
                return false;
            }
            SSA* value = ssa->data.store.value;
            Type* value_type = ssa_evaluate_type(value->type);
            Type* underlying_address_type = address_type->data.ptr.type;
            if (ssa_type_equal(value_type, underlying_address_type) == false) {
                log_msg_ssa("Can't store value of type into pointer of a different type", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Pointer_Type: {
            Type* underlying_type = ssa_evaluate_type(ssa->data.pointer_type.type);
            if (underlying_type == NULL) return false;
            return true;
        }
        case SSA_Kind_Underlying_Type: {
            Type* ptr_type = ssa_evaluate_type(ssa->data.underlying_type.type);
            if (ptr_type == NULL) return false;
            if (ptr_type->kind != Type_Kind_Ptr) {
                log_msg_ssa("Can only evaluate underlying type if type is a pointer", log_error, ssa);
                return false;
            }
            return true;
        }
        case SSA_Kind_Build: {
            SSA_Build* build = &ssa->data.build;
            SSA_Block* block = &build->block;
            return ssa_type_check_block(block);
        }
        case SSA_Kind_Function_Declaration:
        case SSA_Kind_Parameter_Type:
        case SSA_Kind_Argument_Type:
        case SSA_Kind_Call_Return_Type:
        case SSA_Kind_Return_Type:
        case SSA_Kind_Return:
        case SSA_Kind_Default_Value:
        case SSA_Kind_Argument:
        case SSA_Kind_Parameter:
        case SSA_Kind_Function_Type:
        case SSA_Kind_Type_Type:
        case SSA_Kind_Multi_Value:
        case SSA_Kind_Index_Multi_Value:
        case SSA_Kind_Int_Literal_Type:
        case SSA_Kind_Int_Literal:
        case SSA_Kind_Float_Literal_Type:
        case SSA_Kind_Float_Literal:
        case SSA_Kind_Int_Type:
        case SSA_Kind_Uint_Type:
        case SSA_Kind_Float_Type:
        case SSA_Kind_Void_Type:
        case SSA_Kind_Compile_To_LLVM_IR:
        case SSA_Kind_Call_Setup_Type: {
            return true;
        }
        case SSA_Kind_Invalid: {
            internal_compiler_error();
            return false;
        }
    }
}

bool ssa_type_check(SSA* ssa) {
    bool res = false;
    if (ssa_has_been_type_checked(ssa, &res)) return res;
    res = _ssa_type_check(ssa);

    // Cache value
    SSA_Has_Been_Type_Checked has_been_type_checked = {0};
    has_been_type_checked.function_context = ssa_get_function_context();
    has_been_type_checked.type_check_result = res;
    ptr_append(ssa->has_been_type_checked, ssa->has_been_type_checked_count, ssa->has_been_type_checked_capacity, has_been_type_checked);

    if (res == true) {
        log_msg_ssa("Successfully type checked", log_debug, ssa);
    } else {
        log_msg_ssa("Failed to type check", log_info, ssa);
    }

    return res;
}

bool ssa_type_check_speculative(SSA* ssa) {
    u32 log_location = log_end_location();
    bool res = ssa_type_check(ssa);
    if (res == false && log_has_msg_after(log_location)) {
        log_clear_after(log_location);
        Log* new_logs = context.logs + log_location;
        u32 new_logs_count = context.logs_count - log_location;
        Log* new_logs_memory = alloc(sizeof(Log) * new_logs_count);
        memcpy(new_logs_memory, new_logs, sizeof(Log) * new_logs_count);
        new_logs = new_logs_memory;

        for (u32 i = 0; i < ssa->has_been_type_checked_count; i++) {
            SSA_Has_Been_Type_Checked* has_been_type_checked = ssa->has_been_type_checked + i;
            if (has_been_type_checked->function_context == ssa_get_function_context()) {
                has_been_type_checked->displayed_logs = false;
                has_been_type_checked->log = new_logs;
                has_been_type_checked->log_count = new_logs_count;
                break;
            }
        }
        log_clear_after(log_location);
    }
    return res;
}

SSA* ssa_ast_to_ssa(Ast* ast, SSA_Block* block) {
    switch (ast->kind) {
        case Ast_Kind_Function_Declaration: {
            SSA* function_declaration_ast_prototype = ssa_function_declaration_ast_prototype(ast, block);
            ssa_function_declaration_ast_implement(function_declaration_ast_prototype, block);
            return function_declaration_ast_prototype;
        }
        case Ast_Kind_Int: {
            SSA* int_literal_type = ssa_int_literal_type();
            SSA* memory = ssa_stack_alloc(int_literal_type, block, ast);
            SSA* int_literal = ssa_int_literal(ast->data.int_.value, block, ast);
            SSA* store = ssa_store(int_literal, memory, block, ast);
            return memory;
        }
        case Ast_Kind_Float: {
            SSA* float_literal_type = ssa_float_literal_type();
            SSA* memory = ssa_stack_alloc(float_literal_type, block, ast);
            SSA* float_literal = ssa_float_literal(ast->data.float_.value, block, ast);
            SSA* store = ssa_store(float_literal, memory, block, ast);
            return memory;
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
                case Ast_Kind_Intrinsic: {
                    declaration_ssa = ssa_ast_to_ssa(declaration, block);
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
                SSA* return_value_ssa = ssa_ast_to_ssa(return_value, block);
                SSA* load = ssa_load(return_value_ssa, block, return_value);

                SSA* value_return_type = NULL;
                if (return_values_count == 1) {
                    value_return_type = return_type;
                } else {
                    value_return_type = ssa_index_multi_value(return_type, i, block, return_value);
                }
                SSA* implicit_cast = ssa_implicit_cast(load, value_return_type, block, return_value);
                return_values[i] = implicit_cast;
            }

            if (return_values_count == 1) {
                return ssa_return(return_values[0], block, ast);
            }
            SSA* multi_value = ssa_multi_value(return_values, return_values_count, block, ast);
            return ssa_return(multi_value, block, ast);
        }
        case Ast_Kind_Variable_Declaration: {
            SSA* type = ssa_ast_to_ssa(ast->data.variable_declaration.type, block);
            SSA* variable = ssa_stack_alloc(type, block, ast);
            ast->data.variable_declaration.value = variable;
            SSA* default_value = ssa_default_value(type, block, ast);
            SSA* store = ssa_store(default_value, variable, block, ast);
            return variable;
        }
        case Ast_Kind_Call: {
            Ast* callee = ast->data.call.callee;
            SSA* callee_ssa = ssa_ast_to_ssa(callee, block);
            SSA* loaded_callee = ssa_load(callee_ssa, block, callee);

            Ast* argument_list = ast->data.call.argument_list;
            u32 arguments_count = argument_list->data.argument_list.arguments_count;

            SSA** arguments = alloc(sizeof(SSA*) * arguments_count);
            for (u32 i = 0; i < arguments_count; i++) {
                Ast* argument = argument_list->data.argument_list.arguments + i;
                SSA* argument_ssa = ssa_ast_to_ssa(argument, block);
                SSA* loaded_argument = ssa_load(argument_ssa, block, argument);
                arguments[i] = loaded_argument;
            }

            SSA* call_setup = ssa_call_setup(loaded_callee, arguments, arguments_count, block, ast);
            SSA* call = ssa_call(call_setup, block, ast);

            SSA* return_type = call->type;
            SSA* memory = ssa_stack_alloc(return_type, block, ast);
            SSA* store = ssa_store(call, memory, block, ast);
            return memory;
        }
        case Ast_Kind_Build: {
            SSA* build_ast_prototype = ssa_build_ast_prototype(ast, block);
            ssa_build_ast_implement(build_ast_prototype, block);
            return build_ast_prototype;
        }
        case Ast_Kind_Intrinsic: {
            SSA* value = ssa_ast_intrinsic(ast->data.intrinsic);
            SSA* value_type = value->type;
            SSA* memory = ssa_stack_alloc(value_type, block, ast);
            SSA* store = ssa_store(value, memory, block, ast);
            return memory;
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

SSA* ssa_function_declaration_ast_prototype(Ast* ast, SSA_Block* block) {
    SSA* function_type = ssa_function_type();
    SSA* function_memory = ssa_stack_alloc(function_type, block, ast);
    SSA* function_declaration = ssa_function_declaration(block, ast);
    SSA* store = ssa_store(function_declaration, function_memory, block, ast);
    ast->data.function_declaration.value = function_memory;

    Function* function = &function_declaration->data.function_declaration.function;

    function->kind = Function_Kind_Internal;
    function->name = ast->data.function_declaration.name;
    function->setup_block.kind = SSA_Block_Kind_Function_Setup;

    Ast* parameter_list = ast->data.function_declaration.parameter_list;
    u32 parameter_types_count = parameter_list->data.parameter_list.parameters_count;

    SSA** parameter_types = alloc(sizeof(SSA*) * parameter_types_count);
    SSA** parameters = alloc(sizeof(SSA*) * parameter_types_count);
    for (u32 i = 0; i < parameter_types_count; i++) {
        Ast* parameter = parameter_list->data.parameter_list.parameter_semantic_parse_order[i];
        u32 parameter_index = parameter - parameter_list->data.parameter_list.parameters;
        assert(parameter->kind == Ast_Kind_Parameter);
        Ast* type = parameter->data.parameter.type;
        SSA* type_ssa = ssa_ast_to_ssa(type, &function->setup_block);
        SSA* loaded_type_ssa = ssa_load(type_ssa, &function->setup_block, type);
        parameter_types[parameter_index] = loaded_type_ssa;
        SSA* argument = ssa_argument(parameter_index, &function->setup_block, parameter);
        SSA* casted_argument = ssa_implicit_cast(argument, loaded_type_ssa, &function->setup_block, parameter);
        parameter->data.parameter.value = casted_argument;
        parameters[parameter_index] = casted_argument;
    }
    function->parameters = parameters;
    function->parameter_types = parameter_types;
    function->parameter_types_count = parameter_types_count;

    for (u32 i = 0; i < parameter_types_count; i++) {
        Ast* parameter = parameter_list->data.parameter_list.parameters + i;
        SSA* argument = parameter->data.parameter.value;
        SSA* parameter_type = parameter_types[i];
        ssa_implicit_cast(argument, parameter_type, &function->setup_block, parameter);
    }

    u32 return_types_count = ast->data.function_declaration.return_types_count;
    SSA** return_types = alloc(sizeof(SSA*) * return_types_count);
    for (u32 i = 0; i < return_types_count; i++) {
        Ast* return_type = ast->data.function_declaration.return_types + i;
        SSA* return_type_ssa = ssa_ast_to_ssa(return_type, &function->setup_block);
        SSA* loaded_type_ssa = ssa_load(return_type_ssa, &function->setup_block, return_type);
        return_types[i] = loaded_type_ssa;
    }
    SSA* return_type = NULL;
    if (return_types_count == 1) {
        return_type = return_types[0];
    } else {
        return_type = ssa_multi_value(return_types, return_types_count, &function->setup_block, NULL);
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
        parameter->data.parameter.value = ssa_parameter(i, body_block, parameter);
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
    build_body->kind = SSA_Block_Kind_Scope;

    SSA_Block* global_block = &context.global_block;
    ptr_append(build_body->branchs_to_this_block, build_body->branchs_to_this_block_count, build_body->branchs_to_this_block_capacity, global_block);

    ssa_build_scope(scope, build_body);
}

SSA* ssa_stack_alloc(SSA* type, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Stack_Alloc;
    ssa.ast = ast;
    ssa.data.stack_alloc.type = type;
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
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_load(SSA* address, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Load;
    ssa.ast = ast;
    ssa.data.load.address = address;
    ssa.type = ssa_underlying_type(address->type, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_multi_value(SSA** values, u32 values_count, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Multi_Value;
    ssa.ast = ast;
    ssa.data.multi_value.values = values;
    ssa.data.multi_value.values_count = values_count;

    SSA** types = alloc(sizeof(SSA*) * values_count);
    for (u32 i = 0; i < values_count; i++) {
        SSA* value = values[i];
        types[i] = value->type;
    }
    ssa.type = ssa_multi_type(types, values_count, block, ast);

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

SSA* ssa_index_multi_value(SSA* multi_value, u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Index_Multi_Value;
    ssa.ast = ast;
    ssa.data.index_multi_value.multi_value = multi_value;
    ssa.data.index_multi_value.index = index;
    ssa.type = ssa_index_multi_type(multi_value->type, index, block, ast);
    return ssa_add_to_block(ssa, block);
}

SSA* ssa_index_multi_type(SSA* multi_type, u32 index, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Index_Multi_Type;
    ssa.ast = ast;
    ssa.data.index_multi_type.multi_type = multi_type;
    ssa.data.index_multi_type.index = index;
    ssa.type = ssa_type_type();
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

SSA* ssa_multi_type(SSA** types, u32 types_count, SSA_Block* block, Ast* ast) {
    SSA ssa = {0};
    ssa.kind = SSA_Kind_Multi_Type;
    ssa.ast = ast;
    ssa.data.multi_type.types = types;
    ssa.data.multi_type.types_count = types_count;
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

SSA* ssa_ast_intrinsic(Ast_Intrinsic intrinsic) {
    switch (intrinsic) {
        case Ast_Intrinsic_Int_Type: {
            return ssa_int_type();
        }
        case Ast_Intrinsic_Uint_Type: {
            return ssa_uint_type();
        }
        case Ast_Intrinsic_Float_Type: {
            return ssa_float_type();
        }
        case Ast_Intrinsic_Compile_To_LLVM_IR: {
            return ssa_compile_to_llvm_ir();
        }
        case Ast_Intrinsic_Type: {
            return ssa_type_type();
        }
        case Ast_Intrinsic_Function: {
            return ssa_function_type();
        }
        case Ast_Intrinsic_Void: {
            return ssa_void_type();
        }
        case Ast_Intrinsic_Int_Literal: {
            return ssa_int_literal_type();
        }
        case Ast_Intrinsic_Float_Literal: {
            return ssa_float_literal_type();
        }
        case Ast_Intrinsic_Invalid: {
            internal_compiler_error();
            return NULL;
        }
    }
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

void* ssa_interpreter_call_function() {
    alkfdj;
}

void ssa_interpreter_block(SSA_Block* block, SSA_Interpreter_Context* interpreter_context) {
    asldfkj;
}

void* ssa_interpreter_ssa(SSA* ssa, SSA_Interpreter_Context* interpreter_context) {
    asfjdk;
    if (ssa->kind == SSA_Kind_Load) {
        SSA* address = ssa->data.load.address;
        Pointer_Providence* providence = ssa_interpreter_ssa(address, interpreter_context);
        if (providence == NULL) return NULL;
        for (u32 i = 0; i < interpreter_context->value_map_count; i++) {
            SSA_Interpreter_Value_Map_Pair* pair = interpreter_context->value_map + i;
            if (pair->providence.providence == providence->providence) {
                return pair->value;
            }
        }
        return ssa_evaluate(ssa);
    } else if (ssa->kind == SSA_Kind_Store) {
        SSA* value_ssa = ssa->data.store.value;
        void* value = ssa_interpreter_ssa(value_ssa, interpreter_context);
        if (value == NULL) return NULL;
        SSA* address = ssa->data.store.address;
        Pointer_Providence* providence = ssa_interpreter_ssa(address, interpreter_context);
        if (providence == NULL) return NULL;

        SSA_Interpreter_Value_Map_Pair pair = {0};
        pair.providence = *providence;
        pair.value = value;
        ptr_append(interpreter_context->value_map, interpreter_context->value_map_count, interpreter_context->value_map_capacity, pair);
        return SSA_SUCCESS_VOID_VALUE;
    } else {
        return ssa_evaluate(ssa);
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

    utf8 str = {0};
    str.data = new_utf8_memory;
    str.count = 0;
    for (u32 i = 0; i < block_strings_count; i++) {
        utf8_append_with_capacity(&str, new_utf8_count, block_strings[i]);
        utf8_append_with_capacity(&str, new_utf8_count, utf8_str("\n"));
    }
    return str;
}

utf8 ssa_block_to_string(SSA_Block* block) {
    u32 buffer_capacity = 8096;
    char buffer[8096] = {0};
    utf8 buffer_utf8 = {0};
    buffer_utf8.data = buffer;
    buffer_utf8.count = 0;
    // char* buffer = alloc(buffer_capacity);

    utf8 block_name = ssa_get_ssa_block_name(block);
    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, block_name);
    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(":\n"));

    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* ssa = list->statements + j;
            utf8 ssa_name = ssa_get_ssa_name(ssa);
            utf8 ssa_type_name = ssa_get_ssa_name(ssa->type);
            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, ssa_type_name);

            u32 type_padding = 18;
            u32 current_padding = utf8_visual_len(ssa_type_name);
            for (u32 k = current_padding; k < type_padding; k++) {
                utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(" "));
            }

            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, ssa_name);
            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(": "));
            switch (ssa->kind) {
                case SSA_Kind_Invalid: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Invalid"));
                    break;
                }
                case SSA_Kind_Store: {
                    SSA* value = ssa->data.store.value;
                    utf8 value_name = ssa_get_ssa_name(value);
                    SSA* address = ssa->data.store.address;
                    utf8 address_name = ssa_get_ssa_name(address);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Store: (Value: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", Memory: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, address_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Load: {
                    SSA* address = ssa->data.load.address;
                    utf8 address_name = ssa_get_ssa_name(address);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Load: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, address_name);
                    break;
                }
                case SSA_Kind_Stack_Alloc: {
                    SSA* type = ssa->data.stack_alloc.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Stack_Alloc: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
                case SSA_Kind_Function_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Function_Type"));
                    break;
                }
                case SSA_Kind_Type_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Type_Type"));
                    break;
                }
                case SSA_Kind_Function_Declaration: {
                    utf8 function_name = ssa->data.function_declaration.function.name;

                    SSA_Block* setup_block = &ssa->data.function_declaration.function.setup_block;
                    utf8 setup_block_name = ssa_get_ssa_block_name(setup_block);

                    SSA_Block* function_body_block = &ssa->data.function_declaration.function.data.internal.body;
                    utf8 function_body_block_name = ssa_get_ssa_block_name(function_body_block);

                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Function_Declaration "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, function_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(": (Setup: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, setup_block_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", Body: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, function_body_block_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Multi_Value: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Multi_Value: "));
                    for (u32 i = 0; i < ssa->data.multi_value.values_count; i++) {
                        SSA* value = ssa->data.multi_value.values[i];
                        utf8 value_name = ssa_get_ssa_name(value);
                        utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_name);
                        if (i != ssa->data.multi_value.values_count - 1) {
                            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", "));
                        }
                    }
                    break;
                }
                case SSA_Kind_Index_Multi_Value: {
                    u32 index = ssa->data.index_multi_value.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Index_Multi_Value: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    break;
                }
                case SSA_Kind_Parameter: {
                    u32 index = ssa->data.parameter.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Parameter: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    break;
                }
                case SSA_Kind_Argument: {
                    u32 index = ssa->data.argument.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Argument: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    break;
                }
                case SSA_Kind_Int_Literal_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Int_Literal_Type"));
                    break;
                }
                case SSA_Kind_Int_Literal: {
                    Big_Int value = ssa->data.int_literal.value;
                    char value_buf[32];
                    snprintf(value_buf, 32, "%llu", value.data);
                    utf8 value_utf8 = {value_buf, strlen(value_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Int_Literal: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_utf8);
                    break;
                }
                case SSA_Kind_Float_Literal_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Float_Literal_Type"));
                    break;
                }
                case SSA_Kind_Float_Literal: {
                    f64 value = ssa->data.float_literal.value;
                    char value_buf[32];
                    snprintf(value_buf, 32, "%f", value);
                    utf8 value_utf8 = {value_buf, strlen(value_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Float_Literal: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_utf8);
                    break;
                }
                case SSA_Kind_Return: {
                    SSA* return_value = ssa->data.return_.return_value;
                    utf8 return_value_name = ssa_get_ssa_name(return_value);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Return: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, return_value_name);
                    break;
                }
                case SSA_Kind_Return_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Return_Type"));
                    break;
                }
                case SSA_Kind_Explicit_Cast: {
                    SSA* value = ssa->data.explicit_cast.value;
                    SSA* type = ssa->data.explicit_cast.type;
                    utf8 value_name = ssa_get_ssa_name(value);
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Explicit_Cast: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(" -> "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
                case SSA_Kind_Implicit_Cast: {
                    SSA* value = ssa->data.implicit_cast.value;
                    SSA* type = ssa->data.implicit_cast.type;
                    utf8 value_name = ssa_get_ssa_name(value);
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Implicit_Cast: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, value_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(" -> "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
                case SSA_Kind_Int_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Int_Type"));
                    break;
                }
                case SSA_Kind_Uint_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Uint_Type"));
                    break;
                }
                case SSA_Kind_Float_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Float_Type"));
                    break;
                }
                case SSA_Kind_Void_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Void_Type"));
                    break;
                }
                case SSA_Kind_Compile_To_LLVM_IR: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Compile_To_LLVM_IR"));
                    break;
                }
                case SSA_Kind_Build: {
                    SSA_Block* build_block = &ssa->data.build.block;
                    utf8 build_block_name = ssa_get_ssa_block_name(build_block);

                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Build("));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, build_block_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Call_Setup: {
                    SSA* callee = ssa->data.call_setup.callee;
                    utf8 callee_name = ssa_get_ssa_name(callee);
                    SSA** arguments = ssa->data.call_setup.arguments;
                    u32 arguments_count = ssa->data.call_setup.arguments_count;
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Call_Setup "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, callee_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("("));
                    for (u32 i = 0; i < arguments_count; i++) {
                        SSA* argument = arguments[i];
                        utf8 argument_name = ssa_get_ssa_name(argument);
                        utf8_append_with_capacity(&buffer_utf8, buffer_capacity, argument_name);
                        if (i != arguments_count - 1) {
                            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", "));
                        }
                    }
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Call: {
                    SSA* setup = ssa->data.call.setup;
                    utf8 setup_name = ssa_get_ssa_name(setup);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Call: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, setup_name);
                    break;
                }
                case SSA_Kind_Multi_Type: {
                    SSA** types = ssa->data.multi_type.types;
                    u32 types_count = ssa->data.multi_type.types_count;
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Multi_Type("));
                    for (u32 i = 0; i < types_count; i++) {
                        SSA* type = types[i];
                        utf8 type_name = ssa_get_ssa_name(type);
                        utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                        if (i != types_count - 1) {
                            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", "));
                        }
                    }
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Index_Multi_Type: {
                    SSA* multi_type = ssa->data.index_multi_type.multi_type;
                    u32 index = ssa->data.index_multi_type.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Index_Multi_Type(Multi_Type:"));
                    utf8 multi_type_name = ssa_get_ssa_name(multi_type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, multi_type_name);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(", "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Index: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str(")"));
                    break;
                }
                case SSA_Kind_Parameter_Type: {
                    u32 index = ssa->data.parameter_type.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Parameter_Type: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    break;
                }
                case SSA_Kind_Argument_Type: {
                    u32 index = ssa->data.argument_type.index;
                    char index_buf[32];
                    snprintf(index_buf, 32, "%u", index);
                    utf8 index_utf8 = {index_buf, strlen(index_buf)};
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Argument_Type: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, index_utf8);
                    break;
                }
                case SSA_Kind_Call_Setup_Type: {
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Call_Setup_Type"));
                    break;
                }
                case SSA_Kind_Call_Return_Type: {
                    SSA* setup = ssa->data.call_return_type.setup;
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Call_Return_Type: "));
                    utf8 setup_name = ssa_get_ssa_name(setup);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, setup_name);
                    break;
                }
                case SSA_Kind_Pointer_Type: {
                    SSA* type = ssa->data.pointer_type.type;
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Pointer_Type: "));
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
                case SSA_Kind_Underlying_Type: {
                    SSA* type = ssa->data.underlying_type.type;
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Underlying_Type: "));
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
                case SSA_Kind_Default_Value: {
                    SSA* type = ssa->data.default_value.type;
                    utf8 type_name = ssa_get_ssa_name(type);
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("Default_Value: "));
                    utf8_append_with_capacity(&buffer_utf8, buffer_capacity, type_name);
                    break;
                }
            }

            utf8_append_with_capacity(&buffer_utf8, buffer_capacity, utf8_str("\n"));
        }
    }

    char* memory = alloc(buffer_utf8.count + 1);
    memcpy(memory, buffer_utf8.data, buffer_utf8.count + 1);

    utf8 str = {0};
    str.data = memory;
    str.count = buffer_utf8.count;
    return str;
}

void ssa_init_intrinsic_block() {
    SSA_Block* block = &context.intrinsic_ssa_block.block;

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
        int_type_function->parameter_types_count = 2;
        int_type_function->parameter_types = alloc(sizeof(SSA*) * 2);
        int_type_function->parameters = alloc(sizeof(SSA*) * 2);
        int_type_function->parameter_types[1] = ssa_type_type();
        SSA* argument_1 = ssa_argument(1, &int_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, int_type_function->parameter_types[1], &int_type_function->setup_block, NULL);
        int_type_function->parameters[1] = casted_argument_1;
        int_type_function->parameter_types[0] = argument_1;
        SSA* argument_0 = ssa_argument(0, &int_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, int_type_function->parameter_types[0], &int_type_function->setup_block, NULL);
        int_type_function->parameters[0] = casted_argument_0;
        int_type_function->return_type = ssa_type_type();
        context.intrinsic_ssa_block.int_type_value = int_type_function;
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
        uint_type_function->parameter_types_count = 2;
        uint_type_function->parameter_types = alloc(sizeof(SSA*) * 2);
        uint_type_function->parameters = alloc(sizeof(SSA*) * 2);
        uint_type_function->parameter_types[1] = ssa_type_type();
        SSA* argument_1 = ssa_argument(1, &uint_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, uint_type_function->parameter_types[1], &uint_type_function->setup_block, NULL);
        uint_type_function->parameters[1] = casted_argument_1;
        uint_type_function->parameter_types[0] = argument_1;
        SSA* argument_0 = ssa_argument(0, &uint_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, uint_type_function->parameter_types[0], &uint_type_function->setup_block, NULL);
        uint_type_function->parameters[0] = casted_argument_0;
        uint_type_function->return_type = ssa_type_type();
        context.intrinsic_ssa_block.uint_type_value = uint_type_function;
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
        float_type_function->parameter_types_count = 2;
        float_type_function->parameter_types = alloc(sizeof(SSA*) * 2);
        float_type_function->parameters = alloc(sizeof(SSA*) * 2);
        SSA* argument_1 = ssa_argument(1, &float_type_function->setup_block, NULL);
        SSA* casted_argument_1 = ssa_implicit_cast(argument_1, float_type_function->parameter_types[1], &float_type_function->setup_block, NULL);
        float_type_function->parameters[1] = casted_argument_1;
        float_type_function->parameter_types[0] = argument_1;
        SSA* argument_0 = ssa_argument(0, &float_type_function->setup_block, NULL);
        SSA* casted_argument_0 = ssa_implicit_cast(argument_0, float_type_function->parameter_types[0], &float_type_function->setup_block, NULL);
        float_type_function->parameters[0] = casted_argument_0;
        float_type_function->return_type = ssa_type_type();
        context.intrinsic_ssa_block.float_type_value = float_type_function;
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
        compile_to_llvm_ir_function->parameter_types_count = 1;
        compile_to_llvm_ir_function->parameter_types = alloc(sizeof(SSA*) * 1);
        compile_to_llvm_ir_function->parameters = alloc(sizeof(SSA*) * 1);
        compile_to_llvm_ir_function->parameter_types[0] = ssa_function_type();
        SSA* argument_0 = ssa_argument(0, &compile_to_llvm_ir_function->setup_block, NULL);
        SSA* casted_argument_0 =
            ssa_implicit_cast(argument_0, compile_to_llvm_ir_function->parameter_types[0], &compile_to_llvm_ir_function->setup_block, NULL);
        compile_to_llvm_ir_function->parameters[0] = casted_argument_0;
        compile_to_llvm_ir_function->return_type = ssa_void_type();
        context.intrinsic_ssa_block.compile_to_llvm_ir_value = compile_to_llvm_ir_function;
    }
}

Function_Context* ssa_get_function_context() {
    return context.ssa_evaluation_context->function_context_stack[context.ssa_evaluation_context->function_context_stack_count - 1];
}

Function_Context* ssa_pop_function_context() {
    Function_Context* function_context = ssa_get_function_context();
    context.ssa_evaluation_context->function_context_stack_count--;
    return function_context;
}

void ssa_push_function_context(Function_Context* function_context) {
    ptr_append(context.ssa_evaluation_context->function_context_stack, context.ssa_evaluation_context->function_context_stack_count,
               context.ssa_evaluation_context->function_context_stack_capacity, function_context);
}

void ssa_copy_evaluation_context_into_other_context(SSA_Evaluation_Context* evaluation_context, SSA_Evaluation_Context* other_evaluation_context) {
    other_evaluation_context->function_context_stack_count = 0;
    for (u32 i = 0; i < evaluation_context->function_context_stack_count; i++) {
        Function_Context* function_context = evaluation_context->function_context_stack[i];
        ptr_append(other_evaluation_context->function_context_stack, other_evaluation_context->function_context_stack_count,
                   other_evaluation_context->function_context_stack_capacity, function_context);
    }
}

bool ssa_compile_time_value_equal(void* value1, void* value2, Type* type) {
    if (value1 == NULL && value2 == NULL) return true;
    switch (type->kind) {
        case Type_Kind_Int_Literal: {
            Big_Int* int1 = value1;
            Big_Int* int2 = value2;
            return int1->data == int2->data;
        }
        case Type_Kind_Float_Literal: {
            f64* float1 = value1;
            f64* float2 = value2;
            f64 diff = fabs(*float1 - *float2);
            return diff < 0.00000001;
        }
        case Type_Kind_Type: {
            return ssa_type_equal(value1, value2);
        }
        case Type_Kind_Void:
        case Type_Kind_Invalid: {
            return true;
        }
        case Type_Kind_Multi:
        case Type_Kind_Ptr:
        case Type_Kind_Uint:
        case Type_Kind_Float:
        case Type_Kind_Int: {
            assert(false);
            return false;
        }
        case Type_Kind_Call_Setup:
        case Type_Kind_Function: {
            return value1 == value2;
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
        case Type_Kind_Multi: {
            assert(false);
            return 0;
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
            return 0;
        }
    }
}

bool ssa_can_explicit_cast(Type* type, Type* cast_type) {
    if (ssa_type_equal(type, cast_type)) return true;
    if (type->kind == Type_Kind_Ptr && cast_type->kind == Type_Kind_Ptr) return true;
    if (ssa_type_math_type(type) && ssa_type_math_type(cast_type)) return true;
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

void* ssa_get_type_default_value(Type* type) {
    switch (type->kind) {
        case Type_Kind_Invalid:
            return NULL;
        case Type_Kind_Type:
            return context.intrinsic_ssa_block.type_type_value;
        case Type_Kind_Int: {
            u32 bits = type->data.int_.bits;
            u32 bytes = (bits + 7) / 8;
            void* memory = alloc(bytes);
            memset(memory, 0, bytes);
            return memory;
        }
        case Type_Kind_Int_Literal: {
            Big_Int* big_int = alloc(sizeof(Big_Int));
            big_int->data = 0;
            return big_int;
        }
        case Type_Kind_Uint: {
            u32 bits = type->data.uint.bits;
            u32 bytes = (bits + 7) / 8;
            void* memory = alloc(bytes);
            memset(memory, 0, bytes);
            return memory;
        }
        case Type_Kind_Float: {
            u32 bits = type->data.int_.bits;
            u32 bytes = (bits + 7) / 8;
            void* memory = alloc(bytes);
            memset(memory, 0, bytes);
            return memory;
        }
        case Type_Kind_Float_Literal: {
            f64* float_ = alloc(sizeof(f64));
            *float_ = 0;
            return float_;
        }
        case Type_Kind_Void: {
            return SSA_SUCCESS_VOID_VALUE;
        }
        case Type_Kind_Function: {
            Function* function = alloc(sizeof(Function));
            return function;
        }
        case Type_Kind_Ptr: {
            return NULL;
        }
        case Type_Kind_Multi: {
            assert(false);
            return NULL;
        }
        case Type_Kind_Call_Setup: {
            SSA_Call_Setup* call_setup = alloc(sizeof(SSA_Call_Setup));
            return call_setup;
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
        case Type_Kind_Invalid:
        case Type_Kind_Type:
        case Type_Kind_Int_Literal:
        case Type_Kind_Float_Literal:
        case Type_Kind_Void:
        case Type_Kind_Function:
        case Type_Kind_Multi:
        case Type_Kind_Call_Setup:
            return true;
    }
}

bool ssa_type_math_type(Type* type) {
    return type->kind == Type_Kind_Int || type->kind == Type_Kind_Uint || type->kind == Type_Kind_Float || type->kind == Type_Kind_Float_Literal ||
           type->kind == Type_Kind_Int_Literal;
}

bool ssa_has_been_evaluated(SSA* ssa, SSA_Possible_Values* out_possible_values) {
    for (u32 i = 0; i < ssa->compile_time_value_count; i++) {
        SSA_Compile_Time_Value* compile_time_value = ssa->compile_time_value + i;

        Function_Context* ssa_function_context = compile_time_value->function_context;
        Function_Context* context_function_context = ssa_get_function_context();
        bool function_contexts_match = ssa_function_context == context_function_context;

        if (function_contexts_match) {
            if (compile_time_value->displayed_logs == false) {
                compile_time_value->displayed_logs = true;
                for (u32 i = 0; i < compile_time_value->log_count; i++) {
                    Log* log = compile_time_value->log + i;
                    log_append(log);
                }
            }

            *out_possible_values = compile_time_value->possible_values;
            return true;
        }
    }
    return false;
}

bool ssa_has_been_type_checked(SSA* ssa, bool* type_check_result) {
    for (u32 i = 0; i < ssa->has_been_type_checked_count; i++) {
        SSA_Has_Been_Type_Checked* has_been_type_checked = &ssa->has_been_type_checked[i];

        Function_Context* val_function_context = has_been_type_checked->function_context;
        Function_Context* context_function_context = ssa_get_function_context();

        bool function_contexts_match = val_function_context == context_function_context;
        if (function_contexts_match) {
            if (has_been_type_checked->displayed_logs == false) {
                has_been_type_checked->displayed_logs = true;
                for (u32 i = 0; i < has_been_type_checked->log_count; i++) {
                    Log* log = has_been_type_checked->log + i;
                    log_append(log);
                }
            }
            *type_check_result = has_been_type_checked->type_check_result;
            return true;
        }
    }
    return false;
}

SSA_Possible_Values ssa_possible_single_value(void* value) {
    SSA_Possible_Values values = {0};
    values.values = alloc(sizeof(void*));
    values.values_count = 1;
    values.values_capacity = 1;
    values.values[0] = value;
    return values;
}

void ssa_possible_providence_values_consolidate(SSA_Possible_Providence_Values* values, Type* type, i64 value_size) {
    for (u32 i = 0; i < values->values_count; i++) {
        SSA_Possible_Providence_Value* value = &values->values[i];
        void* value_ptr = value->value;
        for (u32 j = i + 1; j < values->values_count; j++) {
            SSA_Possible_Providence_Value* value2 = &values->values[j];
            void* value2_ptr = value2->value;

            bool value_ptr_match = ssa_compile_time_value_equal(value_ptr, value2_ptr, type);
            bool filled_same_bytes_same = true;
            for (i64 k = 0; k < value_size; k++) {
                if (value->bytes_filled[k] != value2->bytes_filled[k]) filled_same_bytes_same = false;
            }

            if (value_ptr_match && filled_same_bytes_same) {
                values->values_count--;
                values->values[j] = values->values[values->values_count];
                j--;
            }
        }
    }
}

void ssa_possible_values_consolidate(SSA_Possible_Values* values, Type* type) {
    if (type->kind == Type_Kind_Ptr) {
        for (u32 i = 0; i < values->values_count; i++) {
            void* value = values->values[i];
            if (value == NULL) continue;
            Pointer_Providence* providence = value;
            if (providence->providence == 0) {
                values->values[i] = NULL;
            }
        }
    }

    for (u32 i = 0; i < values->values_count; i++) {
        void* value = values->values[i];
        for (u32 j = i + 1; j < values->values_count; j++) {
            void* value2 = values->values[j];
            if (ssa_compile_time_value_equal(value, value2, type)) {
                values->values_count--;
                values->values[j] = values->values[values->values_count];
                j--;
            }
        }
    }

    if (values->values_count == 1) {
        void* value = values->values[0];
        if (value == NULL) {
            values->values_count = 0;
        }
    }
}

bool ssa_possible_providence_value_full(SSA_Possible_Providence_Value value, i64 value_size) {
    if (value_size == 0) {
        return value.value != NULL;
    }
    for (i64 i = 0; i < value_size; i++) {
        if (value.bytes_filled[i] == 0) return false;
    }
    return true;
}

SSA_Possible_Providence_Value ssa_copy_possible_providence_value(SSA_Possible_Providence_Value value, i64 value_size) {
    if (value_size == 0) return value;
    SSA_Possible_Providence_Value new_value = {0};
    new_value.value = alloc(value_size);
    memcpy(new_value.value, value.value, value_size);
    new_value.bytes_filled = alloc(value_size);
    memcpy(new_value.bytes_filled, value.bytes_filled, value_size);
    return new_value;
}

bool ssa_in_providence_and_will_change(Pointer_Providence* base_providence, i64 base_value_size, Pointer_Providence* address_providence, i64 address_value_size,
                                       SSA_Possible_Providence_Values* values) {
    if (base_providence->providence == 0 || address_providence->providence == 0) return false;
    if (base_value_size == 0) {
        if (address_value_size == 0) {
            if (base_providence->providence == address_providence->providence) {
                return true;
            }
        }
        return false;
    }
    i64 providence_offset = address_providence->providence - base_providence->providence;
    if (providence_offset >= base_value_size) return false;
    if (providence_offset < 0) {
        address_value_size += providence_offset;
        providence_offset = 0;
        if (address_value_size <= 0) return false;
    }

    for (u32 i = 0; i < values->values_count; i++) {
        SSA_Possible_Providence_Value* value = &values->values[i];
        for (i64 i = providence_offset; i < base_value_size; i++) {
            if (value->bytes_filled[i] == 0) return true;
        }
    }

    return false;
}

SSA_Possible_Providence_Value ssa_create_possible_providence_value(Pointer_Providence* base_providence, i64 base_value_size,
                                                                   Pointer_Providence* address_providence, i64 address_value_size, void* address_value) {
    void* base_address_value = address_value;
    if (base_providence->providence == 0 || address_providence->providence == 0) return (SSA_Possible_Providence_Value){0};
    if (base_value_size == 0) {
        if (address_value_size == 0) {
            if (base_providence->providence == address_providence->providence) {
                SSA_Possible_Providence_Value value = {0};
                value.value = address_value;
                value.bytes_filled = NULL;
                return value;
            }
        }
        return (SSA_Possible_Providence_Value){0};
    }
    i64 providence_offset = address_providence->providence - base_providence->providence;
    if (providence_offset >= base_value_size) return (SSA_Possible_Providence_Value){0};
    if (providence_offset < 0) {
        address_value = ((char*)address_value) - providence_offset;
        address_value_size += providence_offset;
        providence_offset = 0;
        if (address_value_size <= 0) return (SSA_Possible_Providence_Value){0};
    }

    void* value = NULL;
    u8* bytes_filled = alloc(base_value_size);
    if (base_address_value == NULL) {
        for (i64 i = 0; i < base_value_size; i++) {
            bytes_filled[i] = 1;
        }
    } else {
        value = alloc(base_value_size);
        for (i64 i = providence_offset; i < base_value_size; i++) {
            bytes_filled[i] = 1;
            ((char*)value)[i] = ((char*)address_value)[i];
        }
    }

    SSA_Possible_Providence_Value possible_providence_value = {0};
    possible_providence_value.value = value;
    possible_providence_value.bytes_filled = bytes_filled;
    return possible_providence_value;
}

SSA_Block_Location ssa_get_location(SSA* ssa) {
    SSA_Block* block = ssa->block;
    for (u32 i = 0; i < block->statement_lists_count; i++) {
        SSA_List* list = block->statement_lists + i;
        for (u32 j = 0; j < list->statements_count; j++) {
            SSA* statement = list->statements + j;
            if (statement == ssa) {
                SSA_Block_Location location = {i, j};
                return location;
            }
        }
    }
    internal_compiler_error();
    return (SSA_Block_Location){0};
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
        case SSA_Kind_Multi_Value:
            return utf8_str("SSA_Kind_Multi_Value");
        case SSA_Kind_Index_Multi_Value:
            return utf8_str("SSA_Kind_Index_Multi_Value");
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
        case SSA_Kind_Multi_Type:
            return utf8_str("SSA_Kind_Multi_Type");
        case SSA_Kind_Index_Multi_Type:
            return utf8_str("SSA_Kind_Index_Multi_Type");
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
    }
}
