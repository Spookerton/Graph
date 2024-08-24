/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef PyVclMethodH
#define PyVclMethodH
#include <Rtti.hpp>
//---------------------------------------------------------------------------
namespace Python
{
  bool VclMethod_InitType();
  class TVclObject;
	PyObject* VclMethod_Create(TVclObject *Object, TObject *Instance, const DynamicArray<TRttiMethod*> &Methods);
	PyObject* VclMethod_Create(class TVclRecord *Object, void *Instance, const DynamicArray<TRttiMethod*> &Methods);
  PyObject* VclMethod_Create(TRttiType *Type, const DynamicArray<TRttiMethod*> &Methods);

	PyObject* CallObjectMethod(TRttiType *Type, TObject *Instance, DynamicArray<TRttiMethod*> &Methods, PyObject *Args);
  PyObject* CallRecordMethod(const TValue &Instance, DynamicArray<TRttiMethod*> &Methods, PyObject *Args);
}
//---------------------------------------------------------------------------
#endif
