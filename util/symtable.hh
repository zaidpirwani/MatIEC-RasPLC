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
/*
 * A generic symbol table.
 *
 * This is used to create symbol tables such as a list of
 * variables currently in scope, a list of previously defined
 * functions, etc...
 */



#ifndef _SYMTABLE_HH
#define _SYMTABLE_HH

#include "../absyntax/absyntax.hh"

#include <map>
#include <string>




template<typename value_type, value_type null_value> class symtable_c {
  /* Case insensitive string compare copied from
   * "The C++ Programming Language" - 3rd Edition
   * by Bjarne Stroustrup, ISBN 0201889544.
   */
  class nocase_c {
    public:
      bool operator() (const std::string& x, const std::string& y) const {
        std::string::const_iterator ix = x.begin();
        std::string::const_iterator iy = y.begin();

        for(; (ix != x.end()) && (iy != y.end()) && (toupper(*ix) == toupper(*iy)); ++ix, ++iy);
        if (ix == x.end()) return (iy != y.end());
        if (iy == y.end()) return false;
        return (toupper(*ix) < toupper(*iy));
      };
  };

  public:
    typedef value_type value_t;

  private:
    /* Comparison between identifiers must ignore case, therefore the use of nocase_c */
    typedef std::map<std::string, value_t, nocase_c> base_t;
    base_t _base;

  public:
  typedef typename base_t::iterator iterator;
  typedef typename base_t::const_iterator const_iterator;
  typedef typename base_t::reverse_iterator reverse_iterator;
  typedef typename base_t::const_reverse_iterator const_reverse_iterator;

  private:
      /* pointer to symbol table of the next inner scope */
    symtable_c *inner_scope;

  public:
    symtable_c(void);

    void reset(void); /* clear all entries... */

    void push(void); /* create new inner scope */
    int  pop(void);  /* clear most inner scope */

    void set(const char *identifier_str, value_t value);
    void set(const symbol_c *symbol, value_t value);
    void insert(const char *identifier_str, value_t value);
    void insert(const symbol_c *symbol, value_t value);

    /* Search for an entry. Will return end_value() if not found */
    value_t end_value(void) {return null_value;}
    value_t find_value(const char *identifier_str);
    value_t find_value(const symbol_c *symbol);

    iterator find(const char *identifier_str) {return _base.find(identifier_str);}

  /* iterators pointing to beg/end of map... */
    iterator begin() 			{return _base.begin();}
    const_iterator begin() const	{return _base.begin();}
    iterator end()			{return _base.end();}
    const_iterator end() const 		{return _base.end();}
    reverse_iterator rbegin()		{return _base.rbegin();}
    const_reverse_iterator rbegin() const {return _base.rbegin();}
    reverse_iterator rend() 		{return _base.rend();}
    const_reverse_iterator rend() const	{return _base.rend();}

    /* debuging function... */
    void print(void);
};



/* Templates must include the source into the code! */
#include "symtable.cc"

#endif /*  _SYMTABLE_HH */
