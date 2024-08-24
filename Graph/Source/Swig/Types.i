%typemap(out) const std::vector<TPoint>&, const std::vector<Func32::TCoordSet<> >&, const std::vector<unsigned>& 
{
  $result = Python::ToPyObject(*$1);
}


%typemap(out) String {
  $result = PyUnicode_FromUnicode($1.c_str(), $1.Length());
}

%typemap(out) const TRect& {
  $result = Py_BuildValue("iiii", $1->Left, $1->Top, $1->Right, $1->Bottom);
}

%typemap(out) TRect {
  $result = Py_BuildValue("iiii", $1.Left, $1.Top, $1.Right, $1.Bottom);
}

%typemap(out) TTextValue 
{
  return Python::ToPyObject($1);
}

%typemap(in) TTextValue  
{
  if(!Python::FromPyObject($input, $1))
    SWIG_fail;
}

%typemap(out) TFont* {
  $result = Python::VclObject_Create($1, false);
}

%typemap(in) TPngImage*
{
  $1 = Python::VclObject_AsObjectNoThrow<TPngImage>($input);
  if($1 == NULL)
  {
    Python::SetErrorString(Python::PyVclException, "Failed to convert from " + Python::GetTypeName($input) + " to TPngImage.");
    SWIG_fail;
  }
}

%typemap(in) TObject*
{
  $1 = Python::VclObject_AsObjectNoThrow($input);
  if($1 == NULL)
  {
    Python::SetErrorString(Python::PyVclException, "Failed to convert from " + Python::GetTypeName($input) + " to TObject.");
    SWIG_fail;
  }
}

%typemap(out) boost::shared_ptr<TGraphElem>
{
  $result = DownCastSharedPtr($1);
}

%typemap(out) const boost::shared_ptr<TGraphElem>&
{
  $result = DownCastSharedPtr(*$1);
}

%typemap(out) const boost::shared_ptr<TBaseFuncType>& = const boost::shared_ptr<TGraphElem>&;

%typemap(out) Func32::TCoord<long double> {
  $result = Py_BuildValue("dd", (double)$1.x, (double)$1.y);
}

%typemap(out) std::pair<double,double>* {
  $result = Py_BuildValue("dd", $1->first, $1->second);
}

%define TUPLE(StructName, V...)
%typemap(out) StructName
{
  $1_type *p = &$1;
  $result = Python::CreateTuple(V);
}

%typemap(out) const StructName&
{
  $*1_ltype *p = $1;
  $result = Python::CreateTuple(V);
}

%typemap(out) StructName*
{
  $*1_ltype *p = $1;
  $result = Python::CreateTuple(V);
}

%typemap(in) StructName
{
  $1_type *p = &$1;
  if(!Python::FromTuple($input, V))
      SWIG_fail;
}

%typemap(in) StructName*(StructName temp)
{
  $*1_ltype *p = &temp;
  $1 = p;
  if(!Python::FromTuple($input, V))
      SWIG_fail;
}

%typemap(in) const StructName&
{
  $*1_ltype *p = $1;
  if(!Python::FromTuple($input, V))
      SWIG_fail;
}
%enddef

%typemap(throws) Func32::EFuncError %{
  PyErr_SetString(Python::PyEFuncError, ToString(GetErrorMsg($1)).c_str());
  SWIG_fail;
%}

%typemap(throws) Exception %{
  Python::PyVclHandleException();
  SWIG_fail;
%}

%define HANDLE_FPU(function)
%exception function
%{
  SET_DEFAULT_FPU_MASK();
  $action
  SET_PYTHON_FPU_MASK();
%}
%enddef

%define HANDLE_PUSH_UNDO_ELEM(function)
%exception function
%{
  SET_DEFAULT_FPU_MASK();
  PushUndoElem(arg1);
  $action
  SET_PYTHON_FPU_MASK();
%}
%enddef

%define HANDLE_ENUM(TEnum)
%typemap(out) TEnum
{
  try
  {
    $result = Python::ToPyObject(TValue::From($1));
  }
  catch(...)
  {
    Python::PyVclHandleException();
    SWIG_fail;
  }
}

%typemap(in) TEnum
{
  try
  {
    $1 = Python::ToValue($input, __delphirtti(TEnum)).AsType<TEnum>();    
  }
  catch(...)
  {
    Python::PyVclHandleException();
    SWIG_fail;
  }
}
%enddef


TUPLE(TPointSeriesPoint, p->First, p->Second, p->xError.Text, p->yError.Text)
TUPLE(Func32::TDblPoint, p->x, p->y)
TUPLE(TDefaultData, p->Style, p->Color, p->Size)
TUPLE(%arg(std::pair<unsigned,unsigned>), p->first, p->second)
TUPLE(%arg(std::pair<std::wstring,std::wstring>), p->first, p->second)
TUPLE(%arg(std::pair<TTextValue,TTextValue>), p->first, p->second)

%apply double {long double};
typedef unsigned TColor;
typedef unsigned TBrushStyle;

