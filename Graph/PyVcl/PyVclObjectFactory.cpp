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
namespace Python
{
PyTypeObject *VclObjectFactory_Type = NULL;
//---------------------------------------------------------------------------
struct TVclObjectFactory
{
	PyObject_HEAD
	TRttiType *RttiType;
};
//---------------------------------------------------------------------------
/** Returns a Python string that represent the VclObjectFactory.
 */
static PyObject *VclObjectFactory_Repr(TVclObjectFactory* self)
{
	try
	{
		String Str = "<class '" + self->RttiType->Name + "'>";
		return ToPyObject(Str);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Constructor for the proxy type. This will retrieve a list of Delphi constructors
 *  for the referenced type and call the first one that fits the arguments.
 *  This will be stored in an returned in a new VclObject.
 *  \param self: The object factory.
 *  \param args: Tuple with arguments converted and passed to the actual constructor.
 *  \param kwds: Keyword arguments used to set properties after the object is created.
 *  \return New reference to VclObject.
 */
static PyObject* VclObjectFactory_Call(TVclObjectFactory *self, PyObject *args, PyObject *kwds)
{
	try
	{
    DynamicArray<TRttiMethod*> Methods = self->RttiType->GetMethods("Create");

    if(self->RttiType->IsRecord)
    {
      TValue Result;
      TValue::Make((void*)NULL, self->RttiType->Handle, Result); //void* cast is needed; Otherwise NULL is treated as an int
      if(Methods.Length > 0)
      {
        CallRecordMethod(Result, Methods, args);
      }
      else
      {
        DynamicArray<TRttiField*> Fields = self->RttiType->GetFields();
        if(PyTuple_Size(args) != Fields.Length)
          throw EPyVclError(L"Expected %u arguments", Fields.Length);
        for(int I = 0; I < Fields.Length; I++)
          Fields[I]->SetValue(Result.GetReferenceToRawData(), ToValue(PyTuple_GetItem(args, I), Fields[I]->FieldType->Handle));
      }
      return VclRecord_Create(Result);
    }
    else
    {
      if(Methods.Length == 0)
        throw EPyVclError("No constructor found");
      PyObject *VclObject = CallObjectMethod(self->RttiType, NULL, Methods, args);
      if(kwds)
      {
        PyObject *Key, *Value;
        Py_ssize_t Pos = 0;
        while(PyDict_Next(kwds, &Pos, &Key, &Value))
          if(PyObject_SetAttr(VclObject, Key, Value) == -1)
          {
            Py_DECREF(VclObject);
            return NULL;
          }
      }
      return VclObject;
    }
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Retrieve a class method of the type.
 *  A TVclMethod object is returned, which may be used to call the method.
 */
static PyObject* VclObjectFactory_GetAttro(TVclObjectFactory *self, PyObject *attr_name)
{
	try
	{
		String Name = FromPyObject<String>(attr_name);
		TRttiType *Type = self->RttiType;
    DynamicArray<TRttiMethod*> Methods = Type->GetMethods(Name);
    DynamicArray<TRttiMethod*> ClassMethods;
    for(int I = 0; I < Methods.Length; I++)
      if(Methods[I]->IsStatic || Methods[I]->IsClassMethod || Methods[I]->IsConstructor)
      {
        ClassMethods.Length = ClassMethods.Length + 1;
        ClassMethods[ClassMethods.Length - 1] = Methods[I];
      }
    if(ClassMethods.Length != 0)
      return VclMethod_Create(self->RttiType, ClassMethods);
    return SetErrorString(PyExc_AttributeError, "'" + Type->Name + "' class has no class method '" + Name + "'");
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Returns a list of all class methods and static functions of the class.
 */
static PyObject* VclObjectFactory_Dir(TVclObjectFactory *self, PyObject *arg)
{
	PyObject *List = PyList_New(0);
  DynamicArray<TRttiMethod*> Methods = self->RttiType->GetMethods();
  int MethodCount = Methods.Length;
  for(int I = 0; I < MethodCount; I++)
  {
    TRttiMethod *Method = Methods[I];
    if(Method->IsStatic || Method->IsClassMethod || Method->IsConstructor)
    {
      PyObject *Value = ToPyObject(Methods[I]->Name);
      if(!PySequence_Contains(List, Value))
        PyList_Append(List, Value);
      Py_DECREF(Value);
		}
	}
	return List;
}
//---------------------------------------------------------------------------
static PyMethodDef VclObjectFactory_Methods[] =
{
	{"__dir__", (PyCFunction)VclObjectFactory_Dir, METH_NOARGS, ""},
	{NULL, NULL, 0, NULL}
};
//---------------------------------------------------------------------------
/** VclObjectFactory is a fake type. A VclObjectFactory object will be created when accessing a type like vcl.TForm.
 *  It is a wrapper around a Delphi type. When the object is called, like vcl.TForm(), a VclObject object is created
 *  owning a new instance of the Delphi type. Unnamed arguments will be passed to the constructor.
 *  Named arguments will be assigned as if they were set as attributes afterwards.
 *  F = vcl.TForm(None, Caption="Hello world")
 *  is the same as:
 *  F = vcl.TForm(None)
 *  F.Caption = "Hello world"
 *
 *  An object factory is used instead of a real type, because it doesn't seem to be possible to create a type with
 *  the limited API that has space for the TRttiType pointer.
 */
static PyType_Slot VclObjectFactory_Slots[] =
{
  {Py_tp_doc,	(void*) PROJECT_NAME " object factory"},
  {Py_tp_repr, VclObjectFactory_Repr},
  {Py_tp_call, VclObjectFactory_Call},
  {Py_tp_getattro, VclObjectFactory_GetAttro},
	{Py_tp_methods, VclObjectFactory_Methods},
  {0, NULL}
};

static PyType_Spec VclObjectFactory_Spec =
{
  GUI_TYPE "ObjectFactory",   //name
  sizeof(TVclObjectFactory),  //basicsize
  0,                    //itemsize
  Py_TPFLAGS_DEFAULT,   //flags
  VclObjectFactory_Slots,     //slots
};
//---------------------------------------------------------------------------
/** Initialize the VclObjectFactory_Type type object.
 *  \return true on success
 */
bool VclObjectFactory_InitType()
{
  VclObjectFactory_Type = reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&VclObjectFactory_Spec));
  return VclObjectFactory_Type != NULL;
}
//---------------------------------------------------------------------------
/** Create a new VclObjectFactory object representing a Delphi class.
 *  This could for example be vcl.TForm
 *  \param Type: The Delphi class to represent.
 *  \return New reference to a VclObjectFactory object.
 */
PyObject* VclObjectFactory_Create(TRttiType *Type)
{
	TVclObjectFactory *VclObjectFactory = PyObject_New(TVclObjectFactory, VclObjectFactory_Type);
	VclObjectFactory->RttiType = Type;
  //We need to increment refcnt for the type as it is decremented by the default tp_dealloc when the object is destroyed
  Py_INCREF(VclObjectFactory_Type);
	return reinterpret_cast<PyObject*>(VclObjectFactory);
}
//---------------------------------------------------------------------------
} //namespace Python
