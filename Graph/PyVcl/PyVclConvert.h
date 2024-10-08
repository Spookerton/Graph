//---------------------------------------------------------------------------
#ifndef PyVclConvertH
#define PyVclConvertH
#include "PythonBind.h"
#include <vector>
#include <Rtti.hpp>
//---------------------------------------------------------------------------
namespace Python
{
	class EPyVclError : public Exception
	{
	public:
		EPyVclError(const String &Str);
    EPyVclError(const wchar_t *Format, ...);
	};

	extern Rtti::TRttiContext Context;
	extern PyObject *PyVclException;

	Rtti::TValue ToValue(PyObject *O, TTypeInfo *TypeInfo, bool ConstValue=false);
	void TupleToValues(PyObject *O, std::vector<Rtti::TValue> &Values, const DynamicArray<Rtti::TRttiParameter*> &Parameters);
  void ReverseTupleToValues(PyObject *O, const std::vector<TValue> &Values, const DynamicArray<TRttiParameter*> &Parameters);
  String GetTypeName(PyObject *O);
  TValue CreateMethodImplementation(PyObject *O, TTypeInfo *TypeInfo);
  bool CheckValidImplementation(void *Impl);
	PyObject* PyVclHandleException();

	PyObject* ToPyObject(bool Value);
	PyObject* ToPyObject(int Value);
	PyObject* ToPyObject(long Value);
	PyObject* ToPyObject(long long Value);
	PyObject* ToPyObject(char Value);
	PyObject* ToPyObject(wchar_t Value);
	inline PyObject* ToPyObject(unsigned Value) {return ToPyObject(static_cast<int>(Value));}
	PyObject* ToPyObject(double Value);
	inline PyObject* ToPyObject(long double Value) {return ToPyObject(static_cast<double>(Value));}
	PyObject* ToPyObject(const std::wstring &Str);
	PyObject* ToPyObject(const String &Str);
	PyObject* ToPyObject(const Rtti::TValue &Value);
	PyObject* ToPyObject(TObject *Object);
	PyObject* ToPyObject(const char *Str);
	PyObject* ToPyObject(const wchar_t *Str);
	PyObject* ToPyObject(const Variant &V);
	template<typename T>
	TPyObjectPtr ToPyObjectPtr(const T &Value) {return TPyObjectPtr(ToPyObject(Value), false);}

	template<typename T> T FromPyObject(PyObject *O);
	template<> long FromPyObject<long>(PyObject *O);
	template<> int FromPyObject<int>(PyObject *O);
	template<> double FromPyObject<double>(PyObject *O);
	template<> inline unsigned FromPyObject<unsigned>(PyObject *O) {return FromPyObject<int>(O);}
	template<> inline TColor FromPyObject<TColor>(PyObject *O) {return static_cast<TColor>(FromPyObject<int>(O));}
	template<> inline long double FromPyObject<long double>(PyObject *O) {return FromPyObject<double>(O);}
	template<> std::wstring FromPyObject<std::wstring>(PyObject *O);
	template<> String FromPyObject<String>(PyObject *O);
	template<> bool FromPyObject<bool>(PyObject *O);
	template<typename T> inline bool FromPyObject(PyObject *O, T &Value)
  	{
    try
    {
      Value = FromPyObject<T>(O);
      return true;
    }
    catch(EPyVclError &E)
    {
      PyVclHandleException();
      return false;
    }
  }
  template<typename T> T FromPyObjectPtr(const TPyObjectPtr &Ptr) {return FromPyObject<T>(Ptr.get());}
}
#endif
