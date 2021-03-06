/*******************************************************************\

Module: C++ Language Type Checking

Author: Daniel Kroening, kroening@cs.cmu.edu

\*******************************************************************/

#include <util/source_location.h>

#include <ansi-c/c_qualifiers.h>

#include "cpp_typecheck.h"
#include "cpp_convert_type.h"
#include "expr2cpp.h"

/*******************************************************************\

Function: cpp_typecheckt::typecheck_type

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void cpp_typecheckt::typecheck_type(typet &type)
{
  assert(type.id()!=irep_idt());
  assert(type.is_not_nil());

  try
  {
    cpp_convert_plain_type(type);
  }

  catch(const char *err)
  {
    error().source_location=type.source_location();
    error() << err << eom;
    throw 0;
  }

  catch(const std::string &err)
  {
    error().source_location=type.source_location();
    error() << err << eom;
    throw 0;
  }

  if(type.id()==ID_cpp_name)
  {
    c_qualifierst qualifiers(type);

    cpp_namet cpp_name;
    cpp_name.swap(type);

    exprt symbol_expr=resolve(
      cpp_name,
      cpp_typecheck_resolvet::TYPE,
      cpp_typecheck_fargst());

    if(symbol_expr.id()!=ID_type)
    {
      error().source_location=type.source_location();
      error() << "error: expected type" << eom;
      throw 0;
    }
    
    type=symbol_expr.type();
    assert(type.is_not_nil());

    if(type.get_bool(ID_C_constant))
      qualifiers.is_constant = true;

    qualifiers.write(type);
  }
  else if(type.id()==ID_struct ||
          type.id()==ID_union)
  {
    typecheck_compound_type(to_struct_union_type(type));
  }
  else if(type.id()==ID_pointer)
  {
    // the pointer might have a qualifier, but do subtype first
    typecheck_type(type.subtype());

    // Check if it is a pointer-to-member
    if(type.find("to-member").is_not_nil())
    {
      // these can point either to data members or member functions
      // of a class

      typet &class_object=static_cast<typet &>(type.add("to-member"));

      if(class_object.id()==ID_cpp_name)
      {
        assert(class_object.get_sub().back().id()=="::");
        class_object.get_sub().pop_back();
      }

      typecheck_type(class_object);

      // there may be parameters if this is a pointer to member function
      if(type.subtype().id()==ID_code)
      {
        irept::subt &parameters=type.subtype().add(ID_parameters).get_sub();

        if(parameters.empty() ||
           parameters.front().get(ID_C_base_name)!=ID_this)
        {
          // Add 'this' to the parameters
          exprt a0(ID_parameter);
          a0.set(ID_C_base_name, ID_this);
          a0.type().id(ID_pointer);
          a0.type().subtype() = class_object;
          parameters.insert(parameters.begin(), a0);
        }
      }
    }

    // now do qualifier
    if(type.find(ID_C_qualifier).is_not_nil())
    {
      typet &t=static_cast<typet &>(type.add(ID_C_qualifier));
      cpp_convert_plain_type(t);
      c_qualifierst q(t);
      q.write(type);
    }

    type.remove(ID_C_qualifier);
  }
  else if(type.id()==ID_array)
  {
    exprt &size_expr=to_array_type(type).size();

    if(size_expr.is_not_nil())
      typecheck_expr(size_expr);

    typecheck_type(type.subtype());

    if(type.subtype().get_bool(ID_C_constant))
      type.set(ID_C_constant, true);

    if(type.subtype().get_bool(ID_C_volatile))
      type.set(ID_C_volatile, true);
  }
  else if(type.id()==ID_code)
  {
    code_typet &code_type=to_code_type(type);
    typecheck_type(code_type.return_type());

    code_typet::parameterst &parameters=code_type.parameters();

    for(code_typet::parameterst::iterator it=parameters.begin();
        it!=parameters.end();
        it++)
    {
      typecheck_type(it->type());

      // see if there is a default value
      if(it->has_default_value())
      {
        typecheck_expr(it->default_value());
        implicit_typecast(it->default_value(), it->type());
      }
    }
  }
  else if(type.id()==ID_template)
  {
    typecheck_type(type.subtype());
  }
  else if(type.id()==ID_c_enum)
  {
    typecheck_enum_type(type);
  }
  else if(type.id()==ID_c_enum_tag)
  {
  }
  else if(type.id()==ID_c_bit_field)
  {
    typecheck_c_bit_field_type(to_c_bit_field_type(type));
  }
  else if(type.id()==ID_unsignedbv ||
          type.id()==ID_signedbv ||
          type.id()==ID_bool ||
          type.id()==ID_floatbv ||
          type.id()==ID_fixedbv ||
          type.id()==ID_empty)
  {
  }
  else if(type.id()==ID_symbol)
  {
  }
  else if(type.id()==ID_constructor ||
          type.id()==ID_destructor)
  {
  }
  else if(type.id()=="cpp-cast-operator")
  {
  }
  else if(type.id()=="cpp-template-type")
  {
  }
  else if(type.id()==ID_typeof)
  {
    exprt e=static_cast<const exprt &>(type.find(ID_expr_arg));

    if(e.is_nil())
    {
      typet tmp_type=
        static_cast<const typet &>(type.find(ID_type_arg));

      if(tmp_type.id()==ID_cpp_name)
      {
        // this may be ambiguous -- it can be either a type or
        // an expression

        cpp_typecheck_fargst fargs;

        exprt symbol_expr=resolve(
          to_cpp_name(static_cast<const irept &>(tmp_type)),
          cpp_typecheck_resolvet::BOTH,
          fargs);

        type=symbol_expr.type();
      }
      else
      {
        typecheck_type(tmp_type);
        type=tmp_type;
      }
    }
    else
    {
      typecheck_expr(e);
      type=e.type();
    }
  }
  else if(type.id()==ID_decltype)
  {
    exprt e=static_cast<const exprt &>(type.find(ID_expr_arg));
    typecheck_expr(e);
    type=e.type();
  }
  else if(type.id()==ID_unassigned)
  {
    // ignore, for template parameter guessing
  }
  else if(type.id()==ID_template_class_instance)
  {
    // ok (internally generated)
  }
  else if(type.id()==ID_block_pointer)
  {
    // This is an Apple extension for lambda-like constructs.
    // http://thirdcog.eu/pwcblocks/
  }
  else if(type.id()==ID_nullptr)
  {
  }
  else
  {
    error().source_location=type.source_location();
    error() << "unexpected cpp type: " << type.pretty() << eom;
    throw 0;
  }
  
  assert(type.is_not_nil());
}
