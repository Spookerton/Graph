/* Graph (http://sourceforge.net/projects/graph)
 * Copyright 2007 Ivan Johansen
 *
 * Graph is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */
//---------------------------------------------------------------------------
#ifndef PyVclObjectH
#define PyVclObjectH
//---------------------------------------------------------------------------
#include "Python.hpp"
namespace Python
{
	struct TVclObject
	{
		PyObject_HEAD
		TObject *Instance;
		char Owned; //bool
	};

  extern PyTypeObject *VclObject_Type;

  bool VclObject_InitType();
	PyObject* VclObject_Create(TObject *Instance, bool Owned);
	bool VclObject_Check(PyObject *O);
	TObject* VclObject_AsObject(PyObject *O);
	TObject* VclObject_AsObjectNoThrow(PyObject *O);

  template<typename T> T* VclObject_AsObjectNoThrow(PyObject *O)
  {
    TObject *Object = VclObject_AsObjectNoThrow(O);
    if(Object == NULL)
      return NULL;
    return dynamic_cast<T*>(Object);
  }
}
//---------------------------------------------------------------------------
#endif
