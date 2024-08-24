/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2019 Ivan Johansen
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
#include "PyVclRecord.h"
#include "PyVclMethod.h"
#include "PyVclConvert.h"

namespace Python
{
struct TVclRecord
{
  PyObject_HEAD
  TValue Value;
  void *Instance;
  TRttiType *Type;
  PyObject *Object; //Optional reference to a VclRecord or VclObject, which the record is part of.
};

PyTypeObject *VclRecord_Type = NULL;
//---------------------------------------------------------------------------
/** Returns a Python string that represent the VclRecord.
 */
static PyObject *VclRecord_Repr(TVclRecord* self)
{
	try
	{
    String Str = GUI_NAME "." + self->Type->Name + "(";
    DynamicArray<TRttiField*> Fields = self->Type->GetFields();
    for(int I = 0; I < Fields.Length; I++)
      if(Fields[I]->Visibility == mvPublic)
      {
        Str += Fields[I]->Name;
        Str += "=";
        TPyObjectPtr ValueObject(ToPyObject(Fields[I]->GetValue(self->Instance)), false);
        TPyObjectPtr Repr(PyObject_Repr(ValueObject.get()), false);
        Str += FromPyObject<String>(Repr.get());

        if(I < Fields.Length - 1)
          Str += ", ";
      }
    Str += ")";
		return ToPyObject(Str);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Returns a list of all properties, methods and public fields of the record.
 */
static PyObject* VclRecord_Dir(TVclRecord *self, PyObject *arg)
{
  //Notice: There are no RTTI available for record properties.
  //See https://stackoverflow.com/questions/23445775/how-to-access-record-properties/23450935#23450935
	PyObject *List = PyList_New(0);
  DynamicArray<TRttiMethod*> Methods = self->Type->GetMethods();
  DynamicArray<TRttiField*> Fields = self->Type->GetFields();
  int MethodCount = Methods.Length;
  int FieldCount = Fields.Length;
  PyObject *Value;
  for(int I = 0; I < MethodCount; I++)
  {
    Value = ToPyObject(Methods[I]->Name);
    if(!PySequence_Contains(List, Value))
      PyList_Append(List, Value);
    Py_DECREF(Value);
  }
  for(int I = 0; I < FieldCount; I++)
    if(Fields[I]->Visibility == mvPublic)
    {
      Value = ToPyObject(Fields[I]->Name);
      PyList_Append(List, Value);
      Py_DECREF(Value);
    }
	return List;
}
//---------------------------------------------------------------------------
/** Destructor for the TVclRecord.
 */
static void VclRecord_Dealloc(TVclRecord* self)
{
  Py_XDECREF(self->Object);
  delete self;
}
//---------------------------------------------------------------------------
/** Retrieve a method, property or field. If a property or field is retrieved,
 *  we try to convert it to a built-in type, else a new TVclObject is created to
 *  reference it without owning it.
 *  If a method is retrieved, a TVclMethod object is returned, which may be used
 *  to call the method.
 */
static PyObject* VclRecord_GetAttro(TVclRecord *self, PyObject *attr_name)
{
  //There are no RTTI available for record properties.
	try
	{
		String Name = FromPyObject<String>(attr_name);
    DynamicArray<TRttiMethod*> Methods = self->Type->GetMethods(Name);
	  if(Methods.Length != 0 && !Methods[0]->IsDestructor)
			return VclMethod_Create(self, self->Instance, Methods);

    TRttiField *Field = self->Type->GetField(Name);
    if(Field == NULL || Field->Visibility != mvPublic)
    {
      PyObject *Result = PyObject_GenericGetAttr(reinterpret_cast<PyObject*>(self), attr_name);
      if(Result == NULL)
        SetErrorString(PyExc_AttributeError, "'" + self->Type->Name + "' object has no attribute '" + Name + "'");
      return Result;
    }

    if(Field->FieldType->IsRecord)
      return VclRecord_Create(static_cast<BYTE*>(self->Instance) + Field->Offset, Field->FieldType, reinterpret_cast<PyObject*>(self));
    TValue Result = Field->GetValue(self->Instance);
    return ToPyObject(Result);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Set the value of a public property or field in a TVclRecord. If an event is
 *  set, we use a generic event, which will convert the parameters and call the
 *  Python event. If the Python event is a bound method, we store a weak reference
 *  to the instance of the bound method to avoid circular references.
 */
static int VclRecord_SetAttro(TVclRecord *self, PyObject *attr_name, PyObject *v)
{
  //There are not RTTI available for record properties.
	try
	{
		String Name = FromPyObject<String>(attr_name);
		TRttiField *Field = self->Type->GetField(Name);
		if(Field == NULL || Field->Visibility != mvPublic)
		{
			int Result = PyObject_GenericSetAttr(reinterpret_cast<PyObject*>(self), attr_name, v);
			if(Result == -1)
				SetErrorString(PyExc_AttributeError, "'" + self->Type->Name + "' object has no attribute '" + Name + "'");
			return Result;
		}

    Field->SetValue(self->Instance, ToValue(v, Field->FieldType->Handle));
		return 0;
	}
	catch(...)
	{
		PyVclHandleException();
		return -1;
	}
}
//---------------------------------------------------------------------------
/** Compares two TVclRecord objects. Unordered compare (<, <=, >, >=) is not
 *  supported.
 *  \param self: The TVclRecord object.
 *  \param other: TVclRecord object to compare with.
 *  \return True or False depending on the compare operator.
 */
static PyObject* VclRecord_RichCompare(TVclRecord *self, PyObject *other, int op)
{
  return PyErr_Format(PyExc_TypeError, "Operator not supported between types '%s' and '%s'", GetTypeName(&self->ob_base).c_str(), GetTypeName(other).c_str());
}
//---------------------------------------------------------------------------
static PyMethodDef VclRecord_Methods[] =
{
	{"__dir__", (PyCFunction)VclRecord_Dir, METH_NOARGS, ""},
	{NULL, NULL, 0, NULL}
};
//---------------------------------------------------------------------------
/** Proxy object for a VCL object. If _owned is set to True, the VCL object is
 *  owned by the proxy and destroyed when the proxy is destroyed.
 *  If possible the VCL object will own a delete handler, which will clear the
 *  reference in the proxy if the VCL object is destroyed before the proxy.
 */
static PyType_Slot VclRecord_Slots[] =
{
  {Py_tp_doc,	(void*) PROJECT_NAME " record object"},
  {Py_tp_dealloc, VclRecord_Dealloc},
  {Py_tp_repr, VclRecord_Repr},
  {Py_tp_getattro, VclRecord_GetAttro},
  {Py_tp_setattro, VclRecord_SetAttro},
  {Py_tp_richcompare, VclRecord_RichCompare},
	{Py_tp_methods, VclRecord_Methods},
  {0, NULL}
};

static PyType_Spec VclRecord_Spec =
{
  GUI_TYPE "Record",   //name
  sizeof(TVclRecord),  //basicsize
  0,                   //itemsize
  Py_TPFLAGS_DEFAULT,  //flags
  VclRecord_Slots,     //slots
};
//---------------------------------------------------------------------------
/** Initialize the VclRecord_Type type object.
 *  \return true on success
 */
bool VclRecord_InitType()
{
  VclRecord_Type = reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&VclRecord_Spec));
  return VclRecord_Type != NULL;
}
//---------------------------------------------------------------------------
/** Create new VclRecord.
 *  \param Value: VCL record value to be copied to the python object.
 *  \return New reference to VclRecord.
 */
PyObject* VclRecord_Create(const TValue &Value)
{
  TVclRecord *O = new TVclRecord;
  PyObject_Init(reinterpret_cast<PyObject*>(O), VclRecord_Type);
  O->Value = Value;
  O->Instance = O->Value.GetReferenceToRawData();
  O->Type = Context.GetType(O->Value.TypeInfo);
  O->Object = NULL;
  return reinterpret_cast<PyObject*>(O);
}
//---------------------------------------------------------------------------
/** Create new VclRecord.
 *  \param Instance: Pointer to the record instance.
 *  \param Type: The type of the record.
 *  \param Object: A PyObject to keep a reference to.
 *  \return New reference to VclRecord.
 */
PyObject* VclRecord_Create(void *Instance, TRttiType *Type, PyObject *Object)
{
  TVclRecord *O = new TVclRecord;
  PyObject_Init(reinterpret_cast<PyObject*>(O), VclRecord_Type);
  O->Instance = Instance;
  O->Type = Type;
  O->Object = Object;
  Py_XINCREF(Object);
  return reinterpret_cast<PyObject*>(O);
}
//---------------------------------------------------------------------------
/** Return true if O is a VclRecord.
 */
bool VclRecord_Check(PyObject *O)
{
	return O->ob_type == VclRecord_Type;
}
//---------------------------------------------------------------------------
/** Return the record TValue, which the given VclRecord is storing.
 *  \param O: Pointer to a VclObject instance.
 *  \return TValue of the record
 *  \throw EPyVclError if O is not a VclRecord.
 */
TValue VclRecord_AsValue(PyObject *O)
{
	if(O->ob_type != VclRecord_Type)
		throw EPyVclError("Object is not a " GUI_TYPE "Record type");
  TVclRecord *Record = reinterpret_cast<TVclRecord*>(O);
  if(!Record->Value.IsEmpty)
    return Record->Value;
  TValue Result;
  TValue::Make(Record->Instance, Record->Type->Handle, Result);
  return Result;
}
//---------------------------------------------------------------------------
TTypeInfo* VclRecord_GetTypeInfo(PyObject *O)
{
  if(O->ob_type != VclRecord_Type)
    return NULL;
  TVclRecord *Record = reinterpret_cast<TVclRecord*>(O);
  return Record->Type->Handle;
}
//---------------------------------------------------------------------------
} //namespace Python
