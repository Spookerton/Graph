/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#define _XKEYCHECK_H	//Disable check for defining keywords
#define private public  //Nasty hack to get access to TMethodImplementation::FUserData
#define protected public //Because TMethodImplementation::FInvokeInfo is protected in XE4
#include <Rtti.hpp>
#undef private
#undef protected
#include "Platform.h"
#pragma hdrstop
#include "Python.hpp"
#include "PyVclObject.h"
#include "PyVclMethod.h"
#include "PyVcl.h"
#include "PyVclConvert.h"
#include "PyVclIndexedProperty.h"
#include "PyVclRef.h"
#include <deque>
#include <algorithm>

namespace Python
{
PyTypeObject *VclObject_Type = NULL;
//---------------------------------------------------------------------------
/** This object is created when needed and owned by a TComponent object which should
 *  be monitored for destruction. Delphi objects not derived from TComponent cannot
 *  be monitored. Only one TObjectDeleteHandler is needed for every object monitored,
 *  and it can always be found by its name "ObjectDeleteHandler".
 *
 *  VclObject objects will register themself to be notified when the referred object
 *  is destroyed. Only one VCL object is registered. This object is reused instead of creating
 *  more proxy objects to the same instance.
 *  Callable Python objects used in events are also tracked for cleanup when
 *  the monitored object is destroyed.
 */
class TObjectDeleteHandler : public TComponent
{
	TVclObject* VclObject;
  std::deque<PyObject*> Objects;
public:
	__fastcall TObjectDeleteHandler(TComponent *AOwner) : TComponent(AOwner)
	{
		Name = L"ObjectDeleteHandler";
	}

	__fastcall ~TObjectDeleteHandler()
	{
		if(VclObject)
		{
			VclObject->Instance = NULL;
			VclObject->Owned = false;
    }

    //This may be called after Py_Finalize() has been called.
    if(Py_IsInitialized() && !Objects.empty())
    {
      TLockGIL Dummy; //Remember to hold the GIL when Py_DECREF is called
  		for(std::deque<PyObject*>::iterator Iter = Objects.begin(); Iter != Objects.end(); ++Iter)
        Py_DECREF(*Iter);
    }
	}

  TVclObject* GetVclObject() {return VclObject;}
	void SetVclObject(TVclObject *AVclObject) {VclObject = AVclObject;}
  void Add(PyObject *O) {Objects.push_back(O);}
	void Delete(PyObject *O)
	{
		std::deque<PyObject*>::iterator Iter = std::find(Objects.begin(), Objects.end(), O);
		if(Iter != Objects.end())
    {
      Py_DECREF(O);
		  Objects.erase(Iter);
    }
	}
};
//---------------------------------------------------------------------------
/** Find and optionally create a TObjectDeleteHandler associated to a TComponent.
 */
TObjectDeleteHandler* FindDeleteHandler(TObject *Object, bool Create)
{
	if(TComponent *Component = dynamic_cast<TComponent*>(Object))
	{
		TObjectDeleteHandler *DeleteHandler =
			dynamic_cast<TObjectDeleteHandler*>(Component->FindComponent("ObjectDeleteHandler"));
		if(DeleteHandler == NULL && Create)
			DeleteHandler = new TObjectDeleteHandler(Component);
		return DeleteHandler;
	}
  return NULL;
}
//---------------------------------------------------------------------------
/** Returns a Python string that represent the VclObject.
 */
static PyObject *VclObject_Repr(TVclObject* self)
{
	try
	{
		String Str;
		TComponent *Component = dynamic_cast<TComponent*>(self->Instance);
		if(self->Instance == NULL)
			Str = "<Reference to released object>";
		else if(Component != NULL)
			Str = "<object '" + Component->Name + "' of type '" + Component->ClassName() + "'>";
		else
			Str = "<object of type '" + self->Instance->ClassName() + "'>";
		return ToPyObject(Str);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Returns a list of all properties, methods and public fields of the object.
 */
static PyObject* VclObject_Dir(TVclObject *self, PyObject *arg)
{
	PyObject *List = PyList_New(0);
	if(self->Instance != NULL)
	{
		TRttiType *Type = Context.GetType(self->Instance->ClassType());
		DynamicArray<TRttiProperty*> Properties = Type->GetProperties();
		DynamicArray<TRttiMethod*> Methods = Type->GetMethods();
		DynamicArray<TRttiField*> Fields = Type->GetFields();
		int PropertyCount = Properties.Length;
		int MethodCount = Methods.Length;
		int FieldCount = Fields.Length;
		PyObject *Value;
		for(int I = 0; I < PropertyCount; I++)
		{
			Value = ToPyObject(Properties[I]->Name);
			if(!PySequence_Contains(List, Value))
				PyList_Append(List, Value);
			Py_DECREF(Value);
		}
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
	}
	return List;
}
//---------------------------------------------------------------------------
/** Destructor for the VclObject. If the instance is owned, first remove all objects
 *  that have the instance as parent, and then free the owned instance.
 *  If not owned, unregister from the delete handler.
 */
static void VclObject_Dealloc(TVclObject* self)
{
	if(self->Owned)
	{
#if !FIREMONKEY
		if(TWinControl *Control = dynamic_cast<TWinControl*>(self->Instance))
			while(Control->ControlCount)
				Control->Controls[Control->ControlCount-1]->Parent = NULL;
#endif
		delete self->Instance;
	}
	else
    if(TObjectDeleteHandler *DeleteHandler = FindDeleteHandler(self->Instance, false))
	  	DeleteHandler->SetVclObject(NULL);

  //It is okay to call PyObject_Del() directly as the object was created with PyObject_New() and cannot be subclassed.
  PyObject_Del(self);
}
//---------------------------------------------------------------------------
/** Retrieve a method, property or field. If a property or field is retrieved,
 *  we try to convert it to a built-in type, else a new VclObject is created to
 *  refference it without owning it.
 *  If a method is retrieved, a TVclMethod object is returned, which may be used
 *  to call the method.
 */
static PyObject* VclObject_GetAttro(TVclObject *self, PyObject *attr_name)
{
	try
	{
		if(self->Instance == NULL)
			throw EPyVclError("Referenced object has been deleted.");

		TObject *Object = self->Instance;
		String Name = FromPyObject<String>(attr_name);
		TRttiType *Type = Context.GetType(Object->ClassType());
		if(Type == NULL)
			throw EPyVclError("Type not found.");
		TRttiProperty *Property = Type->GetProperty(Name);
		if(Property == NULL)
		{
			DynamicArray<TRttiMethod*> Methods = Type->GetMethods(Name);
			if(Methods.Length != 0 && !Methods[0]->IsDestructor)
				return VclMethod_Create(self, self->Instance, Methods);

			TRttiField *Field = Type->GetField(Name);
			if(Field == NULL || Field->Visibility != mvPublic)
			{
				PyObject *ArrayProperty = VclIndexedProperty_Create(Object, Name);
				if(ArrayProperty != NULL)
					return ArrayProperty;
				PyObject *Result = PyObject_GenericGetAttr(reinterpret_cast<PyObject*>(self), attr_name);
				if(Result == NULL)
					SetErrorString(PyExc_AttributeError, "'" + Type->Name + "' object has no attribute '" + Name + "'");
				return Result;
			}
			TValue Result = Field->GetValue(Object);
			return ToPyObject(Result);
		}
		TValue Result = Property->GetValue(Object);
		return ToPyObject(Result);
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** This creates a weak reference to a callable object. If Func is a bound method,
 *  this will return a bound method weakproxy Func.__self__ to Func.__func__.
 *  Otherwise a normal strong reference is stored to Func to make sure the object is
 *  not destroyed if there are no references, e.g. lambda function.
 *
 *  We use a weak reference to avoid a circular reference like this case:
 *  class FooClass:
 *    def OnShow(self, Sender): pass
 *    def __init__(self):
 *      self.Form = vcl.TForm(None, OnShow=self.OnShow)
 *  Foo = FooClass()
 *  del Foo
 *
 *  In this case Foo contains a reference to Foo.Form. But Foo.Form.OnShow contains
 *  a bound method with a reference Foo. This will prevent Foo from being destroyed.
 *
 *  We cannot use a weak reference to the bound method as the bound method will loose
 *  its strong reference as soon as the weak reference is created.
 *
 *  We cannot change the bound method to use a weak reference as the __self__ attribute
 *  is read only.
 *  \param Func: Callable object used as event.
 *  \return New reference to event object. NULL if a Python exception has been set.
 */
PyObject* CreateWeakEvent(PyObject* Func)
{
  if(Func == Py_None)
    Py_RETURN_NONE;

  if(!PyCallable_Check(Func))
  {
    SetErrorString(PyExc_TypeError, "Event must be callable.");
    throw EAbort(""); //Python exception has already been set
  }

  //Foo.OnShow.__self__ is a reference to Foo and Foo.OnShow.__func__ is a reference to FooClass.OnShow
  if(!PyObject_HasAttrString(Func, "__self__") || !PyObject_HasAttrString(Func, "__func__"))
  {
    Py_INCREF(Func);
    return Func;
  }

  TPyObjectPtr Obj(PyObject_GetAttrString(Func, "__self__"), false);
  TPyObjectPtr Fn(PyObject_GetAttrString(Func, "__func__"), false);
  if(!Obj || !Fn)
    throw EAbort(""); //Should never happen. We already checked that the attributes exist
  TPyObjectPtr Proxy(PyWeakref_NewProxy(Obj.get(), NULL), false);
  if(!Proxy)
    throw EAbort(""); //Python exception has already been set
  //Instead of using PyMethod_New(Fn, Proxy), which doesn't exist in the limited API,
  //we create a new method object using the type passing (Fn, Proxy) to the constructor.
  return PyObject_Call(reinterpret_cast<PyObject*>(Func->ob_type), PyTuple_Pack(2, Fn.get(), Proxy.get()), NULL);
}
//---------------------------------------------------------------------------
/** Set the value of a public property or field in a VclObject. If an event is
 *  set, we use a generic event, which will convert the parameters and call the
 *  Python event. If the Python event is a bound method, we store a weak reference
 *  to the instance of the bound method to avoid circular references.
 */
static int VclObject_SetAttro(TVclObject *self, PyObject *attr_name, PyObject *v)
{
	try
	{
		if(self->Instance == NULL)
			throw EPyVclError("Referenced object has been deleted.");

		String Name = FromPyObject<String>(attr_name);
		TRttiType *Type = Context.GetType(self->Instance->ClassType());
		TRttiProperty *Property = Type->GetProperty(Name);
		if(Property == NULL)
		{
			int Result = PyObject_GenericSetAttr(reinterpret_cast<PyObject*>(self), attr_name, v);
			if(Result == -1)
				SetErrorString(PyExc_AttributeError, "'" + Type->Name + "' object has no attribute '" + Name + "'");
			return Result;
		}

		TTypeInfo *TypeInfo = Property->PropertyType->Handle;
		TValue Value;
		if(TypeInfo->Kind == tkMethod)
		{
      //Free exisitng event if it is a PyObject
      Value = Property->GetValue(self->Instance);
      TMethod Method;
			Value.ExtractRawDataNoCopy(&Method);
			if(CheckValidImplementation(Method.Code))
				if(TObjectDeleteHandler *DeleteHandler = FindDeleteHandler(self->Instance, false))
					DeleteHandler->Delete(static_cast<PyObject*>(Method.Data));

      PyObject *Func = CreateWeakEvent(v); //Create weak reference or inc ref count
      Value = CreateMethodImplementation(Func, TypeInfo);

      //Register the new event with the delete handler
      if(v != Py_None)
        if(TObjectDeleteHandler *DeleteHandler = FindDeleteHandler(self->Instance, true))
          DeleteHandler->Add(Func);
		}
    else
    	Value = ToValue(v, TypeInfo);
		Property->SetValue(self->Instance, Value);
		return 0;
	}
	catch(...)
	{
		PyVclHandleException();
		return -1;
	}
}
//---------------------------------------------------------------------------
/** Compares two VCL proxy objects. Unordered compare (<, <=, >, >=) is not
 *  supported. Two proxy objects are equal if they refer to the same VCL object.
 *  \param self: The VCL proxy object.
 *  \param other: VCL proxy object to compare with.
 *  \return True or False depending on the compare operator.
 */
static PyObject* VclObject_RichCompare(TVclObject *self, PyObject *other, int op)
{
  if(op == Py_EQ)
  {
    if(!VclObject_Check(other))
      Py_RETURN_FALSE;
    if(self->Instance == reinterpret_cast<TVclObject*>(other)->Instance)
      Py_RETURN_TRUE;
    Py_RETURN_FALSE;
  }

  if(op == Py_NE)
  {
    if(!VclObject_Check(other))
      Py_RETURN_TRUE;
    if(self->Instance == reinterpret_cast<TVclObject*>(other)->Instance)
      Py_RETURN_FALSE;
    Py_RETURN_TRUE;
  }

  return PyErr_Format(PyExc_TypeError, "Unorderable types: VclObject() < %s()", AnsiString(GetTypeName(other)).c_str());
}
//---------------------------------------------------------------------------
/** Convert the object to True or False.
 *  \param self: The VCL proxy object.
 *  \return True if it is bound to an instance.
 */
static int VclObject_Bool(TVclObject *self)
{
  return self->Instance != NULL;
}
//---------------------------------------------------------------------------
/** Helper function for VclObject_Subscript() and VclObject_SetSubscript().
 *  \param self: VCL object to get default indexed property for.
 *  \return The default indexed property for the object if one exists.
 *  \throw EPyVclError on any error.
 */
static TRttiIndexedProperty* GetDefaultIndexedProeprty(TVclObject *self)
{
  if(self->Instance == NULL)
    throw EPyVclError("Referenced object has been deleted.");
  TRttiType *Type = Context.GetType(self->Instance->ClassType());
  System::DynamicArray<TRttiIndexedProperty*>  Properties = Type->GetIndexedProperties();
  for(int I = 0; I < Properties.Length; I++)
    if(Properties[I]->IsDefault)
      return Properties[I];
  throw EPyVclError("Instance of class '" + String(Type->Name) + "' has no default indexed property.");
}
//---------------------------------------------------------------------------
/** Get an element of the default indexed property if it exists.
 *  \param self: The VCL proxy object.
 *  \param key: The subscription index. May be a tuple.
 *  \return New reference to the retrieved object.
 */
static PyObject* VclObject_Subscript(TVclObject *self, PyObject *key)
{
	try
	{
    TRttiIndexedProperty *Property = GetDefaultIndexedProeprty(self);
    if(!Property->IsReadable)
      throw EPyVclError("Default indexed property is not readable.");
    DynamicArray<TRttiParameter*> ParameterTypes = Property->ReadMethod->GetParameters();
    std::vector<TValue> Parameters;
    if(PyTuple_Check(key))
      TupleToValues(key, Parameters, ParameterTypes);
    else
      Parameters.push_back(ToValue(key, ParameterTypes[0]->ParamType->Handle));
    return ToPyObject(Property->GetValue(self->Instance, &Parameters[0], Parameters.size()-1));
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Set an element of the default indexed property if it exists.
 *  \param self: The VCL proxy object.
 *  \param key: The subscription index. May be a tuple.
 *  \param v: The value to set. This is checked for corret type.
 *  \return 0 on success and -1 on failure.
 */
static int VclObject_SetSubscript(TVclObject *self, PyObject *key, PyObject *v)
{
	try
	{
    TRttiIndexedProperty *Property = GetDefaultIndexedProeprty(self);
    if(!Property->IsWritable)
      throw EPyVclError("Property is not writable.");
    DynamicArray<TRttiParameter*> ParameterTypes = Property->WriteMethod->GetParameters();
    std::vector<TValue> Parameters;
    if(PyTuple_Check(key))
      TupleToValues(key, Parameters, ParameterTypes);
    else
      Parameters.push_back(ToValue(key, ParameterTypes[0]->ParamType->Handle));
    Property->SetValue(
      self->Instance,
      &Parameters[0], Parameters.size()-1,
      ToValue(v, Property->PropertyType->Handle));
    return 0;
	}
	catch(...)
	{
		PyVclHandleException();
		return -1;
	}
}
//---------------------------------------------------------------------------
/** This is the same as the member method __iter__().
 *  It calls the member function GetEnumerator() of the VCL object if possible and return
 *  the returned VCL enumerator object.
 *  If GetEnumerator() does not exist but MoveNext() does, we return a new reference to
 *  self as we are already an intertor.
 *  \param self: The VCL proxy object.
 *  \return New iterator object.
 */
static PyObject* VclObject_Iter(TVclObject *self, PyObject *key, PyObject *v)
{
	try
	{
		TRttiType *Type = Context.GetType(self->Instance->ClassType());
		TRttiMethod* Method = Type->GetMethod("GetEnumerator");
		if(Method)
			return VclObject_Create(Method->Invoke(self->Instance, NULL, -1).AsObject(), true); //Invoke GetEnumerator without arguments
		if(Type->GetMethod("MoveNext"))
    {
      Py_INCREF(self);
      return reinterpret_cast<PyObject*>(self);
    }
		return SetErrorString(PyExc_TypeError, "'" + self->Instance->ClassName() + "' object is not iterable");
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** This is the same as the member method __next__().
 *  It will call MoveNext() and throw StopIterator if it returns False.
 *  Otherwise it will call GetCurrent() and return the result.
 *  \param self: The VCL proxy object.
 *  \return Result after incrementing the iterator.
 */
static PyObject* VclObject_IterNext(TVclObject *self, PyObject *key, PyObject *v)
{
	try
	{
		TRttiType *Type = Context.GetType(self->Instance->ClassType());
		TRttiMethod* Method = Type->GetMethod("MoveNext");
    if(Method)
    {
      if(!Method->Invoke(self->Instance, NULL, -1).AsBoolean())
        return SetErrorString(PyExc_StopIteration);
      Method = Type->GetMethod("GetCurrent");
      if(Method)
        return ToPyObject(Method->Invoke(self->Instance, NULL, -1));
    }
    return SetErrorString(PyExc_TypeError, "'" + self->Instance->ClassName() + "' object is not an iterator");
	}
	catch(...)
	{
		return PyVclHandleException();
	}
}
//---------------------------------------------------------------------------
/** Returns the hash of the object. We just use the address of the VCL instance.
 *  \param self: The VCL proxy object.
 *  \return Hash of object
 */
static Py_hash_t VclObject_Hash(TVclObject *self)
{
  return reinterpret_cast<Py_hash_t>(self->Instance);
}
//---------------------------------------------------------------------------
static PyMemberDef VclObject_Members[] =
{
	{(char*)"_owned", T_BOOL, offsetof(TVclObject, Owned), 0, (char*)"Indicates if the " PROJECT_NAME " object is freed when the proxy is destroyed"},
	{NULL, 0, 0, 0, NULL}
};
//---------------------------------------------------------------------------
static PyMethodDef VclObject_Methods[] =
{
	{"__dir__", (PyCFunction)VclObject_Dir, METH_NOARGS, ""},
	{NULL, NULL, 0, NULL}
};
//---------------------------------------------------------------------------
/** Proxy object for a VCL object. If _owned is set to True, the VCL object is
 *  owned by the proxy and destroyed when the proxy is destroyed.
 *  If possible the VCL object will own a delete handler, which will clear the
 *  reference in the proxy if the VCL object is destroyed before the proxy.
 */
static PyType_Slot VclObject_Slots[] =
{
  {Py_tp_doc,	(void*) PROJECT_NAME " object"},
  {Py_tp_dealloc, VclObject_Dealloc},
  {Py_tp_repr, VclObject_Repr},
  {Py_tp_getattro, VclObject_GetAttro},
  {Py_tp_setattro, VclObject_SetAttro},
  {Py_tp_richcompare, VclObject_RichCompare},
  {Py_nb_bool, VclObject_Bool},
  {Py_mp_subscript, VclObject_Subscript},
  {Py_mp_ass_subscript, VclObject_SetSubscript},
	{Py_tp_methods, VclObject_Methods},
	{Py_tp_members, VclObject_Members},
  {Py_tp_iter, VclObject_Iter},
  {Py_tp_iternext, VclObject_IterNext},
  {Py_tp_hash, VclObject_Hash},
  {0, NULL}
};

static PyType_Spec VclObject_Spec =
{
  GUI_TYPE "Object",   //name
  sizeof(TVclObject),  //basicsize
  0,                    //itemsize
  Py_TPFLAGS_DEFAULT,   //flags
  VclObject_Slots,     //slots
};
//---------------------------------------------------------------------------
/** Initialize the VclObject_Type type object.
 *  \return true on success
 */
bool VclObject_InitType()
{
  VclObject_Type = reinterpret_cast<PyTypeObject*>(PyType_FromSpec(&VclObject_Spec));
  return VclObject_Type != NULL;
}
//---------------------------------------------------------------------------
/** Create new VclObject.
 *  \param Instance: VCL instance, which the VclObject will be a proxy for.
 *  \param Owned: If True, Instance will be owned by the proxy and destroyed
 *                when the proxy is destroyed.
 *  \return New reference to VclObject.
 */
PyObject* VclObject_Create(TObject *Instance, bool Owned)
{
  TObjectDeleteHandler *DeleteHandler = FindDeleteHandler(Instance, true);
	TVclObject *VclObject = NULL;
  if(DeleteHandler)
    VclObject = DeleteHandler->GetVclObject();
  if(VclObject != NULL)
  {
    if(Owned)
      VclObject->Owned = true;
    Py_INCREF(VclObject);
  }
  else
  {
    VclObject = PyObject_New(TVclObject, VclObject_Type);
	  VclObject->Instance = Instance;
	  VclObject->Owned = Owned;
    if(DeleteHandler)
      DeleteHandler->SetVclObject(VclObject);
  }
  return reinterpret_cast<PyObject*>(VclObject);
}
//---------------------------------------------------------------------------
/** Return true if O is a VclObject.
 */
bool VclObject_Check(PyObject *O)
{
	return O->ob_type == VclObject_Type;
}
//---------------------------------------------------------------------------
/** Return the VCL object, which the given VclObject is a proxy for.
 *  \param O: Pointer to a VclObject instance.
 *  \return VCL object refered to by O. This may be NULL if the object has been destroyed
 *  \throw EPyVclError if O is not a VclObject.
 */
TObject* VclObject_AsObject(PyObject *O)
{
	if(O->ob_type != VclObject_Type)
		throw EPyVclError("Object is not a " GUI_TYPE "Object type");
	return reinterpret_cast<TVclObject*>(O)->Instance;
}
//---------------------------------------------------------------------------
/** Return the VCL object, which the given VclObject is a proxy for.
 *  \param O: Pointer to a VclObject instance.
 *  \return VCL object refered to by O. This may be NULL if the conversion fails or the object has been destroyed
 */
TObject* VclObject_AsObjectNoThrow(PyObject *O)
{
	if(O->ob_type != VclObject_Type)
		return NULL;
	return reinterpret_cast<TVclObject*>(O)->Instance;
}
//---------------------------------------------------------------------------
} //namespace Python
