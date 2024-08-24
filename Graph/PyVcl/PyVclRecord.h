/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2019 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef PyVclRecordH
#define PyVclRecordH
//---------------------------------------------------------------------------
#include "Python.hpp"
#include <System.Rtti.hpp>
namespace Python
{
  bool VclRecord_InitType();
  PyObject* VclRecord_Create(const TValue &Value);
  PyObject* VclRecord_Create(void *Instance, TRttiType *Type, PyObject *Object);
  bool VclRecord_Check(PyObject *O);
  TValue VclRecord_AsValue(PyObject *O);
  TTypeInfo* VclRecord_GetTypeInfo(PyObject *O);
}
//---------------------------------------------------------------------------
#endif
