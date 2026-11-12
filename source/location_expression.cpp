#include "minidayz/location_expression.hpp"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <utility>
namespace minidayz { namespace {
struct N { enum K{Null,Num,Str,Bool,Arr} k{Null}; double n{}; bool b{}; std::string s; std::vector<N> a; };
class Reader {
 std::string s; std::size_t p{};
 void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\n'||s[p]=='\r'||s[p]=='\t'))++p;}
 bool str(std::string& o){if(p>=s.size()||s[p++]!='"')return false;while(p<s.size()){char c=s[p++];if(c=='"')return true;if(c=='\\'){if(p>=s.size())return false;char e=s[p++];switch(e){case '"':case '\\':case '/':o+=e;break;case 'n':o+='\n';break;case 'r':o+='\r';break;case 't':o+='\t';break;case 'b':o+='\b';break;case 'f':o+='\f';break;default:return false;}}else{if((unsigned char)c<32)return false;o+=c;}}return false;}
 bool val(N& x){ws();if(p>=s.size())return false;if(s[p]=='['){++p;x.k=N::Arr;ws();if(p<s.size()&&s[p]==']'){++p;return true;}for(;;){N v;if(!val(v))return false;x.a.push_back(std::move(v));ws();if(p>=s.size())return false;char c=s[p++];if(c==']')return true;if(c!=',')return false;}}
 if(s[p]=='"'){x.k=N::Str;return str(x.s);}if(s.substr(p,4)=="null"){p+=4;return true;}if(s.substr(p,4)=="true"){p+=4;x.k=N::Bool;x.b=true;return true;}if(s.substr(p,5)=="false"){p+=5;x.k=N::Bool;return true;}
 char* e=nullptr;errno=0;x.n=std::strtod(s.c_str()+p,&e);if(e==s.c_str()+p||errno==ERANGE||!std::isfinite(x.n))return false;p+=(e-(s.c_str()+p));x.k=N::Num;return true;}
public: explicit Reader(std::string_view v):s(v){} bool parse(N& x){if(!val(x))return false;ws();return p==s.size();}
};
bool run(const N& x,const LocationExpressionContext& c,LocationExpressionValue& o){
 using T=LocationExpressionValue; if(x.k==N::Num){o=T::numeric(x.n);return true;}if(x.k==N::Str){o=T::text(x.s);return true;}if(x.k==N::Bool){o.type=T::Type::Boolean;o.boolean=x.b;return true;}if(x.k==N::Null){o={};return true;}
 if(x.a.empty()||x.a[0].k!=N::Num)return false;auto op=(std::uint32_t)x.a[0].n;
 if(op==0){return x.a.size()==2&&run(x.a[1],c,o);}
 if(op==23){return x.a.size()==2&&x.a[1].k==N::Str&&c.variable&&c.variable(x.a[1].s,o);}
 if(op==4||op==5){if(x.a.size()!=3)return false;T a,b;if(!run(x.a[1],c,a)||!run(x.a[2],c,b))return false;o=T::numeric(op==4?a.as_number()+b.as_number():a.as_number()-b.as_number());return true;}
 if(op==19){if(x.a.size()<2||x.a[1].k!=N::Num||!c.function)return false;std::vector<T> args;if(x.a.size()>2){if(x.a[2].k!=N::Arr)return false;for(const auto& v:x.a[2].a){T z;if(!run(v,c,z))return false;args.push_back(std::move(z));}}return c.function((std::uint32_t)x.a[1].n,args,o);}
 if(op==20){if(!c.object)return false;std::vector<T> args;for(const auto& v:x.a){T z;if(!run(v,c,z))return false;args.push_back(std::move(z));}return c.object(args,o);}
 // Preserve unsupported source opcodes as explicit delegated cases.
 return c.special&&c.special(op,{},o);
}
}
double LocationExpressionValue::as_number()const noexcept{switch(type){case Type::Number:return number;case Type::Boolean:return boolean?1.0:0.0;case Type::String:{char* e=nullptr;double v=std::strtod(string.c_str(),&e);return e==string.c_str()?0.0:v;}case Type::Null:return 0.0;}return 0.0;}
bool LocationExpressionEvaluator::evaluate(std::string_view json,LocationExpressionValue& out)const{N root;return Reader(json).parse(root)&&run(root,context_,out);}
}
