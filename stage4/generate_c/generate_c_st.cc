/*
 *  matiec - a compiler for the programming languages defined in IEC 61131-3
 *
 *  Copyright (C) 2003-2011  Mario de Sousa (msousa@fe.up.pt)
 *  Copyright (C) 2007-2011  Laurent Bessard and Edouard Tisserant
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This code is made available on the understanding that it will not be
 * used in safety-critical situations without a full and competent review.
 *
 * Based on the
 * FINAL DRAFT - IEC 61131-3, 2nd Ed. (2001-12-10)
 *
 */


/*
 * Conversion of st statements (i.e. ST code).
 *
 * This is part of the 4th stage that generates
 * a C source program equivalent to the IL and ST, or SFC
 * code.
 */



#include "../../util/strdup.hh"

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/


class generate_c_st_c: public generate_c_typedecl_c {

  public:
    typedef enum {
      expression_vg,
      assignment_vg,
      complextype_base_vg,
      complextype_base_assignment_vg,
      complextype_suffix_vg,
      fparam_output_vg
    } variablegeneration_t;

    typedef enum {
      single_cg,
      subrange_cg,
      none_cg
    } casegeneration_t;

  private:
    /* When calling a function block, we must first find it's type,
     * by searching through the declarations of the variables currently
     * in scope.
     * This class does just that...
     * A new class is instantiated whenever we begin generating the code
     * for a function block type declaration, or a program declaration.
     * This object instance will then later be called while the
     * function block's or the program's body is being handled.
     *
     * Note that functions cannot contain calls to function blocks,
     * so we do not create an object instance when handling
     * a function declaration.
     */
    search_fb_instance_decl_c *search_fb_instance_decl;

    search_varfb_instance_type_c *search_varfb_instance_type;
    search_var_instance_decl_c   *search_var_instance_decl;

    symbol_c* current_array_type;
    symbol_c* current_param_type;

    int fcall_number;
    symbol_c *fbname;

    bool first_subrange_case_list;

    variablegeneration_t wanted_variablegeneration;
    casegeneration_t wanted_casegeneration;

  public:
    generate_c_st_c(stage4out_c *s4o_ptr, symbol_c *name, symbol_c *scope, const char *variable_prefix = NULL)
    : generate_c_typedecl_c(s4o_ptr) {
      search_fb_instance_decl    = new search_fb_instance_decl_c   (scope);
      search_varfb_instance_type = new search_varfb_instance_type_c(scope);
      search_var_instance_decl   = new search_var_instance_decl_c  (scope);
      
      this->set_variable_prefix(variable_prefix);
      current_array_type = NULL;
      current_param_type = NULL;
      fcall_number = 0;
      fbname = name;
      wanted_variablegeneration = expression_vg;
      wanted_casegeneration = none_cg;
    }

    virtual ~generate_c_st_c(void) {
      delete search_fb_instance_decl;
      delete search_varfb_instance_type;
      delete search_var_instance_decl;
    }


  public:
    void generate(statement_list_c *stl) {
      stl->accept(*this);
    }

  private:
    
    




void *print_getter(symbol_c *symbol) {
  unsigned int vartype = search_var_instance_decl->get_vartype(symbol);
  if (wanted_variablegeneration == fparam_output_vg) {
    if (vartype == search_var_instance_decl_c::external_vt) {
      if (search_var_instance_decl->type_is_fb(symbol))
        s4o.print(GET_EXTERNAL_FB_BY_REF);
      else
        s4o.print(GET_EXTERNAL_BY_REF);
    }
    else if (vartype == search_var_instance_decl_c::located_vt)
      s4o.print(GET_LOCATED_BY_REF);
    else
      s4o.print(GET_VAR_BY_REF);
  }
  else {
    if (vartype == search_var_instance_decl_c::external_vt) {
      if (search_var_instance_decl->type_is_fb(symbol))
        s4o.print(GET_EXTERNAL_FB);
      else
        s4o.print(GET_EXTERNAL);
    }
    else if (vartype == search_var_instance_decl_c::located_vt)
      s4o.print(GET_LOCATED);
    else
      s4o.print(GET_VAR);
  }
  s4o.print("(");

  variablegeneration_t old_wanted_variablegeneration = wanted_variablegeneration;
  wanted_variablegeneration = complextype_base_vg;
  symbol->accept(*this);
  if (search_var_instance_decl->type_is_complex(symbol))
    s4o.print(",");
  wanted_variablegeneration = complextype_suffix_vg;
  symbol->accept(*this);
  s4o.print(")");
  wanted_variablegeneration = old_wanted_variablegeneration;
  return NULL;
}

void *print_setter(symbol_c* symbol,
        symbol_c* type,
        symbol_c* value,
        symbol_c* fb_symbol = NULL,
        symbol_c* fb_value = NULL) {
  
  bool type_is_complex = false;
  if (fb_symbol == NULL) {
    unsigned int vartype = search_var_instance_decl->get_vartype(symbol);
    type_is_complex = search_var_instance_decl->type_is_complex(symbol);
    if (vartype == search_var_instance_decl_c::external_vt) {
      if (search_var_instance_decl->type_is_fb(symbol))
        s4o.print(SET_EXTERNAL_FB);
      else
        s4o.print(SET_EXTERNAL);
    }
    else if (vartype == search_var_instance_decl_c::located_vt)
      s4o.print(SET_LOCATED);
    else
      s4o.print(SET_VAR);
  }
  else {
    unsigned int vartype = search_var_instance_decl->get_vartype(fb_symbol);
    if (vartype == search_var_instance_decl_c::external_vt)
      s4o.print(SET_EXTERNAL_FB);
    else
      s4o.print(SET_VAR);
  }
  s4o.print("(");

  if (fb_symbol != NULL) {
    print_variable_prefix();
    fb_symbol->accept(*this);
    s4o.print(".,");
  }
  else if (type_is_complex)
    wanted_variablegeneration = complextype_base_assignment_vg;
  else
    wanted_variablegeneration = assignment_vg;
  
  symbol->accept(*this);
  s4o.print(",");
  wanted_variablegeneration = expression_vg;
  print_check_function(type, value, fb_value);
  if (type_is_complex) {
    s4o.print(",");
    wanted_variablegeneration = complextype_suffix_vg;
    symbol->accept(*this);
  }
  s4o.print(")");
  wanted_variablegeneration = expression_vg;
  return NULL;
}

/********************************/
/* B 1.3.3 - Derived data types */
/********************************/

/*  signed_integer DOTDOT signed_integer */
void *visit(subrange_c *symbol) {
  switch (wanted_casegeneration) {
    case subrange_cg:
      s4o.print("__case_expression >= ");
      symbol->lower_limit->accept(*this);
      s4o.print(" && __case_expression <= ");
      symbol->upper_limit->accept(*this);
      break;
    default:
      symbol->lower_limit->accept(*this);
      break;
  }
  return NULL;
}

/* ARRAY '[' array_subrange_list ']' OF non_generic_type_name */
void *visit(array_specification_c *symbol) {
  symbol->non_generic_type_name->accept(*this);
  return NULL;
}

/*  enumerated_type_name ':' enumerated_spec_init */
void *visit(enumerated_type_declaration_c *symbol) {
  symbol->enumerated_type_name->accept(*this);
  return NULL;
}

/*********************/
/* B 1.4 - Variables */
/*********************/
void *visit(symbolic_variable_c *symbol) {
  switch (wanted_variablegeneration) {
    case complextype_base_assignment_vg:
    case assignment_vg:
      this->print_variable_prefix();
      s4o.print(",");
      symbol->var_name->accept(*this);
      break;
    case complextype_base_vg:
      generate_c_base_c::visit(symbol);
      break;
    case complextype_suffix_vg:
      break;
    default:
      if (this->is_variable_prefix_null()) {
        if (wanted_variablegeneration == fparam_output_vg) {
          s4o.print("&(");
          generate_c_base_c::visit(symbol);
          s4o.print(")");
        }
        else {
          generate_c_base_c::visit(symbol);
        }
      }
      else
        print_getter(symbol);
      break;
  }
  return NULL;
}

/********************************************/
/* B.1.4.1   Directly Represented Variables */
/********************************************/
// direct_variable: direct_variable_token   {$$ = new direct_variable_c($1);};
void *visit(direct_variable_c *symbol) {
  TRACE("direct_variable_c");
  /* Do not use print_token() as it will change everything into uppercase */
  if (strlen(symbol->value) == 0) ERROR;
  if (this->is_variable_prefix_null()) {
    if (wanted_variablegeneration != fparam_output_vg)
      s4o.print("*(");
  }
  else {
    switch (wanted_variablegeneration) {
      case expression_vg:
        s4o.print(GET_LOCATED);
        s4o.print("(");
        break;
      case fparam_output_vg:
        s4o.print(GET_LOCATED_BY_REF);
        s4o.print("(");
        break;
      default:
        break;
    }
  }
  this->print_variable_prefix();
  s4o.printlocation(symbol->value + 1);
  if ((this->is_variable_prefix_null() && wanted_variablegeneration != fparam_output_vg) ||
      wanted_variablegeneration != assignment_vg)
    s4o.print(")");
  return NULL;
}

/*************************************/
/* B.1.4.2   Multi-element Variables */
/*************************************/

// SYM_REF2(structured_variable_c, record_variable, field_selector)
void *visit(structured_variable_c *symbol) {
  TRACE("structured_variable_c");
  bool type_is_complex = search_var_instance_decl->type_is_complex(symbol->record_variable);
  switch (wanted_variablegeneration) {
    case complextype_base_vg:
    case complextype_base_assignment_vg:
      symbol->record_variable->accept(*this);
      if (!type_is_complex) {
        s4o.print(".");
        symbol->field_selector->accept(*this);
      }
      break;
    case complextype_suffix_vg:
      symbol->record_variable->accept(*this);
      if (type_is_complex) {
        s4o.print(".");
        symbol->field_selector->accept(*this);
      }
      break;
    case assignment_vg:
      symbol->record_variable->accept(*this);
      s4o.print(".");
      symbol->field_selector->accept(*this);
      break;
    default:
      if (this->is_variable_prefix_null()) {
        symbol->record_variable->accept(*this);
        s4o.print(".");
        symbol->field_selector->accept(*this);
      }
      else
        print_getter(symbol);
      break;
  }
  return NULL;
}

/*  subscripted_variable '[' subscript_list ']' */
//SYM_REF2(array_variable_c, subscripted_variable, subscript_list)
void *visit(array_variable_c *symbol) {
  switch (wanted_variablegeneration) {
    case complextype_base_vg:
    case complextype_base_assignment_vg:
      symbol->subscripted_variable->accept(*this);
      break;
    case complextype_suffix_vg:
      symbol->subscripted_variable->accept(*this);

      current_array_type = search_varfb_instance_type->get_basetype_decl(symbol->subscripted_variable);
      if (current_array_type == NULL) ERROR;

      s4o.print(".table");
      wanted_variablegeneration = expression_vg;
      symbol->subscript_list->accept(*this);
      wanted_variablegeneration = complextype_suffix_vg;

      current_array_type = NULL;
      break;
    default:
      if (this->is_variable_prefix_null()) {
        symbol->subscripted_variable->accept(*this);

        current_array_type = search_varfb_instance_type->get_basetype_decl(symbol->subscripted_variable);
        if (current_array_type == NULL) ERROR;

        s4o.print(".table");
        symbol->subscript_list->accept(*this);

        current_array_type = NULL;
      }
      else
        print_getter(symbol);
      break;
  }
  return NULL;
}

/* subscript_list ',' subscript */
void *visit(subscript_list_c *symbol) {
  array_dimension_iterator_c* array_dimension_iterator = new array_dimension_iterator_c(current_array_type);
  for (int i =  0; i < symbol->n; i++) {
    symbol_c* dimension = array_dimension_iterator->next();
    if (dimension == NULL) ERROR;

    s4o.print("[(");
    symbol->elements[i]->accept(*this);
    s4o.print(") - (");
    dimension->accept(*this);
    s4o.print(")]");
  }
  delete array_dimension_iterator;
  return NULL;
}

/******************************************/
/* B 1.4.3 - Declaration & Initialisation */
/******************************************/

/* helper symbol for structure_initialization */
/* structure_element_initialization_list ',' structure_element_initialization */
void *visit(structure_element_initialization_list_c *symbol) {
  generate_c_structure_initialization_c *structure_initialization = new generate_c_structure_initialization_c(&s4o);
  structure_initialization->init_structure_default(this->current_param_type);
  structure_initialization->init_structure_values(symbol);
  delete structure_initialization;
  return NULL;
}

/* helper symbol for array_initialization */
/* array_initial_elements_list ',' array_initial_elements */
void *visit(array_initial_elements_list_c *symbol) {
  generate_c_array_initialization_c *array_initialization = new generate_c_array_initialization_c(&s4o);
  array_initialization->init_array_size(this->current_param_type);
  array_initialization->init_array_values(symbol);
  delete array_initialization;
  return NULL;
}

/***************************************/
/* B.3 - Language ST (Structured Text) */
/***************************************/
/***********************/
/* B 3.1 - Expressions */
/***********************/
void *visit(or_expression_c *symbol) {
  if (get_datatype_info_c::is_BOOL_compatible(symbol->datatype))
    return print_binary_expression(symbol->l_exp, symbol->r_exp, " || ");
  if (get_datatype_info_c::is_ANY_nBIT_compatible(symbol->datatype))
    return print_binary_expression(symbol->l_exp, symbol->r_exp, " | ");
  ERROR;
  return NULL;
}

void *visit(xor_expression_c *symbol) {
  if (get_datatype_info_c::is_BOOL_compatible(symbol->datatype)) {
    s4o.print("(");
    symbol->l_exp->accept(*this);
    s4o.print(" && !");
    symbol->r_exp->accept(*this);
    s4o.print(") || (!");
    symbol->l_exp->accept(*this);
    s4o.print(" && ");
    symbol->r_exp->accept(*this);
    s4o.print(")");
    return NULL;
  }
  if (get_datatype_info_c::is_ANY_nBIT_compatible(symbol->datatype))
    return print_binary_expression(symbol->l_exp, symbol->r_exp, " ^ ");
  ERROR;
  return NULL;
}

void *visit(and_expression_c *symbol) {
  if (get_datatype_info_c::is_BOOL_compatible(symbol->datatype))
    return print_binary_expression(symbol->l_exp, symbol->r_exp, " && ");
  if (get_datatype_info_c::is_ANY_nBIT_compatible(symbol->datatype))
    return print_binary_expression(symbol->l_exp, symbol->r_exp, " & ");
  ERROR;
return NULL;
}

void *visit(equ_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("EQ_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " == ");
}

void *visit(notequ_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("NE_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " != ");
}

void *visit(lt_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("LT_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " < ");
}

void *visit(gt_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("GT_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " > ");
}

void *visit(le_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("LE_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " <= ");
}

void *visit(ge_expression_c *symbol) {
  if (get_datatype_info_c::is_TIME_compatible      (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->l_exp->datatype) ||
      get_datatype_info_c::is_ANY_STRING_compatible(symbol->l_exp->datatype))
    return print_compare_function("GE_", symbol->l_exp->datatype, symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " >= ");
}

void *visit(add_expression_c *symbol) {
/*
  symbol_c *left_type  = symbol->l_exp->datatype;
  symbol_c *right_type = symbol->r_exp->datatype;
  if ((typeid(*left_type) == typeid(time_type_name_c) && typeid(*right_type) == typeid(time_type_name_c)) ||
      (typeid(*left_type) == typeid(tod_type_name_c)  && typeid(*right_type) == typeid(time_type_name_c)) ||
      (typeid(*left_type) == typeid(dt_type_name_c)   && typeid(*right_type) == typeid(time_type_name_c)))
    return print_binary_function("__time_add", symbol->l_exp, symbol->r_exp);
*/
  if (get_datatype_info_c::is_TIME_compatible      (symbol->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->datatype))
    return print_binary_function("__time_add", symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " + ");
}

void *visit(sub_expression_c *symbol) {
/*
  symbol_c *left_type  = symbol->l_exp->datatype;
  symbol_c *right_type = symbol->r_exp->datatype;
  if ((typeid(*left_type) == typeid(time_type_name_c) && typeid(*right_type) == typeid(time_type_name_c)) ||
      (typeid(*left_type) == typeid(date_type_name_c) && typeid(*right_type) == typeid(date_type_name_c)) ||
      (typeid(*left_type) == typeid(tod_type_name_c)  && typeid(*right_type) == typeid(time_type_name_c)) ||
      (typeid(*left_type) == typeid(tod_type_name_c)  && typeid(*right_type) == typeid(tod_type_name_c))  ||
      (typeid(*left_type) == typeid(dt_type_name_c)   && typeid(*right_type) == typeid(time_type_name_c)) ||
      (typeid(*left_type) == typeid(dt_type_name_c)   && typeid(*right_type) == typeid(dt_type_name_c)))
    return print_binary_function("__time_sub", symbol->l_exp, symbol->r_exp);
*/  
  if (get_datatype_info_c::is_TIME_compatible      (symbol->datatype) ||
      get_datatype_info_c::is_ANY_DATE_compatible  (symbol->datatype))
    return print_binary_function("__time_sub", symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " - ");
}

void *visit(mul_expression_c *symbol) {
/*
  symbol_c *left_type  = symbol->l_exp->datatype;
  symbol_c *right_type = symbol->r_exp->datatype;
  if ((typeid(*left_type) == typeid(time_type_name_c) && get_datatype_info_c::is_ANY_INT_compatible (right_type)) ||
      (typeid(*left_type) == typeid(time_type_name_c) && get_datatype_info_c::is_ANY_REAL_compatible(right_type)))
    return print_binary_function("__time_mul", symbol->l_exp, symbol->r_exp);
*/  
  if (get_datatype_info_c::is_TIME_compatible      (symbol->datatype))
    return print_binary_function("__time_mul", symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " * ");
}

void *visit(div_expression_c *symbol) {
/*
  symbol_c *left_type  = symbol->l_exp->datatype;
  symbol_c *right_type = symbol->r_exp->datatype;
  if ((typeid(*left_type) == typeid(time_type_name_c) && get_datatype_info_c::is_ANY_INT_compatible (right_type)) ||
      (typeid(*left_type) == typeid(time_type_name_c) && get_datatype_info_c::is_ANY_REAL_compatible(right_type)))
    return print_binary_function("__time_div", symbol->l_exp, symbol->r_exp);
*/
  if (get_datatype_info_c::is_TIME_compatible      (symbol->datatype))
    return print_binary_function("__time_div", symbol->l_exp, symbol->r_exp);
  return print_binary_expression(symbol->l_exp, symbol->r_exp, " / ");
}

void *visit(mod_expression_c *symbol) {
  s4o.print("((");
  symbol->r_exp->accept(*this);
  s4o.print(" == 0)?0:");
  print_binary_expression(symbol->l_exp, symbol->r_exp, " % ");
  s4o.print(")");
  return NULL;
}

void *visit(power_expression_c *symbol) {
  s4o.print("EXPT__LREAL__LREAL__LREAL((BOOL)__BOOL_LITERAL(TRUE),\n");
  s4o.indent_right();
  s4o.print(s4o.indent_spaces + "NULL,\n");
  s4o.print(s4o.indent_spaces + "(LREAL)(");
  symbol->l_exp->accept(*this);
  s4o.print("),\n");
  s4o.print(s4o.indent_spaces + "(LREAL)(");
  symbol->r_exp->accept(*this);
  s4o.print("))");
  return NULL;
}

void *visit(neg_expression_c *symbol) {
  return print_unary_expression(symbol->exp, " -");
}

void *visit(not_expression_c *symbol) {
  return print_unary_expression(symbol->exp, get_datatype_info_c::is_BOOL_compatible(symbol->datatype)?"!":"~");
}

void *visit(function_invocation_c *symbol) {
  symbol_c* function_name = NULL;
  DECLARE_PARAM_LIST()

  symbol_c *parameter_assignment_list = NULL;
  if (NULL != symbol->   formal_param_list) parameter_assignment_list = symbol->   formal_param_list;
  if (NULL != symbol->nonformal_param_list) parameter_assignment_list = symbol->nonformal_param_list;
  if (NULL == parameter_assignment_list) ERROR;

  function_call_param_iterator_c function_call_param_iterator(symbol);

  function_declaration_c *f_decl = (function_declaration_c *)symbol->called_function_declaration;
  if (f_decl == NULL) ERROR;
  
  function_name = symbol->function_name;
  
  /* loop through each function parameter, find the value we should pass
   * to it, and then output the c equivalent...
   */
  function_param_iterator_c fp_iterator(f_decl);
  identifier_c *param_name;
    /* flag to cirreclty handle calls to extensible standard functions (i.e. functions with variable number of input parameters) */
  bool found_first_extensible_parameter = false;  
  for(int i = 1; (param_name = fp_iterator.next()) != NULL; i++) {
    if (fp_iterator.is_extensible_param() && (!found_first_extensible_parameter)) {
      /* We are calling an extensible function. Before passing the extensible
       * parameters, we must add a dummy paramater value to tell the called
       * function how many extensible parameters we will be passing.
       *
       * Note that stage 3 has already determined the number of extensible
       * paramters, and stored that info in the abstract syntax tree. We simply
       * re-use that value.
       */
      /* NOTE: we are not freeing the malloc'd memory. This is not really a bug.
       *       Since we are writing a compiler, which runs to termination quickly,
       *       we can consider this as just memory required for the compilation process
       *       that will be free'd when the program terminates.
       */
      char *tmp = (char *)malloc(32); /* enough space for a call with 10^31 (larger than 2^64) input parameters! */
      if (tmp == NULL) ERROR;
      int res = snprintf(tmp, 32, "%d", symbol->extensible_param_count);
      if ((res >= 32) || (res < 0)) ERROR;
      identifier_c *param_value = new identifier_c(tmp);
      uint_type_name_c *param_type  = new uint_type_name_c();
      identifier_c *param_name = new identifier_c("");
      ADD_PARAM_LIST(param_name, param_value, param_type, function_param_iterator_c::direction_in)
      found_first_extensible_parameter = true;
    }

    if (fp_iterator.is_extensible_param()) {      
      /* since we are handling an extensible parameter, we must add the index to the
       * parameter name so we can go looking for the value passed to the correct
       * extended parameter (e.g. IN1, IN2, IN3, IN4, ...)
       */
      char *tmp = (char *)malloc(32); /* enough space for a call with 10^31 (larger than 2^64) input parameters! */
      int res = snprintf(tmp, 32, "%d", fp_iterator.extensible_param_index());
      if ((res >= 32) || (res < 0)) ERROR;
      param_name = new identifier_c(strdup2(param_name->value, tmp));
      if (param_name->value == NULL) ERROR;
    }
    
    symbol_c *param_type = fp_iterator.param_type();
    if (param_type == NULL) ERROR;

    function_param_iterator_c::param_direction_t param_direction = fp_iterator.param_direction();
    
    symbol_c *param_value = NULL;
    
    /* Get the value from a foo(<param_name> = <param_value>) style call */
    if (param_value == NULL)
      param_value = function_call_param_iterator.search_f(param_name);

    /* Get the value from a foo(<param_value>) style call */
    if ((param_value == NULL) && !fp_iterator.is_en_eno_param_implicit()) {
      param_value = function_call_param_iterator.next_nf();
    }

    /* if no more parameter values in function call, and the current parameter
     * of the function declaration is an extensible parameter, we
     * have reached the end, and should simply jump out of the for loop.
     */
    if ((param_value == NULL) && (fp_iterator.is_extensible_param())) {
      break;
    }
    
    if ((param_value == NULL) && (param_direction == function_param_iterator_c::direction_in)) {
      /* No value given for parameter, so we must use the default... */
      /* First check whether default value specified in function declaration...*/
      param_value = fp_iterator.default_value();
    }
    
    ADD_PARAM_LIST(param_name, param_value, param_type, param_direction)
  } /* for(...) */
  // symbol->parameter_assignment->accept(*this);
  
  if (function_call_param_iterator.next_nf() != NULL) ERROR;

  bool has_output_params = false;

  if (!this->is_variable_prefix_null()) {
    PARAM_LIST_ITERATOR() {
      if ((PARAM_DIRECTION == function_param_iterator_c::direction_out ||
           PARAM_DIRECTION == function_param_iterator_c::direction_inout) &&
           PARAM_VALUE != NULL) {
        has_output_params = true;
      }
    }
  }

  /* Check whether we are calling an overloaded function! */
  /* (fdecl_mutiplicity > 1)  => calling overloaded function */
  int fdecl_mutiplicity =  function_symtable.count(symbol->function_name);
  if (fdecl_mutiplicity == 0) ERROR;

  if (has_output_params) {
    fcall_number++;
    s4o.print("__");
    fbname->accept(*this);
    s4o.print("_");
    function_name->accept(*this);
    if (fdecl_mutiplicity > 1) {
      /* function being called is overloaded! */
      s4o.print("__");
      print_function_parameter_data_types_c overloaded_func_suf(&s4o);
      f_decl->accept(overloaded_func_suf);
    }
    s4o.print(fcall_number);
  }
  else {
    function_name->accept(*this);
    if (fdecl_mutiplicity > 1) {
      /* function being called is overloaded! */
      s4o.print("__");
      print_function_parameter_data_types_c overloaded_func_suf(&s4o);
      f_decl->accept(overloaded_func_suf);
    }
  }
  s4o.print("(");
  s4o.indent_right();
  
  int nb_param = 0;
  PARAM_LIST_ITERATOR() {
    symbol_c *param_value = PARAM_VALUE;
    current_param_type = PARAM_TYPE;
          
    switch (PARAM_DIRECTION) {
      case function_param_iterator_c::direction_in:
        if (nb_param > 0)
          s4o.print(",\n"+s4o.indent_spaces);
        if (param_value == NULL) {
          /* If not, get the default value of this variable's type */
          param_value = type_initial_value_c::get(current_param_type);
        }
        if (param_value == NULL) ERROR;
        s4o.print("(");
        if      (get_datatype_info_c::is_ANY_INT_literal(current_param_type))
          get_datatype_info_c::lint_type_name.accept(*this);
        else if (get_datatype_info_c::is_ANY_REAL_literal(current_param_type))
          get_datatype_info_c::lreal_type_name.accept(*this);
        else
          current_param_type->accept(*this);
        s4o.print(")");
        print_check_function(current_param_type, param_value);
        nb_param++;
        break;
      case function_param_iterator_c::direction_out:
      case function_param_iterator_c::direction_inout:
        if (!has_output_params) {
          if (nb_param > 0)
            s4o.print(",\n"+s4o.indent_spaces);
          if (param_value == NULL)
            s4o.print("NULL");
          else {
            wanted_variablegeneration = fparam_output_vg;
            param_value->accept(*this);
            wanted_variablegeneration = expression_vg;
          }
          nb_param++;
        }
        break;
      case function_param_iterator_c::direction_extref:
        /* TODO! */
        ERROR;
        break;
    } /* switch */
  }
  if (has_output_params) {
    if (nb_param > 0)
      s4o.print(",\n"+s4o.indent_spaces);
    s4o.print(FB_FUNCTION_PARAM);
  }
  s4o.print(")");
  s4o.indent_left();

  CLEAR_PARAM_LIST()

  return NULL;
}

/********************/
/* B 3.2 Statements */
/********************/
void *visit(statement_list_c *symbol) {
  return print_list(symbol, s4o.indent_spaces, ";\n" + s4o.indent_spaces, ";\n");
}

/*********************************/
/* B 3.2.1 Assignment Statements */
/*********************************/
void *visit(assignment_statement_c *symbol) {
  symbol_c *left_type = search_varfb_instance_type->get_type_id(symbol->l_exp);
  
  if (this->is_variable_prefix_null()) {
    symbol->l_exp->accept(*this);
    s4o.print(" = ");
    print_check_function(left_type, symbol->r_exp);
  }
  else {
    print_setter(symbol->l_exp, left_type, symbol->r_exp);
  }
  return NULL;
}

/*****************************************/
/* B 3.2.2 Subprogram Control Statements */
/*****************************************/

void *visit(return_statement_c *symbol) {
  s4o.print("goto "); s4o.print(END_LABEL);
  return NULL;
}


/* fb_name '(' [param_assignment_list] ')' */
/* param_assignment_list -> may be NULL ! */
//SYM_REF2(fb_invocation_c, fb_name, param_assignment_list)
void *visit(fb_invocation_c *symbol) {
  TRACE("fb_invocation_c");
  /* first figure out what is the name of the function block type of the function block being called... */
  symbol_c *function_block_type_name = this->search_fb_instance_decl->get_type_name(symbol->fb_name);
    /* should never occur. The function block instance MUST have been declared... */
  if (function_block_type_name == NULL) ERROR;

  /* Now find the declaration of the function block type being called... */
  function_block_declaration_c *fb_decl = function_block_type_symtable.find_value(function_block_type_name);
    /* should never occur. The function block type being called MUST be in the symtable... */
  if (fb_decl == function_block_type_symtable.end_value()) ERROR;

  /* loop through each function block parameter, find the value we should pass
   * to it, and then output the c equivalent...
   */
  function_param_iterator_c fp_iterator(fb_decl);
  identifier_c *param_name;
  function_call_param_iterator_c function_call_param_iterator(symbol);
  for(int i = 1; (param_name = fp_iterator.next()) != NULL; i++) {
    function_param_iterator_c::param_direction_t param_direction = fp_iterator.param_direction();
    
    /*fprintf(stderr, "param : %s\n", param_name->value);*/
    
    /* Get the value from a foo(<param_name> = <param_value>) style call */
    symbol_c *param_value = function_call_param_iterator.search_f(param_name);

    /* Get the value from a foo(<param_value>) style call */
    /* When using the informal invocation style, user can not pass values to EN or ENO parameters if these
     * were implicitly defined!
     */
    if ((param_value == NULL) && !fp_iterator.is_en_eno_param_implicit())
      param_value = function_call_param_iterator.next_nf();

    symbol_c *param_type = fp_iterator.param_type();
    if (param_type == NULL) ERROR;
    
    /* now output the value assignment */
    if (param_value != NULL)
      if ((param_direction == function_param_iterator_c::direction_in) ||
          (param_direction == function_param_iterator_c::direction_inout)) {
        if (this->is_variable_prefix_null()) {
          symbol->fb_name->accept(*this);
          s4o.print(".");
          param_name->accept(*this);
          s4o.print(" = ");
          print_check_function(param_type, param_value);
        }
        else {
          print_setter(param_name, param_type, param_value, symbol->fb_name);
        }
        s4o.print(";\n" + s4o.indent_spaces);
      }
  } /* for(...) */

  /* now call the function... */
  function_block_type_name->accept(*this);
  s4o.print(FB_FUNCTION_SUFFIX);
  s4o.print("(");
  if (search_var_instance_decl->get_vartype(symbol->fb_name) != search_var_instance_decl_c::external_vt)
    s4o.print("&");
  print_variable_prefix();
  symbol->fb_name->accept(*this);
  s4o.print(")");

  /* loop through each function parameter, find the variable to which
   * we should atribute the value of all output or inoutput parameters.
   */
  fp_iterator.reset();
  function_call_param_iterator.reset();
  for(int i = 1; (param_name = fp_iterator.next()) != NULL; i++) {
    function_param_iterator_c::param_direction_t param_direction = fp_iterator.param_direction();

    /* Get the value from a foo(<param_name> = <param_value>) style call */
    symbol_c *param_value = function_call_param_iterator.search_f(param_name);

    /* Get the value from a foo(<param_value>) style call */
    /* When using the informal invocation style, user can not pass values to EN or ENO parameters if these
     * were implicitly defined!
     */
    if ((param_value == NULL) && !fp_iterator.is_en_eno_param_implicit())
      param_value = function_call_param_iterator.next_nf();

    /* now output the value assignment */
    if (param_value != NULL)
      if ((param_direction == function_param_iterator_c::direction_out) ||
          (param_direction == function_param_iterator_c::direction_inout)) {
        symbol_c *param_type = search_varfb_instance_type->get_type_id(param_value);
        s4o.print(";\n" + s4o.indent_spaces);
        if (this->is_variable_prefix_null()) {
          param_value->accept(*this);
          s4o.print(" = ");
          print_check_function(param_type, param_name, symbol->fb_name);
        }
        else {
          print_setter(param_value, param_type, param_name, NULL, symbol->fb_name);
        }
      }
  } /* for(...) */

  return NULL;
}




/* helper symbol for fb_invocation */
/* param_assignment_list ',' param_assignment */
void *visit(param_assignment_list_c *symbol) {
  TRACE("param_assignment_list_c");
  /* this should never be called... */
  ERROR;
  return NULL;
//  return print_list(symbol, "", ", ");
}


void *visit(input_variable_param_assignment_c *symbol) {
  TRACE("input_variable_param_assignment_c");
  /* this should never be called... */
  ERROR;
  return NULL;
/*
  symbol->variable_name->accept(*this);
  s4o.print(" = ");
  symbol->expression->accept(*this);
  return NULL;
*/
}

void *visit(output_variable_param_assignment_c *symbol) {
  TRACE("output_variable_param_assignment_c");
  /* this should never be called... */
  ERROR;
  return NULL;
/*
  s4o.print(s4o.indent_spaces);
  if (symbol->not_param != NULL)
    symbol->not_param->accept(*this);
  symbol->variable_name->accept(*this);
  s4o.print(" => ");
  symbol->variable->accept(*this);
  return NULL;
*/
}

// TODO: the NOT symbol in function (block) calls...
void *visit(not_paramassign_c *symbol) {
  TRACE("not_paramassign_c");
  /* this should never be called... */
  ERROR;
  return NULL;
/*
  s4o.print("NOT ");
  return NULL;
*/
}


/********************************/
/* B 3.2.3 Selection Statements */
/********************************/
void *visit(if_statement_c *symbol) {
  s4o.print("if (");
  symbol->expression->accept(*this);
  s4o.print(") {\n");
  s4o.indent_right();
  symbol->statement_list->accept(*this);
  s4o.indent_left();
  symbol->elseif_statement_list->accept(*this);

  if (symbol->else_statement_list != NULL) {
    s4o.print(s4o.indent_spaces); s4o.print("} else {\n");
    s4o.indent_right();
    symbol->else_statement_list->accept(*this);
    s4o.indent_left();
  }
  s4o.print(s4o.indent_spaces); s4o.print("}");
  return NULL;
}

/* helper symbol for if_statement */
void *visit(elseif_statement_list_c *symbol) {return print_list(symbol);}

/* helper symbol for elseif_statement_list */
void *visit(elseif_statement_c *symbol) {
  s4o.print(s4o.indent_spaces); s4o.print("} else if (");
  symbol->expression->accept(*this);
  s4o.print(") {\n");
  s4o.indent_right();
  symbol->statement_list->accept(*this);
  s4o.indent_left();
  return NULL;
}

void *visit(case_statement_c *symbol) {
  symbol_c *expression_type = symbol->expression->datatype;
  s4o.print("{\n");
  s4o.indent_right();
  s4o.print(s4o.indent_spaces);
  if      (get_datatype_info_c::is_ANY_INT_literal(expression_type))
           get_datatype_info_c::lint_type_name.accept(*this);
  else if (get_datatype_info_c::is_ANY_REAL_literal(expression_type))
           get_datatype_info_c::lreal_type_name.accept(*this);
  else
    expression_type->accept(*this);
  s4o.print(" __case_expression = ");
  symbol->expression->accept(*this);
  s4o.print(";\n" + s4o.indent_spaces + "switch (__case_expression) {\n");
  s4o.indent_right();
  wanted_casegeneration = single_cg;
  symbol->case_element_list->accept(*this);
  wanted_casegeneration = subrange_cg;
  s4o.print(s4o.indent_spaces + "default:\n");
  s4o.indent_right();
  first_subrange_case_list = true;
  symbol->case_element_list->accept(*this);
  if (symbol->statement_list != NULL) {
    if (!first_subrange_case_list) {
      s4o.print(s4o.indent_spaces + "else {\n");
      s4o.indent_right();
    }
    symbol->statement_list->accept(*this);
    if (!first_subrange_case_list) {
      s4o.indent_left();
      s4o.print(s4o.indent_spaces + "}\n");
    }
  }
  s4o.print(s4o.indent_spaces + "break;\n");
  s4o.indent_left();
  wanted_casegeneration = none_cg;
  s4o.indent_left();
  s4o.print(s4o.indent_spaces + "}\n");
  s4o.indent_left();
  s4o.print(s4o.indent_spaces + "}");
  return NULL;
}

/* helper symbol for case_statement */
void *visit(case_element_list_c *symbol) {return print_list(symbol);}

void *visit(case_element_c *symbol) {
  case_element_iterator_c *case_element_iterator;
  symbol_c* element = NULL;
  bool first_element = true;

  switch (wanted_casegeneration) {
    case single_cg:
      case_element_iterator = new case_element_iterator_c(symbol->case_list, case_element_iterator_c::element_single);
      for (element = case_element_iterator->next(); element != NULL; element = case_element_iterator->next()) {
        if (first_element) first_element = false;
        s4o.print(s4o.indent_spaces + "case ");
        element->accept(*this);
        s4o.print(":\n");
      }
      delete case_element_iterator;
      break;

    case subrange_cg:
      case_element_iterator = new case_element_iterator_c(symbol->case_list, case_element_iterator_c::element_subrange);
      for (element = case_element_iterator->next(); element != NULL; element = case_element_iterator->next()) {
        if (first_element) {
          if (first_subrange_case_list) {
            s4o.print(s4o.indent_spaces + "if (");
            first_subrange_case_list = false;
          }
          else {
            s4o.print(s4o.indent_spaces + "else if (");
          }
          first_element = false;
        }
        else {
          s4o.print(" && ");
        }
        element->accept(*this);
      }
      delete case_element_iterator;
      if (!first_element) {
        s4o.print(") {\n");
      }
      break;

    default:
      break;
  }

  if (!first_element) {
    s4o.indent_right();
    symbol->statement_list->accept(*this);
    switch (wanted_casegeneration) {
      case single_cg:
        s4o.print(s4o.indent_spaces + "break;\n");
        s4o.indent_left();
        break;
      case subrange_cg:
        s4o.indent_left();
        s4o.print(s4o.indent_spaces + "}\n");
        break;
      default:
        break;
    }
  }
  return NULL;
}

/********************************/
/* B 3.2.4 Iteration Statements */
/********************************/
void *visit(for_statement_c *symbol) {
  s4o.print("for(");
  symbol->control_variable->accept(*this);
  s4o.print(" = ");
  symbol->beg_expression->accept(*this);
  s4o.print("; ");
  if (symbol->by_expression == NULL) {
    /* increment by 1 */
    symbol->control_variable->accept(*this);
    s4o.print(" <= ");
    symbol->end_expression->accept(*this);
    s4o.print("; ");
    symbol->control_variable->accept(*this);
    s4o.print("++");
  } else {
    /* increment by user defined value  */
    /* The user defined increment value may be negative, in which case
     * the expression to determine whether we have reached the end of the loop
     * changes from a '<=' to a '>='.
     * Since the increment value may change during runtime (remember, it is
     * an expression, so may contain variables), choosing which test
     * to use has to be done at runtime.
     */
    s4o.print("((");
    symbol->by_expression->accept(*this);
    s4o.print(") > 0)? (");
    symbol->control_variable->accept(*this);
    s4o.print(" <= (");
    symbol->end_expression->accept(*this);
    s4o.print(")) : (");
    symbol->control_variable->accept(*this);
    s4o.print(" >= (");
    symbol->end_expression->accept(*this);
    s4o.print(")); ");
    symbol->control_variable->accept(*this);
    s4o.print(" += (");
    symbol->by_expression->accept(*this);
    s4o.print(")");
  }
  s4o.print(")");
  
  s4o.print(" {\n");
  s4o.indent_right();
  symbol->statement_list->accept(*this);
  s4o.indent_left();
  s4o.print(s4o.indent_spaces); s4o.print("}");
  return NULL;
}

void *visit(while_statement_c *symbol) {
  s4o.print("while (");
  symbol->expression->accept(*this);
  s4o.print(") {\n");
  s4o.indent_right();
  symbol->statement_list->accept(*this);
  s4o.indent_left();
  s4o.print(s4o.indent_spaces); s4o.print("}");
  return NULL;
}

void *visit(repeat_statement_c *symbol) {
  s4o.print("do {\n");
  s4o.indent_right();
  symbol->statement_list->accept(*this);
  s4o.indent_left();
  s4o.print(s4o.indent_spaces); s4o.print("} while(");
  symbol->expression->accept(*this);
  s4o.print(")");
  return NULL;
}

void *visit(exit_statement_c *symbol) {
  s4o.print("break");
  return NULL;
}



}; /* generate_c_st_c */









