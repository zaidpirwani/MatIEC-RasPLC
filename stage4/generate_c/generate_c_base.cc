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
 *
 * This code is made available on the understanding that it will not be
 * used in safety-critical situations without a full and competent review.
 */

#include <string.h>




typedef struct
{
  symbol_c *param_name;
  symbol_c *param_value;
  symbol_c *param_type;
  function_param_iterator_c::param_direction_t param_direction;
} FUNCTION_PARAM;

#define DECLARE_PARAM_LIST()\
  std::list<FUNCTION_PARAM*> param_list;\
  std::list<FUNCTION_PARAM*>::iterator pt;\
  FUNCTION_PARAM *param;

#define ADD_PARAM_LIST(name, value, type, direction)\
  param = new FUNCTION_PARAM;\
  param->param_name = name;\
  param->param_value = value;\
  param->param_type = type;\
  param->param_direction = direction;\
  param_list.push_back(param);

#define PARAM_LIST_ITERATOR() for(pt = param_list.begin(); pt != param_list.end(); pt++)

#define PARAM_NAME (*pt)->param_name
#define PARAM_VALUE (*pt)->param_value
#define PARAM_TYPE (*pt)->param_type
#define PARAM_DIRECTION (*pt)->param_direction

#define CLEAR_PARAM_LIST()\
  PARAM_LIST_ITERATOR()\
    delete *pt;\
  param_list.clear();


class generate_c_base_c: public iterator_visitor_c {

  protected:
    stage4out_c &s4o;

  private:
    /* Unlike programs that are mapped onto C++ classes, Function Blocks are mapped onto a data structure type
     * and a separate function containing the code. This function is passed a pointer to an instance of the data
     * structure. This means that the code inside the functions must insert a pointer to the data structure whenever
     * it wishes to access a Function Block variable.
     * The variable_prefix_ variable will contain the correct string which needs to be prefixed to all variable accesses.
     * This string is set with the set_variable_prefix() member function.
     */
    const char *variable_prefix_;

  public:
    generate_c_base_c(stage4out_c *s4o_ptr): s4o(*s4o_ptr) {
      variable_prefix_ = NULL;
    }
    ~generate_c_base_c(void) {}

    void set_variable_prefix(const char *variable_prefix) {variable_prefix_ = variable_prefix;}
    const char *get_variable_prefix(void) {return variable_prefix_;}
    bool is_variable_prefix_null(void) {return variable_prefix_ == NULL;}
    void print_variable_prefix(void) {
      if (variable_prefix_ != NULL)
        s4o.print(variable_prefix_);
    }

    void *print_token(token_c *token, int offset = 0) {
      return s4o.printupper((token->value)+offset);
    }

    void *print_literal(symbol_c *type, symbol_c *value) {
      s4o.print("__");
      type->accept(*this);
      s4o.print("_LITERAL(");
      value->accept(*this);
      s4o.print(")");
      return NULL;
    }

    void *print_striped_token(token_c *token, int offset = 0) {
      std::string str = "";
      bool leading_zero = true;
      for (unsigned int i = offset; i < strlen(token->value); i++) {
        if (leading_zero &&
        	(token->value[i] != '0' ||
        	 i == strlen(token->value) - 1 ||
        	 token->value[i + 1] == '.'
        	))
          leading_zero = false;
    	if (!leading_zero && token->value[i] != '_')
          str += token->value[i];
      }
      return s4o.printupper(str);
    }

    void *print_striped_binary_token(token_c *token, unsigned int offset = 0) {
      /* convert the binary value to hexadecimal format... */
      unsigned char value, bit_mult;
      unsigned int i;
      int total_bits;
      char str[2] = {'A', '\0'};  /* since the s4o object is not prepared to print out one character at a time... */

      s4o.print("0x");

      total_bits = 0;
      for (i = offset; i < strlen(token->value); i++)
        if (token->value[i] != '_')
	  total_bits++;

      value = 0;
      bit_mult = (unsigned char)1 << (((total_bits+3)%4)+1);
      for (i = offset; i < strlen(token->value); i++) {
        if (token->value[i] != '_') {
	  bit_mult /= 2;
	  value += bit_mult * ((token->value[i] == '0')? 0:1);
	  if (bit_mult == 1) {
	    str[0] = (value <= 9)? (char)'0' + value : (char)'A' + value - 10;
	    s4o.print(str);
            bit_mult = 0x10;
            value = 0;
	  }
	}
      }

      return NULL;
    }

    void *print_list(list_c *list,
		     std::string pre_elem_str = "",
		     std::string inter_elem_str = "",
		     std::string post_elem_str = "",
		     visitor_c *visitor = NULL) {
      if (visitor == NULL) visitor = this;

      if (list->n > 0) {
//std::cout << "generate_c_base_c::print_list(n = " << list->n << ")   000\n";
        s4o.print(pre_elem_str);
        list->elements[0]->accept(*visitor);
      }

      for(int i = 1; i < list->n; i++) {
//std::cout << "generate_c_base_c::print_list   " << i << "\n";
        s4o.print(inter_elem_str);
        list->elements[i]->accept(*visitor);
      }

      if (list->n > 0)
        s4o.print(post_elem_str);

      return NULL;
    }


    void *print_binary_expression(symbol_c *l_exp,
				  symbol_c *r_exp,
				  const char *operation) {
      s4o.print("(");
      l_exp->accept(*this);
      s4o.print(operation);
      r_exp->accept(*this);
      s4o.print(")");
      return NULL;
    }

    void *print_unary_expression(symbol_c *exp,
				 const char *operation) {
      s4o.print(operation);
      s4o.print("(");
      exp->accept(*this);
      s4o.print(")");
      return NULL;
    }

    void *print_binary_function(const char *function,
          symbol_c *l_exp,
				  symbol_c *r_exp) {
      s4o.print(function);
      s4o.print("(");
      l_exp->accept(*this);
      s4o.print(", ");
      r_exp->accept(*this);
      s4o.print(")");
      return NULL;
   	}

    void *print_compare_function(const char *function,
          symbol_c *compare_type,
          symbol_c *l_exp,
          symbol_c *r_exp) {
      s4o.print(function);
      compare_type->accept(*this);
      s4o.print("(__BOOL_LITERAL(TRUE), NULL, 2, ");
      l_exp->accept(*this);
      s4o.print(", ");
      r_exp->accept(*this);
      s4o.print(")");
      return NULL;
    }

    void *print_check_function(symbol_c *type,
          symbol_c *value,
          symbol_c *fb_name = NULL,
          bool temp = false) {
      bool is_subrange = search_base_type_c::type_is_subrange(type);
      if (is_subrange) {
		s4o.print("__CHECK_");
		type->accept(*this);
		s4o.print("(");
      }
      if (fb_name != NULL) {
        s4o.print(GET_VAR);
        s4o.print("(");
        print_variable_prefix();
        fb_name->accept(*this);
        s4o.print(".");
        value->accept(*this);
        s4o.print(")");
      }
      else {
        if (temp)
    	  s4o.print(TEMP_VAR);
        value->accept(*this);
      }
      if (is_subrange)
        s4o.print(")");
      return NULL;
    }

/********************/
/* 2.1.6 - Pragmas  */
/********************/
    void *visit(enable_code_generation_pragma_c * symbol)   {s4o.enable_output();  return NULL;}
    void *visit(disable_code_generation_pragma_c * symbol)  {s4o.disable_output(); return NULL;} 

    /* Do not use print_token() as it will change everything into uppercase */
    void *visit(pragma_c *symbol) {
        s4o.print(s4o.indent_spaces);
        s4o.print("#define GetFbVar(var,...) __GET_VAR(data__->var,__VA_ARGS__)\n");
        s4o.print(s4o.indent_spaces);
        s4o.print("#define SetFbVar(var,val,...) __SET_VAR(data__->,var,val,__VA_ARGS__)\n");
        s4o.print(symbol->value);
        s4o.print("\n");
        s4o.print(s4o.indent_spaces);
        s4o.print("#undef GetFbVar\n");
        s4o.print(s4o.indent_spaces);
        s4o.print("#undef SetFbVar\n");
        return NULL;
    }


/***************************/
/* B 0 - Programming Model */
/***************************/
  /* leave for derived classes... */



/*************************/
/* B.1 - Common elements */
/*************************/
/*******************************************/
/* B 1.1 - Letters, digits and identifiers */
/*******************************************/
    void *visit(identifier_c *symbol) {return print_token(symbol);}

/*********************/
/* B 1.2 - Constants */
/*********************/
  /* originally empty... */

/******************************/
/* B 1.2.1 - Numeric Literals */
/******************************/
    void *visit(real_c *symbol) {return print_striped_token(symbol);}
    void *visit(integer_c *symbol) {return print_striped_token(symbol);}
    void *visit(binary_integer_c *symbol) {return print_striped_binary_token(symbol, 2);}
    void *visit(octal_integer_c *symbol) {s4o.print("0"); return print_striped_token(symbol, 2);}
    void *visit(hex_integer_c *symbol) {s4o.print("0x"); return print_striped_token(symbol, 3);}

    void *visit(neg_real_c *symbol) {
      s4o.print("-");
      symbol->exp->accept(*this);
      return NULL;
    }

    void *visit(neg_integer_c *symbol) {
      s4o.print("-");
      symbol->exp->accept(*this);
      return NULL;
    }

    void *visit(integer_literal_c *symbol) {return print_literal(symbol->type, symbol->value);}
    void *visit(real_literal_c *symbol) {return print_literal(symbol->type, symbol->value);}
    void *visit(bit_string_literal_c *symbol) {return print_literal(symbol->type, symbol->value);}
    void *visit(boolean_literal_c *symbol) {
      if (NULL != symbol->type)
        return print_literal(symbol->type, symbol->value);
      else {
	bool_type_name_c bool_type;
	return print_literal(&bool_type, symbol->value);
      }
    }

    /* helper class for boolean_literal_c */
    void *visit(boolean_true_c *symbol) {s4o.print("TRUE"); return NULL;}
    void *visit(boolean_false_c *symbol) {s4o.print("FALSE"); return NULL;}

    void *visit(neg_expression_c *symbol) {
      s4o.print("-");
      symbol->exp->accept(*this);
      return NULL;
    }

/*******************************/
/* B.1.2.2   Character Strings */
/*******************************/
    void *visit(double_byte_character_string_c *symbol) {
      // TO DO ...
      ERROR;
      return print_token(symbol);
    }

    void *visit(single_byte_character_string_c *symbol) {
      std::string str = "";
      unsigned int count = 0; 
      str += '"';
      /* we ignore the first and last bytes, they will be the character ' */
      for (unsigned int i = 1; i < strlen(symbol->value) - 1; i++) {
        char c = symbol->value[i];
        if ((c == '\\') || (c == '"'))
          {str += '\\'; str += c; count ++; continue;}
        if (c != '$')
          {str += c; count++; continue;}
        /* this should be safe, since the code has passed the syntax parser!! */
        c = symbol->value[++i];
        switch (c) {
          case '$':
          case '\'':
            {str += c; count++; continue;}
          case 'L':
          case 'l':
            {str += "\x0A"; /* LF */; count++; continue;}
          case 'N':
          case 'n':
            {str += "\\x0A"; /* NL */; count++; continue;}
          case 'P':
          case 'p':
            {str += "\\f"; /* FF */; count++; continue;}
          case 'R':
          case 'r':
            {str += "\\r"; /* CR */; count++; continue;}
          case 'T':
          case 't':
            {str += "\\t"; /* tab */; count++; continue;}
          default: {
            if (isxdigit(c)) {
              /* this should be safe, since the code has passed the syntax parser!! */
	      char c2 = symbol->value[++i];
	      if (isxdigit(c2)) {
	        str += '\\'; str += 'x'; str += c; str += c2;
	        count++; continue;
	      }
	    }
          }
          /* otherwise we have an invalid string!! */
          /* This should not have got through the syntax parser! */
          ERROR;
        } /* switch() */
      } /* for() */

      str += '"';
      s4o.print("__STRING_LITERAL(");
      s4o.print(count); 
      s4o.print(",");
      s4o.print(str);
      s4o.print(")");
      return NULL;
    }


/***************************/
/* B 1.2.3 - Time Literals */
/***************************/

/************************/
/* B 1.2.3.1 - Duration */
/************************/
/* The following output is actually the parameters to the constructor of the TIME class! */

/* SYM_REF0(neg_time_c) */
void *visit(neg_time_c *symbol) {s4o.print("-1"); /* negative time value */; return NULL;}


/* SYM_REF2(duration_c, neg, interval) */
void *visit(duration_c *symbol) {
  TRACE("duration_c");
  s4o.print("__time_to_timespec(");
  
  if (NULL == symbol->neg)    s4o.print("1");  /* positive time value */
  else                        symbol->neg->accept(*this);  /* this will print '-1'   :-) */

  s4o.print(", ");
  symbol->interval->accept(*this);
  s4o.print(")");
  return NULL;
}


/* SYM_TOKEN(fixed_point_c) */
void *visit(fixed_point_c *symbol) {return print_striped_token(symbol);}

/* SYM_REF5(interval_c, days, hours, minutes, seconds, milliseconds) */
void *visit(interval_c *symbol) {
  TRACE("interval_c");
  /* s4o.print("0, 0, 0, 0, 0");  // milliseconds, seconds, minutes, hours, days */
  if (NULL == symbol->milliseconds) s4o.print("0");  /* milliseconds */
  else                              symbol->milliseconds->accept(*this);
  s4o.print(", ");

  if (NULL == symbol->seconds)      s4o.print("0");  /* seconds */
  else                              symbol->seconds->accept(*this);
  s4o.print(", ");

  if (NULL == symbol->minutes)      s4o.print("0");  /* minutes */
  else                              symbol->minutes->accept(*this);
  s4o.print(", ");

  if (NULL == symbol->hours)        s4o.print("0");  /* hours */
  else                              symbol->hours->accept(*this);
  s4o.print(", ");

  if (NULL == symbol->days)         s4o.print("0");  /* days */
  else                              symbol->days->accept(*this);

  return NULL;
}


/************************************/
/* B 1.2.3.2 - Time of day and Date */
/************************************/

/* SYM_REF2(time_of_day_c, daytime, unused) */
void *visit(time_of_day_c *symbol) {
  TRACE("time_of_day_c");
  s4o.print("__tod_to_timespec(");
  symbol->daytime->accept(*this);
  s4o.print(")");
  return NULL;
}


/* SYM_REF4(daytime_c, day_hour, day_minute, day_second, unused) */
void *visit(daytime_c *symbol) {
  TRACE("daytime_c");
  symbol->day_second->accept(*this);
  s4o.print(", ");
  symbol->day_minute->accept(*this);
  s4o.print(", ");
  symbol->day_hour->accept(*this);
  return NULL;
}


/* SYM_REF2(date_c, date_literal, unused) */
void *visit(date_c *symbol) {
  TRACE("date_c");
  s4o.print("__date_to_timespec(");
  symbol->date_literal->accept(*this);
  s4o.print(")");
  return NULL;
}


/* SYM_REF4(date_literal_c, year, month, day, unused) */
void *visit(date_literal_c *symbol) {
  TRACE("date_literal_c");
  symbol->day->accept(*this);
  s4o.print(", ");
  symbol->month->accept(*this);
  s4o.print(", ");
  symbol->year->accept(*this);
  return NULL;
}


/* SYM_REF2(date_and_time_c, date_literal, daytime) */
void *visit(date_and_time_c *symbol) {
  TRACE("date_and_time_c");
  s4o.print("__dt_to_timespec(");
  symbol->daytime->accept(*this);
  s4o.print(", ");
  symbol->date_literal->accept(*this);
  s4o.print(")");
  return NULL;
}


/**********************/
/* B.1.3 - Data types */
/**********************/
/***********************************/
/* B 1.3.1 - Elementary Data Types */
/***********************************/
    void *visit(time_type_name_c *symbol)        {s4o.print("TIME");        return NULL;}
    void *visit(bool_type_name_c *symbol)        {s4o.print("BOOL");        return NULL;}
    void *visit(sint_type_name_c *symbol)        {s4o.print("SINT");        return NULL;}
    void *visit(int_type_name_c *symbol)         {s4o.print("INT");         return NULL;}
    void *visit(dint_type_name_c *symbol)        {s4o.print("DINT");        return NULL;}
    void *visit(lint_type_name_c *symbol)        {s4o.print("LINT");        return NULL;}
    void *visit(usint_type_name_c *symbol)       {s4o.print("USINT");       return NULL;}
    void *visit(uint_type_name_c *symbol)        {s4o.print("UINT");        return NULL;}
    void *visit(udint_type_name_c *symbol)       {s4o.print("UDINT");       return NULL;}
    void *visit(ulint_type_name_c *symbol)       {s4o.print("ULINT");       return NULL;}
    void *visit(real_type_name_c *symbol)        {s4o.print("REAL");        return NULL;}
    void *visit(lreal_type_name_c *symbol)       {s4o.print("LREAL");       return NULL;}
    void *visit(date_type_name_c *symbol)        {s4o.print("DATE");        return NULL;}
    void *visit(tod_type_name_c *symbol)         {s4o.print("TOD");         return NULL;}
    void *visit(dt_type_name_c *symbol)          {s4o.print("DT");          return NULL;}
    void *visit(byte_type_name_c *symbol)        {s4o.print("BYTE");        return NULL;}
    void *visit(word_type_name_c *symbol)        {s4o.print("WORD");        return NULL;}
    void *visit(lword_type_name_c *symbol)       {s4o.print("LWORD");       return NULL;}
    void *visit(dword_type_name_c *symbol)       {s4o.print("DWORD");       return NULL;}
    void *visit(string_type_name_c *symbol)      {s4o.print("STRING");      return NULL;}
    void *visit(wstring_type_name_c *symbol)     {s4o.print("WSTRING");     return NULL;}

    void *visit(safetime_type_name_c *symbol)    {s4o.print("TIME");    return NULL;}
    void *visit(safebool_type_name_c *symbol)    {s4o.print("BOOL");    return NULL;}
    void *visit(safesint_type_name_c *symbol)    {s4o.print("SINT");    return NULL;}
    void *visit(safeint_type_name_c *symbol)     {s4o.print("INT");     return NULL;}
    void *visit(safedint_type_name_c *symbol)    {s4o.print("DINT");    return NULL;}
    void *visit(safelint_type_name_c *symbol)    {s4o.print("LINT");    return NULL;}
    void *visit(safeusint_type_name_c *symbol)   {s4o.print("USINT");   return NULL;}
    void *visit(safeuint_type_name_c *symbol)    {s4o.print("UINT");    return NULL;}
    void *visit(safeudint_type_name_c *symbol)   {s4o.print("UDINT");   return NULL;}
    void *visit(safeulint_type_name_c *symbol)   {s4o.print("ULINT");   return NULL;}
    void *visit(safereal_type_name_c *symbol)    {s4o.print("REAL");    return NULL;}
    void *visit(safelreal_type_name_c *symbol)   {s4o.print("LREAL");   return NULL;}
    void *visit(safedate_type_name_c *symbol)    {s4o.print("DATE");    return NULL;}
    void *visit(safetod_type_name_c *symbol)     {s4o.print("TOD");     return NULL;}
    void *visit(safedt_type_name_c *symbol)      {s4o.print("DT");      return NULL;}
    void *visit(safebyte_type_name_c *symbol)    {s4o.print("BYTE");    return NULL;}
    void *visit(safeword_type_name_c *symbol)    {s4o.print("WORD");    return NULL;}
    void *visit(safelword_type_name_c *symbol)   {s4o.print("LWORD");   return NULL;}
    void *visit(safedword_type_name_c *symbol)   {s4o.print("DWORD");   return NULL;}
    void *visit(safestring_type_name_c *symbol)  {s4o.print("STRING");  return NULL;}
    void *visit(safewstring_type_name_c *symbol) {s4o.print("WSTRING"); return NULL;}

/********************************/
/* B.1.3.2 - Generic data types */
/********************************/
  /* originally empty... */

/********************************/
/* B 1.3.3 - Derived data types */
/********************************/
  /* leave for derived classes... */

/*********************/
/* B 1.4 - Variables */
/*********************/
void *visit(symbolic_variable_c *symbol) {
  TRACE("symbolic_variable_c");
  this->print_variable_prefix();
  symbol->var_name->accept(*this);
  return NULL;
}

/********************************************/
/* B.1.4.1   Directly Represented Variables */
/********************************************/
void *visit(direct_variable_c *symbol) {
  TRACE("direct_variable_c");
  /* Do not use print_token() as it will change everything into uppercase */
  return s4o.printlocation(symbol->value);
}



/*************************************/
/* B.1.4.2   Multi-element Variables */
/*************************************/
#if 0
/*  subscripted_variable '[' subscript_list ']' */
SYM_REF2(array_variable_c, subscripted_variable, subscript_list)

#endif

/*  record_variable '.' field_selector */
/*  WARNING: input and/or output variables of function blocks
 *           may be accessed as fields of a structured variable!
 *           Code handling a structured_variable_c must take
 *           this into account!
 */
// SYM_REF2(structured_variable_c, record_variable, field_selector)
void *visit(structured_variable_c *symbol) {
  TRACE("structured_variable_c");

  symbol->record_variable->accept(*this);
  s4o.print(".");
  symbol->field_selector->accept(*this);
  return NULL;
}

/******************************************/
/* B 1.4.3 - Declaration & Initialisation */
/******************************************/
  /* leave for derived classes... */

/**************************************/
/* B.1.5 - Program organization units */
/**************************************/
/***********************/
/* B 1.5.1 - Functions */
/***********************/
  /* leave for derived classes... */

/*****************************/
/* B 1.5.2 - Function Blocks */
/*****************************/
  /* leave for derived classes... */

/**********************/
/* B 1.5.3 - Programs */
/**********************/
  /* leave for derived classes... */

/*********************************************/
/* B.1.6  Sequential function chart elements */
/*********************************************/

/********************************/
/* B 1.7 Configuration elements */
/********************************/
  /* leave for derived classes... */

/****************************************/
/* B.2 - Language IL (Instruction List) */
/****************************************/
/***********************************/
/* B 2.1 Instructions and Operands */
/***********************************/
  /* leave for derived classes... */

/*******************/
/* B 2.2 Operators */
/*******************/
  /* leave for derived classes... */


/***************************************/
/* B.3 - Language ST (Structured Text) */
/***************************************/
/***********************/
/* B 3.1 - Expressions */
/***********************/
  /* leave for derived classes... */

/********************/
/* B 3.2 Statements */
/********************/
  /* leave for derived classes... */

/*********************************/
/* B 3.2.1 Assignment Statements */
/*********************************/
  /* leave for derived classes... */

/*****************************************/
/* B 3.2.2 Subprogram Control Statements */
/*****************************************/
  /* leave for derived classes... */

/********************************/
/* B 3.2.3 Selection Statements */
/********************************/
  /* leave for derived classes... */

/********************************/
/* B 3.2.4 Iteration Statements */
/********************************/
  /* leave for derived classes... */


}; /* class generate_c_basic_c */














