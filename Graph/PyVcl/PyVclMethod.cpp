/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#include "Platform.h"
#pragma hdrstop
#include "Python.hpp"
#include "PyVclMethod.h"
#include "PyVcl.h"
#include "PyVclConvert.h"
#include "PyVclObject.h"
#include "PyVclRecord.h"
#include "FindClass.hpp"

namespace Python
{
PyTypeObject *VclMethod_Type = NULL;
//---------------------------------------------------------------------------
struct TVclMethod
{
	PyObject_HEAD
  TObject *Instance;  //Pointer to object. NULL if we only have class methods
  void *RecordInstance;
  TRttiType *Type;    //Pointer to type of instance. Used for class methods; Can be NULL if Instance is not NULL.
	PyObject *Object; //NULL if we are not bound to a Python object
	DynamicArray<TRttiMethod*> Methods;
};
//---------------------------------------------------------------------------
/** Return a string reprecentation of the VCL method.
 *  \return New reference
 */
static PyObject *VclMethod_Repr(TVclMethod* self)
{
	String Str = L'<';
	int VirtualIndex = MaxInt;
  TRttiType *Parent = NULL;
	for(int I = 0; I < self->Methods.get_length(); I++)
	{
		TRttiMethod *Method = self->Methods[I];
    //Ignore constructor that are not defined in the last class with constructors
    if(Method->IsConstructor)
    {
      if(Parent == NULL)
        Parent = Method->Parent;
      else if(Method->Parent != Parent)
        continue;
    }

    //Filter away methods that are the same as the previous in a derived class.
		if(Method->VirtualIndex != VirtualIndex)
			Str += Method->ToString() + ", ";
    if(Method->DispatchKind == dkVtable)
  		VirtualIndex = Method->VirtualIndex;
	}
	Str.Delete(Str.Length()-1, 2); //Remove last ", "
	TComponent *Component = dynamic_cast<TComponent*>(self->Instance);
	if(Component != NULL)
		Str += " of <object '" + Component->Name + "' of type '" + Component->ClassName() + "'>>";
	else if(self->Instance)
		Str += " of <object of type '" + self->Instance->ClassName() + "'>>";
  else
    Str += " of <class '" + self->Type->Name + "'>>";
	return ToPyObject(Str);
}
//---------------------------------------------------------------------------
/** Call a method of a VCL object or a constructor of a VCL type.
 *  The methods are considered in order and the first that matches the arguments
 *  will be used.
 *  \param Type: The type of the object.
 *  \param Instance: The VCL object of the method; NULL if it is a constructor
 *  \param Methods: The methods to consider
 *  \param Args: Arguments that will be passed to the method.
 *  \return The result of the called method or the newly constructed object if it is a constructor.
 *          The returned object is only owned if the method was a constructor.
 */
PyObject* CallObjectMethod(TRttiType *Type, TObject *Instance, DynamicArray<TRttiMethod*> &Methods, PyObject *Args)
{
	unsigned MethodCount = Methods.get_length();
	TRttiType *Parent = Methods[0]->Parent;
	for(unsigned I = 0; I < MethodCount; I++)
	{
		TRttiMethod *Method = Methods[I];
		if(Instance == NULL) //If looking for constructor
			if(Method->Parent != Parent)
			  break; //Only look for constructors defined in the last class with constructors

		DynamicArray<TRttiParameter*> ParameterTypes = Method->GetParameters();
		std::vector<TValue> Parameters;
		int ParamCount = PyTuple_Size(Args);
		if(ParamCount != ParameterTypes.Length)
    {
			if(MethodCount == 1)
				throw EPyVclError(Method->Name + "() takes exactly " + ParameterTypes.get_length() + " arguments (" + ParamCount + " given)");
			else
				continue;
    }

		try
		{
			TupleToValues(Args, Parameters, ParameterTypes);
		}
		catch(Exception &E)
		{
			if(MethodCount == 1)
				throw;
			continue;
		}

		if(Method->IsConstructor)
		{
			if(Type == NULL)
				throw EPyVclError("Cannot call constructor");

      TMetaClass *MetaClass = Instance ? Instance->ClassType() : Type->AsInstance->MetaclassType;
      TValue *P = Parameters.size() == 0 ? NULL : &Parameters[0];
      int Count = Parameters.size()-1;
			TValue Result = Method->Invoke(MetaClass, P, Count);
      ReverseTupleToValues(Args, Parameters, ParameterTypes);
      return VclObject_Create(Result.AsObject(), true);
		}
		else
    {
      TValue Result;
      {
        TUnlockGIL Dummy; //Only free the GIL while calling the method; ToPyObject() needs the GIL
        if(Method->IsClassMethod || Method->IsStatic)
          Result = Method->Invoke(Instance ? Instance->ClassType() : Type->AsInstance->MetaclassType, Parameters.size() == 0 ? NULL : &Parameters[0], Parameters.size()-1);
        else
          Result = Method->Invoke(Instance, Parameters.size() == 0 ? NULL : &Parameters[0], Parameters.size()-1);
      }
      ReverseTupleToValues(Args, Parameters, ParameterTypes);
      return ToPyObject(Result);
    }
	}

	throw EPyVclError("No suitable overload found for the function " + Methods[0]->Name);
}
//---------------------------------------------------------------------------
/** Call a method of a VCL record.
 *  The methods are considered in order and the first that matches the arguments
 *  will be used.
 *  \param Type: The type of the record.
 *  \param Instance: The record instance, even for constructors
 *  \param Methods: The methods to consider
 *  \param Args: Arguments that will be passed to the method.
 *  \return The result of the called method or Py_None if it was a constructor.
 */
PyObject* CallRecordMethod(const TValue &Instance, DynamicArray<TRttiMethod*> &Methods, PyObject *Args)
{
	unsigned MethodCount = Methods.get_length();
	TRttiType *Parent = Methods[0]->Parent;
	for(unsigned I = 0; I < MethodCount; I++)
	{
		TRttiMethod *Method = Methods[I];
		DynamicArray<TRttiParameter*> ParameterTypes = Method->GetParameters();
		std::vector<TValue> Parameters;
    if(VclRecord_Check(Args))
    {
      if(ParameterTypes.Length != 1)
        continue;
      Parameters.push_back(VclRecord_AsValue(Args));
      if(Parameters.back().TypeInfo != ParameterTypes[0]->ParamType->Handle)
        continue;
    }
    else
    {
      int ParamCount = PyTuple_Size(Args);
      if(ParamCount != ParameterTypes.Length)
      {
        if(MethodCount == 1)
          throw EPyVclError(Method->Name + "() takes exactly " + ParameterTypes.get_length() + " arguments (" + ParamCount + " given)");
        else
          continue;
      }
      try
      {
        TupleToValues(Args, Parameters, ParameterTypes);
      }
      catch(Exception &E)
      {
        if(MethodCount == 1)
          throw;
        continue;
      }
    }

    TValue *P = Parameters.size() == 0 ? NULL : &Parameters[0];
    int Count = Parameters.size()-1;
    TValue Result;
    if(Method->IsStatic && Method->ReturnType && Method->ReturnType->IsRecord)
    {
      //Workaround of bug in TRttiRecordMethod.DispatchInvoke().
      //PushSelfFirst() and PushSelfLast() should not be called when IsStatic is true.
      //This causes the function to put the result in what is actually the instance when the result is a record.
      TValue::Make((void*)NULL, Method->ReturnType->Handle, Result);
      Method->Invoke(Result, P, Count);
    }
    else
      Result = Method->Invoke(Instance, P, Count);
    return ToPyObject(Result);
	}

	throw EPyVclError("No suitable overload found for the function " + Methods[0]->Name);
}
//---------------------------------------------------------------------------
/** Call the first suitable method refferenced by the VclMethod object.
 *  Keyword arguments are not allowed as they cannot be mapped to arguemnts to a
 *  VCL method.
 */
static PyObject *VclMethod_Call(TVclMethod* self, PyObject *args, PyObject *keywds)
{
	try
	{
		if(keywds != NULL)
			return SetErrorString(PyExc_TypeError, self->Methods[0]->Name + "() does not accept keyword arguments");
    if(self->RecordInstance || (self->Type && self->Type->IsRecord))
    {
      TValue Inst;
      TValue::Make(&self->RecordInstance, PointerTypeInfo, Inst);
      return CallRecordMethod(Inst, self->Methods, args);
    }
		return CallObjectMethod(self->Type, self->Instance, self->Methods, args);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Destructor for VclMethod.
 */
static void VclMethod_Dealloc(TVclMethod* self)
{
	self->Methods.~DynamicArray();
  Py_XDECREF(self->Object);

  //It is okay to call PyObject_Del() directly when the object was allocated with PyObject_New()
  //and the type cannot be subclassed. Otherwise we should use this, which does not compile with the limited API:
  //Py_TYPE(self)->tp_free(reinterpret_cast<PyObject*>(self));
  PyObject_Del(reinterpret_cast<PyObject*>(self));
}
//---------------------------------------------------------------------------
/** VclMethod is a proxy for one or more methods of a VCL object.
 */
static PyType_Slot VclMethod_Slots[] =
{
  {Py_tp_doc,	(void*) PROJECT_NAME " method object"},
  {Py_tp_dealloc, VclMethod_Dealloc},
  {Py_tp_repr, VclMethod_Repr},
  {Py_tp_call, VclMethod_Call},
  {0, NULL}
};

static PyType_Spec VclMethod_Spec =
{
  GUI_TYPE "Method",   //name
  sizeof(TVclMethod),  //basicsize
  0,                   //itemsize
  Py_TPFLAGS_DEFAULT,  //flags
  VclMethod_Slots,     //slots
};
//---------------------------------------------------------------------------
/** Initialize the VclMethod_Type type object.
 *  \return true on success
 */
bool VclMethod_InitType()
{
  VclMethod_Type = reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&VclMethod_Spec));
  return VclMethod_Type != NULL;
}
//---------------------------------------------------------------------------
/** Create new VclMethod object, which is a proxy object to one or more methods of
 *  a VCL object.
 *  \param Object: Reference to Python object; Can be NULL. Only used to keep a reference to the object alive.
 *  \param Instance: The VCL object of the methods.
 *  \param Methods: The methods we are creating a proxy for.
 *  \return New reference to a VclMethod
 */
PyObject* VclMethod_Create(TVclObject *Object, TObject *Instance, const DynamicArray<TRttiMethod*> &Methods)
{
	TVclMethod *VclMethod = PyObject_New(TVclMethod, VclMethod_Type);
	new(&VclMethod->Methods) DynamicArray<TRttiMethod*>(Methods);
  Py_XINCREF(Object);
	VclMethod->Object = reinterpret_cast<PyObject*>(Object);
  VclMethod->Instance = Instance;
  VclMethod->RecordInstance = NULL;
  VclMethod->Type = NULL;
	return reinterpret_cast<PyObject*>(VclMethod);
}
//---------------------------------------------------------------------------
/** Create new VclMethod object, which is a proxy object to one or more methods of
 *  a VCL record.
 *  \param Object: Reference to Python object; Can be NULL. Only used to keep a reference to the object alive.
 *  \param Instance: The record instance of the methods.
 *  \param Methods: The methods we are creating a proxy for.
 *  \return New reference to a VclMethod
 */
PyObject* VclMethod_Create(TVclRecord *Object, void *Instance, const DynamicArray<TRttiMethod*> &Methods)
{
	TVclMethod *VclMethod = PyObject_New(TVclMethod, VclMethod_Type);
	new(&VclMethod->Methods) DynamicArray<TRttiMethod*>(Methods);
  Py_XINCREF(Object);
	VclMethod->Object = reinterpret_cast<PyObject*>(Object);
  VclMethod->Instance = NULL;
  VclMethod->RecordInstance = Instance;
  VclMethod->Type = NULL;
	return reinterpret_cast<PyObject*>(VclMethod);
}
//---------------------------------------------------------------------------
/** Create new VclMethod object, which is a proxy object to one or more class
 *   methods of a VCL class. This functions is used for class methods and static functions.
 *  \param Type: Type for class methods.
 *  \param Methods: The methods we are creating a proxy for.
 *  \return New reference to a VclMethod
 */
PyObject* VclMethod_Create(TRttiType *Type, const DynamicArray<TRttiMethod*> &Methods)
{
	TVclMethod *VclMethod = PyObject_New(TVclMethod, VclMethod_Type);
	new(&VclMethod->Methods) DynamicArray<TRttiMethod*>(Methods);
	VclMethod->Object = NULL;
  VclMethod->Instance = NULL;
  VclMethod->RecordInstance = NULL;
  VclMethod->Type = Type;
	return reinterpret_cast<PyObject*>(VclMethod);
}
//---------------------------------------------------------------------------
} //namespace Python
