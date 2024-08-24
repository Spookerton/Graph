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
#include "PyVclConvert.h"
#include "PyVclObject.h"
#include "PyVclMethod.h"
#include "PyVclClosure.h"
#include "PyVclRef.h"
#include "PyVclObjectFactory.h"
#include "PyVclRecord.h"
#include <float.h>
#include "FindClass.hpp"
#include <map>
#include <set>
#include <cstdarg>
//---------------------------------------------------------------------------
//Create operator== for the built-in ShortString type
inline bool operator ==(System::ShortString &S1, const char *S2)
{
  return memcmp(&S1[1], S2, S1[0]) == 0;    //Nasty hack: S1[0] is the length because Len is a byte before Data
}
//---------------------------------------------------------------------------
namespace Python
{
typedef std::map<PTypeInfo, TMethodImplementation*> TMethodImplMap;

TRttiContext Context;
PyObject *PyVclException = NULL;
TMethodImplMap MethodImplMap;
std::set<PyObject*> ValidObjects; //Used when converting to/from void*
//---------------------------------------------------------------------------
//This is a simple wrapper around a PyObject* used to store a PyObject* as a TObject*.
//We store a TVclObject owning the wrapper as _Wrapper in the object to prevent resource leaks.
//We do not increment the reference count as the VCL object receiving the TObject* may not own
// the object.
//Warning: Do not try to retrieve the PyObject* from VCL after it has been destroyed.
class TPyObjectWrapper : public TObject
{
public:
  PyObject *Object;
  TPyObjectWrapper(PyObject *O) : Object(O) {}
};
//---------------------------------------------------------------------------
//Class used as wrapper for TProc (reference to procedure in Delphi)
class TPythonProcedure : public TCppInterfacedObject<TProc>
{
  TPyObjectPtr Callable;
public:
  TPythonProcedure(PyObject *Ptr) : Callable(Ptr) {}
  void __fastcall Invoke()
  {
  	TLockGIL Dummy;
    {
      TPyObjectPtr Return = PyObject_CallObject(Callable.get(), NULL); //Make sure Result is destroyed before the GIL is released
    }
  	if(PyErr_Occurred())
    	PyErr_Print();
  }
};
//---------------------------------------------------------------------------
/** Class with function used to assign all events that are python objects.
 *  There is only one global instance, in fact the instance is not used but is
 *  necesarry because a closure is needed.
 */
struct TPythonCallback : public TCppInterfacedObject<TMethodImplementationCallback>
{
  TPythonCallback() {AddRef();} //Always keep a global reference; Ref count should never be 0
	void __fastcall Invoke(void * UserData, System::DynamicArray<TValue> Args, TValue &Result);
};
TPythonCallback *PythonCallback = new TPythonCallback;
//---------------------------------------------------------------------------
/** Constructor for the EPyVclError class used for all exceptions thrown by PyVcl
 */
EPyVclError::EPyVclError(const String &Str)
  : Exception(Str)
{
  PyErr_Clear(); //Clear any Python error when throwing it as a C++ exception
}
//---------------------------------------------------------------------------
/** Constructor for the EPyVclError class used for all exceptions thrown by PyVcl
 *  \param Format: A printf like format string followed by additional parameters.
 */
EPyVclError::EPyVclError(const wchar_t *Format, ...)
  : Exception("")
{
  PyErr_Clear(); //Clear any Python error when throwing it as a C++ exception
  String Str;
  va_list ap;
  va_start(ap, Format);
  Str.vprintf(Format, ap);
  va_end(ap);
  Message = Str;
}
//---------------------------------------------------------------------------
/** Function called for all events assigned to Python functions.
 *  The function will convert the arguments passed with the event to Python objects and
 *  call the Python callback function.
 */
void __fastcall TPythonCallback::Invoke(void * UserData, System::DynamicArray<TValue> Args, TValue &Result)
{
	TLockGIL Dummy;
	TMethodImplementation::TInvokeInfo *InvokeInfo = static_cast<TMethodImplementation::TInvokeInfo*>(UserData/*Args[0].AsObject()*/);
	DynamicArray<TMethodImplementation::TParamLoc> Params = InvokeInfo->GetParamLocs();
	PyObject *Object = *static_cast<PyObject**>(Args[0].GetReferenceToRawData()); //Python function is stored in this
	int Count = Args.get_length() - 1;
	PyObject *PyArgs = Count != 0 ? PyTuple_New(Count) : NULL;
	for(int I = 0; I < Count; I++)
		PyTuple_SetItem(PyArgs, I, Params[I+1].FByRefParam ? VclRef_Create(&Args[I+1]) : ToPyObject(Args[I+1]));
	PyObject *PyResult = PyObject_CallObject(Object, PyArgs);

  //Invalidate all VclRef objects used as arguments
	for(int I = 0; I < Count; I++)
  {
    PyObject *Arg = PyTuple_GetItem(PyArgs, I);
    if(VclRef_Check(Arg))
      VclRef_Invalidate(Arg);
  }

	Py_XDECREF(PyArgs);
	if(PyResult != NULL && PyResult != Py_None)
		Result = ToValue(PyResult, NULL); //Bug: Type of result missing
	Py_XDECREF(PyResult);
	if(PyResult == NULL)
  {
    if(PyErr_ExceptionMatches(PyExc_SystemExit))
    {
      if(Application->MainForm)
        Application->MainForm->Close();
      else
        Application->Terminate();
      PyErr_Clear();
    }
    else
	    PyErr_Print();
  }
}
//---------------------------------------------------------------------------
/** Check if a pointer to code matches the address of a known implementation.
 *  The code address is usually retrieved from an event closure.
 */
bool CheckValidImplementation(void *CodeAddr)
{
  for(TMethodImplMap::iterator Iter = MethodImplMap.begin(); Iter != MethodImplMap.end(); ++Iter)
    if(Iter->second->CodeAddress == CodeAddr)
      return true;
  return false;
}
//---------------------------------------------------------------------------
/** Create TInvokeInfo object with parameter information for a Delphi event.
 *  This is used to decode the parameters inside the generic event handler.
 */
TMethodImplementation::TInvokeInfo* CreateInvokeInfo(TTypeInfo *TypeInfo)
{
	TTypeData *TypeData = reinterpret_cast<TTypeData*>(&TypeInfo->Name[TypeInfo->Name[0]+1]);
	int Index = 0;
	std::vector<Typinfo::TParamFlags> ParamFlags;
	for(int I = 0; I < TypeData->ParamCount; I++)
	{
		ParamFlags.push_back(reinterpret_cast<Typinfo::TParamFlags&>(TypeData->ParamList[Index++]));
		Index += TypeData->ParamList[Index] + 1;
		Index += TypeData->ParamList[Index] + 1;
	}
	if(TypeData->MethodKind == mkFunction)
	{
		Index += TypeData->ParamList[Index] + 1;
		Index += sizeof(void*);
	}
	Typinfo::TCallConv CallConv = static_cast<Typinfo::TCallConv>(TypeData->ParamList[Index]);
	Index++;
	TMethodImplementation::TInvokeInfo *InvokeInfo = new TMethodImplementation::TInvokeInfo(CallConv, true);
  //Add the type of this; __delphirtti(void*) doesn't work; It must be implemented in Delphi
	InvokeInfo->AddParameter(PointerTypeInfo, false);//The type of this
	for(int I = 0; I < TypeData->ParamCount; I++)
	{
		TTypeInfo *ParamType = **(TTypeInfo***)&TypeData->ParamList[Index];
		InvokeInfo->AddParameter(ParamType, ParamFlags[I].Contains(Typinfo::pfVar));
		Index += sizeof(void*);
	}
	InvokeInfo->Seal();
	return InvokeInfo;
}
//---------------------------------------------------------------------------
/** Retrieve the type (class) of the object
 *  \return object type name as a String.
 */
String GetTypeName(PyObject *O)
{
  TPyObjectPtr Type(PyObject_GetAttrString(reinterpret_cast<PyObject*>(O->ob_type), "__name__"), false);
  if(Type.get())
    return FromPyObject<String>(Type.get());
  PyErr_Clear();
  return "";
}
//---------------------------------------------------------------------------
/** Return the name of the type.
 */
String GetTypeName(TTypeInfo *TypeInfo)
{
  if(TypeInfo->Name[0] != 0)
    return AnsiString(TypeInfo->Name);

  static const char*const NameList[] =
  {
    "Unknown", "Integer", "Char", "Enumeration", "Float", "String", "Set", "Class",
    "Method", "WChar", "LString", "WString", "Variant", "Array", "Record", "Interface",
    "Int64", "DynArray", "UString", "ClassRef", "Pointer", "Procedure"
  };
  return NameList[TypeInfo->Kind];
}
//---------------------------------------------------------------------------
TValue ToValue(PyObject *O)
{
  if(PyUnicode_Check(O))
    return TValue::From(FromPyObject<String>(O));
	throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'TValue'");
}
//---------------------------------------------------------------------------
/** Creates and stores a TMethodImplementation that works as a proxy object, which can be
 *  used as a closure and forward the call to a Python object.
 *  The TMethodImplementation is stored and reused if the function is called with the same
 *  type information.
 *  \param O: Callable Python object. If Py_None, NULL will be returned.
 *  Warning: The reference count is not incremented. For events, the object will store a
 *  reference. When used for methods like TThread::Synchronize(), the call must ensure the
 *  object is not destoyed. Be cafull with TThread::Queue().
 *  \param TypeInfo: Pointer to type information describing the wanted closure.
 *  \return A closure stored in a TValue. The code part points to the generated code in the
 *  TMethodImplementation. The data part (this) is the callable PyObject*.
 */
TValue CreateMethodImplementation(PyObject *O, TTypeInfo *TypeInfo)
{
  if(O == Py_None)
    return TValue::From<TObject*>(NULL);

  TMethodImplMap::iterator Iter = MethodImplMap.find(TypeInfo);
  TMethodImplementation *Implementation;
  if(Iter != MethodImplMap.end())
    Implementation = Iter->second;
  else
  {
    TMethodImplementation::TInvokeInfo *InvokeInfo = CreateInvokeInfo(TypeInfo);
    Implementation = new TMethodImplementation(InvokeInfo/*Func*/, InvokeInfo, PythonCallback);
    MethodImplMap[TypeInfo] = Implementation;
  }
  TMethod Method = {Implementation->CodeAddress, O}; //Pass Func in this, which can be used as another data pointer
  TValue Result;
  TValue::Make(&Method, TypeInfo, Result);
  return Result;
}
//---------------------------------------------------------------------------
/** Convert a PyObject to a TValue as used when calling functions and setting properties in a generic way through Delphi RTTI.
 *  \param O: PyObject to convert
 *  \param TypeInfo: The expected return type
 *  \param ConstValue: The value will not be changed. Mostly useful for pointers
 *  \return A TValue with a value of the type given by TypeInfo.
 *  \throw EPyVclError on conversion errors.
 */
TValue ToValue(PyObject *O, TTypeInfo *TypeInfo, bool ConstValue)
{
  if(VclRef_Check(O))
    return VclRef_AsValue(O);

	TValue Result;
	switch(TypeInfo->Kind)
	{
		case tkClass:
			if(VclObject_Check(O))
				Result = TValue::From(VclObject_AsObject(O));
			else if(O == Py_None)
				Result = TValue::From((TObject*)NULL);
			else
      {
        //Check if we already have a wrapper
        if(PyObject_HasAttrString(O, "_Wrapper"))
        {
          TPyObjectPtr VclWrapper(PyObject_GetAttrString(O, "_Wrapper"), false);
          Result = TValue::From(VclObject_AsObject(VclWrapper.get()));
        }
        else
        {
          //Create a new wrapper around the PyObject* and return the wrapper in the TValue.
          //Create a TVclObject to hold the wrapper and store it as _Wrapper in the PyObject*.
          //Warning: We are not incrementing the reference count. Do not try to retrieve the object
          //from VCL after it has been destroyed.
          TPyObjectWrapper *PyWrapper = new TPyObjectWrapper(O);
          TPyObjectPtr VclWrapper(VclObject_Create(PyWrapper, true), false);
          if(PyObject_SetAttrString(O, "_Wrapper", VclWrapper.get()) == -1)
    				throw EPyVclError("Cannot convert Python object of type '" + Python::GetTypeName(O) + "' to '" + Python::GetTypeName(TypeInfo) + "'");
    			Result = TValue::From(PyWrapper);
        }
      }
			break;

		case tkEnumeration:
			if(PyLong_Check(O))
				TValue::Make(PyLong_AsLong(O), TypeInfo, Result);
      else
      {
        int Value = GetEnumValue(TypeInfo, FromPyObject<String>(O));
        if(Value == -1)
          throw EPyVclError("'" + FromPyObject<String>(O) + "' is not a valid value for the enum type '" + Python::GetTypeName(TypeInfo) + "'");
				TValue::Make(Value, TypeInfo, Result);
      }
			break;

		case tkSet:
    {
      String SetStr;
      if(PySet_Check(O))
      {
        TPyObjectPtr Sep = ToPyObjectPtr(",");
        TPyObjectPtr Str(PyUnicode_Join(Sep.get(), O), false);
      	if(PyErr_Occurred())
          throw EAbort("");
        SetStr = FromPyObject<String>(Str.get());
      }
      else
        SetStr = FromPyObject<String>(O);
			TValue::Make(StringToSet(TypeInfo, SetStr), TypeInfo, Result);
			break;
    }
		case tkInteger:
			Result = TValue::FromOrdinal(TypeInfo, PyLong_AsUnsignedLongMask(O)); //In some cases we need the exact TypeInfo
			break;

		case tkUString:
    {
      //We may need to use the exact type info as requested and not a compatible one
      String Str = FromPyObject<String>(O);
			TValue::Make(&Str, TypeInfo, Result);
			break;
    }
		case tkString:
		case tkLString:
		case tkWString:
			Result = TValue::From(FromPyObject<String>(O)).Cast(TypeInfo);
			break;

		case tkChar:
		case tkWChar:
    {
			if(PyUnicode_GetSize(O) != 1)
				throw EPyVclError("Expected string with one character");
      String Str = FromPyObject<String>(O);
			TValue::Make(Str[1], TypeInfo, Result);
			break;
    }
		case tkFloat:
			Result = TValue::From(PyFloat_AsDouble(O)).Cast(TypeInfo);
			break;

		case tkRecord:
      if(TypeInfo->Name == "TValue")
      {
        TValue V = ToValue(O);
  			TValue::Make(&V, TypeInfo, Result);
      }
      else
      {
        TRttiType *Type = Context.GetType(TypeInfo);
        TValue::Make((void*)NULL, TypeInfo, Result); //void* cast is needed; Otherwise NULL is treated as an int
        System::DynamicArray<TRttiMethod*> Methods = Type->GetMethods("Create");
        if(VclRecord_GetTypeInfo(O) == TypeInfo)
        {
          Result = VclRecord_AsValue(O);
        }
        else if(Methods.Length > 0)
        {
          CallRecordMethod(Result, Methods, O);
        }
        else
        {
          DynamicArray<TRttiField*> Fields = Type->GetFields();
          if(PyTuple_Size(O) != Fields.Length)
            throw EPyVclError("Expected tuple with " + IntToStr(Fields.Length) + " elements");
          for(int I = 0; I < Fields.Length; I++)
            Fields[I]->SetValue(Result.GetReferenceToRawData(), ToValue(PyTuple_GetItem(O, I), Fields[I]->FieldType->Handle));
        }
      }
			break;

		case tkInt64:
			Result = TValue::FromOrdinal(TypeInfo, PyLong_AsLongLong(O));
			break;

		case tkPointer:
#ifndef Py_LIMITED_API
      if(TypeInfo->Name == "PWideChar" && ConstValue)
    		TValue::Make(reinterpret_cast<int>(PyUnicode_AsUnicode(O)), TypeInfo, Result); //Can we store the string inside TValue?
      else
#endif
      if(TypeInfo->Name == "Pointer")
      {
        Py_INCREF(O); //Increment but never decrement; The object will be kept alive forever as we don't know when it will no longer be used.
        ValidObjects.insert(O);
        TValue::Make(reinterpret_cast<int>(O), TypeInfo, Result);
      }
      else
    		throw EPyVclError("Cannot convert Python object of type '" + Python::GetTypeName(O) + "' to '" + Python::GetTypeName(TypeInfo) + "'");
      break;

		case tkMethod:
      Result = CreateMethodImplementation(O, TypeInfo);
      break;

		case tkInterface:
      if(TypeInfo->Name == "TProc")
      {  //I should find a more generic way to do this.
        if(!PyCallable_Check(O))
        	throw EPyVclError("'" + GetTypeName(O) + "' is not callable");
        _di_TProc Ptr = new TPythonProcedure(O);
        TValue::Make(&Ptr, TypeInfo, Result);
        break;
      }
      //Fall through

		case tkClassRef:
		case tkVariant:
		case tkArray:
		case tkDynArray:
		case tkProcedure:
		case tkUnknown:
		default:
			throw EPyVclError("Cannot convert Python object of type '" + Python::GetTypeName(O) + "' to '" + Python::GetTypeName(TypeInfo) + "'");
	}
	if(PyErr_Occurred())
 		throw EPyVclError("Cannot convert Python object of type '" + Python::GetTypeName(O) + "' to '" + Python::GetTypeName(TypeInfo) + "'");
	return Result;
}
//---------------------------------------------------------------------------
/** Convert the content for a tuple to a vector of TValue that can be passed to the Delphi RTTI system.
 *  \param O: This must be a tuple. Not checked.
 *  \param Values: vector filled with the return values.
 *  \param Parameters: This must be the expected types for the converted values. The number of elements
 *         must be the same as the number of items in the tuple.
 */
void TupleToValues(PyObject *O, std::vector<TValue> &Values, const DynamicArray<TRttiParameter*> &Parameters)
{
	Values.clear();
	unsigned Size = PyTuple_Size(O);
	for(unsigned I = 0; I < Size; I++)
  {
    PyObject *Item = PyTuple_GetItem(O, I);

    //Use a list with one element for arguments passed by reference:
    //L = [5]
    //MyFunc(L) #Calls MyFunc(int &Value)
    if(Parameters[I]->Flags.Contains(pfVar) || Parameters[I]->Flags.Contains(pfOut))
    {
      if(PyList_Check(Item) && PyList_Size(Item) == 1)
        Item = PyList_GetItem(Item, 0);
    }

    //Documentation for TRttiParameter::ParamType: The value of ParamType is nil for untyped var or const parameters
    //It seems to be NULL for reference parameters in general.
    if(Parameters[I]->ParamType == NULL)
    {
      TTypeInfo *TypeInfo = (TTypeInfo*)Parameters[I]->Handle;
  		Values.push_back(ToValue(Item, TypeInfo, Parameters[I]->Flags.Contains(pfConst)));
    }
    else
  		Values.push_back(ToValue(Item, Parameters[I]->ParamType->Handle, Parameters[I]->Flags.Contains(pfConst)));
  }
}
//---------------------------------------------------------------------------
/** Called after a method was executed to convert all values that was passed by reference back into Python objects.
 *  \param O: This must be a tuple. Not checked.
 *  \param Values: vector with the arguments that was passed to the function and maybe changed.
 *  \param Parameters: This must be the types for the arguments. The number of elements
 *         must be the same as the number of items in the tuple.
 */
void ReverseTupleToValues(PyObject *O, const std::vector<TValue> &Values, const DynamicArray<TRttiParameter*> &Parameters)
{
  //Convert variables passed by reference back to Python objects.
  //To allow a function to change an argument, we pass it as a list:
  //L = [5]
  //MyFunc(L) #Calls MyFunc(int &Value)
  for(unsigned I = 0; I < Values.size(); I++)
    if(Parameters[I]->Flags.Contains(pfVar) || Parameters[I]->Flags.Contains(pfOut))
    {
      PyObject *Item = PyTuple_GetItem(O, I);
      if(PyList_Check(Item) && PyList_Size(Item) == 1)
        PyList_SetItem(Item, 0, ToPyObject(Values[I]));
    }
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(bool Value)
{
	return PyBool_FromLong(Value);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(int Value)
{
	return PyLong_FromLong(Value);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(long Value)
{
	return PyLong_FromLong(Value);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(long long Value)
{
	return PyLong_FromLongLong(Value);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(char Value)
{
  char Str[] = {Value, 0};
  return ToPyObject(Str);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(wchar_t Value)
{
  return PyUnicode_FromWideChar(&Value, 1);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(double Value)
{
	return PyFloat_FromDouble(Value);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(const std::wstring &Str)
{
  return PyUnicode_FromWideChar(Str.c_str(), Str.size());
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(const String &Str)
{
#ifdef _Windows
	return PyUnicode_FromWideChar(Str.c_str(), Str.Length());
#else
	return PyUnicode_DecodeUTF16(reinterpret_cast<const char*>(Str.c_str()), Str.Length(), NULL, NULL);
#endif
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(TObject *Object)
{
	return VclObject_Create(Object, false);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(const char *Str)
{
	return PyUnicode_DecodeASCII(Str, strlen(Str), NULL);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(const wchar_t *Str)
{
  return PyUnicode_FromWideChar(Str, -1);
}
//---------------------------------------------------------------------------
PyObject* ToPyObject(const Variant &V)
{
  return ToPyObject(Rtti::TValue::FromVariant(V));
}
//---------------------------------------------------------------------------
/** Convert a TValue from the Delphi RTTI system to a PyObject*.
 *  \param V: The value to convert.
 *  \return New reference to a converted object.
 *  \throw EPyVclError if the conversion fails.
 */
PyObject* ToPyObject(const Rtti::TValue &V)
{
	Rtti::TValue &Value = const_cast<Rtti::TValue&>(V);
	if(Value.IsEmpty)
		Py_RETURN_NONE;
	switch(Value.Kind)
	{
		case tkInteger:
			return ToPyObject(Value.AsInteger());

		case tkUString:
		case tkString:
		case tkLString:
		case tkWString:
			return ToPyObject(Value.AsString());

		case tkEnumeration:
		{
			if(Value.TypeInfo->Name == "Boolean" || Value.TypeInfo->Name == "bool")
				return ToPyObject(static_cast<bool>(Value.AsOrdinal()));
			TRttiEnumerationType *Type = static_cast<TRttiEnumerationType*>(Context.GetType(Value.TypeInfo));
			int Ordinal = Value.AsOrdinal();
			if(Ordinal >= Type->MinValue && Ordinal <= Type->MaxValue)
			  //GetEnumName() lacks range check
				return ToPyObject(GetEnumName(Value.TypeInfo, Ordinal));
			return ToPyObject("0x" + IntToHex(Ordinal, 2));
		}

		case tkClass:
    {
      TObject *Object = Value.AsObject();
      if(TPyObjectWrapper *PyWrapper = dynamic_cast<TPyObjectWrapper*>(Object))
      { //If the object is actually a wrapper around a PyObject*, return the PyObject*
        Py_INCREF(PyWrapper->Object);
        return PyWrapper->Object;
      }
			return VclObject_Create(Object, false);
    }
		case tkSet:
    {
      String Str = SetToString(Value.TypeInfo, *static_cast<int*>(Value.GetReferenceToRawData()), false);
      if(Str.Length() > 0)
      {
        TPyObjectPtr SetStr = ToPyObjectPtr(Str);
        TPyObjectPtr Sep = ToPyObjectPtr(",");
        TPyObjectPtr List(PyUnicode_Split(SetStr.get(), Sep.get(), -1), false);
        return PySet_New(List.get());
      }
      return PySet_New(NULL);
    }
		case tkChar:
			return ToPyObject(Value.AsType<char>());

		case tkWChar:
			return ToPyObject(Value.AsType<wchar_t>());

		case tkFloat:
			return ToPyObject(Value.AsExtended());

		case tkRecord:
		{
      if(Value.TypeInfo->Name == "TValue")
      {
  			void *Data = Value.GetReferenceToRawData();
        return ToPyObject(*static_cast<TValue*>(Data));
      }
			TRttiType *Type = Context.GetType(Value.TypeInfo);
      return VclRecord_Create(Value);
		}

		case tkMethod: //Event
		{
			TMethod Method;
			Value.ExtractRawDataNoCopy(&Method);
			if(Method.Code == NULL)
				Py_RETURN_NONE;
			if(CheckValidImplementation(Method.Code))
			{
				PyObject *Result = static_cast<PyObject*>(Method.Data);
				Py_INCREF(Result);
				return Result;
			}
      return VclClosure_Create(Value);
		}

		case tkInt64:
			return PyLong_FromLongLong(Value.AsInt64());

		case tkArray:
    {
      PyObject *O = PyList_New(Value.GetArrayLength());
      if(O != NULL)
      {
        Py_ssize_t Size = PyList_Size(O);
        for(int I = 0; I < Size; I++)
          PyList_SetItem(O, I, ToPyObject(Value.GetArrayElement(I)));
      }
      return O;
    }

		case tkClassRef:
      //Convert TClass to an object factory object
	    return VclObjectFactory_Create(Context.GetType(Value.AsClass()));

		case tkPointer:
      if(Value.TypeInfo->Name == "Pointer")
      {
        PyObject *Object;
        Value.ExtractRawDataNoCopy(&Object);
        if(ValidObjects.count(Object) == 0) //Check if we previously added this pointer
          throw EPyVclError("Unknown void* pointer returned.");
        Py_INCREF(Object); //Increment ref count as we do not steal a reference
        return Object;
      }
			throw EPyVclError("Unable to convert type '" + Python::GetTypeName(Value.TypeInfo) + "'");

		case tkVariant:
		case tkInterface:
		case tkDynArray:
		case tkProcedure:
		case tkUnknown:
		default:
			throw EPyVclError("Unable to convert type '" + Python::GetTypeName(Value.TypeInfo) + "'");
	}
}
//---------------------------------------------------------------------------
template<> double FromPyObject<double>(PyObject *O)
{
  double Result;
	if(PyComplex_Check(O))
  {
    if(PyComplex_ImagAsDouble(O) != 0)
      throw EPyVclError("Complex number has an imaginary part");
		Result = PyComplex_RealAsDouble(O);
  }
  else
    Result = PyFloat_AsDouble(O);
	if(PyErr_Occurred())
  {
    PyErr_Clear();
		throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'double'");
  }
  return Result;
}
//---------------------------------------------------------------------------
template<> std::wstring FromPyObject<std::wstring>(PyObject *O)
{
  Py_ssize_t Size = PyUnicode_GetSize(O);
  if(Size == -1)
  {
    PyErr_Clear();
		throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'wstring'");
  }
  std::wstring Str(Size, ' ');
  if(Str.size() == 0)
    return Str;
  PyUnicode_AsWideChar(O, &Str[0], Str.size());
	return Str;
}
//---------------------------------------------------------------------------
template<> String FromPyObject<String>(PyObject *O)
{
  String Str;
#ifdef _Windows
  Py_ssize_t Size = PyUnicode_GetSize(O);
  if(Size == -1)
  {
    PyErr_Clear();
		throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'String'");
  }
  Str.SetLength(Size);
  if(Str.Length() == 0)
	  return Str;
  PyUnicode_AsWideChar(O, &Str[1], Str.Length());
#else
	TPyObjectPtr UTF16(PyUnicode_AsUTF16String(O), false);
  if(UTF16)
  {
    Py_ssize_t Size;
    char *Data;
    if(PyBytes_AsStringAndSize(UTF16.get(), &Data, &Size) != -1)
    {
      Str.SetLength(Size / 2);
      memcpy(&Str[1], Data, Size);
    }
  }
#endif
	return Str;
}
//---------------------------------------------------------------------------
template<> long FromPyObject<long>(PyObject *O)
{
  return FromPyObject<int>(O);
}
//---------------------------------------------------------------------------
template<> int FromPyObject<int>(PyObject *O)
{
	int Result = PyLong_AsLong(O);
	if(PyErr_Occurred())
  {
    PyErr_Clear();
		throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'int'");
  }
  return Result;
}
//---------------------------------------------------------------------------
template<> bool FromPyObject<bool>(PyObject *O)
{
	int Result = PyObject_IsTrue(O);
	if(PyErr_Occurred())
  {
    PyErr_Clear();
		throw EPyVclError("Cannot convert Python object of type '" + GetTypeName(O) + "' to 'bool'");
  }
  return Result;
}
//---------------------------------------------------------------------------
/** Exception handling helper function. Called from a catch(...) section and converts an active
 *  C++/Delphi exception to a Python exception.
 */
PyObject* PyVclHandleException()
{
	try
	{
		throw;
	}
  //WARNING: The 64 bit compiler in C++ Builder XE4 does not handle catch of exceptions
  //derived from Exception correctly. See QC #116246
  catch(EAbort &E)
  { //This is used when a Python exception has already been set. Mostly useful for debugging.
  }
	catch(EStringListError &E)
	{
		SetErrorString(PyExc_IndexError, E.Message);
	}
	catch(EListError &E)
	{
		SetErrorString(PyExc_IndexError, E.Message);
	}
  catch(DynArrayOutOfRange &E)
  {
		PyErr_SetString(PyVclException, "Invalid number of indexes in dynamic array");
  }
  catch(DynArrayException &E)
  {
		PyErr_SetString(PyVclException, "Unknown dynamic array exception");
  }
  catch(EPyVclError &E)
  {
		SetErrorString(PyVclException, E.Message);
  }
	catch(Exception &E)
	{
		SetErrorString(PyVclException, E.ClassName() + ": " + E.Message);
	}
	catch(std::exception &E)
	{
		PyErr_SetString(PyVclException, E.what());
	}
	catch(...)
	{
		PyErr_SetString(PyVclException, "Unknown exception");
	}
	return NULL;
}
//---------------------------------------------------------------------------
} //namespace Python
