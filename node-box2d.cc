/**
 * Copyright (c) Flyover Games, LLC.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "node-box2d.h"

#include <Box2D/Box2D.h>

///#include <map>

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "printf", __VA_ARGS__)
#endif

#define countof(_a) (sizeof(_a)/sizeof((_a)[0]))

// nan extensions

#define NANX_STRING(STRING) Nan::New<v8::String>(STRING).ToLocalChecked()
#define NANX_SYMBOL(SYMBOL) Nan::New<v8::String>(SYMBOL).ToLocalChecked()

#define NANX_CONSTANT(TARGET, CONSTANT) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), Nan::New(CONSTANT))
#define NANX_CONSTANT_VALUE(TARGET, CONSTANT, VALUE) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), Nan::New(VALUE))

#define NANX_CONSTANT_STRING(TARGET, CONSTANT) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), NANX_STRING(CONSTANT))
#define NANX_CONSTANT_STRING_VALUE(TARGET, CONSTANT, VALUE) Nan::Set(TARGET, NANX_SYMBOL(#CONSTANT), NANX_STRING(VALUE))

#define NANX_EXPORT_APPLY(OBJECT, NAME) Nan::Export(OBJECT, #NAME, _export_##NAME)
#define NANX_EXPORT(NAME) static NAN_METHOD(_export_##NAME)

#define NANX_METHOD_APPLY(OBJECT_TEMPLATE, NAME) Nan::SetMethod(OBJECT_TEMPLATE, #NAME, _method_##NAME);
#define NANX_METHOD(NAME) static NAN_METHOD(_method_##NAME)

#define NANX_MEMBER_APPLY(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), _get_##NAME, _set_##NAME);
#define NANX_MEMBER_APPLY_GET(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), _get_##NAME, NULL); // get only
#define NANX_MEMBER_APPLY_SET(OBJECT_TEMPLATE, NAME) Nan::SetAccessor(OBJECT_TEMPLATE, NANX_SYMBOL(#NAME), NULL, _set_##NAME); // set only

#define NANX_MEMBER_VALUE(NAME) NANX_MEMBER_VALUE_GET(NAME) NANX_MEMBER_VALUE_SET(NAME)
#define NANX_MEMBER_VALUE_GET(NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Value>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_VALUE_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Value>()); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_BOOLEAN(TYPE, NAME) NANX_MEMBER_BOOLEAN_GET(TYPE, NAME) NANX_MEMBER_BOOLEAN_SET(TYPE, NAME)
#define NANX_MEMBER_BOOLEAN_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Boolean>(static_cast<bool>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_BOOLEAN_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->BooleanValue()); }

#define NANX_MEMBER_NUMBER(TYPE, NAME) NANX_MEMBER_NUMBER_GET(TYPE, NAME) NANX_MEMBER_NUMBER_SET(TYPE, NAME)
#define NANX_MEMBER_NUMBER_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Number>(static_cast<double>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_NUMBER_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->NumberValue()); }

#define NANX_MEMBER_INTEGER(TYPE, NAME) NANX_MEMBER_INTEGER_GET(TYPE, NAME) NANX_MEMBER_INTEGER_SET(TYPE, NAME)
#define NANX_MEMBER_INTEGER_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<int32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_INTEGER_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->IntegerValue()); }

#define NANX_MEMBER_INT32(TYPE, NAME) NANX_MEMBER_INT32_GET(TYPE, NAME) NANX_MEMBER_INT32_SET(TYPE, NAME)
#define NANX_MEMBER_INT32_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Int32>(static_cast<int32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_INT32_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->Int32Value()); }

#define NANX_MEMBER_UINT32(TYPE, NAME) NANX_MEMBER_UINT32_GET(TYPE, NAME) NANX_MEMBER_UINT32_SET(TYPE, NAME)
#define NANX_MEMBER_UINT32_GET(TYPE, NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Uint32>(static_cast<uint32_t>(Peek(info.This())->NAME))); }
#define NANX_MEMBER_UINT32_SET(TYPE, NAME) static NAN_SETTER(_set_##NAME) { Peek(info.This())->NAME = static_cast<TYPE>(value->Uint32Value()); }

#define NANX_MEMBER_STRING(NAME) NANX_MEMBER_STRING_GET(NAME) NANX_MEMBER_STRING_SET(NAME)
#define NANX_MEMBER_STRING_GET(NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::String>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_STRING_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::String>()); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_OBJECT(NAME) NANX_MEMBER_OBJECT_GET(NAME) NANX_MEMBER_OBJECT_SET(NAME)
#define NANX_MEMBER_OBJECT_GET(NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Object>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_OBJECT_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Object>()); info.GetReturnValue().Set(value); }

#define NANX_MEMBER_ARRAY(NAME) NANX_MEMBER_ARRAY_GET(NAME) NANX_MEMBER_ARRAY_SET(NAME)
#define NANX_MEMBER_ARRAY_GET(NAME) static NAN_GETTER(_get_##NAME) { info.GetReturnValue().Set(Nan::New<v8::Array>(Unwrap(info.This())->m_wrap_##NAME)); }
#define NANX_MEMBER_ARRAY_SET(NAME) static NAN_SETTER(_set_##NAME) { Unwrap(info.This())->m_wrap_##NAME.Reset(value.As<v8::Array>()); info.GetReturnValue().Set(value); }

#define NANX_bool(value)		((value)->BooleanValue())
#define NANX_int(value)			((value)->Int32Value())
#define NANX_int8(value)		static_cast<int8>((value)->Int32Value())
#define NANX_uint8(value)		static_cast<uint8>((value)->Uint32Value())
#define NANX_int16(value)		static_cast<int16>((value)->Int32Value())
#define NANX_uint16(value)		static_cast<uint16>((value)->Uint32Value())
#define NANX_int32(value)		static_cast<int32>((value)->Int32Value())
#define NANX_uint32(value)		static_cast<uint32>((value)->Uint32Value())
#define NANX_float32(value)		static_cast<float32>((value)->NumberValue())
#define NANX_b2BodyType(value)	static_cast<b2BodyType>((value)->IntegerValue())

namespace node_box2d {

//// b2Vec2

class WrapVec2 : public Nan::ObjectWrap
{
private:
	b2Vec2 m_v2;
private:
	WrapVec2() {}
	WrapVec2(const b2Vec2& v2) { m_v2 = v2; } // struct copy
	WrapVec2(float32 x, float32 y) : m_v2(x, y) {}
	~WrapVec2() {}
public:
	b2Vec2* Peek() { return &m_v2; }
	//b2Vec2& GetVec2() { return m_v2; }
	const b2Vec2& GetVec2() const { return m_v2; }
	void SetVec2(const b2Vec2& v2) { m_v2 = v2; } // struct copy
public:
	static WrapVec2* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapVec2* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapVec2>(object); }
	static b2Vec2* Peek(v8::Local<v8::Value> value) { WrapVec2* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Vec2& v2)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapVec2* wrap = Unwrap(instance);
		wrap->SetVec2(v2);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Vec2"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, x)
			NANX_MEMBER_APPLY(prototype_template, y)
			NANX_METHOD_APPLY(prototype_template, SetZero)
			NANX_METHOD_APPLY(prototype_template, Set)
			NANX_METHOD_APPLY(prototype_template, Copy)
			NANX_METHOD_APPLY(prototype_template, SelfNeg)
			NANX_METHOD_APPLY(prototype_template, SelfAdd)
			NANX_METHOD_APPLY(prototype_template, SelfAddXY)
			NANX_METHOD_APPLY(prototype_template, SelfSub)
			NANX_METHOD_APPLY(prototype_template, SelfSubXY)
			NANX_METHOD_APPLY(prototype_template, SelfMul)
			NANX_METHOD_APPLY(prototype_template, Length)
			NANX_METHOD_APPLY(prototype_template, LengthSquared)
			NANX_METHOD_APPLY(prototype_template, Normalize)
			NANX_METHOD_APPLY(prototype_template, SelfNormalize)
			NANX_METHOD_APPLY(prototype_template, IsValid)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			float32 x = (info.Length() > 0) ? NANX_float32(info[0]) : 0.0f;
			float32 y = (info.Length() > 1) ? NANX_float32(info[1]) : 0.0f;
			WrapVec2* wrap = new WrapVec2(x, y);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_NUMBER(float32, x)
	NANX_MEMBER_NUMBER(float32, y)
	NANX_METHOD(SetZero)
	{
		b2Vec2* that = Peek(info.This());
		that->SetZero();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(Set)
	{
		b2Vec2* that = Peek(info.This());
		that->Set(NANX_float32(info[0]), NANX_float32(info[1]));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(Copy)
	{
		b2Vec2* that = Peek(info.This());
		b2Vec2* other = Peek(info[0]);
		*that = *other;
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfNeg)
	{
		b2Vec2* that = Peek(info.This());
		*that = that->operator-();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfAdd)
	{
		b2Vec2* that = Peek(info.This());
		b2Vec2* v = Peek(info[0]);
		that->operator+=(*v);
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfAddXY)
	{
		b2Vec2* that = Peek(info.This());
		that->operator+=(b2Vec2(NANX_float32(info[0]), NANX_float32(info[1])));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfSub)
	{
		b2Vec2* that = Peek(info.This());
		b2Vec2* v = Peek(info[0]);
		that->operator-=(*v);
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfSubXY)
	{
		b2Vec2* that = Peek(info.This());
		that->operator-=(b2Vec2(NANX_float32(info[0]), NANX_float32(info[1])));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SelfMul)
	{
		b2Vec2* that = Peek(info.This());
		that->operator*=(NANX_float32(info[0]));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(Length)
	{
		b2Vec2* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->Length()));
	}
	NANX_METHOD(LengthSquared)
	{
		b2Vec2* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->LengthSquared()));
	}
	NANX_METHOD(Normalize)
	{
		b2Vec2* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->Normalize()));
	}
	NANX_METHOD(SelfNormalize)
	{
		b2Vec2* that = Peek(info.This());
		that->Normalize();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(IsValid)
	{
		b2Vec2* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->IsValid()));
	}
};

//// b2Rot

class WrapRot : public Nan::ObjectWrap
{
private:
	b2Rot m_rot;
private:
	WrapRot() {}
	WrapRot(const b2Rot& rot) { m_rot = rot; } // struct copy
	WrapRot(float32 angle) : m_rot(angle) {}
	~WrapRot() {}
public:
	b2Rot* Peek() { return &m_rot; }
	//b2Rot& GetRot() { return m_rot; }
	const b2Rot& GetRot() const { return m_rot; }
	void SetRot(const b2Rot& rot) { m_rot = rot; } // struct copy
public:
	static WrapRot* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRot* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRot>(object); }
	static b2Rot* Peek(v8::Local<v8::Value> value) { WrapRot* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Rot& rot)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapRot* wrap = Unwrap(instance);
		wrap->SetRot(rot);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Rot"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, s)
			NANX_MEMBER_APPLY(prototype_template, c)
			NANX_METHOD_APPLY(prototype_template, Set)
			//void SetIdentity()
			NANX_METHOD_APPLY(prototype_template, GetAngle)
			//b2Vec2 GetXAxis() const
			//b2Vec2 GetYAxis() const
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			float32 angle = (info.Length() > 0) ? NANX_float32(info[0]) : 0.0f;
			WrapRot* wrap = new WrapRot(angle);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_NUMBER(float32, s)
	NANX_MEMBER_NUMBER(float32, c)
	NANX_METHOD(Set)
	{
		b2Rot* that = Peek(info.This());
		that->Set(NANX_float32(info[0]));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(GetAngle)
	{
		b2Rot* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->GetAngle()));
	}
};

//// b2Transform

class WrapTransform : public Nan::ObjectWrap
{
private:
	b2Transform m_xf;
	Nan::Persistent<v8::Object> m_wrap_p; // m_xf.p
	Nan::Persistent<v8::Object> m_wrap_q; // m_xf.q
private:
	WrapTransform()
	{
		m_wrap_p.Reset(WrapVec2::NewInstance(m_xf.p));
		m_wrap_q.Reset(WrapRot::NewInstance(m_xf.q));
	}
	//WrapTransform(const b2Transform& xf) { m_xf = xf; } // struct copy
	//WrapTransform(float32 angle) : m_xf(angle) {}
	~WrapTransform()
	{
		m_wrap_p.Reset();
		m_wrap_q.Reset();
	}
public:
	b2Transform* Peek() { return &m_xf; }
	//b2Transform& GetTransform() { return m_xf; }
	//const b2Transform& GetTransform() const { return m_xf; }
	//void SetTransform(const b2Transform& xf) { m_xf = xf; } // struct copy
	const b2Transform& GetTransform()
	{
		SyncPull();
		return m_xf;
	}
	void SetTransform(const b2Transform& xf)
	{
		m_xf = xf; // struct copy
		SyncPush();
	}
	void SetTransform(const b2Vec2& position, float32 angle)
	{
		m_xf.Set(position, angle);
		SyncPush();
	}
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_xf.p = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_p))->GetVec2(); // struct copy
		m_xf.q = WrapRot::Unwrap(Nan::New<v8::Object>(m_wrap_q))->GetRot(); // struct copy
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_p))->SetVec2(m_xf.p);
		WrapRot::Unwrap(Nan::New<v8::Object>(m_wrap_q))->SetRot(m_xf.q);
	}
public:
	static WrapTransform* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapTransform* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapTransform>(object); }
	static b2Transform* Peek(v8::Local<v8::Value> value) { WrapTransform* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Transform& xf)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapTransform* wrap = Unwrap(instance);
		wrap->SetTransform(xf);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Transform"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, p)
			NANX_MEMBER_APPLY(prototype_template, q)
			NANX_METHOD_APPLY(prototype_template, SetIdentity)
			NANX_METHOD_APPLY(prototype_template, Set)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapTransform* wrap = new WrapTransform();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(p) // m_wrap_p
	NANX_MEMBER_OBJECT(q) // m_wrap_q
	NANX_METHOD(SetIdentity)
	{
		//b2Transform* that = Peek(info.This());
		WrapTransform* wrap = Unwrap(info.This());
		b2Transform* that = &wrap->m_xf;
		that->SetIdentity();
		wrap->SyncPush();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(Set)
	{
		//b2Transform* that = Peek(info.This());
		WrapTransform* wrap = Unwrap(info.This());
		b2Transform* that = &wrap->m_xf;
		b2Vec2* position = WrapVec2::Peek(info[0]);
		float32 angle = NANX_float32(info[1]);
		that->Set(*position, angle);
		wrap->SyncPush();
		info.GetReturnValue().Set(info.This());
	}
};

//// b2AABB

class WrapAABB : public Nan::ObjectWrap
{
private:
	b2AABB m_aabb;
	Nan::Persistent<v8::Object> m_wrap_lowerBound; // m_aabb.lowerBound
	Nan::Persistent<v8::Object> m_wrap_upperBound; // m_aabb.upperBound
private:
	WrapAABB()
	{
		m_wrap_lowerBound.Reset(WrapVec2::NewInstance(m_aabb.lowerBound));
		m_wrap_upperBound.Reset(WrapVec2::NewInstance(m_aabb.upperBound));
	}
	~WrapAABB()
	{
		m_wrap_lowerBound.Reset();
		m_wrap_upperBound.Reset();
	}
public:
	b2AABB* Peek() { return &m_aabb; }
	const b2AABB& GetAABB()
	{
		SyncPull();
		return m_aabb;
	}
	void SetAABB(const b2AABB& mass_data)
	{
		m_aabb = mass_data; // struct copy
		SyncPush();
	}
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_aabb.lowerBound = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_lowerBound))->GetVec2(); // struct copy
		m_aabb.upperBound = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_upperBound))->GetVec2(); // struct copy
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_lowerBound))->SetVec2(m_aabb.lowerBound);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_upperBound))->SetVec2(m_aabb.upperBound);
	}
public:
	static WrapAABB* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapAABB* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapAABB>(object); }
	static b2AABB* Peek(v8::Local<v8::Value> value) { WrapAABB* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2AABB& mass_data)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapAABB* wrap = Unwrap(instance);
		wrap->SetAABB(mass_data);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2AABB"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, lowerBound)
			NANX_MEMBER_APPLY(prototype_template, upperBound)
			NANX_METHOD_APPLY(prototype_template, IsValid)
			NANX_METHOD_APPLY(prototype_template, GetCenter)
			NANX_METHOD_APPLY(prototype_template, GetExtents)
			NANX_METHOD_APPLY(prototype_template, GetPerimeter)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapAABB* wrap = new WrapAABB();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(lowerBound) // m_wrap_lowerBound
	NANX_MEMBER_OBJECT(upperBound) // m_wrap_upperBound
	NANX_METHOD(IsValid)
	{
		b2AABB* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->IsValid()));
	}
	NANX_METHOD(GetCenter)
	{
		//b2AABB* that = Peek(info.This());
		WrapAABB* wrap = Unwrap(info.This());
		b2AABB* that = &wrap->m_aabb;
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(that->GetCenter());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetExtents)
	{
		//b2AABB* that = Peek(info.This());
		WrapAABB* wrap = Unwrap(info.This());
		b2AABB* that = &wrap->m_aabb;
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(that->GetExtents());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetPerimeter)
	{
		b2AABB* that = Peek(info.This());
		info.GetReturnValue().Set(Nan::New(that->GetPerimeter()));
	}
};

//// b2MassData

class WrapMassData : public Nan::ObjectWrap
{
private:
	b2MassData m_mass_data;
	Nan::Persistent<v8::Object> m_wrap_center; // m_mass_data.center
private:
	WrapMassData()
	{
		m_wrap_center.Reset(WrapVec2::NewInstance(m_mass_data.center));
	}
	~WrapMassData()
	{
		m_wrap_center.Reset();
	}
public:
	b2MassData* Peek() { return &m_mass_data; }
	const b2MassData& GetMassData()
	{
		SyncPull();
		return m_mass_data;
	}
	void SetMassData(const b2MassData& mass_data)
	{
		m_mass_data = mass_data; // struct copy
		SyncPush();
	}
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_mass_data.center = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_center))->GetVec2(); // struct copy
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_center))->SetVec2(m_mass_data.center);
	}
public:
	static WrapMassData* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMassData* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMassData>(object); }
	static b2MassData* Peek(v8::Local<v8::Value> value) { WrapMassData* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2MassData& mass_data)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapMassData* wrap = Unwrap(instance);
		wrap->SetMassData(mass_data);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2MassData"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, mass)
			NANX_MEMBER_APPLY(prototype_template, center)
			NANX_MEMBER_APPLY(prototype_template, I)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapMassData* wrap = new WrapMassData();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_NUMBER(float32, mass)
	NANX_MEMBER_OBJECT(center) // m_wrap_center
	NANX_MEMBER_NUMBER(float32, I)
};

//// b2Shape

class WrapShape : public Nan::ObjectWrap
{
protected:
	WrapShape() {}
	~WrapShape() {}
public:
	b2Shape* Peek() { return &GetShape(); }
	virtual b2Shape& GetShape() = 0;
	virtual void SyncPull() {}
	virtual void SyncPush() {}
public:
	const b2Shape& UseShape() { SyncPull(); return GetShape(); }
///private:
///	static std::map<b2Shape*,WrapShape*> g_wrap_map;
public:
	static WrapShape* GetWrap(b2Shape* shape)
	{
		//return static_cast<WrapShape*>(shape->GetUserData());
///		std::map<b2Shape*,WrapShape*>::iterator it = g_wrap_map.find(shape);
///		if (it != g_wrap_map.end())
///		{
///			return it->second;
///		}
		return NULL;
	}

	static void SetWrap(b2Shape* shape, WrapShape* wrap)
	{
		//shape->SetUserData(wrap);
///		std::map<b2Shape*,WrapShape*>::iterator it = g_wrap_map.find(shape);
///		if (it != g_wrap_map.end())
///		{
///			if (wrap)
///			{
///				it->second = wrap;
///			}
///			else
///			{
///				g_wrap_map.erase(it);
///			}
///		}
///		else
///		{
///			if (wrap)
///			{
///				g_wrap_map[shape] = wrap;
///			}
///		}
	}
public:
	static WrapShape* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapShape* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapShape>(object); }
	static b2Shape* Peek(v8::Local<v8::Value> value) { WrapShape* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
//public:
//	static NAN_MODULE_INIT(Init)
//	{
//		v8::Local<v8::Function> constructor = GetConstructor();
//		target->Set(constructor->GetName(), constructor);
//	}
//	static v8::Local<v8::Object> NewInstance()
//	{
//		Nan::EscapableHandleScope scope;
//		v8::Local<v8::Function> constructor = GetConstructor();
//		v8::Local<v8::Object> instance = constructor->NewInstance();
//		return scope.Escape(instance);
//	}
//	static v8::Local<v8::Object> NewInstance(const b2Shape& shape)
//	{
//		Nan::EscapableHandleScope scope;
//		v8::Local<v8::Function> constructor = GetConstructor();
//		v8::Local<v8::Object> instance = constructor->NewInstance();
//		WrapShape* wrap = Unwrap(instance);
//		wrap->SetShape(shape);
//		return scope.Escape(instance);
//	}
public:
//	static v8::Local<v8::Function> GetConstructor()
//	{
//		Nan::EscapableHandleScope scope;
//		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
//		v8::Local<v8::Function> constructor = function_template->GetFunction();
//		return scope.Escape(constructor);
//	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
//			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>();
			function_template->SetClassName(NANX_SYMBOL("b2Shape"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, m_type)
			NANX_MEMBER_APPLY(prototype_template, m_radius)
			NANX_METHOD_APPLY(prototype_template, GetType)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
//	static NAN_METHOD(New)
//	{
//		if (info.IsConstructCall())
//		{
//			WrapShape* wrap = new WrapShape();
//			wrap->Wrap(info.This());
//			info.GetReturnValue().Set(info.This());
//		}
//		else
//		{
//			v8::Local<v8::Function> constructor = GetConstructor();
//			info.GetReturnValue().Set(constructor->NewInstance());
//		}
//	}
	NANX_MEMBER_INTEGER(b2Shape::Type, m_type)
	NANX_MEMBER_NUMBER(float32, m_radius)
	NANX_METHOD(GetType)
	{
		//b2Shape* that = Peek(info.This());
		WrapShape* wrap = Unwrap(info.This());
		//b2Shape* that = &wrap->m_shape;
		//wrap->SyncPush();
		//info.GetReturnValue().Set(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->GetShape().GetType()));
	}
};

//// b2CircleShape

class WrapCircleShape : public WrapShape
{
private:
	b2CircleShape m_circle;
	Nan::Persistent<v8::Object> m_wrap_m_p; // m_circle.m_p
private:
	WrapCircleShape(float32 radius = 0.0f)
	{
		m_circle.m_radius = radius;
		m_wrap_m_p.Reset(WrapVec2::NewInstance(m_circle.m_p));
	}
	~WrapCircleShape()
	{
		m_wrap_m_p.Reset();
	}
public:
	b2CircleShape* Peek() { return &m_circle; }
	virtual b2Shape& GetShape() { return m_circle; } // override WrapShape
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		m_circle.m_p = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_p))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_p))->SetVec2(m_circle.m_p);
	}
public:
	static WrapCircleShape* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapCircleShape* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapCircleShape>(object); }
	static b2CircleShape* Peek(v8::Local<v8::Value> value) { WrapCircleShape* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(float32 radius)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapCircleShape* wrap = Unwrap(instance);
		wrap->m_circle.m_radius = radius;
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->Inherit(WrapShape::GetFunctionTemplate());
			function_template->SetClassName(NANX_SYMBOL("b2CircleShape"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, m_p)
			NANX_METHOD_APPLY(prototype_template, ComputeMass)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			float32 radius = (info.Length() > 0) ? NANX_float32(info[0]) : 0.0f;
			WrapCircleShape* wrap = new WrapCircleShape(radius);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			v8::Local<v8::Value> argv[] = { info[0] };
			info.GetReturnValue().Set(constructor->NewInstance(countof(argv), argv));
		}
	}
	NANX_MEMBER_OBJECT(m_p) // m_wrap_m_p
	NANX_METHOD(ComputeMass)
	{
		b2CircleShape* circle = Peek(info.This());
		b2MassData* mass_data = WrapMassData::Peek(info[0]);
		float32 density = NANX_float32(info[1]);
		circle->ComputeMass(mass_data, density);
	}
};

//// b2EdgeShape

class WrapEdgeShape : public WrapShape
{
private:
	b2EdgeShape m_edge;
	Nan::Persistent<v8::Object> m_wrap_m_vertex1; // m_edge.m_vertex1
	Nan::Persistent<v8::Object> m_wrap_m_vertex2; // m_edge.m_vertex2
	Nan::Persistent<v8::Object> m_wrap_m_vertex0; // m_edge.m_vertex0
	Nan::Persistent<v8::Object> m_wrap_m_vertex3; // m_edge.m_vertex3
private:
	WrapEdgeShape()
	{
		m_wrap_m_vertex1.Reset(WrapVec2::NewInstance(m_edge.m_vertex1));
		m_wrap_m_vertex2.Reset(WrapVec2::NewInstance(m_edge.m_vertex2));
		m_wrap_m_vertex0.Reset(WrapVec2::NewInstance(m_edge.m_vertex0));
		m_wrap_m_vertex3.Reset(WrapVec2::NewInstance(m_edge.m_vertex3));
	}
	~WrapEdgeShape()
	{
		m_wrap_m_vertex1.Reset();
		m_wrap_m_vertex2.Reset();
		m_wrap_m_vertex0.Reset();
		m_wrap_m_vertex3.Reset();
	}
public:
	b2EdgeShape* Peek() { return &m_edge; }
	virtual b2Shape& GetShape() { return m_edge; } // override WrapShape
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		m_edge.m_vertex1 = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex1))->GetVec2(); // struct copy
		m_edge.m_vertex2 = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex2))->GetVec2(); // struct copy
		m_edge.m_vertex0 = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex0))->GetVec2(); // struct copy
		m_edge.m_vertex3 = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex3))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex1))->SetVec2(m_edge.m_vertex1);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex2))->SetVec2(m_edge.m_vertex2);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex0))->SetVec2(m_edge.m_vertex0);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_vertex3))->SetVec2(m_edge.m_vertex3);
	}
public:
	static WrapEdgeShape* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapEdgeShape* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapEdgeShape>(object); }
	static b2EdgeShape* Peek(v8::Local<v8::Value> value) { WrapEdgeShape* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2EdgeShape"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, m_vertex1)
			NANX_MEMBER_APPLY(prototype_template, m_vertex2)
			NANX_MEMBER_APPLY(prototype_template, m_vertex0)
			NANX_MEMBER_APPLY(prototype_template, m_vertex3)
			NANX_METHOD_APPLY(prototype_template, Set)
			NANX_METHOD_APPLY(prototype_template, ComputeMass)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapEdgeShape* wrap = new WrapEdgeShape();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(m_vertex1) // m_wrap_m_vertex1
	NANX_MEMBER_OBJECT(m_vertex2) // m_wrap_m_vertex2
	NANX_MEMBER_OBJECT(m_vertex0) // m_wrap_m_vertex0
	NANX_MEMBER_OBJECT(m_vertex3) // m_wrap_m_vertex3
	NANX_METHOD(Set)
	{
		WrapEdgeShape* wrap = Unwrap(info.This());
		wrap->m_wrap_m_vertex1.Reset(v8::Local<v8::Object>::Cast(info[0]));
		wrap->m_wrap_m_vertex2.Reset(v8::Local<v8::Object>::Cast(info[1]));
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(ComputeMass)
	{
		b2EdgeShape* edge = Peek(info.This());
		b2MassData* mass_data = WrapMassData::Peek(info[0]);
		float32 density = NANX_float32(info[1]);
		edge->ComputeMass(mass_data, density);
	}
};

//// b2PolygonShape

class WrapPolygonShape : public WrapShape
{
private:
	b2PolygonShape m_polygon;
private:
	WrapPolygonShape()
	{
	}
	~WrapPolygonShape()
	{
	}
public:
	b2PolygonShape* Peek() { return &m_polygon; }
	virtual b2Shape& GetShape() { return m_polygon; } // override WrapShape
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
	}
	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
	}
public:
	static WrapPolygonShape* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPolygonShape* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPolygonShape>(object); }
	static b2PolygonShape* Peek(v8::Local<v8::Value> value) { WrapPolygonShape* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2PolygonShape"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, Set)
			NANX_METHOD_APPLY(prototype_template, SetAsBox)
			NANX_METHOD_APPLY(prototype_template, ComputeMass)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapPolygonShape* wrap = new WrapPolygonShape();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(Set)
	{
		WrapPolygonShape* wrap = Unwrap(info.This());
		v8::Local<v8::Array> h_points = v8::Local<v8::Array>::Cast(info[0]);
		int count = (info.Length() > 1)?(NANX_int(info[1])):(h_points->Length());
		int start = (info.Length() > 2)?(NANX_int(info[2])):(0);
		b2Vec2* points = new b2Vec2[count];
		for (int i = 0; i < count; ++i)
		{
			points[i] = WrapVec2::Unwrap(h_points->Get(start + i).As<v8::Object>())->GetVec2(); // struct copy
		}
		wrap->m_polygon.Set(points, count);
		delete[] points;
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SetAsBox)
	{
		WrapPolygonShape* wrap = Unwrap(info.This());
		float32 hw = NANX_float32(info[0]);
		float32 hh = NANX_float32(info[1]);
		if (!info[2]->IsUndefined() && !info[3]->IsUndefined())
		{
			WrapVec2* center = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
			float32 angle = NANX_float32(info[3]);
			wrap->m_polygon.SetAsBox(hw, hh, center->GetVec2(), angle);
		}
		else
		{
			wrap->m_polygon.SetAsBox(hw, hh);
		}
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(ComputeMass)
	{
		b2PolygonShape* polygon = Peek(info.This());
		b2MassData* mass_data = WrapMassData::Peek(info[0]);
		float32 density = NANX_float32(info[1]);
		polygon->ComputeMass(mass_data, density);
	}
};

//// b2ChainShape

class WrapChainShape : public WrapShape
{
private:
	b2ChainShape m_chain;
	Nan::Persistent<v8::Array> m_wrap_m_vertices; // m_chain.m_vertices
	Nan::Persistent<v8::Object> m_wrap_m_prevVertex; // m_chain.m_prevVertex
	Nan::Persistent<v8::Object> m_wrap_m_nextVertex; // m_chain.m_nextVertex
private:
	WrapChainShape()
	{
		m_wrap_m_prevVertex.Reset(WrapVec2::NewInstance(m_chain.m_prevVertex));
		m_wrap_m_nextVertex.Reset(WrapVec2::NewInstance(m_chain.m_nextVertex));
	}
	~WrapChainShape()
	{
		m_wrap_m_vertices.Reset();
		m_wrap_m_prevVertex.Reset();
		m_wrap_m_nextVertex.Reset();
	}
public:
	b2ChainShape* Peek() { return &m_chain; }
	virtual b2Shape& GetShape() { return m_chain; } // override WrapShape
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		if (!m_wrap_m_vertices.IsEmpty())
		{
			v8::Local<v8::Array> vertices = Nan::New<v8::Array>(m_wrap_m_vertices);
			for (int32 i = 0; i < m_chain.m_count; ++i)
			{
				m_chain.m_vertices[i] = WrapVec2::Unwrap(vertices->Get(i).As<v8::Object>())->GetVec2(); // struct copy
			}
		}
		m_chain.m_prevVertex = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_prevVertex))->GetVec2(); // struct copy
		m_chain.m_nextVertex = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_nextVertex))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		if (m_wrap_m_vertices.IsEmpty())
		{
			m_wrap_m_vertices.Reset(Nan::New<v8::Array>(m_chain.m_count));
		}
		v8::Local<v8::Array> vertices = Nan::New<v8::Array>(m_wrap_m_vertices);
		for (int32 i = 0; i < m_chain.m_count; ++i)
		{
			vertices->Set(i, WrapVec2::NewInstance(m_chain.m_vertices[i]));
		}
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_prevVertex))->SetVec2(m_chain.m_prevVertex);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_m_nextVertex))->SetVec2(m_chain.m_nextVertex);
	}
public:
	static WrapChainShape* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapChainShape* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapChainShape>(object); }
	static b2ChainShape* Peek(v8::Local<v8::Value> value) { WrapChainShape* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2ChainShape"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, m_vertices)
			NANX_MEMBER_APPLY(prototype_template, m_count)
			NANX_MEMBER_APPLY(prototype_template, m_prevVertex)
			NANX_MEMBER_APPLY(prototype_template, m_nextVertex)
			NANX_MEMBER_APPLY(prototype_template, m_hasPrevVertex)
			NANX_MEMBER_APPLY(prototype_template, m_hasNextVertex)
			NANX_METHOD_APPLY(prototype_template, CreateLoop)
			NANX_METHOD_APPLY(prototype_template, CreateChain)
			NANX_METHOD_APPLY(prototype_template, SetPrevVertex)
			NANX_METHOD_APPLY(prototype_template, SetNextVertex)
			NANX_METHOD_APPLY(prototype_template, ComputeMass)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapChainShape* wrap = new WrapChainShape();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_ARRAY(m_vertices) // m_wrap_m_vertices
	NANX_MEMBER_INTEGER(int32, m_count)
	NANX_MEMBER_OBJECT(m_prevVertex) // m_wrap_m_prevVertex
	NANX_MEMBER_OBJECT(m_nextVertex) // m_wrap_m_nextVertex
	NANX_MEMBER_BOOLEAN(bool, m_hasPrevVertex)
	NANX_MEMBER_BOOLEAN(bool, m_hasNextVertex)
	NANX_METHOD(CreateLoop)
	{
		WrapChainShape* wrap = Unwrap(info.This());
		v8::Local<v8::Array> h_vertices = v8::Local<v8::Array>::Cast(info[0]);
		int count = (info.Length() > 1)?(NANX_int(info[1])):(h_vertices->Length());
		b2Vec2* vertices = new b2Vec2[count];
		for (int i = 0; i < count; ++i)
		{
			vertices[i] = WrapVec2::Unwrap(h_vertices->Get(i).As<v8::Object>())->GetVec2(); // struct copy
		}
		wrap->m_chain.CreateLoop(vertices, count);
		delete[] vertices;
		wrap->SyncPush();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(CreateChain)
	{
		WrapChainShape* wrap = Unwrap(info.This());
		v8::Local<v8::Array> h_vertices = v8::Local<v8::Array>::Cast(info[0]);
		int count = (info.Length() > 1)?(NANX_int(info[1])):(h_vertices->Length());
		b2Vec2* vertices = new b2Vec2[count];
		for (int i = 0; i < count; ++i)
		{
			vertices[i] = WrapVec2::Unwrap(h_vertices->Get(i).As<v8::Object>())->GetVec2(); // struct copy
		}
		wrap->m_chain.CreateChain(vertices, count);
		delete[] vertices;
		wrap->SyncPush();
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(SetPrevVertex)
	{
		b2ChainShape* chain = Peek(info.This());
		b2Vec2* vertex = WrapVec2::Peek(info[0]);
		chain->SetPrevVertex(*vertex);
	}
	NANX_METHOD(SetNextVertex)
	{
		b2ChainShape* chain = Peek(info.This());
		b2Vec2* vertex = WrapVec2::Peek(info[0]);
		chain->SetNextVertex(*vertex);
	}
	NANX_METHOD(ComputeMass)
	{
		b2ChainShape* chain = Peek(info.This());
		b2MassData* mass_data = WrapMassData::Peek(info[0]);
		float32 density = NANX_float32(info[1]);
		chain->ComputeMass(mass_data, density);
	}
};

//// b2Filter

class WrapFilter : public Nan::ObjectWrap
{
public:
	b2Filter m_filter;
private:
	WrapFilter() {}
	~WrapFilter() {}
public:
	b2Filter* Peek() { return &m_filter; }
	const b2Filter& GetFilter() const { return m_filter; }
	void SetFilter(const b2Filter& filter) { m_filter = filter; } // struct copy
public:
	static WrapFilter* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapFilter* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapFilter>(object); }
	static b2Filter* Peek(v8::Local<v8::Value> value) { WrapFilter* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Filter& o)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapFilter* wrap = Unwrap(instance);
		wrap->SetFilter(o);
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Filter"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, categoryBits)
			NANX_MEMBER_APPLY(prototype_template, maskBits)
			NANX_MEMBER_APPLY(prototype_template, groupIndex)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapFilter* wrap = new WrapFilter();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_MEMBER_UINT32(uint16, categoryBits)
	NANX_MEMBER_UINT32(uint16, maskBits)
	NANX_MEMBER_INT32(int16, groupIndex)
};

//// b2FixtureDef

class WrapFixtureDef : public Nan::ObjectWrap
{
private:
	b2FixtureDef m_fd;
	Nan::Persistent<v8::Object> m_wrap_shape; // m_fd.shape
	Nan::Persistent<v8::Value> m_wrap_userData; // m_fd.userData
	Nan::Persistent<v8::Object> m_wrap_filter; // m_fd.filter
private:
	WrapFixtureDef()
	{
		m_wrap_filter.Reset(WrapFilter::NewInstance());
	}
	~WrapFixtureDef()
	{
		m_wrap_shape.Reset();
		m_wrap_userData.Reset();
		m_wrap_filter.Reset();
	}
public:
	b2FixtureDef* Peek() { SyncPull(); return &m_fd; } // TODO: is SyncPull necessary?
	b2FixtureDef& GetFixtureDef() { return m_fd; }
	const b2FixtureDef& UseFixtureDef() { SyncPull(); return GetFixtureDef(); }
	v8::Local<v8::Object> GetShapeHandle() { return Nan::New<v8::Object>(m_wrap_shape); }
	v8::Local<v8::Value> GetUserDataHandle() { return Nan::New<v8::Value>(m_wrap_userData); }
public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_fd.filter = WrapFilter::Unwrap(Nan::New<v8::Object>(m_wrap_filter))->GetFilter(); // struct copy
		if (!m_wrap_shape.IsEmpty())
		{
			WrapShape* wrap_shape = WrapShape::Unwrap(Nan::New<v8::Object>(m_wrap_shape));
			m_fd.shape = &wrap_shape->UseShape();
		}
		else
		{
			m_fd.shape = NULL;
		}
		//m_fd.userData; // not used
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapFilter::Unwrap(Nan::New<v8::Object>(m_wrap_filter))->SetFilter(m_fd.filter);
		if (m_fd.shape)
		{
			// TODO: m_wrap_shape.Reset(WrapShape::GetWrap(m_fd.shape)->handle());
		}
		else
		{
			// TODO: m_wrap_shape.Reset();
		}
		//m_fd.userData; // not used
	}
public:
	static WrapFixtureDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapFixtureDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapFixtureDef>(object); }
	static b2FixtureDef* Peek(v8::Local<v8::Value> value) { WrapFixtureDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2FixtureDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, shape)
			NANX_MEMBER_APPLY(prototype_template, userData)
			NANX_MEMBER_APPLY(prototype_template, friction)
			NANX_MEMBER_APPLY(prototype_template, restitution)
			NANX_MEMBER_APPLY(prototype_template, density)
			NANX_MEMBER_APPLY(prototype_template, isSensor)
			NANX_MEMBER_APPLY(prototype_template, filter)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapFixtureDef* wrap = new WrapFixtureDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT	(shape) // m_wrap_shape
	NANX_MEMBER_VALUE	(userData) // m_wrap_userData
	NANX_MEMBER_NUMBER	(float32, friction)
	NANX_MEMBER_NUMBER	(float32, restitution)
	NANX_MEMBER_NUMBER	(float32, density)
	NANX_MEMBER_BOOLEAN	(bool, isSensor)
	NANX_MEMBER_OBJECT	(filter) // m_wrap_filter
};

//// b2Fixture

class WrapFixture : public Nan::ObjectWrap
{
private:
	b2Fixture* m_fixture;
	Nan::Persistent<v8::Object> m_fixture_body;
	Nan::Persistent<v8::Object> m_fixture_shape;
	Nan::Persistent<v8::Value> m_fixture_userData;
private:
	WrapFixture() : m_fixture(NULL)
	{
	}
	~WrapFixture()
	{
		m_fixture_body.Reset();
		m_fixture_shape.Reset();
		m_fixture_userData.Reset();
	}
public:
	b2Fixture* Peek() { return m_fixture; }
public:
	void SetupObject(v8::Local<v8::Object> h_body, WrapFixtureDef* wrap_fd, b2Fixture* fixture)
	{
		m_fixture = fixture;
		// set fixture internal data
		WrapFixture::SetWrap(m_fixture, this);
		// set reference to this fixture (prevent GC)
		Ref();
		// set reference to body object
		m_fixture_body.Reset(h_body);
		// set reference to shape object
		m_fixture_shape.Reset(wrap_fd->GetShapeHandle());
		// set reference to user data object
		m_fixture_userData.Reset(wrap_fd->GetUserDataHandle());
	}
	b2Fixture* ResetObject()
	{
		// clear reference to body object
		m_fixture_body.Reset();
		// clear reference to shape object
		m_fixture_shape.Reset();
		// clear reference to user data object
		m_fixture_userData.Reset();
		// clear reference to this fixture (allow GC)
		Unref();
		// clear fixture internal data
		WrapFixture::SetWrap(m_fixture, NULL);
		b2Fixture* fixture = m_fixture;
		m_fixture = NULL;
		return fixture;
	}
public:
	static WrapFixture* GetWrap(const b2Fixture* fixture)
	{
		return static_cast<WrapFixture*>(fixture->GetUserData());
	}
	static void SetWrap(b2Fixture* fixture, WrapFixture* wrap)
	{
		fixture->SetUserData(wrap);
	}
public:
	static WrapFixture* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapFixture* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapFixture>(object); }
	static b2Fixture* Peek(v8::Local<v8::Value> value) { WrapFixture* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Fixture"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetType)
			NANX_METHOD_APPLY(prototype_template, GetShape)
			NANX_METHOD_APPLY(prototype_template, SetSensor)
			NANX_METHOD_APPLY(prototype_template, IsSensor)
			NANX_METHOD_APPLY(prototype_template, SetFilterData)
			NANX_METHOD_APPLY(prototype_template, GetFilterData)
			NANX_METHOD_APPLY(prototype_template, Refilter)
			NANX_METHOD_APPLY(prototype_template, GetBody)
			NANX_METHOD_APPLY(prototype_template, GetNext)
			NANX_METHOD_APPLY(prototype_template, GetUserData)
			NANX_METHOD_APPLY(prototype_template, SetUserData)
			NANX_METHOD_APPLY(prototype_template, TestPoint)
			NANX_METHOD_APPLY(prototype_template, GetDensity)
			NANX_METHOD_APPLY(prototype_template, SetDensity)
			NANX_METHOD_APPLY(prototype_template, GetFriction)
			NANX_METHOD_APPLY(prototype_template, SetFriction)
			NANX_METHOD_APPLY(prototype_template, GetRestitution)
			NANX_METHOD_APPLY(prototype_template, SetRestitution)
			NANX_METHOD_APPLY(prototype_template, GetAABB)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapFixture* wrap = new WrapFixture();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(GetType)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->GetType()));
	}
	NANX_METHOD(GetShape)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture_shape));
	}
	NANX_METHOD(SetSensor)
	{
		WrapFixture* wrap = Unwrap(info.This());
		wrap->m_fixture->SetSensor(NANX_bool(info[0]));
	}
	NANX_METHOD(IsSensor)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->IsSensor()));
	}
	NANX_METHOD(SetFilterData)
	{
		WrapFixture* wrap = Unwrap(info.This());
		WrapFilter* wrap_filter = WrapFilter::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		wrap->m_fixture->SetFilterData(wrap_filter->GetFilter());
	}
	NANX_METHOD(GetFilterData)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(WrapFilter::NewInstance(wrap->m_fixture->GetFilterData()));
	}
	NANX_METHOD(Refilter)
	{
		WrapFixture* wrap = Unwrap(info.This());
		wrap->m_fixture->Refilter();
	}
	NANX_METHOD(GetBody)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture_body));
	}
	NANX_METHOD(GetNext)
	{
		WrapFixture* wrap = Unwrap(info.This());
		b2Fixture* fixture = wrap->m_fixture->GetNext();
		if (fixture)
		{
			// get fixture internal data
			WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
			info.GetReturnValue().Set(wrap_fixture->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetUserData)
	{
		WrapFixture* wrap = Unwrap(info.This());
		if (!wrap->m_fixture_userData.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Value>(wrap->m_fixture_userData));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(SetUserData)
	{
		WrapFixture* wrap = Unwrap(info.This());
		if (!info[0]->IsNull())
		{
			wrap->m_fixture_userData.Reset(info[0]);
		}
		else
		{
			wrap->m_fixture_userData.Reset();
		}
	}
	NANX_METHOD(TestPoint)
	{
		WrapFixture* wrap = Unwrap(info.This());
		WrapVec2* p = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->TestPoint(p->GetVec2())));
	}
	//bool RayCast(b2RayCastOutput* output, const b2RayCastInput& input, int32 childIndex) const;
	//void GetMassData(b2MassData* massData) const;
	NANX_METHOD(GetDensity)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->GetDensity()));
	}
	NANX_METHOD(SetDensity)
	{
		WrapFixture* wrap = Unwrap(info.This());
		wrap->m_fixture->SetDensity(NANX_float32(info[0]));
	}
	NANX_METHOD(GetFriction)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->GetFriction()));
	}
	NANX_METHOD(SetFriction)
	{
		WrapFixture* wrap = Unwrap(info.This());
		wrap->m_fixture->SetFriction(NANX_float32(info[0]));
	}
	NANX_METHOD(GetRestitution)
	{
		WrapFixture* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_fixture->GetRestitution()));
	}
	NANX_METHOD(SetRestitution)
	{
		WrapFixture* wrap = Unwrap(info.This());
		wrap->m_fixture->SetRestitution(NANX_float32(info[0]));
	}
	NANX_METHOD(GetAABB)
	{
		WrapFixture* wrap = Unwrap(info.This());
		int32 childIndex = NANX_int32(info[0]);
		info.GetReturnValue().Set(WrapAABB::NewInstance(wrap->m_fixture->GetAABB(childIndex)));
	}
	///	void Dump(int32 bodyIndex);
};

//// b2BodyDef

class WrapBodyDef : public Nan::ObjectWrap
{
private:
	b2BodyDef m_bd;
	Nan::Persistent<v8::Object> m_wrap_position; // m_bd.position
	Nan::Persistent<v8::Object> m_wrap_linearVelocity; // m_bd.linearVelocity
	Nan::Persistent<v8::Value> m_wrap_userData; // m_bd.userData
private:
	WrapBodyDef()
	{
		m_wrap_position.Reset(WrapVec2::NewInstance(m_bd.position));
		m_wrap_linearVelocity.Reset(WrapVec2::NewInstance(m_bd.linearVelocity));
	}
	~WrapBodyDef()
	{
		m_wrap_position.Reset();
		m_wrap_linearVelocity.Reset();
		m_wrap_userData.Reset();
	}
public:
	b2BodyDef* Peek() { SyncPull(); return &m_bd; } // TODO: is SyncPull necessary
	b2BodyDef& GetBodyDef() { return m_bd; }
	const b2BodyDef& UseBodyDef() { SyncPull(); return GetBodyDef(); }
	v8::Local<v8::Value> GetUserDataHandle() { return Nan::New<v8::Value>(m_wrap_userData); }
public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_bd.position = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->GetVec2(); // struct copy
		m_bd.linearVelocity = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearVelocity))->GetVec2(); // struct copy
		//m_bd.userData; // not used
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->SetVec2(m_bd.position);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearVelocity))->SetVec2(m_bd.linearVelocity);
		//m_bd.userData; // not used
	}
public:
	static WrapBodyDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapBodyDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapBodyDef>(object); }
	static b2BodyDef* Peek(v8::Local<v8::Value> value) { WrapBodyDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2BodyDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, type)
			NANX_MEMBER_APPLY(prototype_template, position)
			NANX_MEMBER_APPLY(prototype_template, angle)
			NANX_MEMBER_APPLY(prototype_template, linearVelocity)
			NANX_MEMBER_APPLY(prototype_template, angularVelocity)
			NANX_MEMBER_APPLY(prototype_template, linearDamping)
			NANX_MEMBER_APPLY(prototype_template, angularDamping)
			NANX_MEMBER_APPLY(prototype_template, allowSleep)
			NANX_MEMBER_APPLY(prototype_template, awake)
			NANX_MEMBER_APPLY(prototype_template, fixedRotation)
			NANX_MEMBER_APPLY(prototype_template, bullet)
			NANX_MEMBER_APPLY(prototype_template, active)
			NANX_MEMBER_APPLY(prototype_template, userData)
			NANX_MEMBER_APPLY(prototype_template, gravityScale)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapBodyDef* wrap = new WrapBodyDef();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_MEMBER_INTEGER	(b2BodyType, type)
	NANX_MEMBER_OBJECT	(position) // m_wrap_position
	NANX_MEMBER_NUMBER	(float32, angle)
	NANX_MEMBER_OBJECT	(linearVelocity) // m_wrap_linearVelocity
	NANX_MEMBER_NUMBER	(float32, angularVelocity)
	NANX_MEMBER_NUMBER	(float32, linearDamping)
	NANX_MEMBER_NUMBER	(float32, angularDamping)
	NANX_MEMBER_BOOLEAN	(bool, allowSleep)
	NANX_MEMBER_BOOLEAN	(bool, awake)
	NANX_MEMBER_BOOLEAN	(bool, fixedRotation)
	NANX_MEMBER_BOOLEAN	(bool, bullet)
	NANX_MEMBER_BOOLEAN	(bool, active)
	NANX_MEMBER_VALUE	(userData) // m_wrap_userData
	NANX_MEMBER_NUMBER	(float32, gravityScale)
};

//// b2Body

class WrapBody : public Nan::ObjectWrap
{
private:
	b2Body* m_body;
	Nan::Persistent<v8::Object> m_body_world;
	Nan::Persistent<v8::Value> m_body_userData;
private:
	WrapBody() : m_body(NULL) {}
	~WrapBody()
	{
		m_body_world.Reset();
		m_body_userData.Reset();
	}
public:
	b2Body* Peek() { return m_body; }
public:
	void SetupObject(v8::Local<v8::Object> h_world, WrapBodyDef* wrap_bd, b2Body* body)
	{
		m_body = body;
		// set body internal data
		WrapBody::SetWrap(m_body, this);
		// set reference to this body (prevent GC)
		Ref();
		// set reference to world object
		m_body_world.Reset(h_world);
		// set reference to user data object
		m_body_userData.Reset(wrap_bd->GetUserDataHandle());
	}
	b2Body* ResetObject()
	{
		// clear reference to world object
		m_body_world.Reset();
		// clear reference to user data object
		m_body_userData.Reset();
		// clear reference to this body (allow GC)
		Unref();
		// clear body internal data
		WrapBody::SetWrap(m_body, NULL);
		b2Body* body = m_body;
		m_body = NULL;
		return body;
	}
public:
	static WrapBody* GetWrap(const b2Body* body)
	{
		return static_cast<WrapBody*>(body->GetUserData());
	}
	static void SetWrap(b2Body* body, WrapBody* wrap)
	{
		body->SetUserData(wrap);
	}
public:
	static WrapBody* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapBody* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapBody>(object); }
	static b2Body* Peek(v8::Local<v8::Value> value) { WrapBody* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}

private:
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2Body"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, CreateFixture)
			NANX_METHOD_APPLY(prototype_template, DestroyFixture)
			NANX_METHOD_APPLY(prototype_template, SetTransform)
			NANX_METHOD_APPLY(prototype_template, GetTransform)
			NANX_METHOD_APPLY(prototype_template, GetPosition)
			NANX_METHOD_APPLY(prototype_template, GetAngle)
			NANX_METHOD_APPLY(prototype_template, GetWorldCenter)
			NANX_METHOD_APPLY(prototype_template, GetLocalCenter)
			NANX_METHOD_APPLY(prototype_template, SetLinearVelocity)
			NANX_METHOD_APPLY(prototype_template, GetLinearVelocity)
			NANX_METHOD_APPLY(prototype_template, SetAngularVelocity)
			NANX_METHOD_APPLY(prototype_template, GetAngularVelocity)
			NANX_METHOD_APPLY(prototype_template, ApplyForce)
			NANX_METHOD_APPLY(prototype_template, ApplyForceToCenter)
			NANX_METHOD_APPLY(prototype_template, ApplyTorque)
			NANX_METHOD_APPLY(prototype_template, ApplyLinearImpulse)
			NANX_METHOD_APPLY(prototype_template, ApplyLinearImpulseToCenter)
			NANX_METHOD_APPLY(prototype_template, ApplyAngularImpulse)
			NANX_METHOD_APPLY(prototype_template, GetMass)
			NANX_METHOD_APPLY(prototype_template, GetInertia)
			NANX_METHOD_APPLY(prototype_template, GetMassData)
			NANX_METHOD_APPLY(prototype_template, SetMassData)
			NANX_METHOD_APPLY(prototype_template, ResetMassData)
			NANX_METHOD_APPLY(prototype_template, GetWorldPoint)
			NANX_METHOD_APPLY(prototype_template, GetWorldVector)
			NANX_METHOD_APPLY(prototype_template, GetLocalPoint)
			NANX_METHOD_APPLY(prototype_template, GetLocalVector)
			NANX_METHOD_APPLY(prototype_template, GetLinearVelocityFromWorldPoint)
			NANX_METHOD_APPLY(prototype_template, GetLinearVelocityFromLocalPoint)
			NANX_METHOD_APPLY(prototype_template, GetLinearDamping)
			NANX_METHOD_APPLY(prototype_template, SetLinearDamping)
			NANX_METHOD_APPLY(prototype_template, GetAngularDamping)
			NANX_METHOD_APPLY(prototype_template, SetAngularDamping)
			NANX_METHOD_APPLY(prototype_template, GetGravityScale)
			NANX_METHOD_APPLY(prototype_template, SetGravityScale)
			NANX_METHOD_APPLY(prototype_template, SetType)
			NANX_METHOD_APPLY(prototype_template, GetType)
			NANX_METHOD_APPLY(prototype_template, SetBullet)
			NANX_METHOD_APPLY(prototype_template, IsBullet)
			NANX_METHOD_APPLY(prototype_template, SetSleepingAllowed)
			NANX_METHOD_APPLY(prototype_template, IsSleepingAllowed)
			NANX_METHOD_APPLY(prototype_template, SetAwake)
			NANX_METHOD_APPLY(prototype_template, IsAwake)
			NANX_METHOD_APPLY(prototype_template, SetActive)
			NANX_METHOD_APPLY(prototype_template, IsActive)
			NANX_METHOD_APPLY(prototype_template, SetFixedRotation)
			NANX_METHOD_APPLY(prototype_template, IsFixedRotation)
			NANX_METHOD_APPLY(prototype_template, GetFixtureList)
			NANX_METHOD_APPLY(prototype_template, GetNext)
			NANX_METHOD_APPLY(prototype_template, GetUserData)
			NANX_METHOD_APPLY(prototype_template, SetUserData)
			NANX_METHOD_APPLY(prototype_template, GetWorld)
			NANX_METHOD_APPLY(prototype_template, ShouldCollideConnected)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapBody* wrap = new WrapBody();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_METHOD(CreateFixture)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapFixtureDef* wrap_fd = WrapFixtureDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		// create box2d fixture
		b2Fixture* fixture = wrap->m_body->CreateFixture(&wrap_fd->UseFixtureDef());
		// create javascript fixture object
		v8::Local<v8::Object> h_fixture = WrapFixture::NewInstance();
		WrapFixture* wrap_fixture = WrapFixture::Unwrap(h_fixture);
		// set up javascript fixture object
		wrap_fixture->SetupObject(info.This(), wrap_fd, fixture);
		info.GetReturnValue().Set(h_fixture);
	}
	NANX_METHOD(DestroyFixture)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> h_fixture = v8::Local<v8::Object>::Cast(info[0]);
		WrapFixture* wrap_fixture = WrapFixture::Unwrap(h_fixture);
		// delete box2d fixture
		wrap->m_body->DestroyFixture(wrap_fixture->Peek());
		// reset javascript fixture object
		wrap_fixture->ResetObject();
	}
	NANX_METHOD(SetTransform)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* position = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		float32 angle = NANX_float32(info[1]);
		wrap->m_body->SetTransform(position->GetVec2(), angle);
	}
	NANX_METHOD(GetTransform)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapTransform::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapTransform::Unwrap(out)->SetTransform(wrap->m_body->GetTransform());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetPosition)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetPosition());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetAngle) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetAngle())); }
	NANX_METHOD(GetWorldCenter)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetWorldCenter());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalCenter)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLocalCenter());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetLinearVelocity)
	{
		WrapBody* wrap = Unwrap(info.This());
		wrap->m_body->SetLinearVelocity(WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]))->GetVec2());
	}
	NANX_METHOD(GetLinearVelocity)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLinearVelocity());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetAngularVelocity) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetAngularVelocity(NANX_float32(info[0])); }
	NANX_METHOD(GetAngularVelocity) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetAngularVelocity())); }
	NANX_METHOD(ApplyForce)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* force = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapVec2* point = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		bool wake = (info.Length() > 2) ? NANX_bool(info[2]) : true;
		wrap->m_body->ApplyForce(force->GetVec2(), point->GetVec2(), wake);
	}
	NANX_METHOD(ApplyForceToCenter)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* force = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		bool wake = (info.Length() > 1) ? NANX_bool(info[1]) : true;
		wrap->m_body->ApplyForceToCenter(force->GetVec2(), wake);
	}
	NANX_METHOD(ApplyTorque)
	{
		WrapBody* wrap = Unwrap(info.This());
		float32 torque = NANX_float32(info[0]);
		bool wake = (info.Length() > 1) ? NANX_bool(info[1]) : true;
		wrap->m_body->ApplyTorque(torque, wake);
	}
	NANX_METHOD(ApplyLinearImpulse)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* impulse = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapVec2* point = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		bool wake = (info.Length() > 2) ? NANX_bool(info[2]) : true;
		wrap->m_body->ApplyLinearImpulse(impulse->GetVec2(), point->GetVec2(), wake);
	}
	NANX_METHOD(ApplyLinearImpulseToCenter)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* impulse = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		bool wake = (info.Length() > 1) ? NANX_bool(info[1]) : true;
		wrap->m_body->ApplyLinearImpulse(impulse->GetVec2(), wrap->m_body->GetWorldCenter(), wake);
	}
	NANX_METHOD(ApplyAngularImpulse)
	{
		WrapBody* wrap = Unwrap(info.This());
		float32 impulse = NANX_float32(info[0]);
		bool wake = (info.Length() > 1) ? NANX_bool(info[1]) : true;
		wrap->m_body->ApplyAngularImpulse(impulse, wake);
	}
	NANX_METHOD(GetMass) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetMass())); }
	NANX_METHOD(GetInertia) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetInertia())); }
	NANX_METHOD(GetMassData)
	{
		WrapBody* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapMassData::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		b2MassData mass_data;
		wrap->m_body->GetMassData(&mass_data);
		WrapMassData::Unwrap(out)->SetMassData(mass_data);
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetMassData)
	{
		WrapBody* wrap = Unwrap(info.This());
		const b2MassData& mass_data = WrapMassData::Unwrap(v8::Local<v8::Object>::Cast(info[0]))->GetMassData();
		wrap->m_body->SetMassData(&mass_data);
	}
	NANX_METHOD(ResetMassData) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->ResetMassData(); }
	NANX_METHOD(GetWorldPoint)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_localPoint = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetWorldPoint(wrap_localPoint->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetWorldVector)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_localVector = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetWorldVector(wrap_localVector->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalPoint)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_worldPoint = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLocalPoint(wrap_worldPoint->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalVector)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_worldVector = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLocalVector(wrap_worldVector->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLinearVelocityFromWorldPoint)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_worldPoint = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLinearVelocityFromWorldPoint(wrap_worldPoint->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLinearVelocityFromLocalPoint)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapVec2* wrap_localPoint = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_body->GetLinearVelocityFromLocalPoint(wrap_localPoint->GetVec2()));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLinearDamping) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetLinearDamping())); }
	NANX_METHOD(SetLinearDamping) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetLinearDamping(NANX_float32(info[0])); }
	NANX_METHOD(GetAngularDamping) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetAngularDamping())); }
	NANX_METHOD(SetAngularDamping) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetAngularDamping(NANX_float32(info[0])); }
	NANX_METHOD(GetGravityScale) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetGravityScale())); }
	NANX_METHOD(SetGravityScale) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetGravityScale(NANX_float32(info[0])); }
	NANX_METHOD(SetType) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetType(NANX_b2BodyType(info[0])); }
	NANX_METHOD(GetType) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->GetType())); }
	NANX_METHOD(SetBullet) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetBullet(NANX_bool(info[0])); }
	NANX_METHOD(IsBullet) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->IsBullet())); }
	NANX_METHOD(SetSleepingAllowed) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetSleepingAllowed(NANX_bool(info[0])); }
	NANX_METHOD(IsSleepingAllowed) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->IsSleepingAllowed())); }
	NANX_METHOD(SetAwake) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetAwake(NANX_bool(info[0])); }
	NANX_METHOD(IsAwake) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->IsAwake())); }
	NANX_METHOD(SetActive) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetActive(NANX_bool(info[0])); }
	NANX_METHOD(IsActive) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->IsActive())); }
	NANX_METHOD(SetFixedRotation) { WrapBody* wrap = Unwrap(info.This()); wrap->m_body->SetFixedRotation(NANX_bool(info[0])); }
	NANX_METHOD(IsFixedRotation) { WrapBody* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_body->IsFixedRotation())); }
	NANX_METHOD(GetFixtureList)
	{
		WrapBody* wrap = Unwrap(info.This());
		b2Fixture* fixture = wrap->m_body->GetFixtureList();
		if (fixture)
		{
			// get fixture internal data
			WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
			info.GetReturnValue().Set(wrap_fixture->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	///b2JointEdge* GetJointList();
	///const b2JointEdge* GetJointList() const;
	///b2ContactEdge* GetContactList();
	///const b2ContactEdge* GetContactList() const;
	NANX_METHOD(GetNext)
	{
		WrapBody* wrap = Unwrap(info.This());
		b2Body* body = wrap->m_body->GetNext();
		if (body)
		{
			// get body internal data
			WrapBody* wrap_body = WrapBody::GetWrap(body);
			info.GetReturnValue().Set(wrap_body->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetUserData)
	{
		WrapBody* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New<v8::Value>(wrap->m_body_userData));
	}
	NANX_METHOD(SetUserData)
	{
		WrapBody* wrap = Unwrap(info.This());
		if (!info[0]->IsUndefined())
		{
			wrap->m_body_userData.Reset(info[0]);
		}
		else
		{
			wrap->m_body_userData.Reset();
		}
	}
	NANX_METHOD(GetWorld)
	{
		WrapBody* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New<v8::Object>(wrap->m_body_world));
	}
	NANX_METHOD(ShouldCollideConnected)
	{
		WrapBody* wrap = Unwrap(info.This());
		WrapBody* wrap_other = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		info.GetReturnValue().Set(Nan::New(wrap->m_body->ShouldCollideConnected(wrap_other->m_body)));
	}
	///void Dump();
};

//// b2JointDef

class WrapJointDef : public Nan::ObjectWrap
{
protected:
	Nan::Persistent<v8::Value> m_wrap_userData; // m_jd.userData
	Nan::Persistent<v8::Object> m_wrap_bodyA; // m_jd.bodyA
	Nan::Persistent<v8::Object> m_wrap_bodyB; // m_jd.bodyB
public:
	WrapJointDef() {}
	~WrapJointDef()
	{
		m_wrap_userData.Reset();
		m_wrap_bodyA.Reset();
		m_wrap_bodyB.Reset();
	}
public:
	b2JointDef* Peek() { return &GetJointDef(); }
	virtual b2JointDef& GetJointDef() = 0;
	const b2JointDef& UseJointDef() { SyncPull(); return GetJointDef(); }
	v8::Local<v8::Value> GetUserDataHandle() { return Nan::New<v8::Value>(m_wrap_userData); }
	v8::Local<v8::Object> GetBodyAHandle() { return Nan::New<v8::Object>(m_wrap_bodyA); }
	v8::Local<v8::Object> GetBodyBHandle() { return Nan::New<v8::Object>(m_wrap_bodyB); }
public:
	virtual void SyncPull()
	{
		b2JointDef& jd = GetJointDef();
		// sync: pull data from javascript objects
		//jd.userData; // not used
		jd.bodyA = m_wrap_bodyA.IsEmpty() ? NULL : WrapBody::Unwrap(Nan::New<v8::Object>(m_wrap_bodyA))->Peek();
		jd.bodyB = m_wrap_bodyB.IsEmpty() ? NULL : WrapBody::Unwrap(Nan::New<v8::Object>(m_wrap_bodyB))->Peek();
	}
	virtual void SyncPush()
	{
		b2JointDef& jd = GetJointDef();
		// sync: push data into javascript objects
		//jd.userData; // not used
		if (jd.bodyA)
		{
			m_wrap_bodyA.Reset(WrapBody::GetWrap(jd.bodyA)->handle());
		}
		else
		{
			m_wrap_bodyA.Reset();
		}
		if (jd.bodyB)
		{
			m_wrap_bodyB.Reset(WrapBody::GetWrap(jd.bodyB)->handle());
		}
		else
		{
			m_wrap_bodyB.Reset();
		}
	}
public:
	static WrapJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapJointDef>(object); }
	static b2JointDef* Peek(v8::Local<v8::Value> value) { WrapJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>();
			function_template->SetClassName(NANX_SYMBOL("b2JointDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, type)
			NANX_MEMBER_APPLY(prototype_template, userData)
			NANX_MEMBER_APPLY(prototype_template, bodyA)
			NANX_MEMBER_APPLY(prototype_template, bodyB)
			NANX_MEMBER_APPLY(prototype_template, collideConnected)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	NANX_MEMBER_INTEGER	(b2JointType, type)
	NANX_MEMBER_VALUE	(userData) // m_wrap_userData
	NANX_MEMBER_OBJECT	(bodyA) // m_wrap_bodyA
	NANX_MEMBER_OBJECT	(bodyB) // m_wrap_bodyB
	NANX_MEMBER_BOOLEAN	(bool, collideConnected)
};

//// b2Joint

class WrapJoint : public Nan::ObjectWrap
{
private:
	Nan::Persistent<v8::Object> m_joint_world;
	Nan::Persistent<v8::Object> m_joint_bodyA;
	Nan::Persistent<v8::Object> m_joint_bodyB;
	Nan::Persistent<v8::Value> m_joint_userData;
public:
	WrapJoint() {}
	~WrapJoint()
	{
		m_joint_bodyA.Reset();
		m_joint_bodyB.Reset();
		m_joint_userData.Reset();
	}
public:
	b2Joint* Peek() { return GetJoint(); }
	virtual b2Joint* GetJoint() = 0;
public:
	void SetupObject(v8::Local<v8::Object> h_world, WrapJointDef* wrap_jd)
	{
		b2Joint* joint = GetJoint();
		// set joint internal data
		WrapJoint::SetWrap(joint, this);
		// set reference to this joint (prevent GC)
		Ref();
		// set reference to joint world object
		m_joint_world.Reset(h_world);
		// set references to joint body objects
		m_joint_bodyA.Reset(wrap_jd->GetBodyAHandle());
		m_joint_bodyB.Reset(wrap_jd->GetBodyBHandle());
		// set reference to joint user data object
		m_joint_userData.Reset(wrap_jd->GetUserDataHandle());
	}
	void ResetObject()
	{
		b2Joint* joint = GetJoint();
		// clear reference to joint world object
		m_joint_world.Reset();
		// clear references to joint body objects
		m_joint_bodyA.Reset();
		m_joint_bodyB.Reset();
		// clear reference to joint user data object
		m_joint_userData.Reset();
		// clear reference to this joint (allow GC)
		Unref();
		// clear joint internal data
		WrapJoint::SetWrap(joint, NULL);
	}
public:
	static WrapJoint* GetWrap(const b2Joint* joint)
	{
		return static_cast<WrapJoint*>(joint->GetUserData());
	}
	static void SetWrap(b2Joint* joint, WrapJoint* wrap)
	{
		joint->SetUserData(wrap);
	}
public:
	static WrapJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapJoint>(object); }
	static b2Joint* Peek(v8::Local<v8::Value> value) { WrapJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>();
			function_template->SetClassName(NANX_SYMBOL("b2Joint"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetType)
			NANX_METHOD_APPLY(prototype_template, GetBodyA)
			NANX_METHOD_APPLY(prototype_template, GetBodyB)
			NANX_METHOD_APPLY(prototype_template, GetAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetReactionForce)
			NANX_METHOD_APPLY(prototype_template, GetReactionTorque)
			NANX_METHOD_APPLY(prototype_template, GetNext)
			NANX_METHOD_APPLY(prototype_template, GetUserData)
			NANX_METHOD_APPLY(prototype_template, SetUserData)
			NANX_METHOD_APPLY(prototype_template, IsActive)
			NANX_METHOD_APPLY(prototype_template, GetCollideConnected)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	NANX_METHOD(GetType)
	{
		WrapJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->GetJoint()->GetType()));
	}
	NANX_METHOD(GetBodyA)
	{
		WrapJoint* wrap = Unwrap(info.This());
		if (!wrap->m_joint_bodyA.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Object>(wrap->m_joint_bodyA));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetBodyB)
	{
		WrapJoint* wrap = Unwrap(info.This());
		if (!wrap->m_joint_bodyB.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Object>(wrap->m_joint_bodyB));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetAnchorA)
	{
		WrapJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->GetJoint()->GetAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetAnchorB)
	{
		WrapJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->GetJoint()->GetAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetReactionForce)
	{
		WrapJoint* wrap = Unwrap(info.This());
		float32 inv_dt = NANX_float32(info[0]);
		v8::Local<v8::Object> out = info[1]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[1]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->GetJoint()->GetReactionForce(inv_dt));
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetReactionTorque)
	{
		WrapJoint* wrap = Unwrap(info.This());
		float32 inv_dt = NANX_float32(info[0]);
		info.GetReturnValue().Set(Nan::New(wrap->GetJoint()->GetReactionTorque(inv_dt)));
	}
	NANX_METHOD(GetNext)
	{
		WrapJoint* wrap = Unwrap(info.This());
		b2Joint* joint = wrap->GetJoint()->GetNext();
		if (joint)
		{
			// get joint internal data
			WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
			info.GetReturnValue().Set(wrap_joint->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetUserData)
	{
		WrapJoint* wrap = Unwrap(info.This());
		if (!wrap->m_joint_userData.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Value>(wrap->m_joint_userData));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(SetUserData)
	{
		WrapJoint* wrap = Unwrap(info.This());
		if (!info[0]->IsUndefined())
		{
			wrap->m_joint_userData.Reset(info[0]);
		}
		else
		{
			wrap->m_joint_userData.Reset();
		}
	}
	NANX_METHOD(IsActive)
	{
		WrapJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->GetJoint()->IsActive()));
	}
	NANX_METHOD(GetCollideConnected)
	{
		WrapJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->GetJoint()->GetCollideConnected()));
	}
	///virtual void Dump() { b2Log("// Dump is not supported for this joint type.\n"); }
	//virtual void ShiftOrigin(const b2Vec2& newOrigin) { B2_NOT_USED(newOrigin);  }
};

//// b2RevoluteJointDef

class WrapRevoluteJointDef : public WrapJointDef
{
private:
	b2RevoluteJointDef m_revolute_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_revolute_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_revolute_jd.localAnchorB
private:
	WrapRevoluteJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_revolute_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_revolute_jd.localAnchorB));
	}
	~WrapRevoluteJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2RevoluteJointDef* Peek() { return &m_revolute_jd; }
	virtual b2JointDef& GetJointDef() { return m_revolute_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_revolute_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_revolute_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_revolute_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_revolute_jd.localAnchorB);
	}
public:
	static WrapRevoluteJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRevoluteJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRevoluteJointDef>(object); }
	static b2RevoluteJointDef* Peek(v8::Local<v8::Value> value) { WrapRevoluteJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->SetClassName(NANX_SYMBOL("b2RevoluteJointDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, referenceAngle)
			NANX_MEMBER_APPLY(prototype_template, enableLimit)
			NANX_MEMBER_APPLY(prototype_template, lowerAngle)
			NANX_MEMBER_APPLY(prototype_template, upperAngle)
			NANX_MEMBER_APPLY(prototype_template, enableMotor)
			NANX_MEMBER_APPLY(prototype_template, motorSpeed)
			NANX_MEMBER_APPLY(prototype_template, maxMotorTorque)
			NANX_METHOD_APPLY(prototype_template, Initialize)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapRevoluteJointDef* wrap = new WrapRevoluteJointDef();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_MEMBER_OBJECT	(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT	(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER	(float32, referenceAngle)
	NANX_MEMBER_BOOLEAN	(bool, enableLimit)
	NANX_MEMBER_NUMBER	(float32, lowerAngle)
	NANX_MEMBER_NUMBER	(float32, upperAngle)
	NANX_MEMBER_BOOLEAN	(bool, enableMotor)
	NANX_MEMBER_NUMBER	(float32, motorSpeed)
	NANX_MEMBER_NUMBER	(float32, maxMotorTorque)
	NANX_METHOD(Initialize)
	{
		WrapRevoluteJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchor = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		wrap->SyncPull();
		wrap->m_revolute_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchor->GetVec2());
		wrap->SyncPush();
	}
};

//// b2RevoluteJoint

class WrapRevoluteJoint : public WrapJoint
{
private:
	b2RevoluteJoint* m_revolute_joint;
private:
	WrapRevoluteJoint() : m_revolute_joint(NULL) {}
	~WrapRevoluteJoint() {}
public:
	b2RevoluteJoint* Peek() { return m_revolute_joint; }
	virtual b2Joint* GetJoint() { return m_revolute_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapRevoluteJointDef* wrap_revolute_jd, b2RevoluteJoint* revolute_joint)
	{
		m_revolute_joint = revolute_joint;
		WrapJoint::SetupObject(h_world, wrap_revolute_jd);
	}
	b2RevoluteJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2RevoluteJoint* revolute_joint = m_revolute_joint;
		m_revolute_joint = NULL;
		return revolute_joint;
	}
public:
	static WrapRevoluteJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRevoluteJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRevoluteJoint>(object); }
	static b2RevoluteJoint* Peek(v8::Local<v8::Value> value) { WrapRevoluteJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->SetClassName(NANX_SYMBOL("b2RevoluteJoint"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetReferenceAngle)
			NANX_METHOD_APPLY(prototype_template, GetJointAngle)
			NANX_METHOD_APPLY(prototype_template, GetJointSpeed)
			NANX_METHOD_APPLY(prototype_template, IsLimitEnabled)
			NANX_METHOD_APPLY(prototype_template, EnableLimit)
			NANX_METHOD_APPLY(prototype_template, GetLowerLimit)
			NANX_METHOD_APPLY(prototype_template, GetUpperLimit)
			NANX_METHOD_APPLY(prototype_template, SetLimits)
			NANX_METHOD_APPLY(prototype_template, IsMotorEnabled)
			NANX_METHOD_APPLY(prototype_template, EnableMotor)
			NANX_METHOD_APPLY(prototype_template, SetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, GetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, SetMaxMotorTorque)
			NANX_METHOD_APPLY(prototype_template, GetMaxMotorTorque)
			NANX_METHOD_APPLY(prototype_template, GetMotorTorque)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		WrapRevoluteJoint* wrap = new WrapRevoluteJoint();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	///	b2Vec2 GetAnchorA() const;
	///	b2Vec2 GetAnchorB() const;
	///	b2Vec2 GetReactionForce(float32 inv_dt) const;
	///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_revolute_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_revolute_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetReferenceAngle)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetReferenceAngle()));
	}
	NANX_METHOD(GetJointAngle)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetJointAngle()));
	}
	NANX_METHOD(GetJointSpeed)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetJointSpeed()));
	}
	NANX_METHOD(IsLimitEnabled)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->IsLimitEnabled()));
	}
	NANX_METHOD(EnableLimit)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		wrap->m_revolute_joint->EnableLimit(NANX_bool(info[0]));
	}
	NANX_METHOD(GetLowerLimit)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetLowerLimit()));
	}
	NANX_METHOD(GetUpperLimit)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetUpperLimit()));
	}
	NANX_METHOD(SetLimits)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		wrap->m_revolute_joint->SetLimits(NANX_float32(info[0]), NANX_float32(info[1]));
	}
	NANX_METHOD(IsMotorEnabled)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->IsMotorEnabled()));
	}
	NANX_METHOD(EnableMotor)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		wrap->m_revolute_joint->EnableMotor(NANX_bool(info[0]));
	}
	NANX_METHOD(SetMotorSpeed)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		wrap->m_revolute_joint->SetMotorSpeed(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMotorSpeed)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetMotorSpeed()));
	}
	NANX_METHOD(SetMaxMotorTorque)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		wrap->m_revolute_joint->SetMaxMotorTorque(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxMotorTorque)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetMaxMotorTorque()));
	}
	NANX_METHOD(GetMotorTorque)
	{
		WrapRevoluteJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_revolute_joint->GetMotorTorque(NANX_float32(info[0]))));
	}
	///	void Dump();
};

//// b2PrismaticJointDef

class WrapPrismaticJointDef : public WrapJointDef
{
private:
	b2PrismaticJointDef m_prismatic_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_prismatic_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_prismatic_jd.localAnchorB
	Nan::Persistent<v8::Object> m_wrap_localAxisA; // m_prismatic_jd.localAxisA
private:
	WrapPrismaticJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_prismatic_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_prismatic_jd.localAnchorB));
		m_wrap_localAxisA.Reset(WrapVec2::NewInstance(m_prismatic_jd.localAxisA));
	}
	~WrapPrismaticJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
		m_wrap_localAxisA.Reset();
	}
public:
	b2PrismaticJointDef* Peek() { return &m_prismatic_jd; }
	virtual b2JointDef& GetJointDef() { return m_prismatic_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_prismatic_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_prismatic_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
		m_prismatic_jd.localAxisA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAxisA))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_prismatic_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_prismatic_jd.localAnchorB);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAxisA))->SetVec2(m_prismatic_jd.localAxisA);
	}
public:
	static WrapPrismaticJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPrismaticJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPrismaticJointDef>(object); }
	static b2PrismaticJointDef* Peek(v8::Local<v8::Value> value) { WrapPrismaticJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->SetClassName(NANX_SYMBOL("b2PrismaticJointDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, localAxisA)
			NANX_MEMBER_APPLY(prototype_template, referenceAngle)
			NANX_MEMBER_APPLY(prototype_template, enableLimit)
			NANX_MEMBER_APPLY(prototype_template, lowerTranslation)
			NANX_MEMBER_APPLY(prototype_template, upperTranslation)
			NANX_MEMBER_APPLY(prototype_template, enableMotor)
			NANX_MEMBER_APPLY(prototype_template, maxMotorForce)
			NANX_MEMBER_APPLY(prototype_template, motorSpeed)
			NANX_METHOD_APPLY(prototype_template, Initialize)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapPrismaticJointDef* wrap = new WrapPrismaticJointDef();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_MEMBER_OBJECT	(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT	(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_OBJECT	(localAxisA) // m_wrap_localAxisA
	NANX_MEMBER_NUMBER	(float32, referenceAngle)
	NANX_MEMBER_BOOLEAN	(bool, enableLimit)
	NANX_MEMBER_NUMBER	(float32, lowerTranslation)
	NANX_MEMBER_NUMBER	(float32, upperTranslation)
	NANX_MEMBER_BOOLEAN	(bool, enableMotor)
	NANX_MEMBER_NUMBER	(float32, maxMotorForce)
	NANX_MEMBER_NUMBER	(float32, motorSpeed)
	NANX_METHOD(Initialize)
	{
		WrapPrismaticJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchor = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapVec2* wrap_axis = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[3]));
		wrap->SyncPull();
		wrap->m_prismatic_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchor->GetVec2(), wrap_axis->GetVec2());
		wrap->SyncPush();
	}
};

//// b2PrismaticJoint

class WrapPrismaticJoint : public WrapJoint
{
private:
	b2PrismaticJoint* m_prismatic_joint;
private:
	WrapPrismaticJoint() : m_prismatic_joint(NULL) {}
	~WrapPrismaticJoint() {}
public:
	b2PrismaticJoint* Peek() { return m_prismatic_joint; }
	virtual b2Joint* GetJoint() { return m_prismatic_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapPrismaticJointDef* wrap_prismatic_jd, b2PrismaticJoint* prismatic_joint)
	{
		m_prismatic_joint = prismatic_joint;
		WrapJoint::SetupObject(h_world, wrap_prismatic_jd);
	}
	b2PrismaticJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2PrismaticJoint* prismatic_joint = m_prismatic_joint;
		m_prismatic_joint = NULL;
		return prismatic_joint;
	}
public:
	static WrapPrismaticJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPrismaticJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPrismaticJoint>(object); }
	static b2PrismaticJoint* Peek(v8::Local<v8::Value> value) { WrapPrismaticJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->SetClassName(NANX_SYMBOL("b2PrismaticJoint"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetLocalAxisA)
			NANX_METHOD_APPLY(prototype_template, GetReferenceAngle)
			NANX_METHOD_APPLY(prototype_template, GetJointTranslation)
			NANX_METHOD_APPLY(prototype_template, GetJointSpeed)
			NANX_METHOD_APPLY(prototype_template, IsLimitEnabled)
			NANX_METHOD_APPLY(prototype_template, EnableLimit)
			NANX_METHOD_APPLY(prototype_template, GetLowerLimit)
			NANX_METHOD_APPLY(prototype_template, GetUpperLimit)
			NANX_METHOD_APPLY(prototype_template, SetLimits)
			NANX_METHOD_APPLY(prototype_template, IsMotorEnabled)
			NANX_METHOD_APPLY(prototype_template, EnableMotor)
			NANX_METHOD_APPLY(prototype_template, SetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, GetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, SetMaxMotorForce)
			NANX_METHOD_APPLY(prototype_template, GetMaxMotorForce)
			NANX_METHOD_APPLY(prototype_template, GetMotorForce)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		WrapPrismaticJoint* wrap = new WrapPrismaticJoint();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	///b2Vec2 GetAnchorA() const;
	///b2Vec2 GetAnchorB() const;
	///b2Vec2 GetReactionForce(float32 inv_dt) const;
	///float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_prismatic_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_prismatic_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAxisA)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_prismatic_joint->GetLocalAxisA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetReferenceAngle)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetReferenceAngle()));
	}
	NANX_METHOD(GetJointTranslation)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetJointTranslation()));
	}
	NANX_METHOD(GetJointSpeed)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetJointSpeed()));
	}
	NANX_METHOD(IsLimitEnabled)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->IsLimitEnabled()));
	}
	NANX_METHOD(EnableLimit)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		wrap->m_prismatic_joint->EnableLimit(NANX_bool(info[0]));
	}
	NANX_METHOD(GetLowerLimit)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetLowerLimit()));
	}
	NANX_METHOD(GetUpperLimit)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetUpperLimit()));
	}
	NANX_METHOD(SetLimits)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		wrap->m_prismatic_joint->SetLimits(NANX_float32(info[0]), NANX_float32(info[1]));
	}
	NANX_METHOD(IsMotorEnabled)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->IsMotorEnabled()));
	}
	NANX_METHOD(EnableMotor)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		wrap->m_prismatic_joint->EnableMotor(NANX_bool(info[0]));
	}
	NANX_METHOD(SetMotorSpeed)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		wrap->m_prismatic_joint->SetMotorSpeed(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMotorSpeed)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetMotorSpeed()));
	}
	NANX_METHOD(SetMaxMotorForce)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		wrap->m_prismatic_joint->SetMaxMotorForce(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxMotorForce)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetMaxMotorForce()));
	}
	NANX_METHOD(GetMotorForce)
	{
		WrapPrismaticJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_prismatic_joint->GetMotorForce(NANX_float32(info[0]))));
	}
	///void Dump();
};

//// b2DistanceJointDef

class WrapDistanceJointDef : public WrapJointDef
{
private:
	b2DistanceJointDef m_distance_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_distance_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_distance_jd.localAnchorB
private:
	WrapDistanceJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_distance_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_distance_jd.localAnchorB));
	}
	~WrapDistanceJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2DistanceJointDef* Peek() { return &m_distance_jd; }
	virtual b2JointDef& GetJointDef() { return m_distance_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_distance_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_distance_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_distance_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_distance_jd.localAnchorB);
	}
public:
	static WrapDistanceJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapDistanceJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapDistanceJointDef>(object); }
	static b2DistanceJointDef* Peek(v8::Local<v8::Value> value) { WrapDistanceJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->SetClassName(NANX_SYMBOL("b2DistanceJointDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, length)
			NANX_MEMBER_APPLY(prototype_template, frequencyHz)
			NANX_MEMBER_APPLY(prototype_template, dampingRatio)
			NANX_METHOD_APPLY(prototype_template, Initialize)
			g_function_template.Reset(function_template);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
private:
	static NAN_METHOD(New)
	{
		WrapDistanceJointDef* wrap = new WrapDistanceJointDef();
		wrap->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER(float32, length)
	NANX_MEMBER_NUMBER(float32, frequencyHz)
	NANX_MEMBER_NUMBER(float32, dampingRatio)
	NANX_METHOD(Initialize)
	{
		WrapDistanceJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchorA = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapVec2* wrap_anchorB = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[3]));
		wrap->SyncPull();
		wrap->m_distance_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchorA->GetVec2(), wrap_anchorB->GetVec2());
		wrap->SyncPush();
	}
};

//// b2DistanceJoint

class WrapDistanceJoint : public WrapJoint
{
private:
	b2DistanceJoint* m_distance_joint;
private:
	WrapDistanceJoint() : m_distance_joint(NULL) {}
	~WrapDistanceJoint() {}
public:
	b2DistanceJoint* Peek() { return m_distance_joint; }
	virtual b2Joint* GetJoint() { return m_distance_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapDistanceJointDef* wrap_distance_jd, b2DistanceJoint* distance_joint)
	{
		m_distance_joint = distance_joint;
		WrapJoint::SetupObject(h_world, wrap_distance_jd);
	}
	b2DistanceJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2DistanceJoint* distance_joint = m_distance_joint;
		m_distance_joint = NULL;
		return distance_joint;
	}
public:
	static WrapDistanceJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapDistanceJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapDistanceJoint>(object); }
	static b2DistanceJoint* Peek(v8::Local<v8::Value> value) { WrapDistanceJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2DistanceJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, SetLength)
			NANX_METHOD_APPLY(prototype_template, GetLength)
			NANX_METHOD_APPLY(prototype_template, SetFrequency)
			NANX_METHOD_APPLY(prototype_template, GetFrequency)
			NANX_METHOD_APPLY(prototype_template, SetDampingRatio)
			NANX_METHOD_APPLY(prototype_template, GetDampingRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapDistanceJoint* wrap = new WrapDistanceJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_distance_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_distance_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetLength)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		wrap->m_distance_joint->SetLength(NANX_float32(info[0]));
	}
	NANX_METHOD(GetLength)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_distance_joint->GetLength()));
	}
	NANX_METHOD(SetFrequency)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		wrap->m_distance_joint->SetFrequency(NANX_float32(info[0]));
	}
	NANX_METHOD(GetFrequency)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_distance_joint->GetFrequency()));
	}
	NANX_METHOD(SetDampingRatio)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		wrap->m_distance_joint->SetDampingRatio(NANX_float32(info[0]));
	}
	NANX_METHOD(GetDampingRatio)
	{
		WrapDistanceJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_distance_joint->GetDampingRatio()));
	}
///	void Dump();
};

//// b2PulleyJointDef

class WrapPulleyJointDef : public WrapJointDef
{
private:
	b2PulleyJointDef m_pulley_jd;
	Nan::Persistent<v8::Object> m_wrap_groundAnchorA; // m_pulley_jd.groundAnchorA
	Nan::Persistent<v8::Object> m_wrap_groundAnchorB; // m_pulley_jd.groundAnchorB
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_pulley_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_pulley_jd.localAnchorB
private:
	WrapPulleyJointDef()
	{
		m_wrap_groundAnchorA.Reset(WrapVec2::NewInstance(m_pulley_jd.groundAnchorA));
		m_wrap_groundAnchorB.Reset(WrapVec2::NewInstance(m_pulley_jd.groundAnchorB));
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_pulley_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_pulley_jd.localAnchorB));
	}
	~WrapPulleyJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2PulleyJointDef* Peek() { return &m_pulley_jd; }
	virtual b2JointDef& GetJointDef() { return m_pulley_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_pulley_jd.groundAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_groundAnchorA))->GetVec2(); // struct copy
		m_pulley_jd.groundAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_groundAnchorB))->GetVec2(); // struct copy
		m_pulley_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_pulley_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_groundAnchorA))->SetVec2(m_pulley_jd.groundAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_groundAnchorB))->SetVec2(m_pulley_jd.groundAnchorB);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_pulley_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_pulley_jd.localAnchorB);
	}
public:
	static WrapPulleyJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPulleyJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPulleyJointDef>(object); }
	static b2PulleyJointDef* Peek(v8::Local<v8::Value> value) { WrapPulleyJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2PulleyJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, groundAnchorA)
			NANX_MEMBER_APPLY(prototype_template, groundAnchorB)
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, lengthA)
			NANX_MEMBER_APPLY(prototype_template, lengthB)
			NANX_MEMBER_APPLY(prototype_template, ratio)
			NANX_METHOD_APPLY(prototype_template, Initialize)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapPulleyJointDef* wrap = new WrapPulleyJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(groundAnchorA) // m_wrap_groundAnchorA
	NANX_MEMBER_OBJECT(groundAnchorB) // m_wrap_groundAnchorB
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER(float32, lengthA)
	NANX_MEMBER_NUMBER(float32, lengthB)
	NANX_MEMBER_NUMBER(float32, ratio)
	NANX_METHOD(Initialize)
	{
		WrapPulleyJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_groundAnchorA = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapVec2* wrap_groundAnchorB = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[3]));
		WrapVec2* wrap_anchorA = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[4]));
		WrapVec2* wrap_anchorB = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[5]));
		float32 ratio = NANX_float32(info[6]);
		wrap->SyncPull();
		wrap->m_pulley_jd.Initialize(
		   wrap_bodyA->Peek(), wrap_bodyB->Peek(),
		   wrap_groundAnchorA->GetVec2(), wrap_groundAnchorB->GetVec2(),
		   wrap_anchorA->GetVec2(), wrap_anchorB->GetVec2(),
		   ratio);
		wrap->SyncPush();
	}
};

//// b2PulleyJoint

class WrapPulleyJoint : public WrapJoint
{
private:
	b2PulleyJoint* m_pulley_joint;
private:
	WrapPulleyJoint() : m_pulley_joint(NULL) {}
	~WrapPulleyJoint() {}
public:
	b2PulleyJoint* Peek() { return m_pulley_joint; }
	virtual b2Joint* GetJoint() { return m_pulley_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapPulleyJointDef* wrap_pulley_jd, b2PulleyJoint* pulley_joint)
	{
		m_pulley_joint = pulley_joint;
		WrapJoint::SetupObject(h_world, wrap_pulley_jd);
	}
	b2PulleyJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2PulleyJoint* pulley_joint = m_pulley_joint;
		m_pulley_joint = NULL;
		return pulley_joint;
	}
public:
	static WrapPulleyJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapPulleyJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapPulleyJoint>(object); }
	static b2PulleyJoint* Peek(v8::Local<v8::Value> value) { WrapPulleyJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2PulleyJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetGroundAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetGroundAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetLengthA)
			NANX_METHOD_APPLY(prototype_template, GetLengthB)
			NANX_METHOD_APPLY(prototype_template, GetRatio)
			NANX_METHOD_APPLY(prototype_template, GetCurrentLengthA)
			NANX_METHOD_APPLY(prototype_template, GetCurrentLengthB)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapPulleyJoint* wrap = new WrapPulleyJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetGroundAnchorA)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_pulley_joint->GetGroundAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetGroundAnchorB)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_pulley_joint->GetGroundAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLengthA)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_pulley_joint->GetLengthA()));
	}
	NANX_METHOD(GetLengthB)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_pulley_joint->GetLengthB()));
	}
	NANX_METHOD(GetRatio)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_pulley_joint->GetRatio()));
	}
	NANX_METHOD(GetCurrentLengthA)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_pulley_joint->GetCurrentLengthA()));
	}
	NANX_METHOD(GetCurrentLengthB)
	{
		WrapPulleyJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_pulley_joint->GetCurrentLengthB()));
	}
///	void Dump();
};

//// b2MouseJointDef

class WrapMouseJointDef : public WrapJointDef
{
private:
	b2MouseJointDef m_mouse_jd;
	Nan::Persistent<v8::Object> m_wrap_target; // m_mouse_jd.target
private:
	WrapMouseJointDef()
	{
		m_wrap_target.Reset(WrapVec2::NewInstance(m_mouse_jd.target));
	}
	~WrapMouseJointDef()
	{
		m_wrap_target.Reset();
	}
public:
	b2MouseJointDef* Peek() { return &m_mouse_jd; }
	virtual b2JointDef& GetJointDef() { return m_mouse_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_mouse_jd.target = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_target))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_target))->SetVec2(m_mouse_jd.target);
	}
public:
	static WrapMouseJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMouseJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMouseJointDef>(object); }
	static b2MouseJointDef* Peek(v8::Local<v8::Value> value) { WrapMouseJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2MouseJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, target)
			NANX_MEMBER_APPLY(prototype_template, maxForce)
			NANX_MEMBER_APPLY(prototype_template, frequencyHz)
			NANX_MEMBER_APPLY(prototype_template, dampingRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapMouseJointDef* wrap = new WrapMouseJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(target) // m_wrap_target
	NANX_MEMBER_NUMBER(float32, maxForce)
	NANX_MEMBER_NUMBER(float32, frequencyHz)
	NANX_MEMBER_NUMBER(float32, dampingRatio)
};

//// b2MouseJoint

class WrapMouseJoint : public WrapJoint
{
private:
	b2MouseJoint* m_mouse_joint;
private:
	WrapMouseJoint() : m_mouse_joint(NULL) {}
	~WrapMouseJoint() {}
public:
	b2MouseJoint* Peek() { return m_mouse_joint; }
	virtual b2Joint* GetJoint() { return m_mouse_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapMouseJointDef* wrap_mouse_jd, b2MouseJoint* mouse_joint)
	{
		m_mouse_joint = mouse_joint;
		WrapJoint::SetupObject(h_world, wrap_mouse_jd);
	}
	b2MouseJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2MouseJoint* mouse_joint = m_mouse_joint;
		m_mouse_joint = NULL;
		return mouse_joint;
	}
public:
	static WrapMouseJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMouseJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMouseJoint>(object); }
	static b2MouseJoint* Peek(v8::Local<v8::Value> value) { WrapMouseJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2MouseJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, SetTarget)
			NANX_METHOD_APPLY(prototype_template, GetTarget)
			NANX_METHOD_APPLY(prototype_template, SetMaxForce)
			NANX_METHOD_APPLY(prototype_template, GetMaxForce)
			NANX_METHOD_APPLY(prototype_template, SetFrequency)
			NANX_METHOD_APPLY(prototype_template, GetFrequency)
			NANX_METHOD_APPLY(prototype_template, SetDampingRatio)
			NANX_METHOD_APPLY(prototype_template, GetDampingRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapMouseJoint* wrap = new WrapMouseJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(SetTarget)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		WrapVec2* wrap_target = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		wrap->m_mouse_joint->SetTarget(wrap_target->GetVec2());
	}
	NANX_METHOD(GetTarget)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_mouse_joint->GetTarget());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetMaxForce)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		wrap->m_mouse_joint->SetMaxForce(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxForce)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_mouse_joint->GetMaxForce()));
	}
	NANX_METHOD(SetFrequency)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		wrap->m_mouse_joint->SetFrequency(NANX_float32(info[0]));
	}
	NANX_METHOD(GetFrequency)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_mouse_joint->GetFrequency()));
	}
	NANX_METHOD(SetDampingRatio)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		wrap->m_mouse_joint->SetDampingRatio(NANX_float32(info[0]));
	}
	NANX_METHOD(GetDampingRatio)
	{
		WrapMouseJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_mouse_joint->GetDampingRatio()));
	}
///	void Dump();
};

//// b2GearJointDef

class WrapGearJointDef : public WrapJointDef
{
private:
	b2GearJointDef m_gear_jd;
	Nan::Persistent<v8::Object> m_wrap_joint1; // m_gear_jd.joint1
	Nan::Persistent<v8::Object> m_wrap_joint2; // m_gear_jd.joint2
private:
	WrapGearJointDef() {}
	~WrapGearJointDef()
	{
		m_wrap_joint1.Reset();
		m_wrap_joint2.Reset();
	}
public:
	b2GearJointDef* Peek() { return & m_gear_jd; }
	virtual b2JointDef& GetJointDef() { return m_gear_jd; } // override WrapJointDef
	v8::Local<v8::Object> GetJoint1Handle() { return Nan::New<v8::Object>(m_wrap_joint1); }
	v8::Local<v8::Object> GetJoint2Handle() { return Nan::New<v8::Object>(m_wrap_joint2); }
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_gear_jd.joint1 = WrapJoint::Unwrap(Nan::New<v8::Object>(m_wrap_joint1))->GetJoint();
		m_gear_jd.joint2 = WrapJoint::Unwrap(Nan::New<v8::Object>(m_wrap_joint2))->GetJoint();
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		m_wrap_joint1.Reset(WrapJoint::GetWrap(m_gear_jd.joint1)->handle());
		m_wrap_joint2.Reset(WrapJoint::GetWrap(m_gear_jd.joint2)->handle());
	}
public:
	static WrapGearJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapGearJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapGearJointDef>(object); }
	static b2GearJointDef* Peek(v8::Local<v8::Value> value) { WrapGearJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2GearJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, joint1)
			NANX_MEMBER_APPLY(prototype_template, joint2)
			NANX_MEMBER_APPLY(prototype_template, ratio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapGearJointDef* wrap = new WrapGearJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(joint1) // m_wrap_joint1
	NANX_MEMBER_OBJECT(joint2) // m_wrap_joint2
	NANX_MEMBER_NUMBER(float32, ratio)
};

//// b2GearJoint

class WrapGearJoint : public WrapJoint
{
private:
	b2GearJoint* m_gear_joint;
	Nan::Persistent<v8::Object> m_gear_joint_joint1;
	Nan::Persistent<v8::Object> m_gear_joint_joint2;
private:
	WrapGearJoint() : m_gear_joint(NULL) {}
	~WrapGearJoint()
	{
		m_gear_joint_joint1.Reset();
		m_gear_joint_joint2.Reset();
	}
public:
	b2GearJoint* Peek() { return m_gear_joint; }
	virtual b2Joint* GetJoint() { return m_gear_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapGearJointDef* wrap_gear_jd, b2GearJoint* gear_joint)
	{
		m_gear_joint = gear_joint;
		m_gear_joint_joint1.Reset(WrapJoint::GetWrap(gear_joint->GetJoint1())->handle());
		m_gear_joint_joint2.Reset(WrapJoint::GetWrap(gear_joint->GetJoint2())->handle());
		WrapJoint::SetupObject(h_world, wrap_gear_jd);
	}
	b2GearJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		m_gear_joint_joint1.Reset();
		m_gear_joint_joint2.Reset();
		b2GearJoint* gear_joint = m_gear_joint;
		m_gear_joint = NULL;
		return gear_joint;
	}
public:
	static WrapGearJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapGearJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapGearJoint>(object); }
	static b2GearJoint* Peek(v8::Local<v8::Value> value) { WrapGearJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2GearJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetJoint1)
			NANX_METHOD_APPLY(prototype_template, GetJoint2)
			NANX_METHOD_APPLY(prototype_template, SetRatio)
			NANX_METHOD_APPLY(prototype_template, GetRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapGearJoint* wrap = new WrapGearJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetJoint1)
	{
		WrapGearJoint* wrap = Unwrap(info.This());
		if (!wrap->m_gear_joint_joint1.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Object>(wrap->m_gear_joint_joint1));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetJoint2)
	{
		WrapGearJoint* wrap = Unwrap(info.This());
		if (!wrap->m_gear_joint_joint2.IsEmpty())
		{
			info.GetReturnValue().Set(Nan::New<v8::Object>(wrap->m_gear_joint_joint2));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(SetRatio)
	{
		WrapGearJoint* wrap = Unwrap(info.This());
		wrap->m_gear_joint->SetRatio(NANX_float32(info[0]));
	}
	NANX_METHOD(GetRatio)
	{
		WrapGearJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_gear_joint->GetRatio()));
	}
///	void Dump();
};

//// b2WheelJointDef

class WrapWheelJointDef : public WrapJointDef
{
private:
	b2WheelJointDef m_wheel_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_wheel_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_wheel_jd.localAnchorB
	Nan::Persistent<v8::Object> m_wrap_localAxisA; // m_wheel_jd.localAxisA
private:
	WrapWheelJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_wheel_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_wheel_jd.localAnchorB));
		m_wrap_localAxisA.Reset(WrapVec2::NewInstance(m_wheel_jd.localAxisA));
	}
	~WrapWheelJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
		m_wrap_localAxisA.Reset();
	}
public:
	b2WheelJointDef* Peek() { return &m_wheel_jd; }
	virtual b2JointDef& GetJointDef() { return m_wheel_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_wheel_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_wheel_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
		m_wheel_jd.localAxisA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAxisA))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_wheel_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_wheel_jd.localAnchorB);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAxisA))->SetVec2(m_wheel_jd.localAxisA);
	}
public:
	static WrapWheelJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWheelJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWheelJointDef>(object); }
	static b2WheelJointDef* Peek(v8::Local<v8::Value> value) { WrapWheelJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2WheelJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, localAxisA)
			NANX_MEMBER_APPLY(prototype_template, enableMotor)
			NANX_MEMBER_APPLY(prototype_template, maxMotorTorque)
			NANX_MEMBER_APPLY(prototype_template, motorSpeed)
			NANX_MEMBER_APPLY(prototype_template, frequencyHz)
			NANX_MEMBER_APPLY(prototype_template, dampingRatio)
			NANX_METHOD_APPLY(prototype_template, Initialize)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapWheelJointDef* wrap = new WrapWheelJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_OBJECT(localAxisA) // m_wrap_localAxisA
	NANX_MEMBER_BOOLEAN(bool, enableMotor)
	NANX_MEMBER_NUMBER(float32, maxMotorTorque)
	NANX_MEMBER_NUMBER(float32, motorSpeed)
	NANX_MEMBER_NUMBER(float32, frequencyHz)
	NANX_MEMBER_NUMBER(float32, dampingRatio)
	NANX_METHOD(Initialize)
	{
		WrapWheelJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchor = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapVec2* wrap_axis = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[3]));
		wrap->SyncPull();
		wrap->m_wheel_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchor->GetVec2(), wrap_axis->GetVec2());
		wrap->SyncPush();
	}
};

//// b2WheelJoint

class WrapWheelJoint : public WrapJoint
{
private:
	b2WheelJoint* m_wheel_joint;
private:
	WrapWheelJoint() : m_wheel_joint(NULL) {}
	~WrapWheelJoint() {}
public:
	b2WheelJoint* Peek() { return m_wheel_joint; }
	virtual b2Joint* GetJoint() { return m_wheel_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapWheelJointDef* wrap_wheel_jd, b2WheelJoint* wheel_joint)
	{
		m_wheel_joint = wheel_joint;
		WrapJoint::SetupObject(h_world, wrap_wheel_jd);
	}
	b2WheelJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2WheelJoint* wheel_joint = m_wheel_joint;
		m_wheel_joint = NULL;
		return wheel_joint;
	}
public:
	static WrapWheelJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWheelJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWheelJoint>(object); }
	static b2WheelJoint* Peek(v8::Local<v8::Value> value) { WrapWheelJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2WheelJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetLocalAxisA)
			NANX_METHOD_APPLY(prototype_template, GetJointTranslation)
			NANX_METHOD_APPLY(prototype_template, GetJointSpeed)
			NANX_METHOD_APPLY(prototype_template, IsMotorEnabled)
			NANX_METHOD_APPLY(prototype_template, EnableMotor)
			NANX_METHOD_APPLY(prototype_template, SetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, GetMotorSpeed)
			NANX_METHOD_APPLY(prototype_template, SetMaxMotorTorque)
			NANX_METHOD_APPLY(prototype_template, GetMaxMotorTorque)
			NANX_METHOD_APPLY(prototype_template, GetMotorTorque)
			NANX_METHOD_APPLY(prototype_template, SetSpringFrequencyHz)
			NANX_METHOD_APPLY(prototype_template, GetSpringFrequencyHz)
			NANX_METHOD_APPLY(prototype_template, SetSpringDampingRatio)
			NANX_METHOD_APPLY(prototype_template, GetSpringDampingRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapWheelJoint* wrap = new WrapWheelJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_wheel_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_wheel_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAxisA)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_wheel_joint->GetLocalAxisA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetJointTranslation)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetJointTranslation()));
	}
	NANX_METHOD(GetJointSpeed)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetJointSpeed()));
	}
	NANX_METHOD(IsMotorEnabled)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->IsMotorEnabled()));
	}
	NANX_METHOD(EnableMotor)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		wrap->m_wheel_joint->EnableMotor(NANX_bool(info[0]));
	}
	NANX_METHOD(SetMotorSpeed)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		wrap->m_wheel_joint->SetMotorSpeed(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMotorSpeed)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetMotorSpeed()));
	}
	NANX_METHOD(SetMaxMotorTorque)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		wrap->m_wheel_joint->SetMaxMotorTorque(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxMotorTorque)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetMaxMotorTorque()));
	}
	NANX_METHOD(GetMotorTorque)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetMotorTorque(NANX_float32(info[0]))));
	}
	NANX_METHOD(SetSpringFrequencyHz)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		wrap->m_wheel_joint->SetSpringFrequencyHz(NANX_float32(info[0]));
	}
	NANX_METHOD(GetSpringFrequencyHz)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetSpringFrequencyHz()));
	}
	NANX_METHOD(SetSpringDampingRatio)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		wrap->m_wheel_joint->SetSpringDampingRatio(NANX_float32(info[0]));
	}
	NANX_METHOD(GetSpringDampingRatio)
	{
		WrapWheelJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_wheel_joint->GetSpringDampingRatio()));
	}
///	void Dump();
};

//// b2WeldJointDef

class WrapWeldJointDef : public WrapJointDef
{
private:
	b2WeldJointDef m_weld_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_weld_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_weld_jd.localAnchorB
private:
	WrapWeldJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_weld_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_weld_jd.localAnchorB));
	}
	~WrapWeldJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2WeldJointDef* Peek() { return &m_weld_jd; }
	virtual b2JointDef& GetJointDef() { return m_weld_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_weld_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_weld_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_weld_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_weld_jd.localAnchorB);
	}
public:
	static WrapWeldJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWeldJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWeldJointDef>(object); }
	static b2WeldJointDef* Peek(v8::Local<v8::Value> value) { WrapWeldJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2WeldJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, referenceAngle)
			NANX_MEMBER_APPLY(prototype_template, frequencyHz)
			NANX_MEMBER_APPLY(prototype_template, dampingRatio)
			NANX_METHOD_APPLY(prototype_template, Initialize)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapWeldJointDef* wrap = new WrapWeldJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER(float32, referenceAngle)
	NANX_MEMBER_NUMBER(float32, frequencyHz)
	NANX_MEMBER_NUMBER(float32, dampingRatio)
	NANX_METHOD(Initialize)
	{
		WrapWeldJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchor = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		wrap->SyncPull();
		wrap->m_weld_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchor->GetVec2());
		wrap->SyncPush();
	}
};

//// b2WeldJoint

class WrapWeldJoint : public WrapJoint
{
private:
	b2WeldJoint* m_weld_joint;
private:
	WrapWeldJoint() : m_weld_joint(NULL) {}
	~WrapWeldJoint() {}
public:
	b2WeldJoint* Peek() { return m_weld_joint; }
	virtual b2Joint* GetJoint() { return m_weld_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapWeldJointDef* wrap_weld_jd, b2WeldJoint* weld_joint)
	{
		m_weld_joint = weld_joint;
		WrapJoint::SetupObject(h_world, wrap_weld_jd);
	}
	b2WeldJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2WeldJoint* weld_joint = m_weld_joint;
		m_weld_joint = NULL;
		return weld_joint;
	}
public:
	static WrapWeldJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWeldJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWeldJoint>(object); }
	static b2WeldJoint* Peek(v8::Local<v8::Value> value) { WrapWeldJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2WeldJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, GetReferenceAngle)
			NANX_METHOD_APPLY(prototype_template, SetFrequency)
			NANX_METHOD_APPLY(prototype_template, GetFrequency)
			NANX_METHOD_APPLY(prototype_template, SetDampingRatio)
			NANX_METHOD_APPLY(prototype_template, GetDampingRatio)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapWeldJoint* wrap = new WrapWeldJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_weld_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_weld_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetReferenceAngle)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_weld_joint->GetReferenceAngle()));
	}
	NANX_METHOD(SetFrequency)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		wrap->m_weld_joint->SetFrequency(NANX_float32(info[0]));
	}
	NANX_METHOD(GetFrequency)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_weld_joint->GetFrequency()));
	}
	NANX_METHOD(SetDampingRatio)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		wrap->m_weld_joint->SetDampingRatio(NANX_float32(info[0]));
	}
	NANX_METHOD(GetDampingRatio)
	{
		WrapWeldJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_weld_joint->GetDampingRatio()));
	}
///	void Dump();
};

//// b2FrictionJointDef

class WrapFrictionJointDef : public WrapJointDef
{
private:
	b2FrictionJointDef m_friction_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_friction_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_friction_jd.localAnchorB
private:
	WrapFrictionJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_friction_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_friction_jd.localAnchorB));
	}
	~WrapFrictionJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2FrictionJointDef* Peek() { return &m_friction_jd; }
	virtual b2JointDef& GetJointDef() { return m_friction_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_friction_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_friction_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_friction_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_friction_jd.localAnchorB);
	}
public:
	static WrapFrictionJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapFrictionJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapFrictionJointDef>(object); }
	static b2FrictionJointDef* Peek(v8::Local<v8::Value> value) { WrapFrictionJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2FrictionJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, maxForce)
			NANX_MEMBER_APPLY(prototype_template, maxTorque)
			NANX_METHOD_APPLY(prototype_template, Initialize)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapFrictionJointDef* wrap = new WrapFrictionJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER(float32, maxForce)
	NANX_MEMBER_NUMBER(float32, maxTorque)
	NANX_METHOD(Initialize)
	{
		WrapFrictionJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* wrap_anchor = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		wrap->SyncPull();
		wrap->m_friction_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek(), wrap_anchor->GetVec2());
		wrap->SyncPush();
	}
};

//// b2FrictionJoint

class WrapFrictionJoint : public WrapJoint
{
private:
	b2FrictionJoint* m_friction_joint;
private:
	WrapFrictionJoint() : m_friction_joint(NULL) {}
	~WrapFrictionJoint() {}
public:
	b2FrictionJoint* Peek() { return m_friction_joint; }
	virtual b2Joint* GetJoint() { return m_friction_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapFrictionJointDef* wrap_friction_jd, b2FrictionJoint* friction_joint)
	{
		m_friction_joint = friction_joint;
		WrapJoint::SetupObject(h_world, wrap_friction_jd);
	}
	b2FrictionJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2FrictionJoint* friction_joint = m_friction_joint;
		m_friction_joint = NULL;
		return friction_joint;
	}
public:
	static WrapFrictionJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapFrictionJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapFrictionJoint>(object); }
	static b2FrictionJoint* Peek(v8::Local<v8::Value> value) { WrapFrictionJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2FrictionJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, SetMaxForce)
			NANX_METHOD_APPLY(prototype_template, GetMaxForce)
			NANX_METHOD_APPLY(prototype_template, SetMaxTorque)
			NANX_METHOD_APPLY(prototype_template, GetMaxTorque)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapFrictionJoint* wrap = new WrapFrictionJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_friction_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_friction_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetMaxForce)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		wrap->m_friction_joint->SetMaxForce(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxForce)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_friction_joint->GetMaxForce()));
	}
	NANX_METHOD(SetMaxTorque)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		wrap->m_friction_joint->SetMaxTorque(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxTorque)
	{
		WrapFrictionJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_friction_joint->GetMaxTorque()));
	}
///	void Dump();
};

//// b2RopeJointDef

class WrapRopeJointDef : public WrapJointDef
{
private:
	b2RopeJointDef m_rope_jd;
	Nan::Persistent<v8::Object> m_wrap_localAnchorA; // m_rope_jd.localAnchorA
	Nan::Persistent<v8::Object> m_wrap_localAnchorB; // m_rope_jd.localAnchorB
private:
	WrapRopeJointDef()
	{
		m_wrap_localAnchorA.Reset(WrapVec2::NewInstance(m_rope_jd.localAnchorA));
		m_wrap_localAnchorB.Reset(WrapVec2::NewInstance(m_rope_jd.localAnchorB));
	}
	~WrapRopeJointDef()
	{
		m_wrap_localAnchorA.Reset();
		m_wrap_localAnchorB.Reset();
	}
public:
	b2RopeJointDef* Peek() { return &m_rope_jd; }
	virtual b2JointDef& GetJointDef() { return m_rope_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_rope_jd.localAnchorA = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->GetVec2(); // struct copy
		m_rope_jd.localAnchorB = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorA))->SetVec2(m_rope_jd.localAnchorA);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localAnchorB))->SetVec2(m_rope_jd.localAnchorB);
	}
public:
	static WrapRopeJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRopeJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRopeJointDef>(object); }
	static b2RopeJointDef* Peek(v8::Local<v8::Value> value) { WrapRopeJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2RopeJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localAnchorA)
			NANX_MEMBER_APPLY(prototype_template, localAnchorB)
			NANX_MEMBER_APPLY(prototype_template, maxLength)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapRopeJointDef* wrap = new WrapRopeJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(localAnchorA) // m_wrap_localAnchorA
	NANX_MEMBER_OBJECT(localAnchorB) // m_wrap_localAnchorB
	NANX_MEMBER_NUMBER(float32, maxLength)
};

//// b2RopeJoint

class WrapRopeJoint : public WrapJoint
{
private:
	b2RopeJoint* m_rope_joint;
private:
	WrapRopeJoint() : m_rope_joint(NULL) {}
	~WrapRopeJoint() {}
public:
	b2RopeJoint* Peek() { return m_rope_joint; }
	virtual b2Joint* GetJoint() { return m_rope_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapRopeJointDef* wrap_rope_jd, b2RopeJoint* rope_joint)
	{
		m_rope_joint = rope_joint;
		WrapJoint::SetupObject(h_world, wrap_rope_jd);
	}
	b2RopeJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2RopeJoint* rope_joint = m_rope_joint;
		m_rope_joint = NULL;
		return rope_joint;
	}
public:
	static WrapRopeJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapRopeJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapRopeJoint>(object); }
	static b2RopeJoint* Peek(v8::Local<v8::Value> value) { WrapRopeJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2RopeJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorA)
			NANX_METHOD_APPLY(prototype_template, GetLocalAnchorB)
			NANX_METHOD_APPLY(prototype_template, SetMaxLength)
			NANX_METHOD_APPLY(prototype_template, GetMaxLength)
			NANX_METHOD_APPLY(prototype_template, GetLimitState)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapRopeJoint* wrap = new WrapRopeJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(GetLocalAnchorA)
	{
		WrapRopeJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_rope_joint->GetLocalAnchorA());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(GetLocalAnchorB)
	{
		WrapRopeJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_rope_joint->GetLocalAnchorB());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetMaxLength)
	{
		WrapRopeJoint* wrap = Unwrap(info.This());
		wrap->m_rope_joint->SetMaxLength(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxLength)
	{
		WrapRopeJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_rope_joint->GetMaxLength()));
	}
	NANX_METHOD(GetLimitState)
	{
		WrapRopeJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_rope_joint->GetLimitState()));
	}
///	void Dump();
};

//// b2MotorJointDef

class WrapMotorJointDef : public WrapJointDef
{
private:
	b2MotorJointDef m_motor_jd;
	Nan::Persistent<v8::Object> m_wrap_linearOffset; // m_motor_jd.linearOffset
private:
	WrapMotorJointDef()
	{
		m_wrap_linearOffset.Reset(WrapVec2::NewInstance(m_motor_jd.linearOffset));
	}
	~WrapMotorJointDef()
	{
		m_wrap_linearOffset.Reset();
	}
public:
	b2MotorJointDef* Peek() { return &m_motor_jd; }
	virtual b2JointDef& GetJointDef() { return m_motor_jd; } // override WrapJointDef
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_motor_jd.linearOffset = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearOffset))->GetVec2(); // struct copy
	}
	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearOffset))->SetVec2(m_motor_jd.linearOffset);
	}
public:
	static WrapMotorJointDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMotorJointDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMotorJointDef>(object); }
	static b2MotorJointDef* Peek(v8::Local<v8::Value> value) { WrapMotorJointDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2MotorJointDef"));
			function_template->Inherit(WrapJointDef::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, linearOffset)
			NANX_MEMBER_APPLY(prototype_template, angularOffset)
			NANX_MEMBER_APPLY(prototype_template, maxForce)
			NANX_MEMBER_APPLY(prototype_template, maxTorque)
			NANX_MEMBER_APPLY(prototype_template, correctionFactor)
			NANX_METHOD_APPLY(prototype_template, Initialize)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapMotorJointDef* wrap = new WrapMotorJointDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(linearOffset) // m_wrap_linearOffset
	NANX_MEMBER_NUMBER(float32, angularOffset)
	NANX_MEMBER_NUMBER(float32, maxForce)
	NANX_MEMBER_NUMBER(float32, maxTorque)
	NANX_MEMBER_NUMBER(float32, correctionFactor)
	NANX_METHOD(Initialize)
	{
		WrapMotorJointDef* wrap = Unwrap(info.This());
		WrapBody* wrap_bodyA = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		WrapBody* wrap_bodyB = WrapBody::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		wrap->SyncPull();
		wrap->m_motor_jd.Initialize(wrap_bodyA->Peek(), wrap_bodyB->Peek());
		wrap->SyncPush();
	}
};

//// b2MotorJoint

class WrapMotorJoint : public WrapJoint
{
private:
	b2MotorJoint* m_motor_joint;
private:
	WrapMotorJoint() : m_motor_joint(NULL) {}
	~WrapMotorJoint() {}
public:
	b2MotorJoint* Peek() { return m_motor_joint; }
	virtual b2Joint* GetJoint() { return m_motor_joint; } // override WrapJoint
	void SetupObject(v8::Local<v8::Object> h_world, WrapMotorJointDef* wrap_motor_jd, b2MotorJoint* motor_joint)
	{
		m_motor_joint = motor_joint;
		WrapJoint::SetupObject(h_world, wrap_motor_jd);
	}
	b2MotorJoint* ResetObject()
	{
		WrapJoint::ResetObject();
		b2MotorJoint* motor_joint = m_motor_joint;
		m_motor_joint = NULL;
		return motor_joint;
	}
public:
	static WrapMotorJoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMotorJoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMotorJoint>(object); }
	static b2MotorJoint* Peek(v8::Local<v8::Value> value) { WrapMotorJoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2MotorJoint"));
			function_template->Inherit(WrapJoint::GetFunctionTemplate());
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, SetLinearOffset)
			NANX_METHOD_APPLY(prototype_template, GetLinearOffset)
			NANX_METHOD_APPLY(prototype_template, SetAngularOffset)
			NANX_METHOD_APPLY(prototype_template, GetAngularOffset)
			NANX_METHOD_APPLY(prototype_template, SetMaxForce)
			NANX_METHOD_APPLY(prototype_template, GetMaxForce)
			NANX_METHOD_APPLY(prototype_template, SetMaxTorque)
			NANX_METHOD_APPLY(prototype_template, GetMaxTorque)
			NANX_METHOD_APPLY(prototype_template, SetCorrectionFactor)
			NANX_METHOD_APPLY(prototype_template, GetCorrectionFactor)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapMotorJoint* wrap = new WrapMotorJoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	NANX_METHOD(SetLinearOffset)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		wrap->m_motor_joint->SetLinearOffset(WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]))->GetVec2());
	}
	NANX_METHOD(GetLinearOffset)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		v8::Local<v8::Object> out = info[0]->IsUndefined() ? WrapVec2::NewInstance() : v8::Local<v8::Object>::Cast(info[0]);
		WrapVec2::Unwrap(out)->SetVec2(wrap->m_motor_joint->GetLinearOffset());
		info.GetReturnValue().Set(out);
	}
	NANX_METHOD(SetAngularOffset)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		wrap->m_motor_joint->SetAngularOffset(NANX_float32(info[0]));
	}
	NANX_METHOD(GetAngularOffset)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_motor_joint->GetAngularOffset()));
	}
	NANX_METHOD(SetMaxForce)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		wrap->m_motor_joint->SetMaxForce(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxForce)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_motor_joint->GetMaxForce()));
	}
	NANX_METHOD(SetMaxTorque)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		wrap->m_motor_joint->SetMaxTorque(NANX_float32(info[0]));
	}
	NANX_METHOD(GetMaxTorque)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_motor_joint->GetMaxTorque()));
	}
	NANX_METHOD(SetCorrectionFactor)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		wrap->m_motor_joint->SetCorrectionFactor(NANX_float32(info[0]));
	}
	NANX_METHOD(GetCorrectionFactor)
	{
		WrapMotorJoint* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_motor_joint->GetCorrectionFactor()));
	}
///	void Dump();
};

//// b2ContactID

class WrapContactID : public Nan::ObjectWrap
{
public:
	b2ContactID m_contact_id;
private:
	WrapContactID() {}
	~WrapContactID() {}
public:
	b2ContactID* Peek() { return &m_contact_id; }
public:
	static WrapContactID* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapContactID* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapContactID>(object); }
	static b2ContactID* Peek(v8::Local<v8::Value> value) { WrapContactID* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ContactID"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			//NANX_MEMBER_APPLY(prototype_template, cf) // TODO
			NANX_MEMBER_APPLY(prototype_template, key)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance(const b2ContactID& contact_id)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapContactID* wrap = Unwrap(instance);
		wrap->m_contact_id = contact_id;
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapContactID* wrap = new WrapContactID();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	//NANX_MEMBER_OBJECT(cf) // TODO
	NANX_MEMBER_UINT32(uint32, key)
};

//// b2ManifoldPoint

class WrapManifoldPoint : public Nan::ObjectWrap
{
public:
	b2ManifoldPoint m_manifold_point;
	Nan::Persistent<v8::Object> m_wrap_localPoint; // m_manifold_point.localPoint
	Nan::Persistent<v8::Object> m_wrap_id; // m_manifold_point.id
private:
	WrapManifoldPoint()
	{
		m_wrap_localPoint.Reset(WrapVec2::NewInstance(m_manifold_point.localPoint));
		m_wrap_id.Reset(WrapContactID::NewInstance(m_manifold_point.id));
	}
	~WrapManifoldPoint()
	{
		m_wrap_localPoint.Reset();
		m_wrap_id.Reset();
	}
public:
	b2ManifoldPoint* Peek() { return &m_manifold_point; }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_manifold_point.localPoint = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localPoint))->GetVec2(); // struct copy
		m_manifold_point.id = WrapContactID::Unwrap(Nan::New<v8::Object>(m_wrap_id))->m_contact_id;
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localPoint))->SetVec2(m_manifold_point.localPoint);
		WrapContactID::Unwrap(Nan::New<v8::Object>(m_wrap_id))->m_contact_id = m_manifold_point.id;
	}
public:
	static WrapManifoldPoint* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapManifoldPoint* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapManifoldPoint>(object); }
	static b2ManifoldPoint* Peek(v8::Local<v8::Value> value) { WrapManifoldPoint* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ManifoldPoint"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, localPoint)
			NANX_MEMBER_APPLY(prototype_template, normalImpulse)
			NANX_MEMBER_APPLY(prototype_template, tangentImpulse)
			NANX_MEMBER_APPLY(prototype_template, id)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance(const b2ManifoldPoint& manifold_point)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapManifoldPoint* wrap = Unwrap(instance);
		wrap->m_manifold_point = manifold_point; // struct copy
		wrap->SyncPush();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapManifoldPoint* wrap = new WrapManifoldPoint();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(localPoint) // m_wrap_localPoint
	NANX_MEMBER_NUMBER(float32, normalImpulse)
	NANX_MEMBER_NUMBER(float32, tangentImpulse)
	NANX_MEMBER_OBJECT(id) // m_wrap_id
};

//// b2Manifold

class WrapManifold : public Nan::ObjectWrap
{
public:
	b2Manifold m_manifold;
	Nan::Persistent<v8::Array> m_wrap_points; // m_manifold.points
	Nan::Persistent<v8::Object> m_wrap_localNormal; // m_manifold.localNormal
	Nan::Persistent<v8::Object> m_wrap_localPoint; // m_manifold.localPoint
private:
	WrapManifold()
	{
		m_wrap_points.Reset(Nan::New<v8::Array>(b2_maxManifoldPoints));
		m_wrap_localNormal.Reset(WrapVec2::NewInstance(m_manifold.localNormal));
		m_wrap_localPoint.Reset(WrapVec2::NewInstance(m_manifold.localPoint));
		v8::Local<v8::Array> manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			manifold_points->Set(i, WrapManifoldPoint::NewInstance(m_manifold.points[i]));
		}
	}
	~WrapManifold()
	{
		m_wrap_points.Reset();
		m_wrap_localNormal.Reset();
		m_wrap_localPoint.Reset();
	}
public:
	b2Manifold* Peek() { return &m_manifold; }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		v8::Local<v8::Array> manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			WrapManifoldPoint* wrap_manifold_point = WrapManifoldPoint::Unwrap(v8::Local<v8::Object>::Cast(manifold_points->Get(i)));
			m_manifold.points[i] = wrap_manifold_point->m_manifold_point; // struct copy
			wrap_manifold_point->SyncPull();
		}
		m_manifold.localNormal = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localNormal))->GetVec2(); // struct copy
		m_manifold.localPoint = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localPoint))->GetVec2(); // struct copy
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		v8::Local<v8::Array> manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			WrapManifoldPoint* wrap_manifold_point = WrapManifoldPoint::Unwrap(v8::Local<v8::Object>::Cast(manifold_points->Get(i)));
			wrap_manifold_point->m_manifold_point = m_manifold.points[i]; // struct copy
			wrap_manifold_point->SyncPush();
		}
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localNormal))->SetVec2(m_manifold.localNormal);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_localPoint))->SetVec2(m_manifold.localPoint);
	}
public:
	static WrapManifold* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapManifold* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapManifold>(object); }
	static b2Manifold* Peek(v8::Local<v8::Value> value) { WrapManifold* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2Manifold"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, points)
			NANX_MEMBER_APPLY(prototype_template, localNormal)
			NANX_MEMBER_APPLY(prototype_template, localPoint)
			NANX_MEMBER_APPLY(prototype_template, type)
			NANX_MEMBER_APPLY(prototype_template, pointCount)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		//wrap->SyncPush();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Manifold& manifold)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapManifold* wrap = Unwrap(instance);
		wrap->m_manifold = manifold; // struct copy
		wrap->SyncPush();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapManifold* wrap = new WrapManifold();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_ARRAY(points) // m_wrap_points
	NANX_MEMBER_OBJECT(localNormal) // m_wrap_localNormal
	NANX_MEMBER_OBJECT(localPoint) // m_wrap_localPoint
	NANX_MEMBER_INTEGER(b2Manifold::Type, type)
	NANX_MEMBER_INTEGER(int32, pointCount)
};

//// b2WorldManifold

class WrapWorldManifold : public Nan::ObjectWrap
{
public:
	b2WorldManifold m_world_manifold;
	Nan::Persistent<v8::Object> m_wrap_normal; // m_world_manifold.normal
	Nan::Persistent<v8::Array> m_wrap_points; // m_world_manifold.points
	Nan::Persistent<v8::Array> m_wrap_separations; // m_world_manifold.separations
private:
	WrapWorldManifold()
	{
		m_wrap_normal.Reset(WrapVec2::NewInstance(m_world_manifold.normal));
		m_wrap_points.Reset(Nan::New<v8::Array>(b2_maxManifoldPoints));
		m_wrap_separations.Reset(Nan::New<v8::Array>(b2_maxManifoldPoints));
		v8::Local<v8::Array> world_manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			world_manifold_points->Set(i, WrapVec2::NewInstance(m_world_manifold.points[i]));
		}
		v8::Local<v8::Array> world_manifold_separations = Nan::New<v8::Array>(m_wrap_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			world_manifold_separations->Set(i, Nan::New(m_world_manifold.separations[i]));
		}
	}
	~WrapWorldManifold()
	{
		m_wrap_normal.Reset();
		m_wrap_points.Reset();
		m_wrap_separations.Reset();
	}
public:
	b2WorldManifold* Peek() { return &m_world_manifold; }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_world_manifold.normal = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_normal))->GetVec2(); // struct copy
		v8::Local<v8::Array> world_manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			m_world_manifold.points[i] = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(world_manifold_points->Get(i)))->GetVec2(); // struct copy
		}
		v8::Local<v8::Array> world_manifold_separations = Nan::New<v8::Array>(m_wrap_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			m_world_manifold.separations[i] = NANX_float32(world_manifold_separations->Get(i));
		}
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_normal))->SetVec2(m_world_manifold.normal);
		v8::Local<v8::Array> world_manifold_points = Nan::New<v8::Array>(m_wrap_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(world_manifold_points->Get(i)))->SetVec2(m_world_manifold.points[i]);
		}
		v8::Local<v8::Array> world_manifold_separations = Nan::New<v8::Array>(m_wrap_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			world_manifold_separations->Set(i, Nan::New(m_world_manifold.separations[i]));
		}
	}
public:
	static WrapWorldManifold* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWorldManifold* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWorldManifold>(object); }
	static b2WorldManifold* Peek(v8::Local<v8::Value> value) { WrapWorldManifold* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2WorldManifold"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, normal)
			NANX_MEMBER_APPLY(prototype_template, points)
			NANX_MEMBER_APPLY(prototype_template, separations)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		//wrap->SyncPush();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2WorldManifold& world_manifold)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapWorldManifold* wrap = Unwrap(instance);
		wrap->m_world_manifold = world_manifold; // struct copy
		wrap->SyncPush();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapWorldManifold* wrap = new WrapWorldManifold();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_OBJECT(normal) // m_wrap_normal
	NANX_MEMBER_ARRAY(points) // m_wrap_points
	NANX_MEMBER_ARRAY(separations) // m_wrap_separations
};

//// b2Contact

class WrapContact : public Nan::ObjectWrap
{
public:
	b2Contact* m_contact;
private:
	WrapContact() : m_contact(NULL) {}
	~WrapContact() { m_contact = NULL; }
public:
	b2Contact* Peek() { return m_contact; }
public:
	static WrapContact* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapContact* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapContact>(object); }
	static b2Contact* Peek(v8::Local<v8::Value> value) { WrapContact* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2Contact"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetManifold)
			NANX_METHOD_APPLY(prototype_template, GetWorldManifold)
			NANX_METHOD_APPLY(prototype_template, IsTouching)
			NANX_METHOD_APPLY(prototype_template, SetEnabled)
			NANX_METHOD_APPLY(prototype_template, IsEnabled)
			NANX_METHOD_APPLY(prototype_template, GetNext)
			NANX_METHOD_APPLY(prototype_template, GetFixtureA)
			NANX_METHOD_APPLY(prototype_template, GetChildIndexA)
			NANX_METHOD_APPLY(prototype_template, GetFixtureB)
			NANX_METHOD_APPLY(prototype_template, GetChildIndexB)
			NANX_METHOD_APPLY(prototype_template, SetFriction)
			NANX_METHOD_APPLY(prototype_template, GetFriction)
			NANX_METHOD_APPLY(prototype_template, ResetFriction)
			NANX_METHOD_APPLY(prototype_template, SetRestitution)
			NANX_METHOD_APPLY(prototype_template, GetRestitution)
			NANX_METHOD_APPLY(prototype_template, ResetRestitution)
			NANX_METHOD_APPLY(prototype_template, SetTangentSpeed)
			NANX_METHOD_APPLY(prototype_template, GetTangentSpeed)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(b2Contact* contact)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapContact* wrap = Unwrap(instance);
		wrap->m_contact = contact; // struct copy
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapContact* wrap = new WrapContact();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(GetManifold)
	{
		WrapContact* wrap = Unwrap(info.This());
		const b2Manifold* manifold = wrap->m_contact->GetManifold();
		info.GetReturnValue().Set(WrapManifold::NewInstance(*manifold));
	}
	NANX_METHOD(GetWorldManifold)
	{
		WrapContact* wrap = Unwrap(info.This());
		WrapWorldManifold* wrap_world_manifold = WrapWorldManifold::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		wrap->m_contact->GetWorldManifold(&wrap_world_manifold->m_world_manifold);
		wrap_world_manifold->SyncPush();
	}
	NANX_METHOD(IsTouching)
	{
		WrapContact* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_contact->IsTouching()));
	}
	NANX_METHOD(SetEnabled)
	{
		WrapContact* wrap = Unwrap(info.This());
		wrap->m_contact->SetEnabled(NANX_bool(info[0]));
	}
	NANX_METHOD(IsEnabled)
	{
		WrapContact* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_contact->IsEnabled()));
	}
	NANX_METHOD(GetNext)
	{
		WrapContact* wrap = Unwrap(info.This());
		b2Contact* contact = wrap->m_contact->GetNext();
		if (contact)
		{
			info.GetReturnValue().Set(WrapContact::NewInstance(contact));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetFixtureA)
	{
		WrapContact* wrap = Unwrap(info.This());
		b2Fixture* fixture = wrap->m_contact->GetFixtureA();
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		info.GetReturnValue().Set(wrap_fixture->handle());
	}
	NANX_METHOD(GetChildIndexA)
	{
		WrapContact* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_contact->GetChildIndexA()));
	}
	NANX_METHOD(GetFixtureB)
	{
		WrapContact* wrap = Unwrap(info.This());
		b2Fixture* fixture = wrap->m_contact->GetFixtureB();
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		info.GetReturnValue().Set(wrap_fixture->handle());
	}
	NANX_METHOD(GetChildIndexB)
	{
		WrapContact* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_contact->GetChildIndexB()));
	}
	NANX_METHOD(SetFriction) { WrapContact* wrap = Unwrap(info.This()); wrap->m_contact->SetFriction(NANX_float32(info[0])); }
	NANX_METHOD(GetFriction) { WrapContact* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_contact->GetFriction())); }
	NANX_METHOD(ResetFriction) { WrapContact* wrap = Unwrap(info.This()); wrap->m_contact->ResetFriction(); }
	NANX_METHOD(SetRestitution) { WrapContact* wrap = Unwrap(info.This()); wrap->m_contact->SetRestitution(NANX_float32(info[0])); }
	NANX_METHOD(GetRestitution) { WrapContact* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_contact->GetRestitution())); }
	NANX_METHOD(ResetRestitution) { WrapContact* wrap = Unwrap(info.This()); wrap->m_contact->ResetRestitution(); }
	NANX_METHOD(SetTangentSpeed) { WrapContact* wrap = Unwrap(info.This()); wrap->m_contact->SetTangentSpeed(NANX_float32(info[0])); }
	NANX_METHOD(GetTangentSpeed) { WrapContact* wrap = Unwrap(info.This()); info.GetReturnValue().Set(Nan::New(wrap->m_contact->GetTangentSpeed())); }
//	virtual void Evaluate(b2Manifold* manifold, const b2Transform& xfA, const b2Transform& xfB) = 0;
};

//// b2ContactImpulse

class WrapContactImpulse : public Nan::ObjectWrap
{
public:
	b2ContactImpulse m_contact_impulse;
	Nan::Persistent<v8::Array> m_wrap_normalImpulses; // m_contact_impulse.normalImpulses
	Nan::Persistent<v8::Array> m_wrap_tangentImpulses; // m_contact_impulse.tangentImpulses
private:
	WrapContactImpulse()
	{
		m_wrap_normalImpulses.Reset(Nan::New<v8::Array>(b2_maxManifoldPoints));
		m_wrap_tangentImpulses.Reset(Nan::New<v8::Array>(b2_maxManifoldPoints));
	}
	~WrapContactImpulse()
	{
		m_wrap_normalImpulses.Reset();
		m_wrap_tangentImpulses.Reset();
	}
public:
	b2ContactImpulse* Peek() { return &m_contact_impulse; }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		v8::Local<v8::Array> contact_impulse_normalImpulses = Nan::New<v8::Array>(m_wrap_normalImpulses);
		for (uint32_t i = 0; i < contact_impulse_normalImpulses->Length(); ++i)
		{
			m_contact_impulse.normalImpulses[i] = NANX_float32(contact_impulse_normalImpulses->Get(i));
		}
		v8::Local<v8::Array> contact_impulse_tangentImpulses = Nan::New<v8::Array>(m_wrap_tangentImpulses);
		for (uint32_t i = 0; i < contact_impulse_tangentImpulses->Length(); ++i)
		{
			m_contact_impulse.tangentImpulses[i] = NANX_float32(contact_impulse_tangentImpulses->Get(i));
		}
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		v8::Local<v8::Array> contact_impulse_normalImpulses = Nan::New<v8::Array>(m_wrap_normalImpulses);
		for (uint32_t i = 0; i < contact_impulse_normalImpulses->Length(); ++i)
		{
			contact_impulse_normalImpulses->Set(i, Nan::New(m_contact_impulse.normalImpulses[i]));
		}
		v8::Local<v8::Array> contact_impulse_tangentImpulses = Nan::New<v8::Array>(m_wrap_tangentImpulses);
		for (uint32_t i = 0; i < contact_impulse_tangentImpulses->Length(); ++i)
		{
			contact_impulse_tangentImpulses->Set(i, Nan::New(m_contact_impulse.tangentImpulses[i]));
		}
	}
public:
	static WrapContactImpulse* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapContactImpulse* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapContactImpulse>(object); }
	static b2ContactImpulse* Peek(v8::Local<v8::Value> value) { WrapContactImpulse* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ContactImpulse"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, normalImpulses)
			NANX_MEMBER_APPLY(prototype_template, tangentImpulses)
			NANX_MEMBER_APPLY(prototype_template, count)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		//wrap->SyncPush();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2ContactImpulse& contact_impulse)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapContactImpulse* wrap = Unwrap(instance);
		wrap->m_contact_impulse = contact_impulse; // struct copy
		wrap->SyncPush();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapContactImpulse* wrap = new WrapContactImpulse();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_ARRAY(normalImpulses) // m_wrap_normalImpulses
	NANX_MEMBER_ARRAY(tangentImpulses) // m_wrap_tangentImpulses
	NANX_MEMBER_INTEGER(int32, count)
};

//// b2Color

class WrapColor : public Nan::ObjectWrap
{
private:
	b2Color m_color;
private:
	WrapColor() {}
	WrapColor(float32 r, float32 g, float32 b, float32 a) : m_color(r, g, b, a) {}
	~WrapColor() {}
public:
	b2Color* Peek() { return &m_color; }
	const b2Color& GetColor() const { return m_color; }
	void SetColor(const b2Color& color) { m_color = color; } // struct copy
public:
	static WrapColor* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapColor* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapColor>(object); }
	static b2Color* Peek(v8::Local<v8::Value> value) { WrapColor* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2Color"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, r)
			NANX_MEMBER_APPLY(prototype_template, g)
			NANX_MEMBER_APPLY(prototype_template, b)
			NANX_MEMBER_APPLY(prototype_template, a)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2Color& color)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapColor* wrap = Unwrap(instance);
		wrap->SetColor(color);
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			float32 r = (info.Length() > 0) ? NANX_float32(info[0]) : 0.5f;
			float32 g = (info.Length() > 1) ? NANX_float32(info[1]) : 0.5f;
			float32 b = (info.Length() > 2) ? NANX_float32(info[2]) : 0.5f;
			float32 a = (info.Length() > 3) ? NANX_float32(info[3]) : 0.5f;
			WrapColor* wrap = new WrapColor(r, g, b, a);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_NUMBER(float32, r)
	NANX_MEMBER_NUMBER(float32, g)
	NANX_MEMBER_NUMBER(float32, b)
	NANX_MEMBER_NUMBER(float32, a)
};

//// b2Draw

#if B2_ENABLE_PARTICLE

//// b2Particle

//// b2ParticleColor

class WrapParticleColor : public Nan::ObjectWrap
{
private:
	b2ParticleColor m_particle_color;
private:
	WrapParticleColor() {}
	WrapParticleColor(const b2ParticleColor& particle_color) : m_particle_color(particle_color) {}
	WrapParticleColor(uint8 r, uint8 g, uint8 b, uint8 a) : m_particle_color(r, g, b, a) {}
	~WrapParticleColor() {}
public:
	b2ParticleColor* Peek() { return &m_particle_color; }
	const b2ParticleColor& GetParticleColor() const { return m_particle_color; }
	void SetParticleColor(const b2ParticleColor& particle_color) { m_particle_color = particle_color; } // struct copy
public:
	static WrapParticleColor* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleColor* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleColor>(object); }
	static b2ParticleColor* Peek(v8::Local<v8::Value> value) { WrapParticleColor* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleColor"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, r)
			NANX_MEMBER_APPLY(prototype_template, g)
			NANX_MEMBER_APPLY(prototype_template, b)
			NANX_MEMBER_APPLY(prototype_template, a)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
	static v8::Local<v8::Object> NewInstance(const b2ParticleColor& particle_color)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapParticleColor* wrap = Unwrap(instance);
		wrap->SetParticleColor(particle_color);
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			uint8 r = (info.Length() > 0) ? NANX_uint8(info[0]) : 0;
			uint8 g = (info.Length() > 1) ? NANX_uint8(info[1]) : 0;
			uint8 b = (info.Length() > 2) ? NANX_uint8(info[2]) : 0;
			uint8 a = (info.Length() > 3) ? NANX_uint8(info[3]) : 0;
			WrapParticleColor* wrap = new WrapParticleColor(r, g, b, a);
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_UINT32(uint8, r)
	NANX_MEMBER_UINT32(uint8, g)
	NANX_MEMBER_UINT32(uint8, b)
	NANX_MEMBER_UINT32(uint8, a)
};

//// b2ParticleDef

class WrapParticleDef : public Nan::ObjectWrap
{
private:
	b2ParticleDef m_pd;
	Nan::Persistent<v8::Object> m_wrap_position; // m_pd.position
	Nan::Persistent<v8::Object> m_wrap_velocity; // m_pd.velocity
	Nan::Persistent<v8::Object> m_wrap_color; // m_pd.color
	Nan::Persistent<v8::Value> m_wrap_userData; // m_pd.userData
private:
	WrapParticleDef()
	{
		m_wrap_position.Reset(WrapVec2::NewInstance(m_pd.position));
		m_wrap_velocity.Reset(WrapVec2::NewInstance(m_pd.velocity));
		m_wrap_color.Reset(WrapParticleColor::NewInstance(m_pd.color));
	}
	~WrapParticleDef()
	{
		m_wrap_position.Reset();
		m_wrap_velocity.Reset();
		m_wrap_color.Reset();
		m_wrap_userData.Reset();
	}
public:
	b2ParticleDef* Peek() { return &m_pd; }
	b2ParticleDef& GetParticleDef() { return m_pd; }
	const b2ParticleDef& UseParticleDef() { SyncPull(); return GetParticleDef(); }
	v8::Local<v8::Value> GetUserDataHandle() { return Nan::New<v8::Value>(m_wrap_userData); }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_pd.position = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->GetVec2(); // struct copy
		m_pd.velocity = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_velocity))->GetVec2(); // struct copy
		m_pd.color = WrapParticleColor::Unwrap(Nan::New<v8::Object>(m_wrap_color))->GetParticleColor(); // struct copy
		//m_pd.userData; // not used
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->SetVec2(m_pd.position);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_velocity))->SetVec2(m_pd.velocity);
		WrapParticleColor::Unwrap(Nan::New<v8::Object>(m_wrap_color))->SetParticleColor(m_pd.color);
		//m_pd.userData; // not used
	}
public:
	static WrapParticleDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleDef>(object); }
	static b2ParticleDef* Peek(v8::Local<v8::Value> value) { WrapParticleDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, flags)
			NANX_MEMBER_APPLY(prototype_template, position)
			NANX_MEMBER_APPLY(prototype_template, velocity)
			NANX_MEMBER_APPLY(prototype_template, color)
			NANX_MEMBER_APPLY(prototype_template, lifetime)
			NANX_MEMBER_APPLY(prototype_template, userData)
			//b2ParticleGroup* group;
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleDef* wrap = new WrapParticleDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_INTEGER(uint32, flags)
	NANX_MEMBER_OBJECT(position) // m_wrap_position
	NANX_MEMBER_OBJECT(velocity) // m_wrap_velocity
	NANX_MEMBER_OBJECT(color) // m_wrap_color
	NANX_MEMBER_NUMBER(float32, lifetime)
	NANX_MEMBER_VALUE(userData) // m_wrap_userData
//	b2ParticleGroup* group;
};

//// b2ParticleHandle

class WrapParticleHandle : public Nan::ObjectWrap
{
private:
	b2ParticleHandle m_particle_handle;
private:
	WrapParticleHandle() {}
	~WrapParticleHandle() {}
public:
	b2ParticleHandle* Peek() { return &m_particle_handle; }
public:
	static WrapParticleHandle* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleHandle* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleHandle>(object); }
	static b2ParticleHandle* Peek(v8::Local<v8::Value> value) { WrapParticleHandle* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleHandle"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, GetIndex)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleHandle* wrap = new WrapParticleHandle();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(GetIndex)
	{
		WrapParticleHandle* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_handle.GetIndex()));
	}
};

//// b2ParticleGroupDef

class WrapParticleGroupDef : public Nan::ObjectWrap
{
private:
	b2ParticleGroupDef m_pgd;
	Nan::Persistent<v8::Object> m_wrap_position; // m_pgd.position
	Nan::Persistent<v8::Object> m_wrap_linearVelocity; // m_pgd.linearVelocity
	Nan::Persistent<v8::Object> m_wrap_color; // m_pgd.color
	Nan::Persistent<v8::Object> m_wrap_shape; // m_pgd.shape
	Nan::Persistent<v8::Value> m_wrap_userData; // m_pgd.userData
private:
	WrapParticleGroupDef()
	{
		m_wrap_position.Reset(WrapVec2::NewInstance(m_pgd.position));
		m_wrap_linearVelocity.Reset(WrapVec2::NewInstance(m_pgd.linearVelocity));
		m_wrap_color.Reset(WrapParticleColor::NewInstance(m_pgd.color));
	}
	~WrapParticleGroupDef()
	{
		m_wrap_position.Reset();
		m_wrap_linearVelocity.Reset();
		m_wrap_color.Reset();
		m_wrap_shape.Reset();
		m_wrap_userData.Reset();
	}
public:
	b2ParticleGroupDef* Peek() { return &m_pgd; }
	b2ParticleGroupDef& GetParticleGroupDef() { return m_pgd; }
	const b2ParticleGroupDef& UseParticleGroupDef() { SyncPull(); return GetParticleGroupDef(); }
	v8::Local<v8::Value> GetUserDataHandle() { return Nan::New<v8::Value>(m_wrap_userData); }
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_pgd.position = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->GetVec2(); // struct copy
		m_pgd.linearVelocity = WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearVelocity))->GetVec2(); // struct copy
		m_pgd.color = WrapParticleColor::Unwrap(Nan::New<v8::Object>(m_wrap_color))->GetParticleColor(); // struct copy
		if (!m_wrap_shape.IsEmpty())
		{
			WrapShape* wrap_shape = WrapShape::Unwrap(Nan::New<v8::Object>(m_wrap_shape));
			m_pgd.shape = &wrap_shape->UseShape();
		}
		else
		{
			m_pgd.shape = NULL;
		}
		//m_pgd.userData; // not used
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_position))->SetVec2(m_pgd.position);
		WrapVec2::Unwrap(Nan::New<v8::Object>(m_wrap_linearVelocity))->SetVec2(m_pgd.linearVelocity);
		WrapParticleColor::Unwrap(Nan::New<v8::Object>(m_wrap_color))->SetParticleColor(m_pgd.color);
		if (m_pgd.shape)
		{
			// TODO: m_wrap_shape.Reset(WrapShape::GetWrap(m_pgd.shape)->handle());
		}
		else
		{
			// TODO: m_wrap_shape.Reset();
		}
		//m_pgd.userData; // not used
	}
public:
	static WrapParticleGroupDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleGroupDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleGroupDef>(object); }
	static b2ParticleGroupDef* Peek(v8::Local<v8::Value> value) { WrapParticleGroupDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleGroupDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, flags)
			NANX_MEMBER_APPLY(prototype_template, groupFlags)
			NANX_MEMBER_APPLY(prototype_template, position)
			NANX_MEMBER_APPLY(prototype_template, angle)
			NANX_MEMBER_APPLY(prototype_template, linearVelocity)
			NANX_MEMBER_APPLY(prototype_template, angularVelocity)
			NANX_MEMBER_APPLY(prototype_template, color)
			NANX_MEMBER_APPLY(prototype_template, strength)
			NANX_MEMBER_APPLY(prototype_template, shape)
			//const b2Shape* const* shapes;
			//int32 shapeCount;
			//float32 stride;
			//int32 particleCount;
			//const b2Vec2* positionData;
			NANX_MEMBER_APPLY(prototype_template, lifetime)
			NANX_MEMBER_APPLY(prototype_template, userData)
			//b2ParticleGroup* group;
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleGroupDef* wrap = new WrapParticleGroupDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_INTEGER(uint32, flags)
	NANX_MEMBER_INTEGER(uint32, groupFlags)
	NANX_MEMBER_OBJECT(position) // m_wrap_position
	NANX_MEMBER_NUMBER(float32, angle)
	NANX_MEMBER_OBJECT(linearVelocity) // m_wrap_linearVelocity
	NANX_MEMBER_NUMBER(float32, angularVelocity)
	NANX_MEMBER_OBJECT(color) // m_wrap_color
	NANX_MEMBER_NUMBER(float32, strength)
	NANX_MEMBER_OBJECT(shape) // m_wrap_shape
	//const b2Shape* const* shapes;
	//int32 shapeCount;
	//float32 stride;
	//int32 particleCount;
	//const b2Vec2* positionData;
	NANX_MEMBER_NUMBER(float32, lifetime)
	NANX_MEMBER_VALUE(userData) // m_wrap_userData
	//b2ParticleGroup* group;
};

//// b2ParticleGroup

class WrapParticleGroup : public Nan::ObjectWrap
{
private:
	b2ParticleGroup* m_particle_group;
	Nan::Persistent<v8::Object> m_particle_group_particle_system;
	Nan::Persistent<v8::Value> m_particle_group_userData;
private:
	WrapParticleGroup() : m_particle_group(NULL) {}
	~WrapParticleGroup()
	{
		m_particle_group_particle_system.Reset();
		m_particle_group_userData.Reset();
	}
public:
	b2ParticleGroup* Peek() { return m_particle_group; }
	b2ParticleGroup* GetParticleGroup() { return m_particle_group; }
	void SetupObject(v8::Local<v8::Object> h_particle_system, WrapParticleGroupDef* wrap_pgd, b2ParticleGroup* particle_group)
	{
		m_particle_group = particle_group;
		// set particle_group internal data
		WrapParticleGroup::SetWrap(m_particle_group, this);
		// set reference to this particle_group (prevent GC)
		Ref();
		// set reference to world object
		m_particle_group_particle_system.Reset(h_particle_system);
		// set reference to user data object
		m_particle_group_userData.Reset(wrap_pgd->GetUserDataHandle());
	}
	b2ParticleGroup* ResetObject()
	{
		// clear reference to world object
		m_particle_group_particle_system.Reset();
		// clear reference to user data object
		m_particle_group_userData.Reset();
		// clear reference to this particle_group (allow GC)
		Unref();
		// clear particle_group internal data
		WrapParticleGroup::SetWrap(m_particle_group, NULL);
		b2ParticleGroup* particle_group = m_particle_group;
		m_particle_group = NULL;
		return particle_group;
	}
public:
	static WrapParticleGroup* GetWrap(const b2ParticleGroup* particle_group)
	{
		return static_cast<WrapParticleGroup*>(particle_group->GetUserData());
	}
	static void SetWrap(b2ParticleGroup* particle_group, WrapParticleGroup* wrap)
	{
		particle_group->SetUserData(wrap);
	}
public:
	static WrapParticleGroup* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleGroup* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleGroup>(object); }
	static b2ParticleGroup* Peek(v8::Local<v8::Value> value) { WrapParticleGroup* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleGroup"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			//b2ParticleGroup* GetNext();
			//b2ParticleSystem* GetParticleSystem();
			NANX_METHOD_APPLY(prototype_template, GetParticleCount)
			NANX_METHOD_APPLY(prototype_template, GetBufferIndex)
			NANX_METHOD_APPLY(prototype_template, ContainsParticle)
			NANX_METHOD_APPLY(prototype_template, DestroyParticles)
			//uint32 GetAllParticleFlags() const;
			//uint32 GetGroupFlags() const;
			//void SetGroupFlags(uint32 flags);
			//float32 GetMass() const;
			//float32 GetInertia() const;
			//b2Vec2 GetCenter() const;
			//b2Vec2 GetLinearVelocity() const;
			//float32 GetAngularVelocity() const;
			//const b2Transform& GetTransform() const;
			//const b2Vec2& GetPosition() const;
			//float32 GetAngle() const;
			//b2Vec2 GetLinearVelocityFromWorldPoint(const b2Vec2& worldPoint) const;
			//void* GetUserData() const;
			//void SetUserData(void* data);
			//void ApplyForce(const b2Vec2& force);
			//void ApplyLinearImpulse(const b2Vec2& impulse);
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleGroup* wrap = new WrapParticleGroup();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	//b2ParticleGroup* GetNext();
	//b2ParticleSystem* GetParticleSystem();
	NANX_METHOD(GetParticleCount)
	{
		WrapParticleGroup* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_group->GetParticleCount()));
	}
	NANX_METHOD(GetBufferIndex)
	{
		WrapParticleGroup* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_group->GetBufferIndex()));
	}
	NANX_METHOD(ContainsParticle)
	{
		WrapParticleGroup* wrap = Unwrap(info.This());
		int32 index = NANX_int32(info[0]);
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_group->ContainsParticle(index)));
	}
	//uint32 GetAllParticleFlags() const;
	//uint32 GetGroupFlags() const;
	//void SetGroupFlags(uint32 flags);
	//float32 GetMass() const;
	//float32 GetInertia() const;
	//b2Vec2 GetCenter() const;
	//b2Vec2 GetLinearVelocity() const;
	//float32 GetAngularVelocity() const;
	//const b2Transform& GetTransform() const;
	//const b2Vec2& GetPosition() const;
	//float32 GetAngle() const;
	//b2Vec2 GetLinearVelocityFromWorldPoint(const b2Vec2& worldPoint) const;
	//void* GetUserData() const;
	//void SetUserData(void* data);
	//void ApplyForce(const b2Vec2& force);
	//void ApplyLinearImpulse(const b2Vec2& impulse);
	NANX_METHOD(DestroyParticles)
	{
		WrapParticleGroup* wrap = Unwrap(info.This());
		bool callDestructionListener = (info.Length() > 0) ? NANX_bool(info[0]) : false;
		wrap->m_particle_group->DestroyParticles(callDestructionListener);
	}
};

//// b2ParticleSystemDef

class WrapParticleSystemDef : public Nan::ObjectWrap
{
private:
	b2ParticleSystemDef m_psd;
private:
	WrapParticleSystemDef() {}
	~WrapParticleSystemDef() {}
public:
	b2ParticleSystemDef* Peek() { return &m_psd; }
	b2ParticleSystemDef& GetParticleSystemDef() { return m_psd; }
	const b2ParticleSystemDef& UseParticleSystemDef() { SyncPull(); return GetParticleSystemDef(); }
	void SyncPull()
	{
		// sync: pull data from javascript objects
	}
	void SyncPush()
	{
		// sync: push data into javascript objects
	}
public:
	static WrapParticleSystemDef* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleSystemDef* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleSystemDef>(object); }
	static b2ParticleSystemDef* Peek(v8::Local<v8::Value> value) { WrapParticleSystemDef* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleSystemDef"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_MEMBER_APPLY(prototype_template, strictContactCheck)
			NANX_MEMBER_APPLY(prototype_template, strictContactCheck)
			NANX_MEMBER_APPLY(prototype_template, density)
			NANX_MEMBER_APPLY(prototype_template, gravityScale)
			NANX_MEMBER_APPLY(prototype_template, radius)
			NANX_MEMBER_APPLY(prototype_template, maxCount)
			NANX_MEMBER_APPLY(prototype_template, pressureStrength)
			NANX_MEMBER_APPLY(prototype_template, dampingStrength)
			NANX_MEMBER_APPLY(prototype_template, elasticStrength)
			NANX_MEMBER_APPLY(prototype_template, springStrength)
			NANX_MEMBER_APPLY(prototype_template, viscousStrength)
			NANX_MEMBER_APPLY(prototype_template, surfaceTensionPressureStrength)
			NANX_MEMBER_APPLY(prototype_template, surfaceTensionNormalStrength)
			NANX_MEMBER_APPLY(prototype_template, repulsiveStrength)
			NANX_MEMBER_APPLY(prototype_template, powderStrength)
			NANX_MEMBER_APPLY(prototype_template, ejectionStrength)
			NANX_MEMBER_APPLY(prototype_template, staticPressureStrength)
			NANX_MEMBER_APPLY(prototype_template, staticPressureRelaxation)
			NANX_MEMBER_APPLY(prototype_template, staticPressureIterations)
			NANX_MEMBER_APPLY(prototype_template, colorMixingStrength)
			NANX_MEMBER_APPLY(prototype_template, destroyByAge)
			NANX_MEMBER_APPLY(prototype_template, lifetimeGranularity)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleSystemDef* wrap = new WrapParticleSystemDef();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_MEMBER_BOOLEAN(bool, strictContactCheck)
	NANX_MEMBER_NUMBER(float32, density)
	NANX_MEMBER_NUMBER(float32, gravityScale)
	NANX_MEMBER_NUMBER(float32, radius)
	NANX_MEMBER_INTEGER(int32, maxCount)
	NANX_MEMBER_NUMBER(float32, pressureStrength)
	NANX_MEMBER_NUMBER(float32, dampingStrength)
	NANX_MEMBER_NUMBER(float32, elasticStrength)
	NANX_MEMBER_NUMBER(float32, springStrength)
	NANX_MEMBER_NUMBER(float32, viscousStrength)
	NANX_MEMBER_NUMBER(float32, surfaceTensionPressureStrength)
	NANX_MEMBER_NUMBER(float32, surfaceTensionNormalStrength)
	NANX_MEMBER_NUMBER(float32, repulsiveStrength)
	NANX_MEMBER_NUMBER(float32, powderStrength)
	NANX_MEMBER_NUMBER(float32, ejectionStrength)
	NANX_MEMBER_NUMBER(float32, staticPressureStrength)
	NANX_MEMBER_NUMBER(float32, staticPressureRelaxation)
	NANX_MEMBER_INTEGER(int32, staticPressureIterations)
	NANX_MEMBER_NUMBER(float32, colorMixingStrength)
	NANX_MEMBER_BOOLEAN(bool, destroyByAge)
	NANX_MEMBER_NUMBER(float32, lifetimeGranularity)
};

//// b2ParticleSystem

class WrapParticleSystem : public Nan::ObjectWrap
{
private:
	b2ParticleSystem* m_particle_system;
	Nan::Persistent<v8::Object> m_particle_system_world;
private:
	WrapParticleSystem() : m_particle_system(NULL) {}
	~WrapParticleSystem()
	{
		m_particle_system_world.Reset();
	}
public:
	b2ParticleSystem* Peek() { return m_particle_system; }
	b2ParticleSystem* GetParticleSystem() { return m_particle_system; }
	void SetupObject(v8::Local<v8::Object> h_world, WrapParticleSystemDef* wrap_psd, b2ParticleSystem* particle_system)
	{
		m_particle_system = particle_system;
		// set reference to this particle_system (prevent GC)
		Ref();
		// set reference to world object
		m_particle_system_world.Reset(h_world);
	}
	b2ParticleSystem* ResetObject()
	{
		// clear reference to world object
		m_particle_system_world.Reset();
		// clear reference to this particle_system (allow GC)
		Unref();
		b2ParticleSystem* particle_system = m_particle_system;
		m_particle_system = NULL;
		return particle_system;
	}
public:
	static WrapParticleSystem* GetWrap(const b2ParticleSystem* particle_system)
	{
		//return static_cast<WrapParticleSystem*>(particle_system->GetUserData());
		return NULL;
	}
	static void SetWrap(b2ParticleSystem* particle_system, WrapParticleSystem* wrap)
	{
		//particle_system->SetUserData(wrap);
	}
public:
	static WrapParticleSystem* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapParticleSystem* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapParticleSystem>(object); }
	static b2ParticleSystem* Peek(v8::Local<v8::Value> value) { WrapParticleSystem* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2ParticleSystem"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, CreateParticle)
			NANX_METHOD_APPLY(prototype_template, DestroyParticle)
			NANX_METHOD_APPLY(prototype_template, DestroyOldestParticle)
			NANX_METHOD_APPLY(prototype_template, CreateParticleGroup)
			NANX_METHOD_APPLY(prototype_template, GetParticleCount)
			NANX_METHOD_APPLY(prototype_template, GetRadius)
			NANX_METHOD_APPLY(prototype_template, SetPositionBuffer)
			NANX_METHOD_APPLY(prototype_template, SetVelocityBuffer)
			NANX_METHOD_APPLY(prototype_template, SetColorBuffer)
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapParticleSystem* wrap = new WrapParticleSystem();
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(CreateParticle)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		WrapParticleDef* wrap_pd = WrapParticleDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		// create box2d particle
		int32 particle_index = wrap->m_particle_system->CreateParticle(wrap_pd->UseParticleDef());
		info.GetReturnValue().Set(Nan::New(particle_index));
	}
//	const b2ParticleHandle* GetParticleHandleFromIndex(const int32 index);
	NANX_METHOD(DestroyParticle)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		int32 particle_index = NANX_int32(info[0]);
		bool callDestructionListener = (info.Length() > 1) ? NANX_bool(info[1]) : false;
		// destroy box2d particle
		wrap->m_particle_system->DestroyParticle(particle_index, callDestructionListener);
	}
	NANX_METHOD(DestroyOldestParticle)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		int32 particle_index = NANX_int32(info[0]);
		bool callDestructionListener = (info.Length() > 1) ? NANX_bool(info[1]) : false;
		// destroy box2d particle
		wrap->m_particle_system->DestroyOldestParticle(particle_index, callDestructionListener);
	}
//	int32 DestroyParticlesInShape(const b2Shape& shape, const b2Transform& xf);
//	int32 DestroyParticlesInShape(const b2Shape& shape, const b2Transform& xf, bool callDestructionListener);
	NANX_METHOD(CreateParticleGroup)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		WrapParticleGroupDef* wrap_pgd = WrapParticleGroupDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		// create box2d particle group
		b2ParticleGroup* particle_group = wrap->m_particle_system->CreateParticleGroup(wrap_pgd->UseParticleGroupDef());
		// create javascript particle group object
		v8::Local<v8::Object> h_particle_group = WrapParticleGroup::NewInstance();
		WrapParticleGroup* wrap_particle_group = WrapParticleGroup::Unwrap(h_particle_group);
		// set up javascript particle group object
		wrap_particle_group->SetupObject(info.This(), wrap_pgd, particle_group);
		info.GetReturnValue().Set(h_particle_group);
	}
//	void JoinParticleGroups(b2ParticleGroup* groupA, b2ParticleGroup* groupB);
//	void SplitParticleGroup(b2ParticleGroup* group);
//	b2ParticleGroup* GetParticleGroupList();
//	int32 GetParticleGroupCount() const;
	NANX_METHOD(GetParticleCount)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_system->GetParticleCount()));
	}
//	int32 GetMaxParticleCount() const;
//	void SetMaxParticleCount(int32 count);
//	uint32 GetAllParticleFlags() const;
//	uint32 GetAllGroupFlags() const;
//	void SetPaused(bool paused);
//	bool GetPaused() const;
//	void SetDensity(float32 density);
//	float32 GetDensity() const;
//	void SetGravityScale(float32 gravityScale);
//	float32 GetGravityScale() const;
//	void SetDamping(float32 damping);
//	float32 GetDamping() const;
//	void SetStaticPressureIterations(int32 iterations);
//	int32 GetStaticPressureIterations() const;
//	void SetRadius(float32 radius);
//	float32 GetRadius() const;
	NANX_METHOD(GetRadius)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_particle_system->GetRadius()));
	}
//	b2Vec2* GetPositionBuffer();
//	b2Vec2* GetVelocityBuffer();
//	b2ParticleColor* GetColorBuffer();
//	b2ParticleGroup* const* GetGroupBuffer();
//	float32* GetWeightBuffer();
//	void** GetUserDataBuffer();
//	const uint32* GetFlagsBuffer() const;
//	void SetParticleFlags(int32 index, uint32 flags);
//	uint32 GetParticleFlags(const int32 index);
//	void SetFlagsBuffer(uint32* buffer, int32 capacity);
//	void SetPositionBuffer(b2Vec2* buffer, int32 capacity);
	NANX_METHOD(SetPositionBuffer)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		#if NODE_VERSION_AT_LEAST(4, 0, 0)
		v8::Local<v8::Float32Array> _buffer = v8::Local<v8::Float32Array>::Cast(info[0]);
		if (!_buffer->HasBuffer())
		{
			_buffer->Get(NANX_SYMBOL("buffer"));
			assert(_buffer->HasBuffer());
		}
		int32 capacity = NANX_int32(info[1]);
		b2Vec2* buffer = reinterpret_cast<b2Vec2*>(static_cast<char*>(_buffer->Buffer()->GetContents().Data()) + _buffer->ByteOffset());
		#else
		v8::Local<v8::Object> _buffer = v8::Local<v8::Object>::Cast(info[0]);
		int32 capacity = NANX_int32(info[1]);
		b2Vec2* buffer = static_cast<b2Vec2*>(_buffer->GetIndexedPropertiesExternalArrayData());
		#endif
		wrap->m_particle_system->SetPositionBuffer(buffer, capacity);
	}
//	void SetVelocityBuffer(b2Vec2* buffer, int32 capacity);
	NANX_METHOD(SetVelocityBuffer)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		#if NODE_VERSION_AT_LEAST(4, 0, 0)
		v8::Local<v8::Float32Array> _buffer = v8::Local<v8::Float32Array>::Cast(info[0]);
		if (!_buffer->HasBuffer())
		{
			_buffer->Get(NANX_SYMBOL("buffer"));
			assert(_buffer->HasBuffer());
		}
		int32 capacity = NANX_int32(info[1]);
		b2Vec2* buffer = reinterpret_cast<b2Vec2*>(static_cast<char*>(_buffer->Buffer()->GetContents().Data()) + _buffer->ByteOffset());
		#else
		v8::Local<v8::Object> _buffer = v8::Local<v8::Object>::Cast(info[0]);
		int32 capacity = NANX_int32(info[1]);
		b2Vec2* buffer = static_cast<b2Vec2*>(_buffer->GetIndexedPropertiesExternalArrayData());
		#endif
		wrap->m_particle_system->SetVelocityBuffer(buffer, capacity);
	}
//	void SetColorBuffer(b2ParticleColor* buffer, int32 capacity);
	NANX_METHOD(SetColorBuffer)
	{
		WrapParticleSystem* wrap = Unwrap(info.This());
		#if NODE_VERSION_AT_LEAST(4, 0, 0)
		v8::Local<v8::Uint8Array> _buffer = v8::Local<v8::Uint8Array>::Cast(info[0]);
		if (!_buffer->HasBuffer())
		{
			_buffer->Get(NANX_SYMBOL("buffer"));
			assert(_buffer->HasBuffer());
		}
		int32 capacity = NANX_int32(info[1]);
		b2ParticleColor* buffer = reinterpret_cast<b2ParticleColor*>(static_cast<char*>(_buffer->Buffer()->GetContents().Data()) + _buffer->ByteOffset());
		#else
		v8::Local<v8::Object> _buffer = v8::Local<v8::Object>::Cast(info[0]);
		int32 capacity = NANX_int32(info[1]);
		b2ParticleColor* buffer = static_cast<b2ParticleColor*>(_buffer->GetIndexedPropertiesExternalArrayData());
		#endif
		wrap->m_particle_system->SetColorBuffer(buffer, capacity);
	}
//	void SetUserDataBuffer(void** buffer, int32 capacity);
//	const b2ParticleContact* GetContacts() const;
//	int32 GetContactCount() const;
//	const b2ParticleBodyContact* GetBodyContacts() const;
//	int32 GetBodyContactCount() const;
//	const b2ParticlePair* GetPairs() const;
//	int32 GetPairCount() const;
//	const b2ParticleTriad* GetTriads() const;
//	int32 GetTriadCount() const;
//	void SetStuckThreshold(int32 iterations);
//	const int32* GetStuckCandidates() const;
//	int32 GetStuckCandidateCount() const;
//	float32 ComputeCollisionEnergy() const;
//	void SetStrictContactCheck(bool enabled);
//	bool GetStrictContactCheck() const;
//	void SetParticleLifetime(const int32 index, const float32 lifetime);
//	float32 GetParticleLifetime(const int32 index);
//	void SetDestructionByAge(const bool enable);
//	bool GetDestructionByAge() const;
//	const int32* GetExpirationTimeBuffer();
//	float32 ExpirationTimeToLifetime(const int32 expirationTime) const;
//	const int32* GetIndexByExpirationTimeBuffer();
//	void ParticleApplyLinearImpulse(int32 index, const b2Vec2& impulse);
//	void ApplyLinearImpulse(int32 firstIndex, int32 lastIndex, const b2Vec2& impulse);
//	void ParticleApplyForce(int32 index, const b2Vec2& force);
//	void ApplyForce(int32 firstIndex, int32 lastIndex, const b2Vec2& force);
//	b2ParticleSystem* GetNext();
//	void QueryAABB(b2QueryCallback* callback, const b2AABB& mass_data) const;
//	void QueryShapeAABB(b2QueryCallback* callback, const b2Shape& shape, const b2Transform& xf) const;
//	void RayCast(b2RayCastCallback* callback, const b2Vec2& point1, const b2Vec2& point2) const;
//	void ComputeAABB(b2AABB* const mass_data) const;
};

#endif

//// b2World

class WrapWorld : public Nan::ObjectWrap
{
private:
	class WrapDestructionListener : public b2DestructionListener
	{
	public:
		WrapWorld* m_wrap_world;
	public:
		WrapDestructionListener(WrapWorld* wrap) : m_wrap_world(wrap) {}
		~WrapDestructionListener() { m_wrap_world = NULL; }
	public:
		virtual void SayGoodbye(b2Joint* joint);
		virtual void SayGoodbye(b2Fixture* fixture);
		#if B2_ENABLE_PARTICLE
		virtual void SayGoodbye(b2ParticleGroup* group);
		virtual void SayGoodbye(b2ParticleSystem* particleSystem, int32 index);
		#endif
	};

private:
	class WrapContactFilter : public b2ContactFilter
	{
	public:
		WrapWorld* m_wrap_world;
	public:
		WrapContactFilter(WrapWorld* wrap) : m_wrap_world(wrap) {}
		~WrapContactFilter() { m_wrap_world = NULL; }
	public:
		virtual bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB);
		#if B2_ENABLE_PARTICLE
		virtual bool ShouldCollide(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 particleIndex);
		virtual bool ShouldCollide(b2ParticleSystem* particleSystem, int32 particleIndexA, int32 particleIndexB);
		#endif
	};

private:
	class WrapContactListener : public b2ContactListener
	{
	public:
		WrapWorld* m_wrap_world;
	public:
		WrapContactListener(WrapWorld* wrap) : m_wrap_world(wrap) {}
		~WrapContactListener() { m_wrap_world = NULL; }
	public:
		virtual void BeginContact(b2Contact* contact);
		virtual void EndContact(b2Contact* contact);
		#if B2_ENABLE_PARTICLE
		virtual void BeginContact(b2ParticleSystem* particleSystem, b2ParticleBodyContact* particleBodyContact);
		virtual void EndContact(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 index);
		virtual void BeginContact(b2ParticleSystem* particleSystem, b2ParticleContact* particleContact);
		virtual void EndContact(b2ParticleSystem* particleSystem, int32 indexA, int32 indexB);
		#endif
		virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
		virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
	};

private:
	class WrapDraw : public b2Draw
	{
	public:
		WrapWorld* m_wrap_world;
	public:
		WrapDraw(WrapWorld* wrap) : m_wrap_world(wrap) {}
		~WrapDraw() { m_wrap_world = NULL; }
	public:
		virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
		virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
		#if B2_ENABLE_PARTICLE
		virtual void DrawParticles(const b2Vec2 *centers, float32 radius, const b2ParticleColor *colors, int32 count);
		#endif
		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
		virtual void DrawTransform(const b2Transform& xf);
	};

private:
	class WrapQueryCallback : public b2QueryCallback
	{
	private:
		v8::Local<v8::Function> m_callback;
	public:
		WrapQueryCallback(v8::Local<v8::Function> callback) : m_callback(callback) {}
		bool ReportFixture(b2Fixture* fixture)
		{
			// get fixture internal data
			WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
			v8::Local<v8::Object> h_fixture = wrap_fixture->handle();
			v8::Local<v8::Value> argv[] = { h_fixture };
			return NANX_bool(Nan::MakeCallback(Nan::GetCurrentContext()->Global(), m_callback, countof(argv), argv));
		}
		#if B2_ENABLE_PARTICLE
		bool ReportParticle(const b2ParticleSystem* particleSystem, int32 index)
		{
			// TODO
			return b2QueryCallback::ReportParticle(particleSystem, index);
		}
		bool ShouldQueryParticleSystem(const b2ParticleSystem* particleSystem)
		{
			// TODO
			return b2QueryCallback::ShouldQueryParticleSystem(particleSystem);
		}
		#endif
	};

private:
	class WrapRayCastCallback : public b2RayCastCallback
	{
	private:
		v8::Local<v8::Function> m_callback;
	public:
		WrapRayCastCallback(v8::Local<v8::Function> callback) : m_callback(callback) {}
		float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
		{
			// get fixture internal data
			WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
			v8::Local<v8::Object> h_fixture = wrap_fixture->handle();
			v8::Local<v8::Object> h_point = WrapVec2::NewInstance(point);
			v8::Local<v8::Object> h_normal = WrapVec2::NewInstance(normal);
			v8::Local<v8::Number> h_fraction = Nan::New(fraction);
			v8::Local<v8::Value> argv[] = { h_fixture, h_point, h_normal, h_fraction };
			return NANX_float32(Nan::MakeCallback(Nan::GetCurrentContext()->Global(), m_callback, countof(argv), argv));
		}
		#if B2_ENABLE_PARTICLE
		float32 ReportParticle(const b2ParticleSystem* particleSystem, int32 index, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
		{
			// TODO:
			return b2RayCastCallback::ReportParticle(particleSystem, index, point, normal, fraction);
		}
		bool ShouldQueryParticleSystem(const b2ParticleSystem* particleSystem)
		{
			// TODO:
			return b2RayCastCallback::ShouldQueryParticleSystem(particleSystem);
		}
		#endif
	};

private:
	b2World m_world;
	Nan::Persistent<v8::Object> m_destruction_listener;
	WrapDestructionListener m_wrap_destruction_listener;
	Nan::Persistent<v8::Object> m_contact_filter;
	WrapContactFilter m_wrap_contact_filter;
	Nan::Persistent<v8::Object> m_contact_listener;
	WrapContactListener m_wrap_contact_listener;
	Nan::Persistent<v8::Object> m_draw;
	WrapDraw m_wrap_draw;
private:
	WrapWorld(const b2Vec2& gravity) :
		m_world(gravity),
		m_wrap_destruction_listener(this),
		m_wrap_contact_filter(this),
		m_wrap_contact_listener(this),
		m_wrap_draw(this)
	{
		m_world.SetDestructionListener(&m_wrap_destruction_listener);
		m_world.SetContactFilter(&m_wrap_contact_filter);
		m_world.SetContactListener(&m_wrap_contact_listener);
		m_world.SetDebugDraw(&m_wrap_draw);
	}
	~WrapWorld()
	{
		m_world.SetDestructionListener(NULL);
		m_world.SetContactFilter(NULL);
		m_world.SetContactListener(NULL);
		m_world.SetDebugDraw(NULL);
		m_destruction_listener.Reset();
		m_contact_filter.Reset();
		m_contact_listener.Reset();
		m_draw.Reset();
	}
public:
	b2World* Peek() { return &m_world; }
public:
	static WrapWorld* GetWrap(const b2World* world)
	{
		//return static_cast<WrapWorld*>(world->GetUserData());
		return NULL;
	}
	static void SetWrap(b2World* world, WrapWorld* wrap)
	{
		//world->SetUserData(wrap);
	}
public:
	static WrapWorld* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapWorld* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapWorld>(object); }
	static b2World* Peek(v8::Local<v8::Value> value) { WrapWorld* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static NAN_MODULE_INIT(Init)
	{
		v8::Local<v8::Function> constructor = GetConstructor();
		target->Set(constructor->GetName(), constructor);
	}
	static v8::Local<v8::Function> GetConstructor()
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::FunctionTemplate> function_template = GetFunctionTemplate();
		v8::Local<v8::Function> constructor = function_template->GetFunction();
		return scope.Escape(constructor);
	}
	static v8::Local<v8::FunctionTemplate> GetFunctionTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::FunctionTemplate> g_function_template;
		if (g_function_template.IsEmpty())
		{
			v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(New);
			g_function_template.Reset(function_template);
			function_template->SetClassName(NANX_SYMBOL("b2World"));
			function_template->InstanceTemplate()->SetInternalFieldCount(1);
			v8::Local<v8::ObjectTemplate> prototype_template = function_template->PrototypeTemplate();
			NANX_METHOD_APPLY(prototype_template, SetDestructionListener)
			NANX_METHOD_APPLY(prototype_template, SetContactFilter)
			NANX_METHOD_APPLY(prototype_template, SetContactListener)
			NANX_METHOD_APPLY(prototype_template, SetDebugDraw)
			NANX_METHOD_APPLY(prototype_template, CreateBody)
			NANX_METHOD_APPLY(prototype_template, DestroyBody)
			NANX_METHOD_APPLY(prototype_template, CreateJoint)
			NANX_METHOD_APPLY(prototype_template, DestroyJoint)
			NANX_METHOD_APPLY(prototype_template, Step)
			NANX_METHOD_APPLY(prototype_template, ClearForces)
			NANX_METHOD_APPLY(prototype_template, DrawDebugData)
			NANX_METHOD_APPLY(prototype_template, QueryAABB)
			#if B2_ENABLE_PARTICLE
			NANX_METHOD_APPLY(prototype_template, QueryShapeAABB)
			#endif
			NANX_METHOD_APPLY(prototype_template, RayCast)
			NANX_METHOD_APPLY(prototype_template, GetBodyList)
			NANX_METHOD_APPLY(prototype_template, GetJointList)
			NANX_METHOD_APPLY(prototype_template, GetContactList)
			NANX_METHOD_APPLY(prototype_template, SetAllowSleeping)
			NANX_METHOD_APPLY(prototype_template, GetAllowSleeping)
			NANX_METHOD_APPLY(prototype_template, SetWarmStarting)
			NANX_METHOD_APPLY(prototype_template, GetWarmStarting)
			NANX_METHOD_APPLY(prototype_template, SetContinuousPhysics)
			NANX_METHOD_APPLY(prototype_template, GetContinuousPhysics)
			NANX_METHOD_APPLY(prototype_template, SetSubStepping)
			NANX_METHOD_APPLY(prototype_template, GetSubStepping)
			NANX_METHOD_APPLY(prototype_template, GetBodyCount)
			NANX_METHOD_APPLY(prototype_template, GetJointCount)
			NANX_METHOD_APPLY(prototype_template, GetContactCount)
			NANX_METHOD_APPLY(prototype_template, SetGravity)
			NANX_METHOD_APPLY(prototype_template, GetGravity)
			NANX_METHOD_APPLY(prototype_template, IsLocked)
			NANX_METHOD_APPLY(prototype_template, SetAutoClearForces)
			NANX_METHOD_APPLY(prototype_template, GetAutoClearForces)
			#if B2_ENABLE_PARTICLE
			NANX_METHOD_APPLY(prototype_template, CreateParticleSystem)
			NANX_METHOD_APPLY(prototype_template, DestroyParticleSystem)
			NANX_METHOD_APPLY(prototype_template, CalculateReasonableParticleIterations)
			#endif
		}
		v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(g_function_template);
		return scope.Escape(function_template);
	}
	static v8::Local<v8::Object> NewInstance(const b2Vec2& gravity)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::Function> constructor = GetConstructor();
		v8::Local<v8::Object> instance = constructor->NewInstance();
		WrapWorld* wrap = Unwrap(instance);
		wrap->m_world.SetGravity(gravity);
		return scope.Escape(instance);
	}
private:
	static NAN_METHOD(New)
	{
		if (info.IsConstructCall())
		{
			WrapVec2* wrap_gravity = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			WrapWorld* wrap = new WrapWorld(wrap_gravity->GetVec2());
			wrap->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			v8::Local<v8::Function> constructor = GetConstructor();
			info.GetReturnValue().Set(constructor->NewInstance());
		}
	}
	NANX_METHOD(SetDestructionListener)
	{
		WrapWorld* wrap = Unwrap(info.This());
		if (info[0]->IsObject())
		{
			wrap->m_destruction_listener.Reset(info[0].As<v8::Object>());
		}
		else
		{
			wrap->m_destruction_listener.Reset();
		}
	}
	NANX_METHOD(SetContactFilter)
	{
		WrapWorld* wrap = Unwrap(info.This());
		if (info[0]->IsObject())
		{
			wrap->m_contact_filter.Reset(info[0].As<v8::Object>());
		}
		else
		{
			wrap->m_contact_filter.Reset();
		}
	}
	NANX_METHOD(SetContactListener)
	{
		WrapWorld* wrap = Unwrap(info.This());
		if (info[0]->IsObject())
		{
			wrap->m_contact_listener.Reset(info[0].As<v8::Object>());
		}
		else
		{
			wrap->m_contact_listener.Reset();
		}
	}
	NANX_METHOD(SetDebugDraw)
	{
		WrapWorld* wrap = Unwrap(info.This());
		if (info[0]->IsObject())
		{
			wrap->m_draw.Reset(info[0].As<v8::Object>());
		}
		else
		{
			wrap->m_draw.Reset();
		}
	}
	NANX_METHOD(CreateBody)
	{
		WrapWorld* wrap = Unwrap(info.This());
		WrapBodyDef* wrap_bd = WrapBodyDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		// create box2d body
		b2Body* body = wrap->m_world.CreateBody(&wrap_bd->UseBodyDef());
		// create javascript body object
		v8::Local<v8::Object> h_body = WrapBody::NewInstance();
		WrapBody* wrap_body = WrapBody::Unwrap(h_body);
		// set up javascript body object
		wrap_body->SetupObject(info.This(), wrap_bd, body);
		info.GetReturnValue().Set(h_body);
	}
	NANX_METHOD(DestroyBody)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Object> h_body = v8::Local<v8::Object>::Cast(info[0]);
		WrapBody* wrap_body = WrapBody::Unwrap(h_body);
		// delete box2d body
		wrap->m_world.DestroyBody(wrap_body->Peek());
		// reset javascript body object
		wrap_body->ResetObject();
	}
	NANX_METHOD(CreateJoint)
	{
		WrapWorld* wrap = Unwrap(info.This());
		WrapJointDef* wrap_jd = WrapJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		switch (wrap_jd->GetJointDef().type)
		{
		case e_unknownJoint:
		default:
			info.GetReturnValue().SetNull();
			break;
		case e_revoluteJoint:
		{
			WrapRevoluteJointDef* wrap_revolute_jd = WrapRevoluteJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d revolute joint
			b2RevoluteJoint* revolute_joint = static_cast<b2RevoluteJoint*>(wrap->m_world.CreateJoint(&wrap_revolute_jd->UseJointDef()));
			// create javascript revolute joint object
			v8::Local<v8::Object> h_revolute_joint = WrapRevoluteJoint::NewInstance();
			WrapRevoluteJoint* wrap_revolute_joint = WrapRevoluteJoint::Unwrap(h_revolute_joint);
			// set up javascript revolute joint object
			wrap_revolute_joint->SetupObject(info.This(), wrap_revolute_jd, revolute_joint);
			info.GetReturnValue().Set(h_revolute_joint);
			break;
		}
		case e_prismaticJoint:
		{
			WrapPrismaticJointDef* wrap_prismatic_jd = WrapPrismaticJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d prismatic joint
			b2PrismaticJoint* prismatic_joint = static_cast<b2PrismaticJoint*>(wrap->m_world.CreateJoint(&wrap_prismatic_jd->UseJointDef()));
			// create javascript prismatic joint object
			v8::Local<v8::Object> h_prismatic_joint = WrapPrismaticJoint::NewInstance();
			WrapPrismaticJoint* wrap_prismatic_joint = WrapPrismaticJoint::Unwrap(h_prismatic_joint);
			// set up javascript prismatic joint object
			wrap_prismatic_joint->SetupObject(info.This(), wrap_prismatic_jd, prismatic_joint);
			info.GetReturnValue().Set(h_prismatic_joint);
			break;
		}
		case e_distanceJoint:
		{
			WrapDistanceJointDef* wrap_distance_jd = WrapDistanceJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d distance joint
			b2DistanceJoint* distance_joint = static_cast<b2DistanceJoint*>(wrap->m_world.CreateJoint(&wrap_distance_jd->UseJointDef()));
			// create javascript distance joint object
			v8::Local<v8::Object> h_distance_joint = WrapDistanceJoint::NewInstance();
			WrapDistanceJoint* wrap_distance_joint = WrapDistanceJoint::Unwrap(h_distance_joint);
			// set up javascript distance joint object
			wrap_distance_joint->SetupObject(info.This(), wrap_distance_jd, distance_joint);
			info.GetReturnValue().Set(h_distance_joint);
			break;
		}
		case e_pulleyJoint:
		{
			WrapPulleyJointDef* wrap_pulley_jd = WrapPulleyJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d pulley joint
			b2PulleyJoint* pulley_joint = static_cast<b2PulleyJoint*>(wrap->m_world.CreateJoint(&wrap_pulley_jd->UseJointDef()));
			// create javascript pulley joint object
			v8::Local<v8::Object> h_pulley_joint = WrapPulleyJoint::NewInstance();
			WrapPulleyJoint* wrap_pulley_joint = WrapPulleyJoint::Unwrap(h_pulley_joint);
			// set up javascript pulley joint object
			wrap_pulley_joint->SetupObject(info.This(), wrap_pulley_jd, pulley_joint);
			info.GetReturnValue().Set(h_pulley_joint);
			break;
		}
		case e_mouseJoint:
		{
			WrapMouseJointDef* wrap_mouse_jd = WrapMouseJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d mouse joint
			b2MouseJoint* mouse_joint = static_cast<b2MouseJoint*>(wrap->m_world.CreateJoint(&wrap_mouse_jd->UseJointDef()));
			// create javascript mouse joint object
			v8::Local<v8::Object> h_mouse_joint = WrapMouseJoint::NewInstance();
			WrapMouseJoint* wrap_mouse_joint = WrapMouseJoint::Unwrap(h_mouse_joint);
			// set up javascript mouse joint object
			wrap_mouse_joint->SetupObject(info.This(), wrap_mouse_jd, mouse_joint);
			info.GetReturnValue().Set(h_mouse_joint);
			break;
		}
		case e_gearJoint:
		{
			WrapGearJointDef* wrap_gear_jd = WrapGearJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d gear joint
			b2GearJoint* gear_joint = static_cast<b2GearJoint*>(wrap->m_world.CreateJoint(&wrap_gear_jd->UseJointDef()));
			// create javascript gear joint object
			v8::Local<v8::Object> h_gear_joint = WrapGearJoint::NewInstance();
			WrapGearJoint* wrap_gear_joint = WrapGearJoint::Unwrap(h_gear_joint);
			// set up javascript gear joint object
			wrap_gear_joint->SetupObject(info.This(), wrap_gear_jd, gear_joint);
			info.GetReturnValue().Set(h_gear_joint);
			break;
		}
		case e_wheelJoint:
		{
			WrapWheelJointDef* wrap_wheel_jd = WrapWheelJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d wheel joint
			b2WheelJoint* wheel_joint = static_cast<b2WheelJoint*>(wrap->m_world.CreateJoint(&wrap_wheel_jd->UseJointDef()));
			// create javascript wheel joint object
			v8::Local<v8::Object> h_wheel_joint = WrapWheelJoint::NewInstance();
			WrapWheelJoint* wrap_wheel_joint = WrapWheelJoint::Unwrap(h_wheel_joint);
			// set up javascript wheel joint object
			wrap_wheel_joint->SetupObject(info.This(), wrap_wheel_jd, wheel_joint);
			info.GetReturnValue().Set(h_wheel_joint);
			break;
		}
	    case e_weldJoint:
		{
			WrapWeldJointDef* wrap_weld_jd = WrapWeldJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d weld joint
			b2WeldJoint* weld_joint = static_cast<b2WeldJoint*>(wrap->m_world.CreateJoint(&wrap_weld_jd->UseJointDef()));
			// create javascript weld joint object
			v8::Local<v8::Object> h_weld_joint = WrapWeldJoint::NewInstance();
			WrapWeldJoint* wrap_weld_joint = WrapWeldJoint::Unwrap(h_weld_joint);
			// set up javascript weld joint object
			wrap_weld_joint->SetupObject(info.This(), wrap_weld_jd, weld_joint);
			info.GetReturnValue().Set(h_weld_joint);
			break;
		}
		case e_frictionJoint:
		{
			WrapFrictionJointDef* wrap_friction_jd = WrapFrictionJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d friction joint
			b2FrictionJoint* friction_joint = static_cast<b2FrictionJoint*>(wrap->m_world.CreateJoint(&wrap_friction_jd->UseJointDef()));
			// create javascript friction joint object
			v8::Local<v8::Object> h_friction_joint = WrapFrictionJoint::NewInstance();
			WrapFrictionJoint* wrap_friction_joint = WrapFrictionJoint::Unwrap(h_friction_joint);
			// set up javascript friction joint object
			wrap_friction_joint->SetupObject(info.This(), wrap_friction_jd, friction_joint);
			info.GetReturnValue().Set(h_friction_joint);
			break;
		}
		case e_ropeJoint:
		{
			WrapRopeJointDef* wrap_rope_jd = WrapRopeJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d rope joint
			b2RopeJoint* rope_joint = static_cast<b2RopeJoint*>(wrap->m_world.CreateJoint(&wrap_rope_jd->UseJointDef()));
			// create javascript rope joint object
			v8::Local<v8::Object> h_rope_joint = WrapRopeJoint::NewInstance();
			WrapRopeJoint* wrap_rope_joint = WrapRopeJoint::Unwrap(h_rope_joint);
			// set up javascript rope joint object
			wrap_rope_joint->SetupObject(info.This(), wrap_rope_jd, rope_joint);
			info.GetReturnValue().Set(h_rope_joint);
			break;
		}
		case e_motorJoint:
		{
			WrapMotorJointDef* wrap_motor_jd = WrapMotorJointDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
			// create box2d motor joint
			b2MotorJoint* motor_joint = static_cast<b2MotorJoint*>(wrap->m_world.CreateJoint(&wrap_motor_jd->UseJointDef()));
			// create javascript motor joint object
			v8::Local<v8::Object> h_motor_joint = WrapMotorJoint::NewInstance();
			WrapMotorJoint* wrap_motor_joint = WrapMotorJoint::Unwrap(h_motor_joint);
			// set up javascript motor joint object
			wrap_motor_joint->SetupObject(info.This(), wrap_motor_jd, motor_joint);
			info.GetReturnValue().Set(h_motor_joint);
			break;
		}
		}
	}
	NANX_METHOD(DestroyJoint)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Object> h_joint = v8::Local<v8::Object>::Cast(info[0]);
		WrapJoint* wrap_joint = WrapJoint::Unwrap(h_joint);
		switch (wrap_joint->GetJoint()->GetType())
		{
		case e_unknownJoint:
			break;
		case e_revoluteJoint:
		{
			v8::Local<v8::Object> h_revolute_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapRevoluteJoint* wrap_revolute_joint = WrapRevoluteJoint::Unwrap(h_revolute_joint);
			// delete box2d revolute joint
			wrap->m_world.DestroyJoint(wrap_revolute_joint->GetJoint());
			// reset javascript revolute joint object
			wrap_revolute_joint->ResetObject();
			break;
		}
		case e_prismaticJoint:
		{
			v8::Local<v8::Object> h_prismatic_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapPrismaticJoint* wrap_prismatic_joint = WrapPrismaticJoint::Unwrap(h_prismatic_joint);
			// delete box2d prismatic joint
			wrap->m_world.DestroyJoint(wrap_prismatic_joint->GetJoint());
			// reset javascript prismatic joint object
			wrap_prismatic_joint->ResetObject();
			break;
		}
		case e_distanceJoint:
		{
			v8::Local<v8::Object> h_distance_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapDistanceJoint* wrap_distance_joint = WrapDistanceJoint::Unwrap(h_distance_joint);
			// delete box2d distance joint
			wrap->m_world.DestroyJoint(wrap_distance_joint->GetJoint());
			// reset javascript distance joint object
			wrap_distance_joint->ResetObject();
			break;
		}
		case e_pulleyJoint:
		{
			v8::Local<v8::Object> h_pulley_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapPulleyJoint* wrap_pulley_joint = WrapPulleyJoint::Unwrap(h_pulley_joint);
			// delete box2d pulley joint
			wrap->m_world.DestroyJoint(wrap_pulley_joint->GetJoint());
			// reset javascript pulley joint object
			wrap_pulley_joint->ResetObject();
			break;
		}
		case e_mouseJoint:
		{
			v8::Local<v8::Object> h_mouse_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapMouseJoint* wrap_mouse_joint = WrapMouseJoint::Unwrap(h_mouse_joint);
			// delete box2d mouse joint
			wrap->m_world.DestroyJoint(wrap_mouse_joint->GetJoint());
			// reset javascript mouse joint object
			wrap_mouse_joint->ResetObject();
			break;
		}
		case e_gearJoint:
		{
			v8::Local<v8::Object> h_gear_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapGearJoint* wrap_gear_joint = WrapGearJoint::Unwrap(h_gear_joint);
			// delete box2d gear joint
			wrap->m_world.DestroyJoint(wrap_gear_joint->GetJoint());
			// reset javascript gear joint object
			wrap_gear_joint->ResetObject();
			break;
		}
		case e_wheelJoint:
		{
			v8::Local<v8::Object> h_wheel_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapWheelJoint* wrap_wheel_joint = WrapWheelJoint::Unwrap(h_wheel_joint);
			// delete box2d wheel joint
			wrap->m_world.DestroyJoint(wrap_wheel_joint->GetJoint());
			// reset javascript wheel joint object
			wrap_wheel_joint->ResetObject();
			break;
		}
	    case e_weldJoint:
		{
			v8::Local<v8::Object> h_weld_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapWeldJoint* wrap_weld_joint = WrapWeldJoint::Unwrap(h_weld_joint);
			// delete box2d weld joint
			wrap->m_world.DestroyJoint(wrap_weld_joint->GetJoint());
			// reset javascript weld joint object
			wrap_weld_joint->ResetObject();
			break;
		}
		case e_frictionJoint:
		{
			v8::Local<v8::Object> h_friction_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapFrictionJoint* wrap_friction_joint = WrapFrictionJoint::Unwrap(h_friction_joint);
			// delete box2d friction joint
			wrap->m_world.DestroyJoint(wrap_friction_joint->GetJoint());
			// reset javascript friction joint object
			wrap_friction_joint->ResetObject();
			break;
		}
		case e_ropeJoint:
		{
			v8::Local<v8::Object> h_rope_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapRopeJoint* wrap_rope_joint = WrapRopeJoint::Unwrap(h_rope_joint);
			// delete box2d rope joint
			wrap->m_world.DestroyJoint(wrap_rope_joint->GetJoint());
			// reset javascript rope joint object
			wrap_rope_joint->ResetObject();
			break;
		}
		case e_motorJoint:
		{
			v8::Local<v8::Object> h_motor_joint = v8::Local<v8::Object>::Cast(info[0]);
			WrapMotorJoint* wrap_motor_joint = WrapMotorJoint::Unwrap(h_motor_joint);
			// delete box2d motor joint
			wrap->m_world.DestroyJoint(wrap_motor_joint->GetJoint());
			// reset javascript motor joint object
			wrap_motor_joint->ResetObject();
			break;
		}
		default:
			break;
		}
	}
	NANX_METHOD(Step)
	{
		WrapWorld* wrap = Unwrap(info.This());
		float32 timeStep = NANX_float32(info[0]);
		int32 velocityIterations = NANX_int32(info[1]);
		int32 positionIterations = NANX_int32(info[2]);
		#if B2_ENABLE_PARTICLE
		int32 particleIterations = (info.Length() > 3)?(NANX_int32(info[3])):(wrap->m_world.CalculateReasonableParticleIterations(timeStep));
		wrap->m_world.Step(timeStep, velocityIterations, positionIterations, particleIterations);
		#else
		wrap->m_world.Step(timeStep, velocityIterations, positionIterations);
		#endif
	}
	NANX_METHOD(ClearForces)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.ClearForces();
	}
	NANX_METHOD(DrawDebugData)
	{
		WrapWorld* wrap = Unwrap(info.This());
		if (!wrap->m_draw.IsEmpty())
		{
			//v8::Local<v8::Object> h_draw = Nan::New<v8::Object>(wrap->m_draw);
			//WrapDraw* wrap_draw = WrapDraw::Unwrap(h_draw);
			//wrap->m_world.SetDebugDraw(&wrap_draw->m_wrap_draw);
			//wrap->m_world.DrawDebugData();
			//wrap->m_world.SetDebugDraw(NULL);
			v8::Local<v8::Object> h_draw = Nan::New<v8::Object>(wrap->m_draw);
			v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_draw->Get(NANX_SYMBOL("GetFlags")));
			uint32 flags = NANX_uint32(Nan::MakeCallback(h_draw, h_method, 0, NULL));
			wrap->m_wrap_draw.SetFlags(flags);
			wrap->m_world.DrawDebugData();
		}
	}
	NANX_METHOD(QueryAABB)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[0]);
		WrapAABB* wrap_mass_data = WrapAABB::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapQueryCallback wrap_callback(callback);
		wrap->m_world.QueryAABB(&wrap_callback, wrap_mass_data->GetAABB());
	}
	#if B2_ENABLE_PARTICLE
	NANX_METHOD(QueryShapeAABB)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[0]);
		WrapShape* wrap_shape = WrapShape::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	    WrapTransform* wrap_transform = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapQueryCallback wrap_callback(callback);
		wrap->m_world.QueryShapeAABB(&wrap_callback, wrap_shape->GetShape(), wrap_transform->GetTransform());
	}
	#endif
	NANX_METHOD(RayCast)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(info[0]);
		WrapVec2* point1 = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
		WrapVec2* point2 = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
		WrapRayCastCallback wrap_callback(callback);
		wrap->m_world.RayCast(&wrap_callback, point1->GetVec2(), point2->GetVec2());
	}
	NANX_METHOD(GetBodyList)
	{
		WrapWorld* wrap = Unwrap(info.This());
		b2Body* body = wrap->m_world.GetBodyList();
		if (body)
		{
			// get body internal data
			WrapBody* wrap_body = WrapBody::GetWrap(body);
			info.GetReturnValue().Set(wrap_body->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetJointList)
	{
		WrapWorld* wrap = Unwrap(info.This());
		b2Joint* joint = wrap->m_world.GetJointList();
		if (joint)
		{
			// get joint internal data
			WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
			info.GetReturnValue().Set(wrap_joint->handle());
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(GetContactList)
	{
		WrapWorld* wrap = Unwrap(info.This());
		b2Contact* contact = wrap->m_world.GetContactList();
		if (contact)
		{
			info.GetReturnValue().Set(WrapContact::NewInstance(contact));
		}
		else
		{
			info.GetReturnValue().SetNull();
		}
	}
	NANX_METHOD(SetAllowSleeping)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.SetAllowSleeping(NANX_bool(info[0]));
	}
	NANX_METHOD(GetAllowSleeping)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetAllowSleeping()));
	}
	NANX_METHOD(SetWarmStarting)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.SetWarmStarting(NANX_bool(info[0]));
	}
	NANX_METHOD(GetWarmStarting)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetWarmStarting()));
	}
	NANX_METHOD(SetContinuousPhysics)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.SetContinuousPhysics(NANX_bool(info[0]));
	}
	NANX_METHOD(GetContinuousPhysics)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetContinuousPhysics()));
	}
	NANX_METHOD(SetSubStepping)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.SetSubStepping(NANX_bool(info[0]));
	}
	NANX_METHOD(GetSubStepping)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetSubStepping()));
	}
//	int32 GetProxyCount() const;
	NANX_METHOD(GetBodyCount)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetBodyCount()));
	}
	NANX_METHOD(GetJointCount)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetJointCount()));
	}
	NANX_METHOD(GetContactCount)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetContactCount()));
	}
//	int32 GetTreeHeight() const;
//	int32 GetTreeBalance() const;
//	float32 GetTreeQuality() const;
	NANX_METHOD(SetGravity)
	{
		WrapWorld* wrap = Unwrap(info.This());
		WrapVec2* wrap_gravity = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		wrap->m_world.SetGravity(wrap_gravity->GetVec2());
	}
	NANX_METHOD(GetGravity)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(WrapVec2::NewInstance(wrap->m_world.GetGravity()));
	}
	NANX_METHOD(IsLocked)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.IsLocked()));
	}
	NANX_METHOD(SetAutoClearForces)
	{
		WrapWorld* wrap = Unwrap(info.This());
		wrap->m_world.SetAutoClearForces(NANX_bool(info[0]));
	}
	NANX_METHOD(GetAutoClearForces)
	{
		WrapWorld* wrap = Unwrap(info.This());
		info.GetReturnValue().Set(Nan::New(wrap->m_world.GetAutoClearForces()));
	}
//	void ShiftOrigin(const b2Vec2& newOrigin);
///	const b2ContactManager& GetContactManager() const;
///	const b2Profile& GetProfile() const;
///	void Dump();
	#if B2_ENABLE_PARTICLE
	NANX_METHOD(CreateParticleSystem)
	{
		WrapWorld* wrap = Unwrap(info.This());
		WrapParticleSystemDef* wrap_psd = WrapParticleSystemDef::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
		// create box2d particle system
		b2ParticleSystem* system = wrap->m_world.CreateParticleSystem(&wrap_psd->UseParticleSystemDef());
		// create javascript particle system object
		v8::Local<v8::Object> h_particle_system = WrapParticleSystem::NewInstance();
		WrapParticleSystem* wrap_particle_system = WrapParticleSystem::Unwrap(h_particle_system);
		// set up javascript particle system object
		wrap_particle_system->SetupObject(info.This(), wrap_psd, system);
		info.GetReturnValue().Set(h_particle_system);
	}
	NANX_METHOD(DestroyParticleSystem)
	{
		WrapWorld* wrap = Unwrap(info.This());
		v8::Local<v8::Object> h_particle_system = v8::Local<v8::Object>::Cast(info[0]);
		WrapParticleSystem* wrap_particle_system = WrapParticleSystem::Unwrap(h_particle_system);
		// delete box2d particle system
		wrap->m_world.DestroyParticleSystem(wrap_particle_system->GetParticleSystem());
		// reset javascript system object
		wrap_particle_system->ResetObject();
	}
	NANX_METHOD(CalculateReasonableParticleIterations)
	{
		WrapWorld* wrap = Unwrap(info.This());
		float32 timeStep = NANX_float32(info[0]);
		info.GetReturnValue().Set(Nan::New(wrap->m_world.CalculateReasonableParticleIterations(timeStep)));
	}
	#endif
};

void WrapWorld::WrapDestructionListener::SayGoodbye(b2Joint* joint)
{
	if (!m_wrap_world->m_destruction_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_destruction_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("SayGoodbyeJoint")));
		// get joint internal data
		WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
		v8::Local<v8::Object> h_joint = wrap_joint->handle();
		v8::Local<v8::Value> argv[] = { h_joint };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDestructionListener::SayGoodbye(b2Fixture* fixture)
{
	if (!m_wrap_world->m_destruction_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_destruction_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("SayGoodbyeFixture")));
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		v8::Local<v8::Object> h_fixture = wrap_fixture->handle();
		v8::Local<v8::Value> argv[] = { h_fixture };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if B2_ENABLE_PARTICLE

void WrapWorld::WrapDestructionListener::SayGoodbye(b2ParticleGroup* group)
{
	if (!m_wrap_world->m_destruction_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_destruction_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("SayGoodbyeParticleGroup")));
		// get particle group internal data
		WrapParticleGroup* wrap_group = WrapParticleGroup::GetWrap(group);
		v8::Local<v8::Object> h_group = wrap_group->handle();
		v8::Local<v8::Value> argv[] = { h_group };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDestructionListener::SayGoodbye(b2ParticleSystem* particleSystem, int32 index)
{
	if (!m_wrap_world->m_destruction_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_destruction_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("SayGoodbyeParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = static_cast<WrapParticleSystem*>(particleSystem->GetUserData());
		///	v8::Local<v8::Object> h_system = wrap_system->handle();
		///	v8::Local<v8::Value> argv[] = { h_system, Nan::New(index) };
		///	Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#endif

bool WrapWorld::WrapContactFilter::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
	if (!m_wrap_world->m_contact_filter.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_filter);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("ShouldCollide")));
		// get fixture internal data
		WrapFixture* wrap_fixtureA = WrapFixture::GetWrap(fixtureA);
		WrapFixture* wrap_fixtureB = WrapFixture::GetWrap(fixtureB);
		v8::Local<v8::Object> h_fixtureA = wrap_fixtureA->handle();
		v8::Local<v8::Object> h_fixtureB = wrap_fixtureB->handle();
		v8::Local<v8::Value> argv[] = { h_fixtureA, h_fixtureB };
		return NANX_bool(Nan::MakeCallback(h_that, h_method, countof(argv), argv));
	}
	return b2ContactFilter::ShouldCollide(fixtureA, fixtureB);
}

#if B2_ENABLE_PARTICLE

bool WrapWorld::WrapContactFilter::ShouldCollide(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 particleIndex)
{
	if (!m_wrap_world->m_contact_filter.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_filter);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("ShouldCollideFixtureParticle")));
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		v8::Local<v8::Object> h_fixture = wrap_fixture->handle();
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Object> h_system = wrap_system->handle();
		///	v8::Local<v8::Value> argv[] = { h_fixture, h_system, Nan::New(particleIndex) };
		///	return NANX_bool(Nan::MakeCallback(h_that, h_method, countof(argv), argv));
	}
	return b2ContactFilter::ShouldCollide(fixture, particleSystem, particleIndex);
}

bool WrapWorld::WrapContactFilter::ShouldCollide(b2ParticleSystem* particleSystem, int32 particleIndexA, int32 particleIndexB)
{
	if (!m_wrap_world->m_contact_filter.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_filter);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("ShouldCollideParticleParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Object> h_system = wrap_system->handle();
		///	v8::Local<v8::Value> argv[] = { h_system, Nan::New(particleIndexA), Nan::New(particleIndexB) };
		///	return NANX_bool(Nan::MakeCallback(h_that, h_method, countof(argv), argv));
	}
	return b2ContactFilter::ShouldCollide(particleSystem, particleIndexA, particleIndexB);
}

#endif

void WrapWorld::WrapContactListener::BeginContact(b2Contact* contact)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("BeginContact")));
		v8::Local<v8::Object> h_contact = WrapContact::NewInstance(contact);
		v8::Local<v8::Value> argv[] = { h_contact };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2Contact* contact)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("EndContact")));
		v8::Local<v8::Object> h_contact = WrapContact::NewInstance(contact);
		v8::Local<v8::Value> argv[] = { h_contact };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if B2_ENABLE_PARTICLE

void WrapWorld::WrapContactListener::BeginContact(b2ParticleSystem* particleSystem, b2ParticleBodyContact* particleBodyContact)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("BeginContactFixtureParticle")));
		// TODO: get particle system internal data, wrap b2ParticleBodyContact
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Value> argv[] = {};
		///	Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 index)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("EndContactFixtureParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Value> argv[] = {};
		///	Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::BeginContact(b2ParticleSystem* particleSystem, b2ParticleContact* particleContact)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("BeginContactParticleParticle")));
		// TODO: get particle system internal data, wrap b2ParticleContact
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Value> argv[] = {};
		///	Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2ParticleSystem* particleSystem, int32 indexA, int32 indexB)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("EndContactParticleParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = WrapParticleSystem::GetWrap(particleSystem);
		///	v8::Local<v8::Value> argv[] = {};
		///	Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#endif

void WrapWorld::WrapContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("PreSolve")));
		v8::Local<v8::Object> h_contact = WrapContact::NewInstance(contact);
		v8::Local<v8::Object> h_oldManifold = WrapManifold::NewInstance(*oldManifold);
		v8::Local<v8::Value> argv[] = { h_contact, h_oldManifold };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
	if (!m_wrap_world->m_contact_listener.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_contact_listener);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("PostSolve")));
		v8::Local<v8::Object> h_contact = WrapContact::NewInstance(contact);
		v8::Local<v8::Object> h_impulse = WrapContactImpulse::NewInstance(*impulse);
		v8::Local<v8::Value> argv[] = { h_contact, h_impulse };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawPolygon")));
		v8::Local<v8::Array> h_vertices = Nan::New<v8::Array>(vertexCount);
		for (int i = 0; i < vertexCount; ++i)
		{
			h_vertices->Set(i, WrapVec2::NewInstance(vertices[i]));
		}
		v8::Local<v8::Integer> h_vertexCount = Nan::New(vertexCount);
		v8::Local<v8::Object> h_color = WrapColor::NewInstance(color);
		v8::Local<v8::Value> argv[] = { h_vertices, h_vertexCount, h_color };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawSolidPolygon")));
		v8::Local<v8::Array> h_vertices = Nan::New<v8::Array>(vertexCount);
		for (int i = 0; i < vertexCount; ++i)
		{
			h_vertices->Set(i, WrapVec2::NewInstance(vertices[i]));
		}
		v8::Local<v8::Integer> h_vertexCount = Nan::New(vertexCount);
		v8::Local<v8::Object> h_color = WrapColor::NewInstance(color);
		v8::Local<v8::Value> argv[] = { h_vertices, h_vertexCount, h_color };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawCircle")));
		v8::Local<v8::Object> h_center = WrapVec2::NewInstance(center);
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		v8::Local<v8::Number> h_radius = Nan::New(radius);
		v8::Local<v8::Object> h_color = WrapColor::NewInstance(color);
		v8::Local<v8::Value> argv[] = { h_center, h_radius, h_color };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawSolidCircle")));
		v8::Local<v8::Object> h_center = WrapVec2::NewInstance(center);
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		v8::Local<v8::Number> h_radius = Nan::New(radius);
		v8::Local<v8::Object> h_axis = WrapVec2::NewInstance(axis);
		v8::Local<v8::Object> h_color = WrapColor::NewInstance(color);
		v8::Local<v8::Value> argv[] = { h_center, h_radius, h_axis, h_color };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if B2_ENABLE_PARTICLE
void WrapWorld::WrapDraw::DrawParticles(const b2Vec2 *centers, float32 radius, const b2ParticleColor *colors, int32 count)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawParticles")));
		v8::Local<v8::Array> h_centers = Nan::New<v8::Array>(count);
		for (int i = 0; i < count; ++i)
		{
			h_centers->Set(i, WrapVec2::NewInstance(centers[i]));
		}
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		v8::Local<v8::Number> h_radius = Nan::New(radius);
		v8::Local<v8::Integer> h_count = Nan::New(count);
		if (colors != NULL)
		{
			v8::Local<v8::Array> h_colors = Nan::New<v8::Array>(count);
			for (int i = 0; i < count; ++i)
			{
				h_colors->Set(i, WrapParticleColor::NewInstance(colors[i]));
			}
			v8::Local<v8::Value> argv[] = { h_centers, h_radius, h_colors, h_count };
			Nan::MakeCallback(h_that, h_method, countof(argv), argv);
		}
		else
		{
			v8::Local<v8::Value> argv[] = { h_centers, h_radius, Nan::Null(), h_count };
			Nan::MakeCallback(h_that, h_method, countof(argv), argv);
		}
	}
}
#endif

void WrapWorld::WrapDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawSegment")));
		v8::Local<v8::Object> h_p1 = WrapVec2::NewInstance(p1);
		v8::Local<v8::Object> h_p2 = WrapVec2::NewInstance(p2);
		v8::Local<v8::Object> h_color = WrapColor::NewInstance(color);
		v8::Local<v8::Value> argv[] = { h_p1, h_p2, h_color };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawTransform(const b2Transform& xf)
{
	if (!m_wrap_world->m_draw.IsEmpty())
	{
		v8::Local<v8::Object> h_that = Nan::New<v8::Object>(m_wrap_world->m_draw);
		v8::Local<v8::Function> h_method = v8::Local<v8::Function>::Cast(h_that->Get(NANX_SYMBOL("DrawTransform")));
		v8::Local<v8::Object> h_xf = WrapTransform::NewInstance(xf);
		v8::Local<v8::Value> argv[] = { h_xf };
		Nan::MakeCallback(h_that, h_method, countof(argv), argv);
	}
}

////

NANX_EXPORT(b2Distance)
{
	WrapVec2* a = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* b = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	info.GetReturnValue().Set(Nan::New(b2Distance(a->GetVec2(), b->GetVec2())));
}

NANX_EXPORT(b2DistanceSquared)
{
	WrapVec2* a = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* b = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	info.GetReturnValue().Set(Nan::New(b2DistanceSquared(a->GetVec2(), b->GetVec2())));
}

NANX_EXPORT(b2Dot_V2_V2)
{
	WrapVec2* a = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* b = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	info.GetReturnValue().Set(Nan::New(b2Dot(a->GetVec2(), b->GetVec2())));
}

NANX_EXPORT(b2Add_V2_V2)
{
	WrapVec2* a = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* b = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapVec2* out = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetVec2(a->GetVec2() + b->GetVec2());
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2Sub_V2_V2)
{
	WrapVec2* a = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* b = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapVec2* out = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetVec2(a->GetVec2() - b->GetVec2());
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2Mul_X_V2)
{
	WrapTransform* T = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* v = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapVec2* out = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetVec2(b2Mul(T->GetTransform(), v->GetVec2()));
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2MulT_X_V2)
{
	WrapTransform* T = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapVec2* v = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapVec2* out = WrapVec2::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetVec2(b2MulT(T->GetTransform(), v->GetVec2()));
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2Mul_X_X)
{
	WrapTransform* A = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapTransform* B = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapTransform* out = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetTransform(b2Mul(A->GetTransform(), B->GetTransform()));
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2MulT_X_X)
{
	WrapTransform* A = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
	WrapTransform* B = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[1]));
	WrapTransform* out = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	out->SetTransform(b2MulT(A->GetTransform(), B->GetTransform()));
	info.GetReturnValue().Set(info[2]);
}

NANX_EXPORT(b2GetPointStates)
{
	v8::Local<v8::Array> wrap_state1 = v8::Local<v8::Array>::Cast(info[0]);
	v8::Local<v8::Array> wrap_state2 = v8::Local<v8::Array>::Cast(info[1]);
	WrapManifold* wrap_manifold1 = WrapManifold::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
	WrapManifold* wrap_manifold2 = WrapManifold::Unwrap(v8::Local<v8::Object>::Cast(info[3]));
	b2PointState state1[b2_maxManifoldPoints];
	b2PointState state2[b2_maxManifoldPoints];
	b2GetPointStates(state1, state2, &wrap_manifold1->m_manifold, &wrap_manifold2->m_manifold);
	for (int i = 0; i < b2_maxManifoldPoints; ++i)
	{
		wrap_state1->Set(i, Nan::New(state1[i]));
		wrap_state2->Set(i, Nan::New(state2[i]));
	}
}

NANX_EXPORT(b2TestOverlap_AABB)
{
	// TODO
	info.GetReturnValue().Set(Nan::New(false));
}

NANX_EXPORT(b2TestOverlap_Shape)
{
	WrapShape* wrap_shapeA = WrapShape::Unwrap(v8::Local<v8::Object>::Cast(info[0]));
    int32 indexA = NANX_int32(info[1]);
	WrapShape* wrap_shapeB = WrapShape::Unwrap(v8::Local<v8::Object>::Cast(info[2]));
    int32 indexB = NANX_int32(info[3]);
    WrapTransform* wrap_transformA = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[4]));
    WrapTransform* wrap_transformB = WrapTransform::Unwrap(v8::Local<v8::Object>::Cast(info[5]));
    const bool overlap = b2TestOverlap(&wrap_shapeA->GetShape(), indexA, &wrap_shapeB->GetShape(), indexB, wrap_transformA->GetTransform(), wrap_transformB->GetTransform());
	info.GetReturnValue().Set(Nan::New(overlap));
}

#if B2_ENABLE_PARTICLE

NANX_EXPORT(b2CalculateParticleIterations)
{
	float32 gravity = NANX_float32(info[0]);
	float32 radius = NANX_float32(info[1]);
	float32 timeStep = NANX_float32(info[2]);
	info.GetReturnValue().Set(Nan::New(::b2CalculateParticleIterations(gravity, radius, timeStep)));
}

#endif

////

NAN_MODULE_INIT(init)
{
	NANX_CONSTANT(target, b2_maxFloat);
	NANX_CONSTANT(target, b2_epsilon);
	NANX_CONSTANT(target, b2_pi);
	NANX_CONSTANT(target, b2_maxManifoldPoints);
	NANX_CONSTANT(target, b2_maxPolygonVertices);
	NANX_CONSTANT(target, b2_aabbExtension);
	NANX_CONSTANT(target, b2_aabbMultiplier);
	NANX_CONSTANT(target, b2_linearSlop);
	NANX_CONSTANT(target, b2_angularSlop);
	NANX_CONSTANT(target, b2_polygonRadius);
	NANX_CONSTANT(target, b2_maxSubSteps);
	NANX_CONSTANT(target, b2_maxTOIContacts);
	NANX_CONSTANT(target, b2_velocityThreshold);
	NANX_CONSTANT(target, b2_maxLinearCorrection);
	NANX_CONSTANT(target, b2_maxAngularCorrection);
	NANX_CONSTANT(target, b2_maxTranslation);
	NANX_CONSTANT(target, b2_maxTranslationSquared);
	NANX_CONSTANT(target, b2_maxRotation);
	NANX_CONSTANT(target, b2_maxRotationSquared);
	NANX_CONSTANT(target, b2_baumgarte);
	NANX_CONSTANT(target, b2_toiBaugarte);
	NANX_CONSTANT(target, b2_timeToSleep);
	NANX_CONSTANT(target, b2_linearSleepTolerance);
	NANX_CONSTANT(target, b2_angularSleepTolerance);

	v8::Local<v8::Object> version = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2_version"), version);
	version->Set(NANX_SYMBOL("major"), Nan::New(b2_version.major));
	version->Set(NANX_SYMBOL("minor"), Nan::New(b2_version.minor));
	version->Set(NANX_SYMBOL("revision"), Nan::New(b2_version.revision));

	v8::Local<v8::Object> WrapShapeType = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2ShapeType"), WrapShapeType);
	NANX_CONSTANT_VALUE(WrapShapeType, e_unknownShape, -1);
	NANX_CONSTANT_VALUE(WrapShapeType, e_circleShape, b2Shape::e_circle);
	NANX_CONSTANT_VALUE(WrapShapeType, e_edgeShape, b2Shape::e_edge);
	NANX_CONSTANT_VALUE(WrapShapeType, e_polygonShape, b2Shape::e_polygon);
	NANX_CONSTANT_VALUE(WrapShapeType, e_chainShape, b2Shape::e_chain);
	NANX_CONSTANT_VALUE(WrapShapeType, e_shapeTypeCount, b2Shape::e_typeCount);

	v8::Local<v8::Object> WrapBodyType = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2BodyType"), WrapBodyType);
	NANX_CONSTANT(WrapBodyType, b2_staticBody);
	NANX_CONSTANT(WrapBodyType, b2_kinematicBody);
	NANX_CONSTANT(WrapBodyType, b2_dynamicBody);

	v8::Local<v8::Object> WrapJointType = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2JointType"), WrapJointType);
	NANX_CONSTANT(WrapJointType, e_unknownJoint);
	NANX_CONSTANT(WrapJointType, e_revoluteJoint);
	NANX_CONSTANT(WrapJointType, e_prismaticJoint);
	NANX_CONSTANT(WrapJointType, e_distanceJoint);
	NANX_CONSTANT(WrapJointType, e_pulleyJoint);
	NANX_CONSTANT(WrapJointType, e_mouseJoint);
	NANX_CONSTANT(WrapJointType, e_gearJoint);
	NANX_CONSTANT(WrapJointType, e_wheelJoint);
	NANX_CONSTANT(WrapJointType, e_weldJoint);
	NANX_CONSTANT(WrapJointType, e_frictionJoint);
	NANX_CONSTANT(WrapJointType, e_ropeJoint);
	NANX_CONSTANT(WrapJointType, e_motorJoint);

	v8::Local<v8::Object> WrapLimitState = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2LimitState"), WrapLimitState);
	NANX_CONSTANT(WrapLimitState, e_inactiveLimit);
	NANX_CONSTANT(WrapLimitState, e_atLowerLimit);
	NANX_CONSTANT(WrapLimitState, e_atUpperLimit);
	NANX_CONSTANT(WrapLimitState, e_equalLimits);

	v8::Local<v8::Object> WrapManifoldType = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2ManifoldType"), WrapManifoldType);
	NANX_CONSTANT_VALUE(WrapManifoldType, e_circles, b2Manifold::e_circles);
	NANX_CONSTANT_VALUE(WrapManifoldType, e_faceA, b2Manifold::e_faceA);
	NANX_CONSTANT_VALUE(WrapManifoldType, e_faceB, b2Manifold::e_faceB);

	v8::Local<v8::Object> WrapPointState = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2PointState"), WrapPointState);
	NANX_CONSTANT(WrapPointState, b2_nullState);
	NANX_CONSTANT(WrapPointState, b2_addState);
	NANX_CONSTANT(WrapPointState, b2_persistState);
	NANX_CONSTANT(WrapPointState, b2_removeState);

	v8::Local<v8::Object> WrapDrawFlags = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2DrawFlags"), WrapDrawFlags);
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_shapeBit, b2Draw::e_shapeBit);
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_jointBit, b2Draw::e_jointBit);
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_aabbBit, b2Draw::e_aabbBit);
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_pairBit, b2Draw::e_pairBit);
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_centerOfMassBit, b2Draw::e_centerOfMassBit);
	#if B2_ENABLE_PARTICLE
	NANX_CONSTANT_VALUE(WrapDrawFlags, e_particleBit, b2Draw::e_particleBit);
	#endif

	#if B2_ENABLE_PARTICLE

	v8::Local<v8::Object> WrapParticleFlag = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2ParticleFlag"), WrapParticleFlag);
	NANX_CONSTANT(WrapParticleFlag, b2_waterParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_zombieParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_wallParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_springParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_elasticParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_viscousParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_powderParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_tensileParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_colorMixingParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_destructionListenerParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_barrierParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_staticPressureParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_reactiveParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_repulsiveParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_fixtureContactListenerParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_particleContactListenerParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_fixtureContactFilterParticle);
	NANX_CONSTANT(WrapParticleFlag, b2_particleContactFilterParticle);

	v8::Local<v8::Object> WrapParticleGroupFlag = Nan::New<v8::Object>();
	Nan::Set(target, NANX_SYMBOL("b2ParticleGroupFlag"), WrapParticleGroupFlag);
	NANX_CONSTANT(WrapParticleGroupFlag, b2_solidParticleGroup);
	NANX_CONSTANT(WrapParticleGroupFlag, b2_rigidParticleGroup);
	NANX_CONSTANT(WrapParticleGroupFlag, b2_particleGroupCanBeEmpty);
	NANX_CONSTANT(WrapParticleGroupFlag, b2_particleGroupWillBeDestroyed);
	NANX_CONSTANT(WrapParticleGroupFlag, b2_particleGroupNeedsUpdateDepth);

	#endif

	WrapVec2::Init(target);
	WrapRot::Init(target);
	WrapTransform::Init(target);
	WrapAABB::Init(target);
	WrapMassData::Init(target);
	//WrapShape::Init(target);
	WrapCircleShape::Init(target);
	WrapEdgeShape::Init(target);
	WrapPolygonShape::Init(target);
	WrapChainShape::Init(target);
	WrapFilter::Init(target);
	WrapFixtureDef::Init(target);
	WrapFixture::Init(target);
	WrapBodyDef::Init(target);
	WrapBody::Init(target);
	WrapJointDef::Init(target);
	WrapJoint::Init(target);
	WrapRevoluteJointDef::Init(target);
	WrapRevoluteJoint::Init(target);
	WrapPrismaticJointDef::Init(target);
	WrapPrismaticJoint::Init(target);
	WrapDistanceJointDef::Init(target);
	WrapDistanceJoint::Init(target);
	WrapPulleyJointDef::Init(target);
	WrapPulleyJoint::Init(target);
	WrapMouseJointDef::Init(target);
	WrapMouseJoint::Init(target);
	WrapGearJointDef::Init(target);
	WrapGearJoint::Init(target);
	WrapWheelJointDef::Init(target);
	WrapWheelJoint::Init(target);
	WrapWeldJointDef::Init(target);
	WrapWeldJoint::Init(target);
	WrapFrictionJointDef::Init(target);
	WrapFrictionJoint::Init(target);
	WrapRopeJointDef::Init(target);
	WrapRopeJoint::Init(target);
	WrapMotorJointDef::Init(target);
	WrapMotorJoint::Init(target);
	WrapContactID::Init(target);
	WrapManifoldPoint::Init(target);
	WrapManifold::Init(target);
	WrapWorldManifold::Init(target);
	WrapContact::Init(target);
	WrapContactImpulse::Init(target);
	WrapColor::Init(target);
	#if 0
	WrapDraw::Init(target);
	#endif
	#if B2_ENABLE_PARTICLE
	WrapParticleColor::Init(target);
	WrapParticleDef::Init(target);
	WrapParticleHandle::Init(target);
	WrapParticleGroupDef::Init(target);
	WrapParticleGroup::Init(target);
	WrapParticleSystemDef::Init(target);
	WrapParticleSystem::Init(target);
	#endif
	WrapWorld::Init(target);

	NANX_EXPORT_APPLY(target, b2Distance);
	NANX_EXPORT_APPLY(target, b2DistanceSquared);
	NANX_EXPORT_APPLY(target, b2Dot_V2_V2);
	NANX_EXPORT_APPLY(target, b2Add_V2_V2);
	NANX_EXPORT_APPLY(target, b2Sub_V2_V2);
	NANX_EXPORT_APPLY(target, b2Mul_X_V2);
	NANX_EXPORT_APPLY(target, b2MulT_X_V2);
	NANX_EXPORT_APPLY(target, b2Mul_X_X);
	NANX_EXPORT_APPLY(target, b2MulT_X_X);

	NANX_EXPORT_APPLY(target, b2GetPointStates);

	NANX_EXPORT_APPLY(target, b2TestOverlap_AABB);
	NANX_EXPORT_APPLY(target, b2TestOverlap_Shape);

	#if B2_ENABLE_PARTICLE
	NANX_EXPORT_APPLY(target, b2CalculateParticleIterations);
	#endif
}

////

} // namespace node_box2d

NODE_MODULE(node_box2d, node_box2d::init);
