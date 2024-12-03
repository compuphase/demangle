/* GNU C++ symbol name demangler
 * Test file.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "demangle.h"

void test(const char *mangled, const char *plain)
{
  char name[256];
  int result = demangle(name, sizeof name, mangled);
  if (!result)
    strcpy(name, "failed");
  assert(strlen(name) < sizeof name);
  printf("%s -> %s\n", mangled, name);
  assert(strcmp(name, plain) == 0);
}

int main(int argc,char *argv[])
{
  test("_Z3funi", "fun(int)");
  test("_Z3funv", "fun()");
  test("_Z3foocis", "foo(char,int,short)");
  test("_Z3fooPKi", "foo(int const*)");
  test("_Z3fooPKiS_", "foo(int const*,int const)");
  test("_Z3fooPKiS0_", "foo(int const*,int const*)");
  test("_Z3foo3bar", "foo(bar)");
  test("_Z3fooPKiS1_", "failed");
  test("_Z10wxOnAssertPKciS0_S0_PKw@@WXU_3.0", "wxOnAssert(char const*,int,char const*,char const*,wchar_t const*)");
  test("_ZN11KeyCfgFrame10GetKeyModeEi", "KeyCfgFrame::GetKeyMode(int)");
  test("_ZN11wxAnyButton19DoSetBitmapPositionE11wxDirection@@WXU_3.0", "wxAnyButton::DoSetBitmapPosition(wxDirection)");
  test("_Z1AIcfE", "A<char,float>");
  test("_ZN19wxNavigationEnabledI16wxTopLevelWindowE8SetFocusEv", "wxNavigationEnabled<wxTopLevelWindow>::SetFocus()");
  test("_ZN10GameOfLifeC1Eii", "GameOfLife::GameOfLife(int,int)");
  test("_ZN10GameOfLifeD1Eii", "GameOfLife::~GameOfLife(int,int)");
  test("_ZN3foo3BarIPcE11some_methodEPS2_S3_S3_", "foo::Bar<char*>::some_method(foo::Bar<char*>*,foo::Bar<char*>*,foo::Bar<char*>*)");
  test("_ZN3foo3BarIiE11some_methodEPS1_S2_S2_", "foo::Bar<int>::some_method(foo::Bar<int>*,foo::Bar<int>*,foo::Bar<int>*)");
  test("_ZN1a3fooENS_1AES0_", "a::foo(a::A,a::A)");
  test("_ZmmAtl", "failed");
  test("_ZZaSFvOEES_", "failed");
  test("_ZZeqFvOEES_z", "failed");
  test("_Z3fo5n", "fo5(__int128)");
  test("_Z3fo5o", "fo5(unsigned __int128)");
  test("_Zrm1XS_", "operator%(X,X)");
  test("_ZplR1XS0_", "operator+(X&,X&)");
  test("_ZlsRK1XS1_", "operator<<(X const&,X const&)");
  test("_ZN3FooIA4_iE3barE", "Foo<int[4]>::bar");
  test("_Z1fIiEvi", "void f<int>(int)");
  test("_Z5firstI3DuoEvS0_", "void first<Duo>(Duo)");
  test("_Z5firstI3DuoEvT_", "void first<Duo>(Duo)");
  test("_Z3fooIiFvdEiEvv", "void foo<int,void(double),int>()");
  test("_Z1fIFvvEEvv", "void f<void()>()");
  test("_ZN6System5Sound4beepEv", "System::Sound::beep()");
  test("_ZN5StackIiiE5levelE", "Stack<int,int>::level");
  test("_Z1fI1XEvPVN1AIT_E1TE", "void f<X>(A<X>::T volatile*)");
  test("_Z4makeI7FactoryiET_IT0_Ev", "Factory<int> make<Factory,int>()");
  test("_Z3foo5Hello5WorldS0_S_", "foo(Hello,World,World,Hello)");
  test("_ZlsRSoRKSs", "operator<<(std::ostream&,std::string const&)");
  test("_Z3fooPM2ABi", "foo(int AB::**)");
  test("_Z1fM1AKFvvE", "f(void (A::*)() const)");
  test("_Z2f0Pu8char16_t", "f0(char16_t*)");
  test("_ZZN1N1fEiE1p", "N::f(int)::p");
  test("_ZZN1N1fEiEs", "N::f(int)::{string-literal}");
  test("_Z1fPFvvEM1SFvvE", "f(void(*)(),void (S::*)())");
  test("_ZN1N1TIiiE2mfES0_IddE", "N::T<int,int>::mf(N::T<double,double>)");
  test("_ZSt5state", "std::state");
  test("_ZNSt3_In4wardE", "std::_In::ward");
  test("_Z1fA37_iPS_", "f(int[37],int(*)[37])");
  test("_Z1fM1AFivEPS0_", "f(int (A::*)(),int(*)())");
  test("_Z1fPKM1AFivE", "f(int (A::**)() const)");
  test("_Z1jM1AFivEPS1_", "j(int (A::*)(),int (A::**)())");
  test("_Z1sPA37_iPS0_", "s(int(*)[37],int(**)[37])");
  test("_Z3fooA30_A_i", "foo(int[30][])");
  test("_Z3kooPA28_A30_i", "koo(int(*)[28][30])");
  test("_Z1fILin1EEvv", "void f<-1>()");
  test("_ZlsRKU3fooU4bart1XS0_", "operator<<(X bart foo const&,X bart)");
  test("_Z1fM1AKFivE", "f(int (A::*)() const)");
  test("_Z3absILi11EEvv", "void abs<11>()");
  test("_Z1fP1cIPFiiEE", "f(c<int(*)(int)>*)");
  test("_Z1fPFPA1_ivE", "f(int(*(*)())[1])");
  test("_ZN1AIsE1BIcEEiT_", "int A<short>::B<char>(char)");
  test("_ZN12libcw_app_ct10add_optionIS_EEvMT_FvPKcES3_cS3_S3_", "void libcw_app_ct::add_option<libcw_app_ct>(void (libcw_app_ct::*)(char const*),char const*,char,char const*,char const*)");
  test("_ZN5libcw5debug13cwprint_usingINS_9_private_12GlobalObjectEEENS0_17cwprint_using_tctIT_EERKS5_MS5_KFvRSt7ostreamE", "libcw::debug::cwprint_using_tct<libcw::_private_::GlobalObject> libcw::debug::cwprint_using<libcw::_private_::GlobalObject>(libcw::_private_::GlobalObject const&,void (libcw::_private_::GlobalObject::*)(std::ostream&) const)");
  test("_ZNKSt15_Deque_iteratorIP15memory_block_stRKS1_PS2_EeqERKS5_", "std::_Deque_iterator<memory_block_st*,memory_block_st* const&,memory_block_st* const*>::operator==(std::_Deque_iterator<memory_block_st*,memory_block_st* const&,memory_block_st* const*> const&) const");
  test("_Z1fI1APS0_PKS0_EvT_T0_T1_PA4_S3_M1CS8_", "void f<A,A*,A const*>(A,A*,A const*,A const*(*)[4],A const*(* C::*)[4])");
  test("_ZNKSt17__normal_iteratorIPK6optionSt6vectorIS0_SaIS0_EEEmiERKS6_", "std::__normal_iterator<option const*,std::vector<option,std::allocator<option> > >::operator-(std::__normal_iterator<option const*,std::vector<option,std::allocator<option> > > const&) const");
  test("_ZNSbIcSt11char_traitsIcEN5libcw5debug27no_alloc_checking_allocatorEE12_S_constructIPcEES6_T_S7_RKS3_", "char* std::basic_string<char,std::char_traits<char>,libcw::debug::no_alloc_checking_allocator>::_S_construct<char*>(char*,char*,libcw::debug::no_alloc_checking_allocator const&)");
  test("_Z10hairyfunc5PFPFilEPcE", "hairyfunc5(int(*(*)(char*))(long))");
  test("_ZNK11__gnu_debug16_Error_formatter14_M_format_wordImEEvPciPKcT_", "void __gnu_debug::_Error_formatter::_M_format_word<unsigned long>(char*,int,char const*,unsigned long) const");
  test("_ZNSdD0Ev", "std::iostream::~iostream()");
  test("_Z1fM1AKiPKS1_", "f(int const A::*,int const A::* const*)");
  test("_ZSA", "failed");
  test("_ZN1fIL_", "failed");
  test("_Za", "failed");
  test("_ZNSA", "failed");
  test("_ZNT", "failed");
  test("_Z1aMark", "failed");
  test("_Z1fM1AKiPKS1_", "f(int const A::*,int const A::* const*)");
  test("_ZZL3foo_2vE4var1", "foo()::var1");
  test("_ZZL3foo_2vE4var1_0", "foo()::var1");
  test("_Z1fN1SUt_E", "f(S::{unnamed type})");
  test("_Z5outerIsEcPFilE", "char outer<short>(int(*)(long))");
  test("_Z6outer2IsEPFilES1_", "int(*outer2<short>(int(*)(long)))(long)");
  test("_Z5outerIsEcPFilE", "char outer<short>(int(*)(long))");
  test("_Z5outerPFsiEl", "outer(short(*)(int),long)");
  test("_Z3fooIA3_iEvRKT_", "void foo<int[3]>(int(&)[3] const)");
  test("_Z3fooIPA3_iEvRKT_", "void foo<int(*)[3]>(int(*&)[3] const)");
  test("_ZZ3BBdI3FooEvvENK3Fob3FabEv", "BBd<Foo>()::Fob::Fab() const");
  test("_ZZZ3BBdI3FooEvvENK3Fob3FabEvENK3Gob3GabEv", "BBd<Foo>()::Fob::Fab() const::Gob::Gab() const");
  test("_ZNK5boost6spirit5matchI13rcs_deltatextEcvMNS0_4impl5dummyEFvvEEv", "boost::spirit::match<rcs_deltatext>::operator void (boost::spirit::impl::dummy::*)()() const");
  test("_ZNK1C1fIiEEPFivEv", "int(*C::f<int>() const)()");
  test("_ZZN7myspaceL3foo_1EvEN11localstruct1fEZNS_3fooEvE16otherlocalstruct", "myspace::foo()::localstruct::f(myspace::foo()::otherlocalstruct)");
  test("_Z1fDfDdDeDhDsDi", "f(decimal32,decimal64,decimal128,decimal16,char16_t,char32_t)");
  test("_ZN1AdlEPv", "A::operator delete(void*)");
  test("_Z1fIiERDaRKT_S1_", "auto& f<int>(int const&,int)");
  test("_Z5totalIdEiT_S0_", "int total<double>(double,double)");
  test("_Z5totalIidEiT_T0_", "int total<int,double>(int,double)");
  test("_Z5totalIidfEiT_T0_T1_", "int total<int,double,float>(int,double,float)");
  test("_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc@@GLIBCXX_3.4", "std::basic_ostream<char,std::char_traits<char> >& std::operator<< <std::char_traits<char> >(std::basic_ostream<char,std::char_traits<char> >&,char const*)");
  test("_Z1gIJidEEvDpT_", "void g<int,double>(int,double)");
  test("_Z1gIidEvDpT_", "void g<int,double>((int)...)");
  test("_Z1fI1SENDtfp_E4typeET_", "decltype({parm#0})::type f<S>(S)");
  test("_Z1fI1AEDtdtfp_srT_1xES1_", "decltype({parm#0}.A::x) f<A>(A)");
  test("_Z3addIidEDTplL_Z1gEfp0_ET_T0_", "decltype(g+{parm#1}) add<int,double>(int,double)");
  test("_Z1fIJPiPfPdEEvDpT_", "void f<int*,float*,double*>(int*,float*,double*)");
  test("_ZngILi42EEvN1AIXplT_Li2EEE1TE", "void operator-<42>(A<42+2>::T)");
  test("_Z1fIT_EvT_", "failed");
  test("_Z20instantiate_with_intI3FooET_IiEv", "Foo<int> instantiate_with_int<Foo>()");
  test("_Z3fooISt6vectorIiEEvv", "void foo<std::vector<int> >()");
  test("_ZN3foo3barE3quxS0_", "foo::bar(qux,qux)");
  test("_ZN4funcI2TyEEN6ResultIT_EES3_", "Result<Ty> func<Ty>(Result<Ty>)");
  test("_ZN4funcI2TyEEN6ResultIT_EES2_", "Result<Ty> func<Ty>(Ty)");
  test("_ZN4funcI2TyEEN6ResultIT_EES1_", "Result<Ty> func<Ty>(Result)");
  test("_ZN4funcI2TyEEN6ResultIT_EES0_", "Result<Ty> func<Ty>(Ty)");
  test("_ZN4funcI2TyEEN6ResultIT_EES_", "Result<Ty> func<Ty>(func)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES_", "void Ty::method<Ty>(void (Ty::*)(char const*),Ty)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES0_", "void Ty::method<Ty>(void (Ty::*)(char const*),Ty::method)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES1_", "void Ty::method<Ty>(void (Ty::*)(char const*),Ty)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES2_", "void Ty::method<Ty>(void (Ty::*)(char const*),char const)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES3_", "void Ty::method<Ty>(void (Ty::*)(char const*),char const*)");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES4_", "void Ty::method<Ty>(void (Ty::*)(char const*),void(char const*))");
  test("_ZN2Ty6methodIS_EEvMT_FvPKcES5_", "void Ty::method<Ty>(void (Ty::*)(char const*),void (Ty::*)(char const*))");
  test("_ZNK1fB5cxx11Ev", "f[abi:cxx11]() const");
  test("_ZUlvE_", "{lambda()#1}");
  test("_ZUlisE_", "{lambda(int,short)#1}");
  test("_ZZ3aaavEUlvE_", "aaa()::{lambda()#1}");
  test("_ZZ3aaavENUlvE_3bbbE", "aaa()::{lambda()#1}::bbb");
  test("_ZN3aaaUlvE_D1Ev", "aaa::{lambda()#1}::~aaa()");
  test("_ZZ3aaavEN3bbbD1Ev", "aaa()::bbb::~bbb()");
  test("_ZZ3aaavENUlvE_D1Ev", "aaa()::{lambda()#1}::~aaa()");
  test("_Z3fooILb0EEvi", "void foo<false>(int)");
  test("_Z3fooILb1EEvi", "void foo<true>(int)");
  test("_Z3fooILb2EEvi", "void foo<(bool)2>(int)");
  test("_ZN6WebKit25WebCacheStorageConnection17didReceiveMessageERN3IPC10ConnectionERNS1_7DecoderE", "WebKit::WebCacheStorageConnection::didReceiveMessage(IPC::Connection&,IPC::Decoder&)");
  test("_ZN3IPC10Connection15dispatchMessageESt10unique_ptrINS_7DecoderESt14default_deleteIS2_EE", "IPC::Connection::dispatchMessage(std::unique_ptr<IPC::Decoder,std::default_delete<IPC::Decoder> >)");
  test("_ZNK1QssERKS_", "Q::operator<=>(Q const&) const");
  test("_ZNSt17_Function_handlerIFviEN3JPH19JobSystemThreadPool19mThreadInitFunctionMUliE_EE9_M_invokeERKSt9_Any_dataOi", "std::_Function_handler<void(int),JPH::JobSystemThreadPool::mThreadInitFunction::{lambda(int)#1}>::_M_invoke(std::_Any_data const&,int&&)");
  test("_ZZZ1fILb0EJiiEEvvENKUlvE_clEvE1n", "f<false,int,int>()::{lambda()#1}::operator()() const::n");
  test("_ZZZ1fILb0EJiiEEvvENKUlvE0_clEvE1n", "f<false,int,int>()::{lambda()#2}::operator()() const::n");

  printf("\nAll tests passed.\n");
  return 0;
}

