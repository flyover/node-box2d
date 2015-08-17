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

#include <node.h>
#include <v8.h>
#include <nan.h>
#include <Box2D/Box2D.h>

///#include <map>

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "printf", __VA_ARGS__)
#endif

#define countof(_a) (sizeof(_a)/sizeof((_a)[0]))

// macros for modules

#define MODULE_CONSTANT(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_VALUE(target, constant, value) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew(value), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_NUMBER(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew<v8::Number>(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_CONSTANT_STRING(target, constant) \
	(target)->ForceSet(NanNew<v8::String>(#constant), NanNew<v8::String>(constant), static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

#define MODULE_EXPORT_APPLY(target, name) NODE_SET_METHOD(target, #name, _native_##name)
#define MODULE_EXPORT_DECLARE(name) static NAN_METHOD(_native_##name);
#define MODULE_EXPORT_IMPLEMENT(name) static NAN_METHOD(_native_##name)
#define MODULE_EXPORT_IMPLEMENT_TODO(name) static NAN_METHOD(_native_##name) { return NanThrowError(NanNew<v8::String>("not implemented: " #name)); }

// macros for object wrapped classes

#define CLASS_METHOD_DECLARE(_name) \
	static NAN_METHOD(_name);

#define CLASS_METHOD_APPLY(_target, _name) \
	NODE_SET_PROTOTYPE_METHOD(_target, #_name, _name);

#define CLASS_METHOD_IMPLEMENT(_class, _name, _method) \
	NAN_METHOD(_class::_name)											\
	{                                                       			\
		NanScope();                                  					\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_method;														\
	}

#define CLASS_MEMBER_DECLARE(_name) \
	CLASS_MEMBER_DECLARE_GET(_name) \
	CLASS_MEMBER_DECLARE_SET(_name)

#define CLASS_MEMBER_DECLARE_GET(_name) \
	static NAN_GETTER(_get_ ## _name);

#define CLASS_MEMBER_DECLARE_SET(_name) \
	static NAN_SETTER(_set_ ## _name);

#define CLASS_MEMBER_APPLY(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, _set_ ## _name);

#define CLASS_MEMBER_APPLY_GET(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), _get_ ## _name, NULL);

#define CLASS_MEMBER_APPLY_SET(_target, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_name), NULL, _set_ ## _name);

#define CLASS_MEMBER_IMPLEMENT(_class, _name, _getter, _setter) \
	CLASS_MEMBER_IMPLEMENT_GET(_class, _name, _getter) \
	CLASS_MEMBER_IMPLEMENT_SET(_class, _name, _setter)

#define CLASS_MEMBER_IMPLEMENT_GET(_class, _name, _getter) \
	NAN_GETTER(_class::_get_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_IMPLEMENT_SET(_class, _name, _setter) \
	NAN_SETTER(_class::_set_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_IMPLEMENT_INT32(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(  NanNew<v8::Int32>((_cast) that->_m._name)), that->_m._name = (_cast) value->Int32Value()		)
#define CLASS_MEMBER_IMPLEMENT_UINT32(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue( NanNew<v8::Uint32>((_cast) that->_m._name)), that->_m._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_IMPLEMENT_INTEGER(_class, _m, _cast, _name)	CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew<v8::Integer>((_cast) that->_m._name)), that->_m._name = (_cast) value->IntegerValue() 	)
#define CLASS_MEMBER_IMPLEMENT_NUMBER(_class, _m, _cast, _name)		CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue( NanNew<v8::Number>((_cast) that->_m._name)), that->_m._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_IMPLEMENT_BOOLEAN(_class, _m, _cast, _name)	CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew<v8::Boolean>((_cast) that->_m._name)), that->_m._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_IMPLEMENT_VALUE(_class, _m, _name)				CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, value))
#define CLASS_MEMBER_IMPLEMENT_STRING(_class, _m, _name)			CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_IMPLEMENT_OBJECT(_class, _m, _name)			CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Object>::Cast(value)))
#define CLASS_MEMBER_IMPLEMENT_ARRAY(_class, _m, _name)				CLASS_MEMBER_IMPLEMENT(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Array>::Cast(value)))

#define CLASS_MEMBER_INLINE(_class, _name, _getter, _setter) \
	CLASS_MEMBER_INLINE_GET(_class, _name, _getter) \
	CLASS_MEMBER_INLINE_SET(_class, _name, _setter)

#define CLASS_MEMBER_INLINE_GET(_class, _name, _getter) \
	static NAN_GETTER(_get_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_INLINE_SET(_class, _name, _setter) \
	static NAN_SETTER(_set_ ## _name)									\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_INLINE_INT32(_class, _m, _cast, _name)		CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(  NanNew<v8::Int32>(that->_m._name)), that->_m._name = (_cast) value->Int32Value()	)
#define CLASS_MEMBER_INLINE_UINT32(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue( NanNew<v8::Uint32>(that->_m._name)), that->_m._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_INLINE_INTEGER(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew<v8::Integer>(that->_m._name)), that->_m._name = (_cast) value->IntegerValue()	)
#define CLASS_MEMBER_INLINE_NUMBER(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue( NanNew<v8::Number>(that->_m._name)), that->_m._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_INLINE_BOOLEAN(_class, _m, _cast, _name)	CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew<v8::Boolean>(that->_m._name)), that->_m._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_INLINE_VALUE(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, value))
#define CLASS_MEMBER_INLINE_STRING(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_INLINE_OBJECT(_class, _m, _name)			CLASS_MEMBER_INLINE(_class, _name, NanReturnValue(NanNew(that->_m##_##_name)), NanAssignPersistent(that->_m##_##_name, v8::Handle<v8::Object>::Cast(value)))

#define CLASS_MEMBER_UNION_APPLY(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), _get_ ## _u ## _name, _set_ ## _u ## _name);

#define CLASS_MEMBER_UNION_APPLY_GET(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), _get_ ## _u ## _name, NULL);

#define CLASS_MEMBER_UNION_APPLY_SET(_target, _u, _name) \
	_target->PrototypeTemplate()->SetAccessor(NanNew<v8::String>(#_u#_name), NULL, _set_ ## _u ## _name);

#define CLASS_MEMBER_UNION_INLINE(_class, _u, _name, _getter, _setter) \
	CLASS_MEMBER_UNION_INLINE_GET(_class, _u, _name, _getter) \
	CLASS_MEMBER_UNION_INLINE_SET(_class, _u, _name, _setter)

#define CLASS_MEMBER_UNION_INLINE_GET(_class, _u, _name, _getter) \
	static NAN_GETTER(_get_ ## _u ## _name)								\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_getter;														\
	}

#define CLASS_MEMBER_UNION_INLINE_SET(_class, _u, _name, _setter) \
	static NAN_SETTER(_set_ ## _u ## _name)								\
	{   																\
		NanScope();  													\
		_class* that = node::ObjectWrap::Unwrap<_class>(args.This());	\
		_setter;														\
	}

#define CLASS_MEMBER_UNION_INLINE_INT32(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(  NanNew<v8::Int32>(that->_m._u._name)), that->_m._u._name = (_cast) value->Int32Value()	)
#define CLASS_MEMBER_UNION_INLINE_UINT32(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue( NanNew<v8::Uint32>(that->_m._u._name)), that->_m._u._name = (_cast) value->Uint32Value() 	)
#define CLASS_MEMBER_UNION_INLINE_INTEGER(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew<v8::Integer>(that->_m._u._name)), that->_m._u._name = (_cast) value->IntegerValue()	)
#define CLASS_MEMBER_UNION_INLINE_NUMBER(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue( NanNew<v8::Number>(that->_m._u._name)), that->_m._u._name = (_cast) value->NumberValue() 	)
#define CLASS_MEMBER_UNION_INLINE_BOOLEAN(_class, _m, _u, _cast, _name)	CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew<v8::Boolean>(that->_m._u._name)), that->_m._u._name = (_cast) value->BooleanValue()	)
#define CLASS_MEMBER_UNION_INLINE_VALUE(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, value))
#define CLASS_MEMBER_UNION_INLINE_STRING(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, v8::Handle<v8::String>::Cast(value)))
#define CLASS_MEMBER_UNION_INLINE_OBJECT(_class, _m, _u, _name)			CLASS_MEMBER_UNION_INLINE(_class, _u, _name, NanReturnValue(NanNew(that->_m##_u##_##_name)), NanAssignPersistent(that->_m##_u##_##_name, v8::Handle<v8::Object>::Cast(value)))

using namespace v8;

namespace node_box2d {

//// b2Vec2

class WrapVec2 : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Vec2& o);

private:
	b2Vec2 m_v2;

private:
	WrapVec2() {}
	WrapVec2(const b2Vec2& o) : m_v2(o.x, o.y) {}
	WrapVec2(float32 x, float32 y) : m_v2(x, y) {}
	~WrapVec2() {}

public:
	const b2Vec2& GetVec2() const { return m_v2; }
	void SetVec2(const b2Vec2& v2) { m_v2 = v2; } // struct copy

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(x)
	CLASS_MEMBER_DECLARE(y)

	CLASS_METHOD_DECLARE(SetZero)
	CLASS_METHOD_DECLARE(Set)
	CLASS_METHOD_DECLARE(Copy)
	CLASS_METHOD_DECLARE(SelfNeg)
	CLASS_METHOD_DECLARE(SelfAdd)
	CLASS_METHOD_DECLARE(SelfAddXY)
	CLASS_METHOD_DECLARE(SelfSub)
	CLASS_METHOD_DECLARE(SelfSubXY)
	CLASS_METHOD_DECLARE(SelfMul)
	CLASS_METHOD_DECLARE(Length)
	CLASS_METHOD_DECLARE(LengthSquared)
	CLASS_METHOD_DECLARE(Normalize)
	CLASS_METHOD_DECLARE(SelfNormalize)
	CLASS_METHOD_DECLARE(IsValid)
};

Persistent<Function> WrapVec2::g_constructor;

void WrapVec2::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplVec2 = NanNew<FunctionTemplate>(New);
	tplVec2->SetClassName(NanNew<String>("b2Vec2"));
	tplVec2->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplVec2, x)
	CLASS_MEMBER_APPLY(tplVec2, y)
	CLASS_METHOD_APPLY(tplVec2, SetZero)
	CLASS_METHOD_APPLY(tplVec2, Set)
	CLASS_METHOD_APPLY(tplVec2, Copy)
	CLASS_METHOD_APPLY(tplVec2, SelfNeg)
	CLASS_METHOD_APPLY(tplVec2, SelfAdd)
	CLASS_METHOD_APPLY(tplVec2, SelfAddXY)
	CLASS_METHOD_APPLY(tplVec2, SelfSub)
	CLASS_METHOD_APPLY(tplVec2, SelfSubXY)
	CLASS_METHOD_APPLY(tplVec2, SelfMul)
	CLASS_METHOD_APPLY(tplVec2, Length)
	CLASS_METHOD_APPLY(tplVec2, LengthSquared)
	CLASS_METHOD_APPLY(tplVec2, Normalize)
	CLASS_METHOD_APPLY(tplVec2, SelfNormalize)
	CLASS_METHOD_APPLY(tplVec2, IsValid)
	NanAssignPersistent(g_constructor, tplVec2->GetFunction());
	exports->Set(NanNew<String>("b2Vec2"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapVec2::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapVec2::NewInstance(const b2Vec2& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapVec2* that = node::ObjectWrap::Unwrap<WrapVec2>(instance);
	that->SetVec2(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapVec2::New)
{
	NanScope();
	float32 x = (args.Length() > 0) ? (float32) args[0]->NumberValue() : 0.0f;
	float32 y = (args.Length() > 1) ? (float32) args[1]->NumberValue() : 0.0f;
	WrapVec2* that = new WrapVec2(x, y);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapVec2, m_v2, float32, x)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapVec2, m_v2, float32, y)

CLASS_METHOD_IMPLEMENT(WrapVec2, SetZero,
{
	that->m_v2.SetZero();
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, Set,
{
	that->m_v2.Set((float32) args[0]->NumberValue(), (float32) args[1]->NumberValue());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, Copy,
{
	WrapVec2* other = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	that->SetVec2(other->GetVec2());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfNeg,
{
	that->m_v2 = -that->m_v2;
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfAdd,
{
	that->m_v2.operator+=(node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]))->GetVec2());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfAddXY,
{
	that->m_v2.operator+=(b2Vec2((float32) args[0]->NumberValue(), (float32) args[1]->NumberValue()));
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfSub,
{
	that->m_v2.operator-=(node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]))->GetVec2());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfSubXY,
{
	that->m_v2.operator-=(b2Vec2((float32) args[0]->NumberValue(), (float32) args[1]->NumberValue()));
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfMul,
{
	that->m_v2.operator*=((float32) args[0]->NumberValue());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, Length,
{
	NanReturnValue(NanNew<Number>(that->m_v2.Length()));
})

CLASS_METHOD_IMPLEMENT(WrapVec2, LengthSquared,
{
	NanReturnValue(NanNew<Number>(that->m_v2.LengthSquared()));
})

CLASS_METHOD_IMPLEMENT(WrapVec2, Normalize,
{
	NanReturnValue(NanNew<Number>(that->m_v2.Normalize()));
})

CLASS_METHOD_IMPLEMENT(WrapVec2, SelfNormalize,
{
	that->m_v2.Normalize();
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapVec2, IsValid,
{
	NanReturnValue(NanNew<Boolean>(that->m_v2.IsValid()));
})

//// b2Rot

class WrapRot : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Rot& o);

private:
	b2Rot m_r;

private:
	WrapRot(float32 angle) : m_r(angle) {}
	WrapRot(const b2Rot& o) : m_r(o.GetAngle()) {}
	~WrapRot() {}

public:
	const b2Rot& GetRot() const { return m_r; }
	void SetRot(const b2Rot& r) { m_r = r; } // struct copy

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(s)
	CLASS_MEMBER_DECLARE(c)

	CLASS_METHOD_DECLARE(Set)
//	void SetIdentity()
	CLASS_METHOD_DECLARE(GetAngle)
//	b2Vec2 GetXAxis() const
//	b2Vec2 GetYAxis() const
};

Persistent<Function> WrapRot::g_constructor;

void WrapRot::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplRot = NanNew<FunctionTemplate>(New);
	tplRot->SetClassName(NanNew<String>("b2Rot"));
	tplRot->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplRot, s)
	CLASS_MEMBER_APPLY(tplRot, c)
	CLASS_METHOD_APPLY(tplRot, Set)
	CLASS_METHOD_APPLY(tplRot, GetAngle)
	NanAssignPersistent(g_constructor, tplRot->GetFunction());
	exports->Set(NanNew<String>("b2Rot"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapRot::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapRot::NewInstance(const b2Rot& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapRot* that = node::ObjectWrap::Unwrap<WrapRot>(instance);
	that->SetRot(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapRot::New)
{
	NanScope();
	float32 angle = (args.Length() > 0) ? (float32) args[0]->NumberValue() : 0.0f;
	WrapRot* that = new WrapRot(angle);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRot, m_r, float32, s)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRot, m_r, float32, c)

CLASS_METHOD_IMPLEMENT(WrapRot, Set,
{
	that->m_r.Set((float32) args[0]->NumberValue());
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapRot, GetAngle,
{
	NanReturnValue(NanNew<Number>(that->m_r.GetAngle()));
})

//// b2Transform

class WrapTransform : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Transform& o);

private:
	b2Transform m_xf;
	Persistent<Object> m_xf_p; // m_xf.p
	Persistent<Object> m_xf_q; // m_xf.q

private:
	WrapTransform()
	{
		// create javascript objects
		NanAssignPersistent(m_xf_p, WrapVec2::NewInstance(m_xf.p));
		NanAssignPersistent(m_xf_q, WrapRot::NewInstance(m_xf.q));
	}
	~WrapTransform()
	{
		NanDisposePersistent(m_xf_p);
		NanDisposePersistent(m_xf_q);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		NanScope();
		m_xf.p = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_xf_p))->GetVec2(); // struct copy
		m_xf.q = node::ObjectWrap::Unwrap<WrapRot>(NanNew<Object>(m_xf_q))->GetRot(); // struct copy
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		NanScope();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_xf_p))->SetVec2(m_xf.p);
		node::ObjectWrap::Unwrap<WrapRot>(NanNew<Object>(m_xf_q))->SetRot(m_xf.q);
	}

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

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(p)
	CLASS_MEMBER_DECLARE(q)

	CLASS_METHOD_DECLARE(SetIdentity)
	CLASS_METHOD_DECLARE(Set)
};

Persistent<Function> WrapTransform::g_constructor;

void WrapTransform::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplTransform = NanNew<FunctionTemplate>(New);
	tplTransform->SetClassName(NanNew<String>("b2Transform"));
	tplTransform->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplTransform, p)
	CLASS_MEMBER_APPLY(tplTransform, q)
	CLASS_METHOD_APPLY(tplTransform, SetIdentity)
	CLASS_METHOD_APPLY(tplTransform, Set)
	NanAssignPersistent(g_constructor, tplTransform->GetFunction());
	exports->Set(NanNew<String>("b2Transform"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapTransform::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapTransform::NewInstance(const b2Transform& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapTransform* that = node::ObjectWrap::Unwrap<WrapTransform>(instance);
	that->SetTransform(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapTransform::New)
{
	NanScope();
	WrapTransform* that = new WrapTransform();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapTransform, m_xf, p) // m_xf_p
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapTransform, m_xf, q) // m_xf_q

CLASS_METHOD_IMPLEMENT(WrapTransform, SetIdentity,
{
	that->m_xf.SetIdentity();
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapTransform, Set,
{
	WrapVec2* position = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	float32 angle = (float32) args[1]->NumberValue();
	that->SetTransform(position->GetVec2(), angle);
	NanReturnValue(args.This());
})

//// b2AABB

class WrapAABB : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2AABB& o);

private:
	b2AABB m_aabb;
	Persistent<Object> m_aabb_lowerBound; // m_aabb.lowerBound
	Persistent<Object> m_aabb_upperBound; // m_aabb.upperBound

private:
	WrapAABB()
	{
		// create javascript objects
		NanAssignPersistent(m_aabb_lowerBound, WrapVec2::NewInstance(m_aabb.lowerBound));
		NanAssignPersistent(m_aabb_upperBound, WrapVec2::NewInstance(m_aabb.upperBound));
	}
	~WrapAABB()
	{
		NanDisposePersistent(m_aabb_lowerBound);
		NanDisposePersistent(m_aabb_upperBound);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_aabb.lowerBound = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_aabb_lowerBound))->GetVec2(); // struct copy
		m_aabb.upperBound = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_aabb_upperBound))->GetVec2(); // struct copy
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_aabb_lowerBound))->SetVec2(m_aabb.lowerBound);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_aabb_upperBound))->SetVec2(m_aabb.upperBound);
	}

	const b2AABB& GetAABB()
	{
		SyncPull();
		return m_aabb;
	}

	void SetAABB(const b2AABB& aabb)
	{
		m_aabb = aabb; // struct copy
		SyncPush();
	}

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(lowerBound)
	CLASS_MEMBER_DECLARE(upperBound)

	CLASS_METHOD_DECLARE(IsValid)
	CLASS_METHOD_DECLARE(GetCenter)
	CLASS_METHOD_DECLARE(GetExtents)
	CLASS_METHOD_DECLARE(GetPerimeter)
};

Persistent<Function> WrapAABB::g_constructor;

void WrapAABB::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplAABB = NanNew<FunctionTemplate>(New);
	tplAABB->SetClassName(NanNew<String>("b2AABB"));
	tplAABB->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplAABB, lowerBound)
	CLASS_MEMBER_APPLY(tplAABB, upperBound)
	CLASS_METHOD_APPLY(tplAABB, IsValid)
	CLASS_METHOD_APPLY(tplAABB, GetCenter)
	CLASS_METHOD_APPLY(tplAABB, GetExtents)
	CLASS_METHOD_APPLY(tplAABB, GetPerimeter)
	NanAssignPersistent(g_constructor, tplAABB->GetFunction());
	exports->Set(NanNew<String>("b2AABB"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapAABB::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapAABB::NewInstance(const b2AABB& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapAABB* that = node::ObjectWrap::Unwrap<WrapAABB>(instance);
	that->SetAABB(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapAABB::New)
{
	NanScope();
	WrapAABB* that = new WrapAABB();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapAABB, m_aabb, lowerBound) // m_aabb_lowerBound
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapAABB, m_aabb, upperBound) // m_aabb_upperBound

CLASS_METHOD_IMPLEMENT(WrapAABB, IsValid,
{
	NanReturnValue(NanNew<Boolean>(that->m_aabb.IsValid()));
})

CLASS_METHOD_IMPLEMENT(WrapAABB, GetCenter,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_aabb.GetCenter());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapAABB, GetExtents,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_aabb.GetExtents());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapAABB, GetPerimeter,
{
	NanReturnValue(NanNew<Number>(that->m_aabb.GetPerimeter()));
})

//// b2MassData

class WrapMassData : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2MassData& o);

private:
	b2MassData m_mass_data;
	Persistent<Object> m_mass_data_center; // m_mass_data.center

private:
	WrapMassData()
	{
		// create javascript objects
		NanAssignPersistent(m_mass_data_center, WrapVec2::NewInstance(m_mass_data.center));
	}
	~WrapMassData()
	{
		NanDisposePersistent(m_mass_data_center);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_mass_data.center = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_mass_data_center))->GetVec2(); // struct copy
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_mass_data_center))->SetVec2(m_mass_data.center);
	}

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

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(mass)
	CLASS_MEMBER_DECLARE(center)
	CLASS_MEMBER_DECLARE(I)
};

Persistent<Function> WrapMassData::g_constructor;

void WrapMassData::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplMassData = NanNew<FunctionTemplate>(New);
	tplMassData->SetClassName(NanNew<String>("b2MassData"));
	tplMassData->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplMassData, mass)
	CLASS_MEMBER_APPLY(tplMassData, center)
	CLASS_MEMBER_APPLY(tplMassData, I)
	NanAssignPersistent(g_constructor, tplMassData->GetFunction());
	exports->Set(NanNew<String>("b2MassData"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapMassData::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapMassData::NewInstance(const b2MassData& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapMassData* that = node::ObjectWrap::Unwrap<WrapMassData>(instance);
	that->SetMassData(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapMassData::New)
{
	NanScope();
	WrapMassData* that = new WrapMassData();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMassData, m_mass_data, float32, mass)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapMassData, m_mass_data, center) // m_mass_data_center
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMassData, m_mass_data, float32, I)

//// b2Shape

class WrapShape : public node::ObjectWrap
{
public:
	static Persistent<FunctionTemplate> g_constructor_template;

///	static std::map<b2Shape*,WrapShape*> g_wrap_map;

public:
	static void Init(Handle<Object> exports);

	static WrapShape* GetWrap(b2Shape* shape)
	{
		//return (WrapShape*) shape->GetUserData();
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
	WrapShape() {}
	~WrapShape() {}

public:
	virtual void SyncPull() {}
	virtual void SyncPush() {}

	virtual b2Shape& GetShape() = 0;

	const b2Shape& UseShape() { SyncPull(); return GetShape(); }

private:
	CLASS_MEMBER_DECLARE(m_type)
	CLASS_MEMBER_DECLARE(m_radius)

	CLASS_METHOD_DECLARE(GetType)
};

Persistent<FunctionTemplate> WrapShape::g_constructor_template;

///std::map<b2Shape*,WrapShape*> WrapShape::g_wrap_map;

void WrapShape::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplShape = NanNew<FunctionTemplate>();
	tplShape->SetClassName(NanNew<String>("b2Shape"));
	tplShape->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplShape, m_type)
	CLASS_MEMBER_APPLY(tplShape, m_radius)
	CLASS_METHOD_APPLY(tplShape, GetType)
	NanAssignPersistent(g_constructor_template, tplShape);
	exports->Set(NanNew<String>("b2Shape"), NanNew<FunctionTemplate>(g_constructor_template)->GetFunction());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapShape, GetShape(), b2Shape::Type, m_type)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapShape, GetShape(), float32, m_radius)

CLASS_METHOD_IMPLEMENT(WrapShape, GetType,
{
	NanReturnValue(NanNew<Integer>(that->GetShape().GetType()));
})

//// b2CircleShape

class WrapCircleShape : public WrapShape
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance(float32 radius = 0.0f);

private:
	b2CircleShape m_circle;
	Persistent<Object> m_circle_m_p; // m_circle.m_p

private:
	WrapCircleShape(float32 radius = 0.0f)
	{
		// create javascript objects
		NanAssignPersistent(m_circle_m_p, WrapVec2::NewInstance(m_circle.m_p));

		m_circle.m_radius = radius;
	}
	~WrapCircleShape()
	{
		NanDisposePersistent(m_circle_m_p);
	}

public:
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		m_circle.m_p = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_circle_m_p))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_circle_m_p))->SetVec2(m_circle.m_p);
	}

	virtual b2Shape& GetShape() { return m_circle; } // override WrapShape

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(m_p)

	CLASS_METHOD_DECLARE(ComputeMass)
};

Persistent<Function> WrapCircleShape::g_constructor;

void WrapCircleShape::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplCircleShape = NanNew<FunctionTemplate>(New);
	tplCircleShape->Inherit(NanNew<FunctionTemplate>(WrapShape::g_constructor_template));
	tplCircleShape->SetClassName(NanNew<String>("b2CircleShape"));
	tplCircleShape->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplCircleShape, m_p)
	CLASS_METHOD_APPLY(tplCircleShape, ComputeMass);
	NanAssignPersistent(g_constructor, tplCircleShape->GetFunction());
	exports->Set(NanNew<String>("b2CircleShape"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapCircleShape::NewInstance(float32 radius)
{
	NanEscapableScope();
	Local<Number> h_radius = NanNew<Number>(radius);
	Handle<Value> argv[] = { h_radius };
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance(countof(argv), argv);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapCircleShape::New)
{
	NanScope();
	float32 radius = (args.Length() > 0) ? (float32) args[0]->NumberValue() : 0.0f;
	WrapCircleShape* that = new WrapCircleShape(radius);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapCircleShape, m_circle, m_p) // m_circle_m_p

CLASS_METHOD_IMPLEMENT(WrapCircleShape, ComputeMass,
{
	WrapMassData* wrap_mass_data = node::ObjectWrap::Unwrap<WrapMassData>(Local<Object>::Cast(args[0]));
	float32 density = (float32) args[1]->NumberValue();
	b2MassData mass_data;
	that->m_circle.ComputeMass(&mass_data, density);
	wrap_mass_data->SetMassData(mass_data);
	NanReturnUndefined();
})

//// b2EdgeShape

class WrapEdgeShape : public WrapShape
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2EdgeShape m_edge;
	Persistent<Object> m_edge_m_vertex1; // m_edge.m_vertex1
	Persistent<Object> m_edge_m_vertex2; // m_edge.m_vertex2
	Persistent<Object> m_edge_m_vertex0; // m_edge.m_vertex0
	Persistent<Object> m_edge_m_vertex3; // m_edge.m_vertex3

private:
	WrapEdgeShape()
	{
		// create javascript objects
		NanAssignPersistent(m_edge_m_vertex1, WrapVec2::NewInstance(m_edge.m_vertex1));
		NanAssignPersistent(m_edge_m_vertex2, WrapVec2::NewInstance(m_edge.m_vertex2));
		NanAssignPersistent(m_edge_m_vertex0, WrapVec2::NewInstance(m_edge.m_vertex0));
		NanAssignPersistent(m_edge_m_vertex3, WrapVec2::NewInstance(m_edge.m_vertex3));
	}
	~WrapEdgeShape()
	{
		NanDisposePersistent(m_edge_m_vertex1);
		NanDisposePersistent(m_edge_m_vertex2);
		NanDisposePersistent(m_edge_m_vertex0);
		NanDisposePersistent(m_edge_m_vertex3);
	}

public:
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		m_edge.m_vertex1 = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex1))->GetVec2(); // struct copy
		m_edge.m_vertex2 = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex2))->GetVec2(); // struct copy
		m_edge.m_vertex0 = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex0))->GetVec2(); // struct copy
		m_edge.m_vertex3 = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex3))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex1))->SetVec2(m_edge.m_vertex1);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex2))->SetVec2(m_edge.m_vertex2);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex0))->SetVec2(m_edge.m_vertex0);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_edge_m_vertex3))->SetVec2(m_edge.m_vertex3);
	}

	virtual b2Shape& GetShape() { return m_edge; } // override WrapShape

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(m_vertex1)
	CLASS_MEMBER_DECLARE(m_vertex2)
	CLASS_MEMBER_DECLARE(m_vertex0)
	CLASS_MEMBER_DECLARE(m_vertex3)
	CLASS_MEMBER_DECLARE(m_hasVertex0)
	CLASS_MEMBER_DECLARE(m_hasVertex3)

	CLASS_METHOD_DECLARE(Set)
	CLASS_METHOD_DECLARE(ComputeMass)
};

Persistent<Function> WrapEdgeShape::g_constructor;

void WrapEdgeShape::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplEdgeShape = NanNew<FunctionTemplate>(New);
	tplEdgeShape->Inherit(NanNew<FunctionTemplate>(WrapShape::g_constructor_template));
	tplEdgeShape->SetClassName(NanNew<String>("b2EdgeShape"));
	tplEdgeShape->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplEdgeShape, m_vertex1)
	CLASS_MEMBER_APPLY(tplEdgeShape, m_vertex2)
	CLASS_MEMBER_APPLY(tplEdgeShape, m_vertex0)
	CLASS_MEMBER_APPLY(tplEdgeShape, m_vertex3)
	CLASS_MEMBER_APPLY(tplEdgeShape, m_hasVertex0)
	CLASS_MEMBER_APPLY(tplEdgeShape, m_hasVertex3)
	CLASS_METHOD_APPLY(tplEdgeShape, Set)
	CLASS_METHOD_APPLY(tplEdgeShape, ComputeMass)
	NanAssignPersistent(g_constructor, tplEdgeShape->GetFunction());
	exports->Set(NanNew<String>("b2EdgeShape"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapEdgeShape::New)
{
	NanScope();
	WrapEdgeShape* that = new WrapEdgeShape();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapEdgeShape, m_edge, m_vertex1) // m_edge_m_vertex1
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapEdgeShape, m_edge, m_vertex2) // m_edge_m_vertex2
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapEdgeShape, m_edge, m_vertex0) // m_edge_m_vertex0
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapEdgeShape, m_edge, m_vertex3) // m_edge_m_vertex3
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapEdgeShape, m_edge, bool, m_hasVertex0)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapEdgeShape, m_edge, bool, m_hasVertex3)

CLASS_METHOD_IMPLEMENT(WrapEdgeShape, Set,
{
	NanAssignPersistent(that->m_edge_m_vertex1, Local<Object>::Cast(args[0]));
	NanAssignPersistent(that->m_edge_m_vertex2, Local<Object>::Cast(args[1]));
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapEdgeShape, ComputeMass,
{
	WrapMassData* wrap_mass_data = node::ObjectWrap::Unwrap<WrapMassData>(Local<Object>::Cast(args[0]));
	float32 density = (float32) args[1]->NumberValue();
	b2MassData mass_data;
	that->m_edge.ComputeMass(&mass_data, density);
	wrap_mass_data->SetMassData(mass_data);
	NanReturnUndefined();
})

//// b2PolygonShape

class WrapPolygonShape : public WrapShape
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2PolygonShape m_polygon;

private:
	WrapPolygonShape() {}
	~WrapPolygonShape() {}

public:
	virtual b2Shape& GetShape() { return m_polygon; } // override WrapShape

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(Set)
	CLASS_METHOD_DECLARE(SetAsBox)
	CLASS_METHOD_DECLARE(ComputeMass)
};

Persistent<Function> WrapPolygonShape::g_constructor;

void WrapPolygonShape::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplPolygonShape = NanNew<FunctionTemplate>(New);
	tplPolygonShape->Inherit(NanNew<FunctionTemplate>(WrapShape::g_constructor_template));
	tplPolygonShape->SetClassName(NanNew<String>("b2PolygonShape"));
	tplPolygonShape->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplPolygonShape, Set)
	CLASS_METHOD_APPLY(tplPolygonShape, SetAsBox)
	CLASS_METHOD_APPLY(tplPolygonShape, ComputeMass)
	NanAssignPersistent(g_constructor, tplPolygonShape->GetFunction());
	exports->Set(NanNew<String>("b2PolygonShape"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapPolygonShape::New)
{
	NanScope();
	WrapPolygonShape* that = new WrapPolygonShape();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapPolygonShape, Set,
{
	Local<Array> h_points = Local<Array>::Cast(args[0]);
	int count = (args.Length() > 1)?(args[1]->Int32Value()):(h_points->Length());
	int start = (args.Length() > 2)?(args[2]->Int32Value()):(0);
	b2Vec2* points = new b2Vec2[count];
	for (int i = 0; i < count; ++i)
	{
		points[i] = node::ObjectWrap::Unwrap<WrapVec2>(h_points->Get(start + i).As<Object>())->GetVec2(); // struct copy
	}
	that->m_polygon.Set(points, count);
	delete[] points;
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPolygonShape, SetAsBox,
{
	float32 hw = (float32) args[0]->NumberValue();
	float32 hh = (float32) args[1]->NumberValue();
	if (!args[2]->IsUndefined() && !args[3]->IsUndefined())
	{
		WrapVec2* center = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
		float32 angle = (float32) args[3]->NumberValue();
		that->m_polygon.SetAsBox(hw, hh, center->GetVec2(), angle);
	}
	else
	{
		that->m_polygon.SetAsBox(hw, hh);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPolygonShape, ComputeMass,
{
	WrapMassData* wrap_mass_data = node::ObjectWrap::Unwrap<WrapMassData>(Local<Object>::Cast(args[0]));
	float32 density = (float32) args[1]->NumberValue();
	b2MassData mass_data;
	that->m_polygon.ComputeMass(&mass_data, density);
	wrap_mass_data->SetMassData(mass_data);
	NanReturnUndefined();
})

//// b2ChainShape

class WrapChainShape : public WrapShape
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2ChainShape m_chain;
	Persistent<Array> m_chain_m_vertices; // m_chain.m_vertices
	Persistent<Object> m_chain_m_prevVertex; // m_chain.m_prevVertex
	Persistent<Object> m_chain_m_nextVertex; // m_chain.m_nextVertex

private:
	WrapChainShape()
	{
		// create javascript objects
		NanAssignPersistent(m_chain_m_prevVertex, WrapVec2::NewInstance(m_chain.m_prevVertex));
		NanAssignPersistent(m_chain_m_nextVertex, WrapVec2::NewInstance(m_chain.m_nextVertex));
	}
	~WrapChainShape()
	{
		NanDisposePersistent(m_chain_m_vertices);
		NanDisposePersistent(m_chain_m_prevVertex);
		NanDisposePersistent(m_chain_m_nextVertex);
	}

public:
	virtual void SyncPull() // override WrapShape
	{
		// sync: pull data from javascript objects
		if (!m_chain_m_vertices.IsEmpty())
		{
			Handle<Array> vertices = NanNew<Array>(m_chain_m_vertices);
			for (int32 i = 0; i < m_chain.m_count; ++i)
			{
				m_chain.m_vertices[i] = node::ObjectWrap::Unwrap<WrapVec2>(vertices->Get(i).As<Object>())->GetVec2(); // struct copy
			}
		}
		m_chain.m_prevVertex = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_chain_m_prevVertex))->GetVec2(); // struct copy
		m_chain.m_nextVertex = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_chain_m_nextVertex))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapShape
	{
		// sync: push data into javascript objects
		if (m_chain_m_vertices.IsEmpty())
		{
			NanAssignPersistent(m_chain_m_vertices, NanNew<Array>(m_chain.m_count));
		}
		Handle<Array> vertices = NanNew<Array>(m_chain_m_vertices);
		for (int32 i = 0; i < m_chain.m_count; ++i)
		{
			vertices->Set(i, WrapVec2::NewInstance(m_chain.m_vertices[i]));
		}
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_chain_m_prevVertex))->SetVec2(m_chain.m_prevVertex);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_chain_m_nextVertex))->SetVec2(m_chain.m_nextVertex);
	}

	virtual b2Shape& GetShape() { return m_chain; } // override WrapShape

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(m_vertices)
	CLASS_MEMBER_DECLARE(m_count)
	CLASS_MEMBER_DECLARE(m_prevVertex)
	CLASS_MEMBER_DECLARE(m_nextVertex)
	CLASS_MEMBER_DECLARE(m_hasPrevVertex)
	CLASS_MEMBER_DECLARE(m_hasNextVertex)
	CLASS_METHOD_DECLARE(CreateLoop)
	CLASS_METHOD_DECLARE(CreateChain)
	CLASS_METHOD_DECLARE(SetPrevVertex)
	CLASS_METHOD_DECLARE(SetNextVertex)
	CLASS_METHOD_DECLARE(ComputeMass)
};

Persistent<Function> WrapChainShape::g_constructor;

void WrapChainShape::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplChainShape = NanNew<FunctionTemplate>(New);
	tplChainShape->Inherit(NanNew<FunctionTemplate>(WrapShape::g_constructor_template));
	tplChainShape->SetClassName(NanNew<String>("b2ChainShape"));
	tplChainShape->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplChainShape, m_vertices)
	CLASS_MEMBER_APPLY(tplChainShape, m_count)
	CLASS_MEMBER_APPLY(tplChainShape, m_prevVertex)
	CLASS_MEMBER_APPLY(tplChainShape, m_nextVertex)
	CLASS_MEMBER_APPLY(tplChainShape, m_hasPrevVertex)
	CLASS_MEMBER_APPLY(tplChainShape, m_hasNextVertex)
	CLASS_METHOD_APPLY(tplChainShape, CreateLoop)
	CLASS_METHOD_APPLY(tplChainShape, CreateChain)
	CLASS_METHOD_APPLY(tplChainShape, SetPrevVertex)
	CLASS_METHOD_APPLY(tplChainShape, SetNextVertex)
	CLASS_METHOD_APPLY(tplChainShape, ComputeMass)
	NanAssignPersistent(g_constructor, tplChainShape->GetFunction());
	exports->Set(NanNew<String>("b2ChainShape"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapChainShape::New)
{
	NanScope();
	WrapChainShape* that = new WrapChainShape();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapChainShape, m_chain, m_vertices) // m_chain_m_vertices
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapChainShape, m_chain, int32, m_count)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapChainShape, m_chain, m_prevVertex) // m_chain_m_prevVertex
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapChainShape, m_chain, m_nextVertex) // m_chain_m_nextVertex
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapChainShape, m_chain, bool, m_hasPrevVertex)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapChainShape, m_chain, bool, m_hasNextVertex)

CLASS_METHOD_IMPLEMENT(WrapChainShape, CreateLoop,
{
	Local<Array> h_vertices = Local<Array>::Cast(args[0]);
	int count = (args.Length() > 1)?(args[1]->Int32Value()):(h_vertices->Length());
	b2Vec2* vertices = new b2Vec2[count];
	for (int i = 0; i < count; ++i)
	{
		vertices[i] = node::ObjectWrap::Unwrap<WrapVec2>(h_vertices->Get(i).As<Object>())->GetVec2(); // struct copy
	}
	that->m_chain.CreateLoop(vertices, count);
	delete[] vertices;
	that->SyncPush();
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapChainShape, CreateChain,
{
	Local<Array> h_vertices = Local<Array>::Cast(args[0]);
	int count = (args.Length() > 1)?(args[1]->Int32Value()):(h_vertices->Length());
	b2Vec2* vertices = new b2Vec2[count];
	for (int i = 0; i < count; ++i)
	{
		vertices[i] = node::ObjectWrap::Unwrap<WrapVec2>(h_vertices->Get(i).As<Object>())->GetVec2(); // struct copy
	}
	that->m_chain.CreateChain(vertices, count);
	delete[] vertices;
	that->SyncPush();
	NanReturnValue(args.This());
})

CLASS_METHOD_IMPLEMENT(WrapChainShape, SetPrevVertex,
{
	that->m_chain.SetPrevVertex(node::ObjectWrap::Unwrap<WrapVec2>(args[0].As<Object>())->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapChainShape, SetNextVertex,
{
	that->m_chain.SetNextVertex(node::ObjectWrap::Unwrap<WrapVec2>(args[0].As<Object>())->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapChainShape, ComputeMass,
{
	WrapMassData* wrap_mass_data = node::ObjectWrap::Unwrap<WrapMassData>(Local<Object>::Cast(args[0]));
	float32 density = (float32) args[1]->NumberValue();
	b2MassData mass_data;
	that->m_chain.ComputeMass(&mass_data, density);
	wrap_mass_data->SetMassData(mass_data);
	NanReturnUndefined();
})

//// b2Filter

class WrapFilter : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Filter& o);

public:
	b2Filter m_filter;

private:
	WrapFilter() {}
	~WrapFilter() {}

public:
	const b2Filter& GetFilter() const { return m_filter; }
	void SetFilter(const b2Filter& filter) { m_filter = filter; } // struct copy

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(categoryBits)
	CLASS_MEMBER_DECLARE(maskBits)
	CLASS_MEMBER_DECLARE(groupIndex)
};

Persistent<Function> WrapFilter::g_constructor;

void WrapFilter::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplFilter = NanNew<FunctionTemplate>(New);
	tplFilter->SetClassName(NanNew<String>("b2Filter"));
	tplFilter->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplFilter, categoryBits)
	CLASS_MEMBER_APPLY(tplFilter, maskBits)
	CLASS_MEMBER_APPLY(tplFilter, groupIndex)
	NanAssignPersistent(g_constructor, tplFilter->GetFunction());
	exports->Set(NanNew<String>("b2Filter"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapFilter::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapFilter::NewInstance(const b2Filter& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapFilter* that = node::ObjectWrap::Unwrap<WrapFilter>(instance);
	that->SetFilter(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapFilter::New)
{
	NanScope();
	WrapFilter* that = new WrapFilter();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapFilter, m_filter, uint16, categoryBits)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapFilter, m_filter, uint16, maskBits)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapFilter, m_filter, int16, groupIndex)

//// b2FixtureDef

class WrapFixtureDef : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2FixtureDef m_fd;
	Persistent<Object> m_fd_shape;		// m_fd.shape
	Persistent<Value> m_fd_userData;	// m_fd.userData
	Persistent<Object> m_fd_filter;		// m_fd.filter

private:
	WrapFixtureDef()
	{
		// create javascript objects
		NanAssignPersistent(m_fd_filter, WrapFilter::NewInstance());
	}
	~WrapFixtureDef()
	{
		NanDisposePersistent(m_fd_shape);
		NanDisposePersistent(m_fd_userData);
		NanDisposePersistent(m_fd_filter);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_fd.filter = node::ObjectWrap::Unwrap<WrapFilter>(NanNew<Object>(m_fd_filter))->GetFilter(); // struct copy
		if (!m_fd_shape.IsEmpty())
		{
			WrapShape* wrap_shape = node::ObjectWrap::Unwrap<WrapShape>(NanNew<Object>(m_fd_shape));
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
		node::ObjectWrap::Unwrap<WrapFilter>(NanNew<Object>(m_fd_filter))->SetFilter(m_fd.filter);
		if (m_fd.shape)
		{
			// TODO: NanAssignPersistent(m_fd_shape, NanObjectWrapHandle(WrapShape::GetWrap(m_fd.shape)));
		}
		else
		{
			NanDisposePersistent(m_fd_shape);
		}
		//m_fd.userData; // not used
	}

	b2FixtureDef& GetFixtureDef() { return m_fd; }

	const b2FixtureDef& UseFixtureDef() { SyncPull(); return GetFixtureDef(); }

	Handle<Object> GetShapeHandle() { return NanNew<Object>(m_fd_shape); }
	Handle<Value> GetUserDataHandle() { return NanNew<Value>(m_fd_userData); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(shape)
	CLASS_MEMBER_DECLARE(userData)
	CLASS_MEMBER_DECLARE(friction)
	CLASS_MEMBER_DECLARE(restitution)
	CLASS_MEMBER_DECLARE(density)
	CLASS_MEMBER_DECLARE(isSensor)
	CLASS_MEMBER_DECLARE(filter)
};

Persistent<Function> WrapFixtureDef::g_constructor;

void WrapFixtureDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplFixtureDef = NanNew<FunctionTemplate>(New);
	tplFixtureDef->SetClassName(NanNew<String>("b2FixtureDef"));
	tplFixtureDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplFixtureDef, shape)
	CLASS_MEMBER_APPLY(tplFixtureDef, userData)
	CLASS_MEMBER_APPLY(tplFixtureDef, friction)
	CLASS_MEMBER_APPLY(tplFixtureDef, restitution)
	CLASS_MEMBER_APPLY(tplFixtureDef, density)
	CLASS_MEMBER_APPLY(tplFixtureDef, isSensor)
	CLASS_MEMBER_APPLY(tplFixtureDef, filter)
	NanAssignPersistent(g_constructor, tplFixtureDef->GetFunction());
	exports->Set(NanNew<String>("b2FixtureDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapFixtureDef::New)
{
	NanScope();
	WrapFixtureDef* that = new WrapFixtureDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapFixtureDef, m_fd, shape				) // m_fd_shape
CLASS_MEMBER_IMPLEMENT_VALUE	(WrapFixtureDef, m_fd, userData				) // m_fd_userData
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapFixtureDef, m_fd, float32, friction	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapFixtureDef, m_fd, float32, restitution	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapFixtureDef, m_fd, float32, density		)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapFixtureDef, m_fd, bool, isSensor		)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapFixtureDef, m_fd, filter				) // m_fd_filter

//// b2Fixture

class WrapFixture : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

	static WrapFixture* GetWrap(const b2Fixture* fixture)
	{
		return (WrapFixture*) fixture->GetUserData();
	}

	static void SetWrap(b2Fixture* fixture, WrapFixture* wrap)
	{
		fixture->SetUserData(wrap);
	}

private:
	b2Fixture* m_fixture;
	Persistent<Object> m_fixture_body;
	Persistent<Object> m_fixture_shape;
	Persistent<Value> m_fixture_userData;

private:
	WrapFixture() :
		m_fixture(NULL)
	{
		// create javascript objects
	}
	~WrapFixture()
	{
		NanDisposePersistent(m_fixture_body);
		NanDisposePersistent(m_fixture_shape);
		NanDisposePersistent(m_fixture_userData);
	}

public:
	b2Fixture* GetFixture() { return m_fixture; }

	void SetupObject(Handle<Object> h_body, WrapFixtureDef* wrap_fd, b2Fixture* fixture)
	{
		m_fixture = fixture;

		// set fixture internal data
		WrapFixture::SetWrap(m_fixture, this);
		// set reference to this fixture (prevent GC)
		Ref();

		// set reference to body object
		NanAssignPersistent(m_fixture_body, h_body);
		// set reference to shape object
		NanAssignPersistent(m_fixture_shape, wrap_fd->GetShapeHandle());
		// set reference to user data object
		NanAssignPersistent(m_fixture_userData, wrap_fd->GetUserDataHandle());
	}

	b2Fixture* ResetObject()
	{
		// clear reference to body object
		NanDisposePersistent(m_fixture_body);
		// clear reference to shape object
		NanDisposePersistent(m_fixture_shape);
		// clear reference to user data object
		NanDisposePersistent(m_fixture_userData);

		// clear reference to this fixture (allow GC)
		Unref();
		// clear fixture internal data
		WrapFixture::SetWrap(m_fixture, NULL);

		b2Fixture* fixture = m_fixture;
		m_fixture = NULL;
		return fixture;
	}

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(GetType)
	CLASS_METHOD_DECLARE(GetShape)
	CLASS_METHOD_DECLARE(SetSensor)
	CLASS_METHOD_DECLARE(IsSensor)
	CLASS_METHOD_DECLARE(SetFilterData)
	CLASS_METHOD_DECLARE(GetFilterData)
	CLASS_METHOD_DECLARE(Refilter)
	CLASS_METHOD_DECLARE(GetBody)
	CLASS_METHOD_DECLARE(GetNext)
	CLASS_METHOD_DECLARE(GetUserData)
	CLASS_METHOD_DECLARE(SetUserData)
	CLASS_METHOD_DECLARE(TestPoint) //bool TestPoint(const b2Vec2& p) const;
//	bool RayCast(b2RayCastOutput* output, const b2RayCastInput& input, int32 childIndex) const;
//	void GetMassData(b2MassData* massData) const;
	CLASS_METHOD_DECLARE(GetDensity)
	CLASS_METHOD_DECLARE(SetDensity)
	CLASS_METHOD_DECLARE(GetFriction)
	CLASS_METHOD_DECLARE(SetFriction)
	CLASS_METHOD_DECLARE(GetRestitution)
	CLASS_METHOD_DECLARE(SetRestitution)
	CLASS_METHOD_DECLARE(GetAABB)
///	void Dump(int32 bodyIndex);
};

Persistent<Function> WrapFixture::g_constructor;

void WrapFixture::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplFixture = NanNew<FunctionTemplate>(New);
	tplFixture->SetClassName(NanNew<String>("b2Fixture"));
	tplFixture->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplFixture, GetType)
	CLASS_METHOD_APPLY(tplFixture, GetShape)
	CLASS_METHOD_APPLY(tplFixture, SetSensor)
	CLASS_METHOD_APPLY(tplFixture, IsSensor)
	CLASS_METHOD_APPLY(tplFixture, SetFilterData)
	CLASS_METHOD_APPLY(tplFixture, GetFilterData)
	CLASS_METHOD_APPLY(tplFixture, Refilter)
	CLASS_METHOD_APPLY(tplFixture, GetBody)
	CLASS_METHOD_APPLY(tplFixture, GetNext)
	CLASS_METHOD_APPLY(tplFixture, GetUserData)
	CLASS_METHOD_APPLY(tplFixture, SetUserData)
	CLASS_METHOD_APPLY(tplFixture, TestPoint)
	CLASS_METHOD_APPLY(tplFixture, GetDensity)
	CLASS_METHOD_APPLY(tplFixture, SetDensity)
	CLASS_METHOD_APPLY(tplFixture, GetFriction)
	CLASS_METHOD_APPLY(tplFixture, SetFriction)
	CLASS_METHOD_APPLY(tplFixture, GetRestitution)
	CLASS_METHOD_APPLY(tplFixture, SetRestitution)
	CLASS_METHOD_APPLY(tplFixture, GetAABB)
	NanAssignPersistent(g_constructor, tplFixture->GetFunction());
	exports->Set(NanNew<String>("b2Fixture"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapFixture::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapFixture::New)
{
	NanScope();
	WrapFixture* that = new WrapFixture();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapFixture, GetType,
{
	NanReturnValue(NanNew<Integer>(that->m_fixture->GetType()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetShape,
{
	NanReturnValue(NanNew(that->m_fixture_shape));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetSensor,
{
	that->m_fixture->SetSensor(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, IsSensor,
{
	NanReturnValue(NanNew<Boolean>(that->m_fixture->IsSensor()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetFilterData,
{
	WrapFilter* wrap_filter = node::ObjectWrap::Unwrap<WrapFilter>(Local<Object>::Cast(args[0]));
	that->m_fixture->SetFilterData(wrap_filter->GetFilter());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetFilterData,
{
	NanReturnValue(WrapFilter::NewInstance(that->m_fixture->GetFilterData()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, Refilter,
{
	that->m_fixture->Refilter();
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetBody,
{
	NanReturnValue(NanNew(that->m_fixture_body));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetNext,
{
	b2Fixture* fixture = that->m_fixture->GetNext();
	if (fixture)
	{
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		NanReturnValue(NanObjectWrapHandle(wrap_fixture));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetUserData,
{
	if (!that->m_fixture_userData.IsEmpty())
	{
		NanReturnValue(NanNew<Value>(that->m_fixture_userData));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetUserData,
{
	if (!args[0]->IsNull())
	{
		NanAssignPersistent(that->m_fixture_userData, args[0]);
	}
	else
	{
		NanDisposePersistent(that->m_fixture_userData);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, TestPoint,
{
	WrapVec2* p = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	NanReturnValue(NanNew<Boolean>(that->m_fixture->TestPoint(p->GetVec2())));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetDensity,
{
	NanReturnValue(NanNew<Number>(that->m_fixture->GetDensity()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetDensity,
{
	that->m_fixture->SetDensity((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetFriction,
{
	NanReturnValue(NanNew<Number>(that->m_fixture->GetFriction()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetFriction,
{
	that->m_fixture->SetFriction((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetRestitution,
{
	NanReturnValue(NanNew<Number>(that->m_fixture->GetRestitution()));
})

CLASS_METHOD_IMPLEMENT(WrapFixture, SetRestitution,
{
	that->m_fixture->SetRestitution((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFixture, GetAABB,
{
	int32 childIndex = args[0]->Int32Value();
	NanReturnValue(WrapAABB::NewInstance(that->m_fixture->GetAABB(childIndex)));
})

//// b2BodyDef

class WrapBodyDef : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2BodyDef m_bd;
	Persistent<Object> m_bd_position;		// m_bd.position
	Persistent<Object> m_bd_linearVelocity;	// m_bd.linearVelocity
	Persistent<Value> m_bd_userData;		// m_bd.userData

private:
	WrapBodyDef()
	{
		// create javascript objects
		NanAssignPersistent(m_bd_position, WrapVec2::NewInstance(m_bd.position));
		NanAssignPersistent(m_bd_linearVelocity, WrapVec2::NewInstance(m_bd.linearVelocity));
	}
	~WrapBodyDef()
	{
		NanDisposePersistent(m_bd_position);
		NanDisposePersistent(m_bd_linearVelocity);
		NanDisposePersistent(m_bd_userData);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_bd.position = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_bd_position))->GetVec2(); // struct copy
		m_bd.linearVelocity = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_bd_linearVelocity))->GetVec2(); // struct copy
		//m_bd.userData; // not used
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_bd_position))->SetVec2(m_bd.position);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_bd_linearVelocity))->SetVec2(m_bd.linearVelocity);
		//m_bd.userData; // not used
	}

	b2BodyDef& GetBodyDef() { return m_bd; }

	const b2BodyDef& UseBodyDef() { SyncPull(); return GetBodyDef(); }

	Handle<Value> GetUserDataHandle() { return NanNew<Value>(m_bd_userData); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(type)
	CLASS_MEMBER_DECLARE(position)
	CLASS_MEMBER_DECLARE(angle)
	CLASS_MEMBER_DECLARE(linearVelocity)
	CLASS_MEMBER_DECLARE(angularVelocity)
	CLASS_MEMBER_DECLARE(linearDamping)
	CLASS_MEMBER_DECLARE(angularDamping)
	CLASS_MEMBER_DECLARE(allowSleep)
	CLASS_MEMBER_DECLARE(awake)
	CLASS_MEMBER_DECLARE(fixedRotation)
	CLASS_MEMBER_DECLARE(bullet)
	CLASS_MEMBER_DECLARE(active)
	CLASS_MEMBER_DECLARE(userData)
	CLASS_MEMBER_DECLARE(gravityScale)
};

Persistent<Function> WrapBodyDef::g_constructor;

void WrapBodyDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplBodyDef = NanNew<FunctionTemplate>(New);
	tplBodyDef->SetClassName(NanNew<String>("b2BodyDef"));
	tplBodyDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplBodyDef, type)
	CLASS_MEMBER_APPLY(tplBodyDef, position)
	CLASS_MEMBER_APPLY(tplBodyDef, angle)
	CLASS_MEMBER_APPLY(tplBodyDef, linearVelocity)
	CLASS_MEMBER_APPLY(tplBodyDef, angularVelocity)
	CLASS_MEMBER_APPLY(tplBodyDef, linearDamping)
	CLASS_MEMBER_APPLY(tplBodyDef, angularDamping)
	CLASS_MEMBER_APPLY(tplBodyDef, allowSleep)
	CLASS_MEMBER_APPLY(tplBodyDef, awake)
	CLASS_MEMBER_APPLY(tplBodyDef, fixedRotation)
	CLASS_MEMBER_APPLY(tplBodyDef, bullet)
	CLASS_MEMBER_APPLY(tplBodyDef, active)
	CLASS_MEMBER_APPLY(tplBodyDef, userData)
	CLASS_MEMBER_APPLY(tplBodyDef, gravityScale)
	NanAssignPersistent(g_constructor, tplBodyDef->GetFunction());
	exports->Set(NanNew<String>("b2BodyDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapBodyDef::New)
{
	NanScope();
	WrapBodyDef* that = new WrapBodyDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapBodyDef, m_bd, b2BodyType, type		)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapBodyDef, m_bd, position				) // m_bd_position
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapBodyDef, m_bd, float32, angle			)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapBodyDef, m_bd, linearVelocity			) // m_bd_linearVelocity
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapBodyDef, m_bd, float32, angularVelocity)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapBodyDef, m_bd, float32, linearDamping	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapBodyDef, m_bd, float32, angularDamping	)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapBodyDef, m_bd, bool, allowSleep		)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapBodyDef, m_bd, bool, awake				)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapBodyDef, m_bd, bool, fixedRotation		)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapBodyDef, m_bd, bool, bullet			)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapBodyDef, m_bd, bool, active			)
CLASS_MEMBER_IMPLEMENT_VALUE	(WrapBodyDef, m_bd, userData				) // m_bd_userData
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapBodyDef, m_bd, float32, gravityScale	)

//// b2Body

class WrapBody : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

	static WrapBody* GetWrap(const b2Body* body)
	{
		return (WrapBody*) body->GetUserData();
	}

	static void SetWrap(b2Body* body, WrapBody* wrap)
	{
		body->SetUserData(wrap);
	}

private:
	b2Body* m_body;
	Persistent<Object> m_body_world;
	Persistent<Value> m_body_userData;

private:
	WrapBody() :
		m_body(NULL)
	{
		// create javascript objects
	}
	~WrapBody()
	{
		NanDisposePersistent(m_body_world);
		NanDisposePersistent(m_body_userData);
	}

public:
	b2Body* GetBody() { return m_body; }

	void SetupObject(Handle<Object> h_world, WrapBodyDef* wrap_bd, b2Body* body)
	{
		m_body = body;

		// set body internal data
		WrapBody::SetWrap(m_body, this);
		// set reference to this body (prevent GC)
		Ref();

		// set reference to world object
		NanAssignPersistent(m_body_world, h_world);
		// set reference to user data object
		NanAssignPersistent(m_body_userData, wrap_bd->GetUserDataHandle());
	}

	b2Body* ResetObject()
	{
		// clear reference to world object
		NanDisposePersistent(m_body_world);
		// clear reference to user data object
		NanDisposePersistent(m_body_userData);

		// clear reference to this body (allow GC)
		Unref();
		// clear body internal data
		WrapBody::SetWrap(m_body, NULL);

		b2Body* body = m_body;
		m_body = NULL;
		return body;
	}

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(CreateFixture)
	CLASS_METHOD_DECLARE(DestroyFixture)
	CLASS_METHOD_DECLARE(SetTransform)
	CLASS_METHOD_DECLARE(GetTransform)
	CLASS_METHOD_DECLARE(GetPosition)
	CLASS_METHOD_DECLARE(GetAngle)
	CLASS_METHOD_DECLARE(GetWorldCenter)
	CLASS_METHOD_DECLARE(GetLocalCenter)
	CLASS_METHOD_DECLARE(SetLinearVelocity)
	CLASS_METHOD_DECLARE(GetLinearVelocity)
	CLASS_METHOD_DECLARE(SetAngularVelocity)
	CLASS_METHOD_DECLARE(GetAngularVelocity)
	CLASS_METHOD_DECLARE(ApplyForce)
	CLASS_METHOD_DECLARE(ApplyForceToCenter)
	CLASS_METHOD_DECLARE(ApplyTorque)
	CLASS_METHOD_DECLARE(ApplyLinearImpulse)
	CLASS_METHOD_DECLARE(ApplyLinearImpulseToCenter)
	CLASS_METHOD_DECLARE(ApplyAngularImpulse)
	CLASS_METHOD_DECLARE(GetMass)
	CLASS_METHOD_DECLARE(GetInertia)
	CLASS_METHOD_DECLARE(GetMassData) //void GetMassData(b2MassData* data) const;
	CLASS_METHOD_DECLARE(SetMassData) //void SetMassData(const b2MassData* data);
	CLASS_METHOD_DECLARE(ResetMassData) //void ResetMassData();
	CLASS_METHOD_DECLARE(GetWorldPoint)
	CLASS_METHOD_DECLARE(GetWorldVector)
	CLASS_METHOD_DECLARE(GetLocalPoint)
	CLASS_METHOD_DECLARE(GetLocalVector)
	CLASS_METHOD_DECLARE(GetLinearVelocityFromWorldPoint)
	CLASS_METHOD_DECLARE(GetLinearVelocityFromLocalPoint)
	CLASS_METHOD_DECLARE(GetLinearDamping)
	CLASS_METHOD_DECLARE(SetLinearDamping)
	CLASS_METHOD_DECLARE(GetAngularDamping)
	CLASS_METHOD_DECLARE(SetAngularDamping)
	CLASS_METHOD_DECLARE(GetGravityScale)
	CLASS_METHOD_DECLARE(SetGravityScale)
	CLASS_METHOD_DECLARE(SetType)
	CLASS_METHOD_DECLARE(GetType)
	CLASS_METHOD_DECLARE(SetBullet)
	CLASS_METHOD_DECLARE(IsBullet)
	CLASS_METHOD_DECLARE(SetSleepingAllowed)
	CLASS_METHOD_DECLARE(IsSleepingAllowed)
	CLASS_METHOD_DECLARE(SetAwake)
	CLASS_METHOD_DECLARE(IsAwake)
	CLASS_METHOD_DECLARE(SetActive)
	CLASS_METHOD_DECLARE(IsActive)
	CLASS_METHOD_DECLARE(SetFixedRotation)
	CLASS_METHOD_DECLARE(IsFixedRotation)
	CLASS_METHOD_DECLARE(GetFixtureList)
///	b2JointEdge* GetJointList();
///	const b2JointEdge* GetJointList() const;
///	b2ContactEdge* GetContactList();
///	const b2ContactEdge* GetContactList() const;
	CLASS_METHOD_DECLARE(GetNext)
	CLASS_METHOD_DECLARE(GetUserData)
	CLASS_METHOD_DECLARE(SetUserData)
	CLASS_METHOD_DECLARE(GetWorld)
	CLASS_METHOD_DECLARE(ShouldCollideConnected)
///	void Dump();
};

Persistent<Function> WrapBody::g_constructor;

void WrapBody::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplBody = NanNew<FunctionTemplate>(New);
	tplBody->SetClassName(NanNew<String>("b2Body"));
	tplBody->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplBody, CreateFixture)
	CLASS_METHOD_APPLY(tplBody, DestroyFixture)
	CLASS_METHOD_APPLY(tplBody, SetTransform)
	CLASS_METHOD_APPLY(tplBody, GetTransform)
	CLASS_METHOD_APPLY(tplBody, GetPosition)
	CLASS_METHOD_APPLY(tplBody, GetAngle)
	CLASS_METHOD_APPLY(tplBody, GetWorldCenter)
	CLASS_METHOD_APPLY(tplBody, GetLocalCenter)
	CLASS_METHOD_APPLY(tplBody, SetLinearVelocity)
	CLASS_METHOD_APPLY(tplBody, GetLinearVelocity)
	CLASS_METHOD_APPLY(tplBody, SetAngularVelocity)
	CLASS_METHOD_APPLY(tplBody, GetAngularVelocity)
	CLASS_METHOD_APPLY(tplBody, ApplyForce)
	CLASS_METHOD_APPLY(tplBody, ApplyForceToCenter)
	CLASS_METHOD_APPLY(tplBody, ApplyTorque)
	CLASS_METHOD_APPLY(tplBody, ApplyLinearImpulse)
	CLASS_METHOD_APPLY(tplBody, ApplyLinearImpulseToCenter)
	CLASS_METHOD_APPLY(tplBody, ApplyAngularImpulse)
	CLASS_METHOD_APPLY(tplBody, GetMass)
	CLASS_METHOD_APPLY(tplBody, GetInertia)
	CLASS_METHOD_APPLY(tplBody, GetMassData)
	CLASS_METHOD_APPLY(tplBody, SetMassData)
	CLASS_METHOD_APPLY(tplBody, ResetMassData)
	CLASS_METHOD_APPLY(tplBody, GetWorldPoint)
	CLASS_METHOD_APPLY(tplBody, GetWorldVector)
	CLASS_METHOD_APPLY(tplBody, GetLocalPoint)
	CLASS_METHOD_APPLY(tplBody, GetLocalVector)
	CLASS_METHOD_APPLY(tplBody, GetLinearVelocityFromWorldPoint)
	CLASS_METHOD_APPLY(tplBody, GetLinearVelocityFromLocalPoint)
	CLASS_METHOD_APPLY(tplBody, GetLinearDamping)
	CLASS_METHOD_APPLY(tplBody, SetLinearDamping)
	CLASS_METHOD_APPLY(tplBody, GetAngularDamping)
	CLASS_METHOD_APPLY(tplBody, SetAngularDamping)
	CLASS_METHOD_APPLY(tplBody, GetGravityScale)
	CLASS_METHOD_APPLY(tplBody, SetGravityScale)
	CLASS_METHOD_APPLY(tplBody, SetType)
	CLASS_METHOD_APPLY(tplBody, GetType)
	CLASS_METHOD_APPLY(tplBody, SetBullet)
	CLASS_METHOD_APPLY(tplBody, IsBullet)
	CLASS_METHOD_APPLY(tplBody, SetSleepingAllowed)
	CLASS_METHOD_APPLY(tplBody, IsSleepingAllowed)
	CLASS_METHOD_APPLY(tplBody, SetAwake)
	CLASS_METHOD_APPLY(tplBody, IsAwake)
	CLASS_METHOD_APPLY(tplBody, SetActive)
	CLASS_METHOD_APPLY(tplBody, IsActive)
	CLASS_METHOD_APPLY(tplBody, SetFixedRotation)
	CLASS_METHOD_APPLY(tplBody, IsFixedRotation)
	CLASS_METHOD_APPLY(tplBody, GetFixtureList)
	CLASS_METHOD_APPLY(tplBody, GetNext)
	CLASS_METHOD_APPLY(tplBody, GetUserData)
	CLASS_METHOD_APPLY(tplBody, SetUserData)
	CLASS_METHOD_APPLY(tplBody, GetWorld)
	CLASS_METHOD_APPLY(tplBody, ShouldCollideConnected)
	NanAssignPersistent(g_constructor, tplBody->GetFunction());
	exports->Set(NanNew<String>("b2Body"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapBody::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapBody::New)
{
	NanScope();
	WrapBody* that = new WrapBody();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapBody, CreateFixture,
{
	WrapFixtureDef* wrap_fd = node::ObjectWrap::Unwrap<WrapFixtureDef>(Local<Object>::Cast(args[0]));

	// create box2d fixture
	b2Fixture* fixture = that->m_body->CreateFixture(&wrap_fd->UseFixtureDef());

	// create javascript fixture object
	Local<Object> h_fixture = NanNew<Object>(WrapFixture::NewInstance());
	WrapFixture* wrap_fixture = node::ObjectWrap::Unwrap<WrapFixture>(h_fixture);

	// set up javascript fixture object
	wrap_fixture->SetupObject(args.This(), wrap_fd, fixture);

	NanReturnValue(h_fixture);
})

CLASS_METHOD_IMPLEMENT(WrapBody, DestroyFixture,
{
	Local<Object> h_fixture = Local<Object>::Cast(args[0]);
	WrapFixture* wrap_fixture = node::ObjectWrap::Unwrap<WrapFixture>(h_fixture);

	// delete box2d fixture
	that->m_body->DestroyFixture(wrap_fixture->GetFixture());

	// reset javascript fixture object
	wrap_fixture->ResetObject();

	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, SetTransform,
{
	WrapVec2* position = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	float32 angle = (float32) args[1]->NumberValue();
	that->m_body->SetTransform(position->GetVec2(), angle);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetTransform,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapTransform::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapTransform>(out)->SetTransform(that->m_body->GetTransform());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetPosition,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetPosition());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetAngle, { NanReturnValue(NanNew<Number>(that->m_body->GetAngle())); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetWorldCenter,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetWorldCenter());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLocalCenter,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLocalCenter());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, SetLinearVelocity,
{
	that->m_body->SetLinearVelocity(node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]))->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLinearVelocity,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLinearVelocity());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, SetAngularVelocity, { that->m_body->SetAngularVelocity((float32) args[0]->NumberValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, GetAngularVelocity, { NanReturnValue(NanNew<Number>(that->m_body->GetAngularVelocity())); })

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyForce,
{
	WrapVec2* force = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* point = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	bool wake = (args.Length() > 2) ? args[2]->BooleanValue() : true;
	that->m_body->ApplyForce(force->GetVec2(), point->GetVec2(), wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyForceToCenter,
{
	WrapVec2* force = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	bool wake = (args.Length() > 1) ? args[1]->BooleanValue() : true;
	that->m_body->ApplyForceToCenter(force->GetVec2(), wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyTorque,
{
	float32 torque = (float32) args[0]->NumberValue();
	bool wake = (args.Length() > 1) ? args[1]->BooleanValue() : true;
	that->m_body->ApplyTorque(torque, wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyLinearImpulse,
{
	WrapVec2* impulse = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* point = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	bool wake = (args.Length() > 2) ? args[2]->BooleanValue() : true;
	that->m_body->ApplyLinearImpulse(impulse->GetVec2(), point->GetVec2(), wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyLinearImpulseToCenter,
{
	WrapVec2* impulse = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	bool wake = (args.Length() > 1) ? args[1]->BooleanValue() : true;
	that->m_body->ApplyLinearImpulse(impulse->GetVec2(), that->m_body->GetWorldCenter(), wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ApplyAngularImpulse,
{
	float32 impulse = (float32) args[0]->NumberValue();
	bool wake = (args.Length() > 1) ? args[1]->BooleanValue() : true;
	that->m_body->ApplyAngularImpulse(impulse, wake);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetMass, { NanReturnValue(NanNew<Number>(that->m_body->GetMass())); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetInertia, { NanReturnValue(NanNew<Number>(that->m_body->GetInertia())); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetMassData,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapMassData::NewInstance()) : Local<Object>::Cast(args[0]);
	b2MassData mass_data;
	that->m_body->GetMassData(&mass_data);
	node::ObjectWrap::Unwrap<WrapMassData>(out)->SetMassData(mass_data);
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, SetMassData,
{
	const b2MassData& mass_data = node::ObjectWrap::Unwrap<WrapMassData>(Local<Object>::Cast(args[0]))->GetMassData();
	that->m_body->SetMassData(&mass_data);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, ResetMassData, { that->m_body->ResetMassData(); NanReturnUndefined(); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetWorldPoint,
{
	WrapVec2* wrap_localPoint = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetWorldPoint(wrap_localPoint->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetWorldVector,
{
	WrapVec2* wrap_localVector = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetWorldVector(wrap_localVector->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLocalPoint,
{
	WrapVec2* wrap_worldPoint = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLocalPoint(wrap_worldPoint->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLocalVector,
{
	WrapVec2* wrap_worldVector = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLocalVector(wrap_worldVector->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLinearVelocityFromWorldPoint,
{
	WrapVec2* wrap_worldPoint = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLinearVelocityFromWorldPoint(wrap_worldPoint->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLinearVelocityFromLocalPoint,
{
	WrapVec2* wrap_localPoint = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_body->GetLinearVelocityFromLocalPoint(wrap_localPoint->GetVec2()));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetLinearDamping, { NanReturnValue(NanNew<Number>(that->m_body->GetLinearDamping())); })
CLASS_METHOD_IMPLEMENT(WrapBody, SetLinearDamping, { that->m_body->SetLinearDamping((float32) args[0]->NumberValue()); NanReturnUndefined(); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetAngularDamping, { NanReturnValue(NanNew<Number>(that->m_body->GetAngularDamping())); })
CLASS_METHOD_IMPLEMENT(WrapBody, SetAngularDamping, { that->m_body->SetAngularDamping((float32) args[0]->NumberValue()); NanReturnUndefined(); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetGravityScale, { NanReturnValue(NanNew<Number>(that->m_body->GetGravityScale())); })
CLASS_METHOD_IMPLEMENT(WrapBody, SetGravityScale, { that->m_body->SetGravityScale((float32) args[0]->NumberValue()); NanReturnUndefined(); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetType, { that->m_body->SetType((b2BodyType) args[0]->Int32Value()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, GetType, { NanReturnValue(NanNew<Integer>(that->m_body->GetType())); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetBullet, { that->m_body->SetBullet(args[0]->BooleanValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, IsBullet, { NanReturnValue(NanNew<Boolean>(that->m_body->IsBullet())); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetSleepingAllowed, { that->m_body->SetSleepingAllowed(args[0]->BooleanValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, IsSleepingAllowed, { NanReturnValue(NanNew<Boolean>(that->m_body->IsSleepingAllowed())); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetAwake, { that->m_body->SetAwake(args[0]->BooleanValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, IsAwake, { NanReturnValue(NanNew<Boolean>(that->m_body->IsAwake())); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetActive, { that->m_body->SetActive(args[0]->BooleanValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, IsActive, { NanReturnValue(NanNew<Boolean>(that->m_body->IsActive())); })

CLASS_METHOD_IMPLEMENT(WrapBody, SetFixedRotation, { that->m_body->SetFixedRotation(args[0]->BooleanValue()); NanReturnUndefined(); })
CLASS_METHOD_IMPLEMENT(WrapBody, IsFixedRotation, { NanReturnValue(NanNew<Boolean>(that->m_body->IsFixedRotation())); })

CLASS_METHOD_IMPLEMENT(WrapBody, GetFixtureList,
{
	b2Fixture* fixture = that->m_body->GetFixtureList();
	if (fixture)
	{
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		NanReturnValue(NanObjectWrapHandle(wrap_fixture));
	}
	NanReturnNull();
})


CLASS_METHOD_IMPLEMENT(WrapBody, GetNext,
{
	b2Body* body = that->m_body->GetNext();
	if (body)
	{
		// get body internal data
		WrapBody* wrap_body = WrapBody::GetWrap(body);
		NanReturnValue(NanObjectWrapHandle(wrap_body));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetUserData,
{
	NanReturnValue(NanNew<Value>(that->m_body_userData));
})

CLASS_METHOD_IMPLEMENT(WrapBody, SetUserData,
{
	if (!args[0]->IsUndefined())
	{
		NanAssignPersistent(that->m_body_userData, args[0]);
	}
	else
	{
		NanDisposePersistent(that->m_body_userData);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapBody, GetWorld,
{
	NanReturnValue(NanNew<Object>(that->m_body_world));
})

CLASS_METHOD_IMPLEMENT(WrapBody, ShouldCollideConnected,
{
	WrapBody* wrap_other = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	NanReturnValue(NanNew<Boolean>(that->m_body->ShouldCollideConnected(wrap_other->m_body)));
})

//// b2JointDef

class WrapJointDef : public node::ObjectWrap
{
public:
	static Persistent<FunctionTemplate> g_constructor_template;

public:
	static void Init(Handle<Object> exports);

protected:
	Persistent<Value> m_jd_userData;	// m_jd.userData
	Persistent<Object> m_jd_bodyA;		// m_jd.bodyA
	Persistent<Object> m_jd_bodyB;		// m_jd.bodyB

public:
	WrapJointDef()
	{
		// create javascript objects
	}
	~WrapJointDef()
	{
		NanDisposePersistent(m_jd_userData);
		NanDisposePersistent(m_jd_bodyA);
		NanDisposePersistent(m_jd_bodyB);
	}

public:
	virtual void SyncPull()
	{
		b2JointDef& jd = GetJointDef();
		// sync: pull data from javascript objects
		//jd.userData; // not used
		jd.bodyA = m_jd_bodyA.IsEmpty() ? NULL : node::ObjectWrap::Unwrap<WrapBody>(NanNew<Object>(m_jd_bodyA))->GetBody();
		jd.bodyB = m_jd_bodyB.IsEmpty() ? NULL : node::ObjectWrap::Unwrap<WrapBody>(NanNew<Object>(m_jd_bodyB))->GetBody();
	}

	virtual void SyncPush()
	{
		b2JointDef& jd = GetJointDef();
		// sync: push data into javascript objects
		//jd.userData; // not used
		if (jd.bodyA)
		{
			NanAssignPersistent(m_jd_bodyA, NanObjectWrapHandle(WrapBody::GetWrap(jd.bodyA)));
		}
		else
		{
			NanDisposePersistent(m_jd_bodyA);
		}
		if (jd.bodyB)
		{
			NanAssignPersistent(m_jd_bodyB, NanObjectWrapHandle(WrapBody::GetWrap(jd.bodyB)));
		}
		else
		{
			NanDisposePersistent(m_jd_bodyB);
		}
	}

	virtual b2JointDef& GetJointDef() = 0;

	const b2JointDef& UseJointDef() { SyncPull(); return GetJointDef(); }

	Handle<Value> GetUserDataHandle() { return NanNew<Value>(m_jd_userData); }
	Handle<Object> GetBodyAHandle() { return NanNew<Object>(m_jd_bodyA); }
	Handle<Object> GetBodyBHandle() { return NanNew<Object>(m_jd_bodyB); }

private:
	CLASS_MEMBER_DECLARE(type)
	CLASS_MEMBER_DECLARE(userData)
	CLASS_MEMBER_DECLARE(bodyA)
	CLASS_MEMBER_DECLARE(bodyB)
	CLASS_MEMBER_DECLARE(collideConnected)
};

Persistent<FunctionTemplate> WrapJointDef::g_constructor_template;

void WrapJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplJointDef = NanNew<FunctionTemplate>();
	tplJointDef->SetClassName(NanNew<String>("b2JointDef"));
	tplJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplJointDef, type)
	CLASS_MEMBER_APPLY(tplJointDef, userData)
	CLASS_MEMBER_APPLY(tplJointDef, bodyA)
	CLASS_MEMBER_APPLY(tplJointDef, bodyB)
	CLASS_MEMBER_APPLY(tplJointDef, collideConnected)
	NanAssignPersistent(g_constructor_template, tplJointDef);
	exports->Set(NanNew<String>("b2JointDef"), NanNew<FunctionTemplate>(g_constructor_template)->GetFunction());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapJointDef, GetJointDef(), b2JointType, type		)
CLASS_MEMBER_IMPLEMENT_VALUE	(WrapJointDef, m_jd, userData						) // m_jd_userData
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapJointDef, m_jd, bodyA							) // m_jd_bodyA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapJointDef, m_jd, bodyB							) // m_jd_bodyB
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapJointDef, GetJointDef(), bool, collideConnected)

//// b2Joint

class WrapJoint : public node::ObjectWrap
{
public:
	static Persistent<FunctionTemplate> g_constructor_template;

public:
	static void Init(Handle<Object> exports);

	static WrapJoint* GetWrap(const b2Joint* joint)
	{
		return (WrapJoint*) joint->GetUserData();
	}

	static void SetWrap(b2Joint* joint, WrapJoint* wrap)
	{
		joint->SetUserData(wrap);
	}

private:
	Persistent<Object> m_joint_bodyA;
	Persistent<Object> m_joint_bodyB;
	Persistent<Value> m_joint_userData;

public:
	WrapJoint()
	{
		// create javascript objects
	}
	~WrapJoint()
	{
		NanDisposePersistent(m_joint_bodyA);
		NanDisposePersistent(m_joint_bodyB);
		NanDisposePersistent(m_joint_userData);
	}

	virtual b2Joint* GetJoint() = 0;

public:
	void SetupObject(WrapJointDef* wrap_jd)
	{
		b2Joint* joint = GetJoint();

		// set joint internal data
		WrapJoint::SetWrap(joint, this);
		// set reference to this joint (prevent GC)
		Ref();

		// set references to joint body objects
		NanAssignPersistent(m_joint_bodyA, wrap_jd->GetBodyAHandle());
		NanAssignPersistent(m_joint_bodyB, wrap_jd->GetBodyBHandle());
		// set reference to joint user data object
		NanAssignPersistent(m_joint_userData, wrap_jd->GetUserDataHandle());
	}

	void ResetObject()
	{
		b2Joint* joint = GetJoint();

		// clear references to joint body objects
		NanDisposePersistent(m_joint_bodyA);
		NanDisposePersistent(m_joint_bodyB);
		// clear reference to joint user data object
		NanDisposePersistent(m_joint_userData);

		// clear reference to this joint (allow GC)
		Unref();
		// clear joint internal data
		WrapJoint::SetWrap(joint, NULL);
	}

private:
	CLASS_METHOD_DECLARE(GetType)
	CLASS_METHOD_DECLARE(GetBodyA)
	CLASS_METHOD_DECLARE(GetBodyB)
	CLASS_METHOD_DECLARE(GetAnchorA)
	CLASS_METHOD_DECLARE(GetAnchorB)
	CLASS_METHOD_DECLARE(GetReactionForce)
	CLASS_METHOD_DECLARE(GetReactionTorque)
	CLASS_METHOD_DECLARE(GetNext)
	CLASS_METHOD_DECLARE(GetUserData)
	CLASS_METHOD_DECLARE(SetUserData)
	CLASS_METHOD_DECLARE(IsActive)
	CLASS_METHOD_DECLARE(GetCollideConnected)
///	virtual void Dump() { b2Log("// Dump is not supported for this joint type.\n"); }
//	virtual void ShiftOrigin(const b2Vec2& newOrigin) { B2_NOT_USED(newOrigin);  }
};

Persistent<FunctionTemplate> WrapJoint::g_constructor_template;

void WrapJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplJoint = NanNew<FunctionTemplate>();
	tplJoint->SetClassName(NanNew<String>("b2Joint"));
	tplJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplJoint, GetType)
	CLASS_METHOD_APPLY(tplJoint, GetBodyA)
	CLASS_METHOD_APPLY(tplJoint, GetBodyB)
	CLASS_METHOD_APPLY(tplJoint, GetAnchorA)
	CLASS_METHOD_APPLY(tplJoint, GetAnchorB)
	CLASS_METHOD_APPLY(tplJoint, GetReactionForce)
	CLASS_METHOD_APPLY(tplJoint, GetReactionTorque)
	CLASS_METHOD_APPLY(tplJoint, GetNext)
	CLASS_METHOD_APPLY(tplJoint, GetUserData)
	CLASS_METHOD_APPLY(tplJoint, SetUserData)
	CLASS_METHOD_APPLY(tplJoint, IsActive)
	CLASS_METHOD_APPLY(tplJoint, GetCollideConnected)
	NanAssignPersistent(g_constructor_template, tplJoint);
	exports->Set(NanNew<String>("b2Joint"), NanNew<FunctionTemplate>(g_constructor_template)->GetFunction());
}

CLASS_METHOD_IMPLEMENT(WrapJoint, GetType,
{
	NanReturnValue(NanNew<Integer>(that->GetJoint()->GetType()));
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetBodyA,
{
	if (!that->m_joint_bodyA.IsEmpty())
	{
		NanReturnValue(NanNew<Object>(that->m_joint_bodyA));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetBodyB,
{
	if (!that->m_joint_bodyB.IsEmpty())
	{
		NanReturnValue(NanNew<Object>(that->m_joint_bodyB));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->GetJoint()->GetAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->GetJoint()->GetAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetReactionForce,
{
	float32 inv_dt = (float32) args[0]->NumberValue();
	Local<Object> out = args[1]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[1]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->GetJoint()->GetReactionForce(inv_dt));
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetReactionTorque,
{
	float32 inv_dt = (float32) args[0]->NumberValue();
	NanReturnValue(NanNew<Number>(that->GetJoint()->GetReactionTorque(inv_dt)));
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetNext,
{
	b2Joint* joint = that->GetJoint()->GetNext();
	if (joint)
	{
		// get joint internal data
		WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
		NanReturnValue(NanObjectWrapHandle(wrap_joint));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetUserData,
{
	if (!that->m_joint_userData.IsEmpty())
	{
		NanReturnValue(NanNew<Value>(that->m_joint_userData));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapJoint, SetUserData,
{
	if (!args[0]->IsUndefined())
	{
		NanAssignPersistent(that->m_joint_userData, args[0]);
	}
	else
	{
		NanDisposePersistent(that->m_joint_userData);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapJoint, IsActive,
{
	NanReturnValue(NanNew<Boolean>(that->GetJoint()->IsActive()));
})

CLASS_METHOD_IMPLEMENT(WrapJoint, GetCollideConnected,
{
	NanReturnValue(NanNew<Boolean>(that->GetJoint()->GetCollideConnected()));
})

//// b2RevoluteJointDef

class WrapRevoluteJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2RevoluteJointDef m_revolute_jd;
	Persistent<Object> m_revolute_jd_localAnchorA; // m_revolute_jd.localAnchorA
	Persistent<Object> m_revolute_jd_localAnchorB; // m_revolute_jd.localAnchorB

private:
	WrapRevoluteJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_revolute_jd_localAnchorA, WrapVec2::NewInstance(m_revolute_jd.localAnchorA));
		NanAssignPersistent(m_revolute_jd_localAnchorB, WrapVec2::NewInstance(m_revolute_jd.localAnchorB));
	}
	~WrapRevoluteJointDef()
	{
		NanDisposePersistent(m_revolute_jd_localAnchorA);
		NanDisposePersistent(m_revolute_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_revolute_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_revolute_jd_localAnchorA))->GetVec2(); // struct copy
		m_revolute_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_revolute_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_revolute_jd_localAnchorA))->SetVec2(m_revolute_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_revolute_jd_localAnchorB))->SetVec2(m_revolute_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_revolute_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(referenceAngle)
	CLASS_MEMBER_DECLARE(enableLimit)
	CLASS_MEMBER_DECLARE(lowerAngle)
	CLASS_MEMBER_DECLARE(upperAngle)
	CLASS_MEMBER_DECLARE(enableMotor)
	CLASS_MEMBER_DECLARE(motorSpeed)
	CLASS_MEMBER_DECLARE(maxMotorTorque)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapRevoluteJointDef::g_constructor;

void WrapRevoluteJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplRevoluteJointDef = NanNew<FunctionTemplate>(New);
	tplRevoluteJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplRevoluteJointDef->SetClassName(NanNew<String>("b2RevoluteJointDef"));
	tplRevoluteJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, referenceAngle)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, enableLimit)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, lowerAngle)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, upperAngle)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, enableMotor)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, motorSpeed)
	CLASS_MEMBER_APPLY(tplRevoluteJointDef, maxMotorTorque)
	CLASS_METHOD_APPLY(tplRevoluteJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplRevoluteJointDef->GetFunction());
	exports->Set(NanNew<String>("b2RevoluteJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapRevoluteJointDef::New)
{
	NanScope();
	WrapRevoluteJointDef* that = new WrapRevoluteJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapRevoluteJointDef, m_revolute_jd, localAnchorA				) // m_revolute_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapRevoluteJointDef, m_revolute_jd, localAnchorB				) // m_revolute_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRevoluteJointDef, m_revolute_jd, float32, referenceAngle	)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapRevoluteJointDef, m_revolute_jd, bool, enableLimit			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRevoluteJointDef, m_revolute_jd, float32, lowerAngle		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRevoluteJointDef, m_revolute_jd, float32, upperAngle		)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapRevoluteJointDef, m_revolute_jd, bool, enableMotor			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRevoluteJointDef, m_revolute_jd, float32, motorSpeed		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRevoluteJointDef, m_revolute_jd, float32, maxMotorTorque	)

CLASS_METHOD_IMPLEMENT(WrapRevoluteJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchor = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	that->SyncPull();
	that->m_revolute_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchor->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2RevoluteJoint

class WrapRevoluteJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2RevoluteJoint* m_revolute_joint;

private:
	WrapRevoluteJoint() : m_revolute_joint(NULL) {}
	~WrapRevoluteJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_revolute_joint; } // override WrapJoint

	void SetupObject(WrapRevoluteJointDef* wrap_revolute_jd, b2RevoluteJoint* revolute_joint)
	{
		m_revolute_joint = revolute_joint;

		WrapJoint::SetupObject(wrap_revolute_jd);
	}

	b2RevoluteJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2RevoluteJoint* revolute_joint = m_revolute_joint;
		m_revolute_joint = NULL;
		return revolute_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(GetReferenceAngle)
	CLASS_METHOD_DECLARE(GetJointAngle)
	CLASS_METHOD_DECLARE(GetJointSpeed)
	CLASS_METHOD_DECLARE(IsLimitEnabled)
	CLASS_METHOD_DECLARE(EnableLimit)
	CLASS_METHOD_DECLARE(GetLowerLimit)
	CLASS_METHOD_DECLARE(GetUpperLimit)
	CLASS_METHOD_DECLARE(SetLimits)
	CLASS_METHOD_DECLARE(IsMotorEnabled)
	CLASS_METHOD_DECLARE(EnableMotor)
	CLASS_METHOD_DECLARE(SetMotorSpeed)
	CLASS_METHOD_DECLARE(GetMotorSpeed)
	CLASS_METHOD_DECLARE(SetMaxMotorTorque)
	CLASS_METHOD_DECLARE(GetMaxMotorTorque)
	CLASS_METHOD_DECLARE(GetMotorTorque)
///	void Dump();
};

Persistent<Function> WrapRevoluteJoint::g_constructor;

void WrapRevoluteJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplRevoluteJoint = NanNew<FunctionTemplate>(New);
	tplRevoluteJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplRevoluteJoint->SetClassName(NanNew<String>("b2RevoluteJoint"));
	tplRevoluteJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetReferenceAngle)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetJointAngle)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetJointSpeed)
	CLASS_METHOD_APPLY(tplRevoluteJoint, IsLimitEnabled)
	CLASS_METHOD_APPLY(tplRevoluteJoint, EnableLimit)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetLowerLimit)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetUpperLimit)
	CLASS_METHOD_APPLY(tplRevoluteJoint, SetLimits)
	CLASS_METHOD_APPLY(tplRevoluteJoint, IsMotorEnabled)
	CLASS_METHOD_APPLY(tplRevoluteJoint, EnableMotor)
	CLASS_METHOD_APPLY(tplRevoluteJoint, SetMotorSpeed)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetMotorSpeed)
	CLASS_METHOD_APPLY(tplRevoluteJoint, SetMaxMotorTorque)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetMaxMotorTorque)
	CLASS_METHOD_APPLY(tplRevoluteJoint, GetMotorTorque)
	NanAssignPersistent(g_constructor, tplRevoluteJoint->GetFunction());
	exports->Set(NanNew<String>("b2RevoluteJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapRevoluteJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapRevoluteJoint::New)
{
	NanScope();
	WrapRevoluteJoint* that = new WrapRevoluteJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_revolute_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_revolute_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetReferenceAngle,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetReferenceAngle()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetJointAngle,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetJointAngle()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetJointSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetJointSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, IsLimitEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_revolute_joint->IsLimitEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, EnableLimit,
{
	that->m_revolute_joint->EnableLimit(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetLowerLimit,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetLowerLimit()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetUpperLimit,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetUpperLimit()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, SetLimits,
{
	that->m_revolute_joint->SetLimits((float32) args[0]->NumberValue(), (float32) args[1]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, IsMotorEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_revolute_joint->IsMotorEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, EnableMotor,
{
	that->m_revolute_joint->EnableMotor(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, SetMotorSpeed,
{
	that->m_revolute_joint->SetMotorSpeed((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetMotorSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetMotorSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, SetMaxMotorTorque,
{
	that->m_revolute_joint->SetMaxMotorTorque((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetMaxMotorTorque,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetMaxMotorTorque()));
})

CLASS_METHOD_IMPLEMENT(WrapRevoluteJoint, GetMotorTorque,
{
	NanReturnValue(NanNew<Number>(that->m_revolute_joint->GetMotorTorque((float32) args[0]->NumberValue())));
})

//// b2PrismaticJointDef

class WrapPrismaticJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2PrismaticJointDef m_prismatic_jd;
	Persistent<Object> m_prismatic_jd_localAnchorA; // m_prismatic_jd.localAnchorA
	Persistent<Object> m_prismatic_jd_localAnchorB; // m_prismatic_jd.localAnchorB
	Persistent<Object> m_prismatic_jd_localAxisA;	// m_prismatic_jd.localAxisA

private:
	WrapPrismaticJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_prismatic_jd_localAnchorA, WrapVec2::NewInstance(m_prismatic_jd.localAnchorA));
		NanAssignPersistent(m_prismatic_jd_localAnchorB, WrapVec2::NewInstance(m_prismatic_jd.localAnchorB));
		NanAssignPersistent(m_prismatic_jd_localAxisA, WrapVec2::NewInstance(m_prismatic_jd.localAxisA));
	}
	~WrapPrismaticJointDef()
	{
		NanDisposePersistent(m_prismatic_jd_localAnchorA);
		NanDisposePersistent(m_prismatic_jd_localAnchorB);
		NanDisposePersistent(m_prismatic_jd_localAxisA);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_prismatic_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAnchorA))->GetVec2(); // struct copy
		m_prismatic_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAnchorB))->GetVec2(); // struct copy
		m_prismatic_jd.localAxisA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAxisA))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAnchorA))->SetVec2(m_prismatic_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAnchorB))->SetVec2(m_prismatic_jd.localAnchorB);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_prismatic_jd_localAxisA))->SetVec2(m_prismatic_jd.localAxisA);
	}

	virtual b2JointDef& GetJointDef() { return m_prismatic_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(localAxisA)
	CLASS_MEMBER_DECLARE(referenceAngle)
	CLASS_MEMBER_DECLARE(enableLimit)
	CLASS_MEMBER_DECLARE(lowerTranslation)
	CLASS_MEMBER_DECLARE(upperTranslation)
	CLASS_MEMBER_DECLARE(enableMotor)
	CLASS_MEMBER_DECLARE(maxMotorForce)
	CLASS_MEMBER_DECLARE(motorSpeed)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapPrismaticJointDef::g_constructor;

void WrapPrismaticJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplPrismaticJointDef = NanNew<FunctionTemplate>(New);
	tplPrismaticJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplPrismaticJointDef->SetClassName(NanNew<String>("b2PrismaticJointDef"));
	tplPrismaticJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, localAxisA)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, referenceAngle)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, enableLimit)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, lowerTranslation)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, upperTranslation)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, enableMotor)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, maxMotorForce)
	CLASS_MEMBER_APPLY(tplPrismaticJointDef, motorSpeed)
	CLASS_METHOD_APPLY(tplPrismaticJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplPrismaticJointDef->GetFunction());
	exports->Set(NanNew<String>("b2PrismaticJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapPrismaticJointDef::New)
{
	NanScope();
	WrapPrismaticJointDef* that = new WrapPrismaticJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPrismaticJointDef, m_prismatic_jd, localAnchorA				) // m_prismatic_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPrismaticJointDef, m_prismatic_jd, localAnchorB				) // m_prismatic_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPrismaticJointDef, m_prismatic_jd, localAxisA					) // m_prismatic_jd_localAxisA
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPrismaticJointDef, m_prismatic_jd, float32, referenceAngle		)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapPrismaticJointDef, m_prismatic_jd, bool, enableLimit			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPrismaticJointDef, m_prismatic_jd, float32, lowerTranslation	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPrismaticJointDef, m_prismatic_jd, float32, upperTranslation	)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapPrismaticJointDef, m_prismatic_jd, bool, enableMotor			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPrismaticJointDef, m_prismatic_jd, float32, maxMotorForce		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPrismaticJointDef, m_prismatic_jd, float32, motorSpeed			)

CLASS_METHOD_IMPLEMENT(WrapPrismaticJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchor = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	WrapVec2* wrap_axis = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[3]));
	that->SyncPull();
	that->m_prismatic_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchor->GetVec2(), wrap_axis->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2PrismaticJoint

class WrapPrismaticJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2PrismaticJoint* m_prismatic_joint;

private:
	WrapPrismaticJoint() : m_prismatic_joint(NULL) {}
	~WrapPrismaticJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_prismatic_joint; } // override WrapJoint

	void SetupObject(WrapPrismaticJointDef* wrap_prismatic_jd, b2PrismaticJoint* prismatic_joint)
	{
		m_prismatic_joint = prismatic_joint;

		WrapJoint::SetupObject(wrap_prismatic_jd);
	}

	b2PrismaticJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2PrismaticJoint* prismatic_joint = m_prismatic_joint;
		m_prismatic_joint = NULL;
		return prismatic_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(GetLocalAxisA)
	CLASS_METHOD_DECLARE(GetReferenceAngle)
	CLASS_METHOD_DECLARE(GetJointTranslation)
	CLASS_METHOD_DECLARE(GetJointSpeed)
	CLASS_METHOD_DECLARE(IsLimitEnabled)
	CLASS_METHOD_DECLARE(EnableLimit)
	CLASS_METHOD_DECLARE(GetLowerLimit)
	CLASS_METHOD_DECLARE(GetUpperLimit)
	CLASS_METHOD_DECLARE(SetLimits)
	CLASS_METHOD_DECLARE(IsMotorEnabled)
	CLASS_METHOD_DECLARE(EnableMotor)
	CLASS_METHOD_DECLARE(SetMotorSpeed)
	CLASS_METHOD_DECLARE(GetMotorSpeed)
	CLASS_METHOD_DECLARE(SetMaxMotorForce)
	CLASS_METHOD_DECLARE(GetMaxMotorForce)
	CLASS_METHOD_DECLARE(GetMotorForce)
///	void Dump();
};

Persistent<Function> WrapPrismaticJoint::g_constructor;

void WrapPrismaticJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplPrismaticJoint = NanNew<FunctionTemplate>(New);
	tplPrismaticJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplPrismaticJoint->SetClassName(NanNew<String>("b2PrismaticJoint"));
	tplPrismaticJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetLocalAxisA)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetReferenceAngle)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetJointTranslation)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetJointSpeed)
	CLASS_METHOD_APPLY(tplPrismaticJoint, IsLimitEnabled)
	CLASS_METHOD_APPLY(tplPrismaticJoint, EnableLimit)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetLowerLimit)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetUpperLimit)
	CLASS_METHOD_APPLY(tplPrismaticJoint, SetLimits)
	CLASS_METHOD_APPLY(tplPrismaticJoint, IsMotorEnabled)
	CLASS_METHOD_APPLY(tplPrismaticJoint, EnableMotor)
	CLASS_METHOD_APPLY(tplPrismaticJoint, SetMotorSpeed)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetMotorSpeed)
	CLASS_METHOD_APPLY(tplPrismaticJoint, SetMaxMotorForce)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetMaxMotorForce)
	CLASS_METHOD_APPLY(tplPrismaticJoint, GetMotorForce)
	NanAssignPersistent(g_constructor, tplPrismaticJoint->GetFunction());
	exports->Set(NanNew<String>("b2PrismaticJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapPrismaticJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapPrismaticJoint::New)
{
	NanScope();
	WrapPrismaticJoint* that = new WrapPrismaticJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_prismatic_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_prismatic_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetLocalAxisA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_prismatic_joint->GetLocalAxisA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetReferenceAngle,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetReferenceAngle()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetJointTranslation,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetJointTranslation()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetJointSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetJointSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, IsLimitEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_prismatic_joint->IsLimitEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, EnableLimit,
{
	that->m_prismatic_joint->EnableLimit(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetLowerLimit,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetLowerLimit()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetUpperLimit,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetUpperLimit()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, SetLimits,
{
	that->m_prismatic_joint->SetLimits((float32) args[0]->NumberValue(), (float32) args[1]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, IsMotorEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_prismatic_joint->IsMotorEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, EnableMotor,
{
	that->m_prismatic_joint->EnableMotor(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, SetMotorSpeed,
{
	that->m_prismatic_joint->SetMotorSpeed((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetMotorSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetMotorSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, SetMaxMotorForce,
{
	that->m_prismatic_joint->SetMaxMotorForce((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetMaxMotorForce,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetMaxMotorForce()));
})

CLASS_METHOD_IMPLEMENT(WrapPrismaticJoint, GetMotorForce,
{
	NanReturnValue(NanNew<Number>(that->m_prismatic_joint->GetMotorForce((float32) args[0]->NumberValue())));
})

//// b2DistanceJointDef

class WrapDistanceJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2DistanceJointDef m_distance_jd;
	Persistent<Object> m_distance_jd_localAnchorA; // m_distance_jd.localAnchorA
	Persistent<Object> m_distance_jd_localAnchorB; // m_distance_jd.localAnchorB

private:
	WrapDistanceJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_distance_jd_localAnchorA, WrapVec2::NewInstance(m_distance_jd.localAnchorA));
		NanAssignPersistent(m_distance_jd_localAnchorB, WrapVec2::NewInstance(m_distance_jd.localAnchorB));
	}
	~WrapDistanceJointDef()
	{
		NanDisposePersistent(m_distance_jd_localAnchorA);
		NanDisposePersistent(m_distance_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_distance_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_distance_jd_localAnchorA))->GetVec2(); // struct copy
		m_distance_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_distance_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_distance_jd_localAnchorA))->SetVec2(m_distance_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_distance_jd_localAnchorB))->SetVec2(m_distance_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_distance_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(length)
	CLASS_MEMBER_DECLARE(frequencyHz)
	CLASS_MEMBER_DECLARE(dampingRatio)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapDistanceJointDef::g_constructor;

void WrapDistanceJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplDistanceJointDef = NanNew<FunctionTemplate>(New);
	tplDistanceJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplDistanceJointDef->SetClassName(NanNew<String>("b2DistanceJointDef"));
	tplDistanceJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplDistanceJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplDistanceJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplDistanceJointDef, length)
	CLASS_MEMBER_APPLY(tplDistanceJointDef, frequencyHz)
	CLASS_MEMBER_APPLY(tplDistanceJointDef, dampingRatio)
	CLASS_METHOD_APPLY(tplDistanceJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplDistanceJointDef->GetFunction());
	exports->Set(NanNew<String>("b2DistanceJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapDistanceJointDef::New)
{
	NanScope();
	WrapDistanceJointDef* that = new WrapDistanceJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapDistanceJointDef, m_distance_jd, localAnchorA				) // m_distance_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapDistanceJointDef, m_distance_jd, localAnchorB				) // m_distance_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapDistanceJointDef, m_distance_jd, float32, length			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapDistanceJointDef, m_distance_jd, float32, frequencyHz		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapDistanceJointDef, m_distance_jd, float32, dampingRatio		)

CLASS_METHOD_IMPLEMENT(WrapDistanceJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchorA = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	WrapVec2* wrap_anchorB = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[3]));
	that->SyncPull();
	that->m_distance_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchorA->GetVec2(), wrap_anchorB->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2DistanceJoint

class WrapDistanceJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2DistanceJoint* m_distance_joint;

private:
	WrapDistanceJoint() : m_distance_joint(NULL) {}
	~WrapDistanceJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_distance_joint; } // override WrapJoint

	void SetupObject(WrapDistanceJointDef* wrap_distance_jd, b2DistanceJoint* distance_joint)
	{
		m_distance_joint = distance_joint;

		WrapJoint::SetupObject(wrap_distance_jd);
	}

	b2DistanceJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2DistanceJoint* distance_joint = m_distance_joint;
		m_distance_joint = NULL;
		return distance_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(SetLength)
	CLASS_METHOD_DECLARE(GetLength)
	CLASS_METHOD_DECLARE(SetFrequency)
	CLASS_METHOD_DECLARE(GetFrequency)
	CLASS_METHOD_DECLARE(SetDampingRatio)
	CLASS_METHOD_DECLARE(GetDampingRatio)
///	void Dump();
};

Persistent<Function> WrapDistanceJoint::g_constructor;

void WrapDistanceJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplDistanceJoint = NanNew<FunctionTemplate>(New);
	tplDistanceJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplDistanceJoint->SetClassName(NanNew<String>("b2DistanceJoint"));
	tplDistanceJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplDistanceJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplDistanceJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplDistanceJoint, SetLength)
	CLASS_METHOD_APPLY(tplDistanceJoint, GetLength)
	CLASS_METHOD_APPLY(tplDistanceJoint, SetFrequency)
	CLASS_METHOD_APPLY(tplDistanceJoint, GetFrequency)
	CLASS_METHOD_APPLY(tplDistanceJoint, SetDampingRatio)
	CLASS_METHOD_APPLY(tplDistanceJoint, GetDampingRatio)
	NanAssignPersistent(g_constructor, tplDistanceJoint->GetFunction());
	exports->Set(NanNew<String>("b2DistanceJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapDistanceJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapDistanceJoint::New)
{
	NanScope();
	WrapDistanceJoint* that = new WrapDistanceJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_distance_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_distance_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, SetLength,
{
	that->m_distance_joint->SetLength((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, GetLength,
{
	NanReturnValue(NanNew<Number>(that->m_distance_joint->GetLength()));
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, SetFrequency,
{
	that->m_distance_joint->SetFrequency((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, GetFrequency,
{
	NanReturnValue(NanNew<Number>(that->m_distance_joint->GetFrequency()));
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, SetDampingRatio,
{
	that->m_distance_joint->SetDampingRatio((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapDistanceJoint, GetDampingRatio,
{
	NanReturnValue(NanNew<Number>(that->m_distance_joint->GetDampingRatio()));
})

//// b2PulleyJointDef

class WrapPulleyJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2PulleyJointDef m_pulley_jd;
	Persistent<Object> m_pulley_jd_groundAnchorA; // m_pulley_jd.groundAnchorA
	Persistent<Object> m_pulley_jd_groundAnchorB; // m_pulley_jd.groundAnchorB
	Persistent<Object> m_pulley_jd_localAnchorA; // m_pulley_jd.localAnchorA
	Persistent<Object> m_pulley_jd_localAnchorB; // m_pulley_jd.localAnchorB

private:
	WrapPulleyJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_pulley_jd_groundAnchorA, WrapVec2::NewInstance(m_pulley_jd.groundAnchorA));
		NanAssignPersistent(m_pulley_jd_groundAnchorB, WrapVec2::NewInstance(m_pulley_jd.groundAnchorB));
		NanAssignPersistent(m_pulley_jd_localAnchorA, WrapVec2::NewInstance(m_pulley_jd.localAnchorA));
		NanAssignPersistent(m_pulley_jd_localAnchorB, WrapVec2::NewInstance(m_pulley_jd.localAnchorB));
	}
	~WrapPulleyJointDef()
	{
		NanDisposePersistent(m_pulley_jd_localAnchorA);
		NanDisposePersistent(m_pulley_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_pulley_jd.groundAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_groundAnchorA))->GetVec2(); // struct copy
		m_pulley_jd.groundAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_groundAnchorB))->GetVec2(); // struct copy
		m_pulley_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_localAnchorA))->GetVec2(); // struct copy
		m_pulley_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_groundAnchorA))->SetVec2(m_pulley_jd.groundAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_groundAnchorB))->SetVec2(m_pulley_jd.groundAnchorB);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_localAnchorA))->SetVec2(m_pulley_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pulley_jd_localAnchorB))->SetVec2(m_pulley_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_pulley_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(groundAnchorA)
	CLASS_MEMBER_DECLARE(groundAnchorB)
	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(lengthA)
	CLASS_MEMBER_DECLARE(lengthB)
	CLASS_MEMBER_DECLARE(ratio)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapPulleyJointDef::g_constructor;

void WrapPulleyJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplPulleyJointDef = NanNew<FunctionTemplate>(New);
	tplPulleyJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplPulleyJointDef->SetClassName(NanNew<String>("b2PulleyJointDef"));
	tplPulleyJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplPulleyJointDef, groundAnchorA)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, groundAnchorB)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, lengthA)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, lengthB)
	CLASS_MEMBER_APPLY(tplPulleyJointDef, ratio)
	CLASS_METHOD_APPLY(tplPulleyJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplPulleyJointDef->GetFunction());
	exports->Set(NanNew<String>("b2PulleyJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapPulleyJointDef::New)
{
	NanScope();
	WrapPulleyJointDef* that = new WrapPulleyJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPulleyJointDef, m_pulley_jd, groundAnchorA		) // m_pulley_jd_groundAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPulleyJointDef, m_pulley_jd, groundAnchorB		) // m_pulley_jd_groundAnchorB
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPulleyJointDef, m_pulley_jd, localAnchorA		) // m_pulley_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapPulleyJointDef, m_pulley_jd, localAnchorB		) // m_pulley_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPulleyJointDef, m_pulley_jd, float32, lengthA	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPulleyJointDef, m_pulley_jd, float32, lengthB	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapPulleyJointDef, m_pulley_jd, float32, ratio	)

CLASS_METHOD_IMPLEMENT(WrapPulleyJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_groundAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	WrapVec2* wrap_groundAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[3]));
	WrapVec2* wrap_anchorA = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[4]));
	WrapVec2* wrap_anchorB = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[5]));
	float32 ratio = (float32) args[6]->NumberValue();
	that->SyncPull();
	that->m_pulley_jd.Initialize(
	   wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), 
	   wrap_groundAnchorA->GetVec2(), wrap_groundAnchorB->GetVec2(), 
	   wrap_anchorA->GetVec2(), wrap_anchorB->GetVec2(), 
	   ratio);
	that->SyncPush();
	NanReturnUndefined();
})

//// b2PulleyJoint

class WrapPulleyJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2PulleyJoint* m_pulley_joint;

private:
	WrapPulleyJoint() : m_pulley_joint(NULL) {}
	~WrapPulleyJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_pulley_joint; } // override WrapJoint

	void SetupObject(WrapPulleyJointDef* wrap_pulley_jd, b2PulleyJoint* pulley_joint)
	{
		m_pulley_joint = pulley_joint;

		WrapJoint::SetupObject(wrap_pulley_jd);
	}

	b2PulleyJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2PulleyJoint* pulley_joint = m_pulley_joint;
		m_pulley_joint = NULL;
		return pulley_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetGroundAnchorA)
	CLASS_METHOD_DECLARE(GetGroundAnchorB)
	CLASS_METHOD_DECLARE(GetLengthA)
	CLASS_METHOD_DECLARE(GetLengthB)
	CLASS_METHOD_DECLARE(GetRatio)
	CLASS_METHOD_DECLARE(GetCurrentLengthA)
	CLASS_METHOD_DECLARE(GetCurrentLengthB)
///	void Dump();
};

Persistent<Function> WrapPulleyJoint::g_constructor;

void WrapPulleyJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplPulleyJoint = NanNew<FunctionTemplate>(New);
	tplPulleyJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplPulleyJoint->SetClassName(NanNew<String>("b2PulleyJoint"));
	tplPulleyJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplPulleyJoint, GetGroundAnchorA)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetGroundAnchorB)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetLengthA)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetLengthB)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetRatio)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetCurrentLengthA)
	CLASS_METHOD_APPLY(tplPulleyJoint, GetCurrentLengthB)
	NanAssignPersistent(g_constructor, tplPulleyJoint->GetFunction());
	exports->Set(NanNew<String>("b2PulleyJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapPulleyJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapPulleyJoint::New)
{
	NanScope();
	WrapPulleyJoint* that = new WrapPulleyJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetGroundAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_pulley_joint->GetGroundAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetGroundAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_pulley_joint->GetGroundAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetLengthA,
{
	NanReturnValue(NanNew<Number>(that->m_pulley_joint->GetLengthA()));
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetLengthB,
{
	NanReturnValue(NanNew<Number>(that->m_pulley_joint->GetLengthB()));
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetRatio,
{
	NanReturnValue(NanNew<Number>(that->m_pulley_joint->GetRatio()));
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetCurrentLengthA,
{
	NanReturnValue(NanNew<Number>(that->m_pulley_joint->GetCurrentLengthA()));
})

CLASS_METHOD_IMPLEMENT(WrapPulleyJoint, GetCurrentLengthB,
{
	NanReturnValue(NanNew<Number>(that->m_pulley_joint->GetCurrentLengthB()));
})

//// b2MouseJointDef

class WrapMouseJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2MouseJointDef m_mouse_jd;
	Persistent<Object> m_mouse_jd_target; // m_mouse_jd.target

private:
	WrapMouseJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_mouse_jd_target, WrapVec2::NewInstance(m_mouse_jd.target));
	}
	~WrapMouseJointDef()
	{
		NanDisposePersistent(m_mouse_jd_target);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_mouse_jd.target = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_mouse_jd_target))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_mouse_jd_target))->SetVec2(m_mouse_jd.target);
	}

	virtual b2JointDef& GetJointDef() { return m_mouse_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(target)
	CLASS_MEMBER_DECLARE(maxForce)
	CLASS_MEMBER_DECLARE(frequencyHz)
	CLASS_MEMBER_DECLARE(dampingRatio)
};

Persistent<Function> WrapMouseJointDef::g_constructor;

void WrapMouseJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplMouseJointDef = NanNew<FunctionTemplate>(New);
	tplMouseJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplMouseJointDef->SetClassName(NanNew<String>("b2MouseJointDef"));
	tplMouseJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplMouseJointDef, target)
	CLASS_MEMBER_APPLY(tplMouseJointDef, maxForce)
	CLASS_MEMBER_APPLY(tplMouseJointDef, frequencyHz)
	CLASS_MEMBER_APPLY(tplMouseJointDef, dampingRatio)
	NanAssignPersistent(g_constructor, tplMouseJointDef->GetFunction());
	exports->Set(NanNew<String>("b2MouseJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapMouseJointDef::New)
{
	NanScope();
	WrapMouseJointDef* that = new WrapMouseJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapMouseJointDef, m_mouse_jd, target					) // m_mouse_jd_target
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMouseJointDef, m_mouse_jd, float32, maxForce		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMouseJointDef, m_mouse_jd, float32, frequencyHz	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMouseJointDef, m_mouse_jd, float32, dampingRatio	)

//// b2MouseJoint

class WrapMouseJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2MouseJoint* m_mouse_joint;

private:
	WrapMouseJoint() : m_mouse_joint(NULL) {}
	~WrapMouseJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_mouse_joint; } // override WrapJoint

	void SetupObject(WrapMouseJointDef* wrap_mouse_jd, b2MouseJoint* mouse_joint)
	{
		m_mouse_joint = mouse_joint;

		WrapJoint::SetupObject(wrap_mouse_jd);
	}

	b2MouseJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2MouseJoint* mouse_joint = m_mouse_joint;
		m_mouse_joint = NULL;
		return mouse_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(SetTarget)
	CLASS_METHOD_DECLARE(GetTarget)
	CLASS_METHOD_DECLARE(SetMaxForce)
	CLASS_METHOD_DECLARE(GetMaxForce)
	CLASS_METHOD_DECLARE(SetFrequency)
	CLASS_METHOD_DECLARE(GetFrequency)
	CLASS_METHOD_DECLARE(SetDampingRatio)
	CLASS_METHOD_DECLARE(GetDampingRatio)
///	void Dump();
};

Persistent<Function> WrapMouseJoint::g_constructor;

void WrapMouseJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplMouseJoint = NanNew<FunctionTemplate>(New);
	tplMouseJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplMouseJoint->SetClassName(NanNew<String>("b2MouseJoint"));
	tplMouseJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplMouseJoint, SetTarget)
	CLASS_METHOD_APPLY(tplMouseJoint, GetTarget)
	CLASS_METHOD_APPLY(tplMouseJoint, SetMaxForce)
	CLASS_METHOD_APPLY(tplMouseJoint, GetMaxForce)
	CLASS_METHOD_APPLY(tplMouseJoint, SetFrequency)
	CLASS_METHOD_APPLY(tplMouseJoint, GetFrequency)
	CLASS_METHOD_APPLY(tplMouseJoint, SetDampingRatio)
	CLASS_METHOD_APPLY(tplMouseJoint, GetDampingRatio)
	NanAssignPersistent(g_constructor, tplMouseJoint->GetFunction());
	exports->Set(NanNew<String>("b2MouseJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapMouseJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapMouseJoint::New)
{
	NanScope();
	WrapMouseJoint* that = new WrapMouseJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, SetTarget,
{
	WrapVec2* wrap_target = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	that->m_mouse_joint->SetTarget(wrap_target->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, GetTarget,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_mouse_joint->GetTarget());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, SetMaxForce,
{
	that->m_mouse_joint->SetMaxForce((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, GetMaxForce,
{
	NanReturnValue(NanNew<Number>(that->m_mouse_joint->GetMaxForce()));
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, SetFrequency,
{
	that->m_mouse_joint->SetFrequency((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, GetFrequency,
{
	NanReturnValue(NanNew<Number>(that->m_mouse_joint->GetFrequency()));
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, SetDampingRatio,
{
	that->m_mouse_joint->SetDampingRatio((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMouseJoint, GetDampingRatio,
{
	NanReturnValue(NanNew<Number>(that->m_mouse_joint->GetDampingRatio()));
})

//// b2GearJointDef

class WrapGearJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2GearJointDef m_gear_jd;
	Persistent<Object> m_gear_jd_joint1; // m_gear_jd.joint1
	Persistent<Object> m_gear_jd_joint2; // m_gear_jd.joint2

private:
	WrapGearJointDef()
	{
		// create javascript objects
	}
	~WrapGearJointDef()
	{
		NanDisposePersistent(m_gear_jd_joint1);
		NanDisposePersistent(m_gear_jd_joint2);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_gear_jd.joint1 = node::ObjectWrap::Unwrap<WrapJoint>(NanNew<Object>(m_gear_jd_joint1))->GetJoint();
		m_gear_jd.joint2 = node::ObjectWrap::Unwrap<WrapJoint>(NanNew<Object>(m_gear_jd_joint2))->GetJoint();
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		NanAssignPersistent(m_gear_jd_joint1, NanObjectWrapHandle(WrapJoint::GetWrap(m_gear_jd.joint1)));
		NanAssignPersistent(m_gear_jd_joint2, NanObjectWrapHandle(WrapJoint::GetWrap(m_gear_jd.joint2)));
	}

	virtual b2JointDef& GetJointDef() { return m_gear_jd; } // override WrapJointDef

	Handle<Object> GetJoint1Handle() { return NanNew<Object>(m_gear_jd_joint1); }
	Handle<Object> GetJoint2Handle() { return NanNew<Object>(m_gear_jd_joint2); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(joint1)
	CLASS_MEMBER_DECLARE(joint2)
	CLASS_MEMBER_DECLARE(ratio)
};

Persistent<Function> WrapGearJointDef::g_constructor;

void WrapGearJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplGearJointDef = NanNew<FunctionTemplate>(New);
	tplGearJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplGearJointDef->SetClassName(NanNew<String>("b2GearJointDef"));
	tplGearJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplGearJointDef, joint1)
	CLASS_MEMBER_APPLY(tplGearJointDef, joint2)
	CLASS_MEMBER_APPLY(tplGearJointDef, ratio)
	NanAssignPersistent(g_constructor, tplGearJointDef->GetFunction());
	exports->Set(NanNew<String>("b2GearJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapGearJointDef::New)
{
	NanScope();
	WrapGearJointDef* that = new WrapGearJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapGearJointDef, m_gear_jd, joint1			) // m_gear_jd_joint1
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapGearJointDef, m_gear_jd, joint2			) // m_gear_jd_joint2
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapGearJointDef, m_gear_jd, float32, ratio	)

//// b2GearJoint

class WrapGearJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2GearJoint* m_gear_joint;
	Persistent<Object> m_gear_joint_joint1;
	Persistent<Object> m_gear_joint_joint2;

private:
	WrapGearJoint() : m_gear_joint(NULL) {}
	~WrapGearJoint()
	{
		NanDisposePersistent(m_gear_joint_joint1);
		NanDisposePersistent(m_gear_joint_joint2);
	}

public:
	virtual b2Joint* GetJoint() { return m_gear_joint; } // override WrapJoint

	void SetupObject(WrapGearJointDef* wrap_gear_jd, b2GearJoint* gear_joint)
	{
		m_gear_joint = gear_joint;

		NanAssignPersistent(m_gear_joint_joint1, NanObjectWrapHandle(WrapJoint::GetWrap(gear_joint->GetJoint1())));
		NanAssignPersistent(m_gear_joint_joint2, NanObjectWrapHandle(WrapJoint::GetWrap(gear_joint->GetJoint2())));

		WrapJoint::SetupObject(wrap_gear_jd);
	}

	b2GearJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		NanDisposePersistent(m_gear_joint_joint1);
		NanDisposePersistent(m_gear_joint_joint2);

		b2GearJoint* gear_joint = m_gear_joint;
		m_gear_joint = NULL;
		return gear_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetJoint1)
	CLASS_METHOD_DECLARE(GetJoint2)
	CLASS_METHOD_DECLARE(SetRatio)
	CLASS_METHOD_DECLARE(GetRatio)
///	void Dump();
};

Persistent<Function> WrapGearJoint::g_constructor;

void WrapGearJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplGearJoint = NanNew<FunctionTemplate>(New);
	tplGearJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplGearJoint->SetClassName(NanNew<String>("b2GearJoint"));
	tplGearJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplGearJoint, GetJoint1)
	CLASS_METHOD_APPLY(tplGearJoint, GetJoint2)
	CLASS_METHOD_APPLY(tplGearJoint, SetRatio)
	CLASS_METHOD_APPLY(tplGearJoint, GetRatio)
	NanAssignPersistent(g_constructor, tplGearJoint->GetFunction());
	exports->Set(NanNew<String>("b2GearJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapGearJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapGearJoint::New)
{
	NanScope();
	WrapGearJoint* that = new WrapGearJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapGearJoint, GetJoint1,  
{
	if (!that->m_gear_joint_joint1.IsEmpty())
	{
		NanReturnValue(NanNew<Object>(that->m_gear_joint_joint1));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapGearJoint, GetJoint2,  
{
	if (!that->m_gear_joint_joint2.IsEmpty())
	{
		NanReturnValue(NanNew<Object>(that->m_gear_joint_joint2));
	}
	else
	{
		NanReturnNull();
	}
})

CLASS_METHOD_IMPLEMENT(WrapGearJoint, SetRatio, 
{
	that->m_gear_joint->SetRatio((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapGearJoint, GetRatio, 
{
	NanReturnValue(NanNew<Number>(that->m_gear_joint->GetRatio()));
})

//// b2WheelJointDef

class WrapWheelJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2WheelJointDef m_wheel_jd;
	Persistent<Object> m_wheel_jd_localAnchorA; // m_wheel_jd.localAnchorA
	Persistent<Object> m_wheel_jd_localAnchorB; // m_wheel_jd.localAnchorB
	Persistent<Object> m_wheel_jd_localAxisA; // m_wheel_jd.localAxisA

private:
	WrapWheelJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_wheel_jd_localAnchorA, WrapVec2::NewInstance(m_wheel_jd.localAnchorA));
		NanAssignPersistent(m_wheel_jd_localAnchorB, WrapVec2::NewInstance(m_wheel_jd.localAnchorB));
		NanAssignPersistent(m_wheel_jd_localAxisA, WrapVec2::NewInstance(m_wheel_jd.localAxisA));
	}
	~WrapWheelJointDef()
	{
		NanDisposePersistent(m_wheel_jd_localAnchorA);
		NanDisposePersistent(m_wheel_jd_localAnchorB);
		NanDisposePersistent(m_wheel_jd_localAxisA);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_wheel_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAnchorA))->GetVec2(); // struct copy
		m_wheel_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAnchorB))->GetVec2(); // struct copy
		m_wheel_jd.localAxisA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAxisA))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAnchorA))->SetVec2(m_wheel_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAnchorB))->SetVec2(m_wheel_jd.localAnchorB);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_wheel_jd_localAxisA))->SetVec2(m_wheel_jd.localAxisA);
	}

	virtual b2JointDef& GetJointDef() { return m_wheel_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(localAxisA)
	CLASS_MEMBER_DECLARE(enableMotor)
	CLASS_MEMBER_DECLARE(maxMotorTorque)
	CLASS_MEMBER_DECLARE(motorSpeed)
	CLASS_MEMBER_DECLARE(frequencyHz)
	CLASS_MEMBER_DECLARE(dampingRatio)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapWheelJointDef::g_constructor;

void WrapWheelJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWheelJointDef = NanNew<FunctionTemplate>(New);
	tplWheelJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplWheelJointDef->SetClassName(NanNew<String>("b2WheelJointDef"));
	tplWheelJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplWheelJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplWheelJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplWheelJointDef, localAxisA)
	CLASS_MEMBER_APPLY(tplWheelJointDef, enableMotor)
	CLASS_MEMBER_APPLY(tplWheelJointDef, maxMotorTorque)
	CLASS_MEMBER_APPLY(tplWheelJointDef, motorSpeed)
	CLASS_MEMBER_APPLY(tplWheelJointDef, frequencyHz)
	CLASS_MEMBER_APPLY(tplWheelJointDef, dampingRatio)
	CLASS_METHOD_APPLY(tplWheelJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplWheelJointDef->GetFunction());
	exports->Set(NanNew<String>("b2WheelJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapWheelJointDef::New)
{
	NanScope();
	WrapWheelJointDef* that = new WrapWheelJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWheelJointDef, m_wheel_jd, localAnchorA			) // m_wheel_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWheelJointDef, m_wheel_jd, localAnchorB			) // m_wheel_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWheelJointDef, m_wheel_jd, localAxisA				) // m_wheel_jd_localAxisA
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapWheelJointDef, m_wheel_jd, bool, enableMotor		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWheelJointDef, m_wheel_jd, float32, maxMotorTorque	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWheelJointDef, m_wheel_jd, float32, motorSpeed		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWheelJointDef, m_wheel_jd, float32, frequencyHz	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWheelJointDef, m_wheel_jd, float32, dampingRatio	)

CLASS_METHOD_IMPLEMENT(WrapWheelJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchor = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	WrapVec2* wrap_axis = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[3]));
	that->SyncPull();
	that->m_wheel_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchor->GetVec2(), wrap_axis->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2WheelJoint

class WrapWheelJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2WheelJoint* m_wheel_joint;

private:
	WrapWheelJoint() : m_wheel_joint(NULL) {}
	~WrapWheelJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_wheel_joint; } // override WrapJoint

	void SetupObject(WrapWheelJointDef* wrap_wheel_jd, b2WheelJoint* wheel_joint)
	{
		m_wheel_joint = wheel_joint;

		WrapJoint::SetupObject(wrap_wheel_jd);
	}

	b2WheelJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2WheelJoint* wheel_joint = m_wheel_joint;
		m_wheel_joint = NULL;
		return wheel_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(GetLocalAxisA)
	CLASS_METHOD_DECLARE(GetJointTranslation)
	CLASS_METHOD_DECLARE(GetJointSpeed)
	CLASS_METHOD_DECLARE(IsMotorEnabled)
	CLASS_METHOD_DECLARE(EnableMotor)
	CLASS_METHOD_DECLARE(SetMotorSpeed)
	CLASS_METHOD_DECLARE(GetMotorSpeed)
	CLASS_METHOD_DECLARE(SetMaxMotorTorque)
	CLASS_METHOD_DECLARE(GetMaxMotorTorque)
	CLASS_METHOD_DECLARE(GetMotorTorque)
	CLASS_METHOD_DECLARE(SetSpringFrequencyHz)
	CLASS_METHOD_DECLARE(GetSpringFrequencyHz)
	CLASS_METHOD_DECLARE(SetSpringDampingRatio)
	CLASS_METHOD_DECLARE(GetSpringDampingRatio)
///	void Dump();
};

Persistent<Function> WrapWheelJoint::g_constructor;

void WrapWheelJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWheelJoint = NanNew<FunctionTemplate>(New);
	tplWheelJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplWheelJoint->SetClassName(NanNew<String>("b2WheelJoint"));
	tplWheelJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplWheelJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplWheelJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplWheelJoint, GetLocalAxisA)
	CLASS_METHOD_APPLY(tplWheelJoint, GetJointTranslation)
	CLASS_METHOD_APPLY(tplWheelJoint, GetJointSpeed)
	CLASS_METHOD_APPLY(tplWheelJoint, IsMotorEnabled)
	CLASS_METHOD_APPLY(tplWheelJoint, EnableMotor)
	CLASS_METHOD_APPLY(tplWheelJoint, SetMotorSpeed)
	CLASS_METHOD_APPLY(tplWheelJoint, GetMotorSpeed)
	CLASS_METHOD_APPLY(tplWheelJoint, SetMaxMotorTorque)
	CLASS_METHOD_APPLY(tplWheelJoint, GetMaxMotorTorque)
	CLASS_METHOD_APPLY(tplWheelJoint, GetMotorTorque)
	CLASS_METHOD_APPLY(tplWheelJoint, SetSpringFrequencyHz)
	CLASS_METHOD_APPLY(tplWheelJoint, GetSpringFrequencyHz)
	CLASS_METHOD_APPLY(tplWheelJoint, SetSpringDampingRatio)
	CLASS_METHOD_APPLY(tplWheelJoint, GetSpringDampingRatio)
	NanAssignPersistent(g_constructor, tplWheelJoint->GetFunction());
	exports->Set(NanNew<String>("b2WheelJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapWheelJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapWheelJoint::New)
{
	NanScope();
	WrapWheelJoint* that = new WrapWheelJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_wheel_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_wheel_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetLocalAxisA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_wheel_joint->GetLocalAxisA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetJointTranslation,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetJointTranslation()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetJointSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetJointSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, IsMotorEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_wheel_joint->IsMotorEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, EnableMotor,
{
	that->m_wheel_joint->EnableMotor(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, SetMotorSpeed,
{
	that->m_wheel_joint->SetMotorSpeed((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetMotorSpeed,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetMotorSpeed()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, SetMaxMotorTorque,
{
	that->m_wheel_joint->SetMaxMotorTorque((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetMaxMotorTorque,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetMaxMotorTorque()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetMotorTorque,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetMotorTorque((float32) args[0]->NumberValue())));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, SetSpringFrequencyHz,
{
	that->m_wheel_joint->SetSpringFrequencyHz((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetSpringFrequencyHz,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetSpringFrequencyHz()));
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, SetSpringDampingRatio,
{
	that->m_wheel_joint->SetSpringDampingRatio((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWheelJoint, GetSpringDampingRatio,
{
	NanReturnValue(NanNew<Number>(that->m_wheel_joint->GetSpringDampingRatio()));
})

//// b2WeldJointDef

class WrapWeldJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2WeldJointDef m_weld_jd;
	Persistent<Object> m_weld_jd_localAnchorA; // m_weld_jd.localAnchorA
	Persistent<Object> m_weld_jd_localAnchorB; // m_weld_jd.localAnchorB

private:
	WrapWeldJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_weld_jd_localAnchorA, WrapVec2::NewInstance(m_weld_jd.localAnchorA));
		NanAssignPersistent(m_weld_jd_localAnchorB, WrapVec2::NewInstance(m_weld_jd.localAnchorB));
	}
	~WrapWeldJointDef()
	{
		NanDisposePersistent(m_weld_jd_localAnchorA);
		NanDisposePersistent(m_weld_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_weld_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_weld_jd_localAnchorA))->GetVec2(); // struct copy
		m_weld_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_weld_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_weld_jd_localAnchorA))->SetVec2(m_weld_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_weld_jd_localAnchorB))->SetVec2(m_weld_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_weld_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(referenceAngle)
	CLASS_MEMBER_DECLARE(frequencyHz)
	CLASS_MEMBER_DECLARE(dampingRatio)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapWeldJointDef::g_constructor;

void WrapWeldJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWeldJointDef = NanNew<FunctionTemplate>(New);
	tplWeldJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplWeldJointDef->SetClassName(NanNew<String>("b2WeldJointDef"));
	tplWeldJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplWeldJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplWeldJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplWeldJointDef, referenceAngle)
	CLASS_MEMBER_APPLY(tplWeldJointDef, frequencyHz)
	CLASS_MEMBER_APPLY(tplWeldJointDef, dampingRatio)
	CLASS_METHOD_APPLY(tplWeldJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplWeldJointDef->GetFunction());
	exports->Set(NanNew<String>("b2WeldJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapWeldJointDef::New)
{
	NanScope();
	WrapWeldJointDef* that = new WrapWeldJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWeldJointDef, m_weld_jd, localAnchorA				) // m_weld_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWeldJointDef, m_weld_jd, localAnchorB				) // m_weld_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWeldJointDef, m_weld_jd, float32, referenceAngle	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWeldJointDef, m_weld_jd, float32, frequencyHz		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapWeldJointDef, m_weld_jd, float32, dampingRatio		)

CLASS_METHOD_IMPLEMENT(WrapWeldJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchor = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	that->SyncPull();
	that->m_weld_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchor->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2WeldJoint

class WrapWeldJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2WeldJoint* m_weld_joint;

private:
	WrapWeldJoint() : m_weld_joint(NULL) {}
	~WrapWeldJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_weld_joint; } // override WrapJoint

	void SetupObject(WrapWeldJointDef* wrap_weld_jd, b2WeldJoint* weld_joint)
	{
		m_weld_joint = weld_joint;

		WrapJoint::SetupObject(wrap_weld_jd);
	}

	b2WeldJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2WeldJoint* weld_joint = m_weld_joint;
		m_weld_joint = NULL;
		return weld_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(GetReferenceAngle)
	CLASS_METHOD_DECLARE(SetFrequency)
	CLASS_METHOD_DECLARE(GetFrequency)
	CLASS_METHOD_DECLARE(SetDampingRatio)
	CLASS_METHOD_DECLARE(GetDampingRatio)
///	void Dump();
};

Persistent<Function> WrapWeldJoint::g_constructor;

void WrapWeldJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWeldJoint = NanNew<FunctionTemplate>(New);
	tplWeldJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplWeldJoint->SetClassName(NanNew<String>("b2WeldJoint"));
	tplWeldJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplWeldJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplWeldJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplWeldJoint, GetReferenceAngle)
	CLASS_METHOD_APPLY(tplWeldJoint, SetFrequency)
	CLASS_METHOD_APPLY(tplWeldJoint, GetFrequency)
	CLASS_METHOD_APPLY(tplWeldJoint, SetDampingRatio)
	CLASS_METHOD_APPLY(tplWeldJoint, GetDampingRatio)
	NanAssignPersistent(g_constructor, tplWeldJoint->GetFunction());
	exports->Set(NanNew<String>("b2WeldJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapWeldJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapWeldJoint::New)
{
	NanScope();
	WrapWeldJoint* that = new WrapWeldJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_weld_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_weld_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, GetReferenceAngle,
{
	NanReturnValue(NanNew<Number>(that->m_weld_joint->GetReferenceAngle()));
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, SetFrequency,
{
	that->m_weld_joint->SetFrequency((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, GetFrequency,
{
	NanReturnValue(NanNew<Number>(that->m_weld_joint->GetFrequency()));
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, SetDampingRatio,
{
	that->m_weld_joint->SetDampingRatio((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWeldJoint, GetDampingRatio,
{
	NanReturnValue(NanNew<Number>(that->m_weld_joint->GetDampingRatio()));
})

//// b2FrictionJointDef

class WrapFrictionJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2FrictionJointDef m_friction_jd;
	Persistent<Object> m_friction_jd_localAnchorA; // m_friction_jd.localAnchorA
	Persistent<Object> m_friction_jd_localAnchorB; // m_friction_jd.localAnchorB

private:
	WrapFrictionJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_friction_jd_localAnchorA, WrapVec2::NewInstance(m_friction_jd.localAnchorA));
		NanAssignPersistent(m_friction_jd_localAnchorB, WrapVec2::NewInstance(m_friction_jd.localAnchorB));
	}
	~WrapFrictionJointDef()
	{
		NanDisposePersistent(m_friction_jd_localAnchorA);
		NanDisposePersistent(m_friction_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_friction_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_friction_jd_localAnchorA))->GetVec2(); // struct copy
		m_friction_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_friction_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_friction_jd_localAnchorA))->SetVec2(m_friction_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_friction_jd_localAnchorB))->SetVec2(m_friction_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_friction_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(maxForce)
	CLASS_MEMBER_DECLARE(maxTorque)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapFrictionJointDef::g_constructor;

void WrapFrictionJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplFrictionJointDef = NanNew<FunctionTemplate>(New);
	tplFrictionJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplFrictionJointDef->SetClassName(NanNew<String>("b2FrictionJointDef"));
	tplFrictionJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplFrictionJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplFrictionJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplFrictionJointDef, maxForce)
	CLASS_MEMBER_APPLY(tplFrictionJointDef, maxTorque)
	CLASS_METHOD_APPLY(tplFrictionJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplFrictionJointDef->GetFunction());
	exports->Set(NanNew<String>("b2FrictionJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapFrictionJointDef::New)
{
	NanScope();
	WrapFrictionJointDef* that = new WrapFrictionJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapFrictionJointDef, m_friction_jd, localAnchorA			) // m_friction_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapFrictionJointDef, m_friction_jd, localAnchorB			) // m_friction_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapFrictionJointDef, m_friction_jd, float32, maxForce		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapFrictionJointDef, m_friction_jd, float32, maxTorque	)

CLASS_METHOD_IMPLEMENT(WrapFrictionJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	WrapVec2* wrap_anchor = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	that->SyncPull();
	that->m_friction_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody(), wrap_anchor->GetVec2());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2FrictionJoint

class WrapFrictionJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2FrictionJoint* m_friction_joint;

private:
	WrapFrictionJoint() : m_friction_joint(NULL) {}
	~WrapFrictionJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_friction_joint; } // override WrapJoint

	void SetupObject(WrapFrictionJointDef* wrap_friction_jd, b2FrictionJoint* friction_joint)
	{
		m_friction_joint = friction_joint;

		WrapJoint::SetupObject(wrap_friction_jd);
	}

	b2FrictionJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2FrictionJoint* friction_joint = m_friction_joint;
		m_friction_joint = NULL;
		return friction_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(SetMaxForce)
	CLASS_METHOD_DECLARE(GetMaxForce)
	CLASS_METHOD_DECLARE(SetMaxTorque)
	CLASS_METHOD_DECLARE(GetMaxTorque)
///	void Dump();
};

Persistent<Function> WrapFrictionJoint::g_constructor;

void WrapFrictionJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplFrictionJoint = NanNew<FunctionTemplate>(New);
	tplFrictionJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplFrictionJoint->SetClassName(NanNew<String>("b2FrictionJoint"));
	tplFrictionJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplFrictionJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplFrictionJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplFrictionJoint, SetMaxForce)
	CLASS_METHOD_APPLY(tplFrictionJoint, GetMaxForce)
	CLASS_METHOD_APPLY(tplFrictionJoint, SetMaxTorque)
	CLASS_METHOD_APPLY(tplFrictionJoint, GetMaxTorque)
	NanAssignPersistent(g_constructor, tplFrictionJoint->GetFunction());
	exports->Set(NanNew<String>("b2FrictionJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapFrictionJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapFrictionJoint::New)
{
	NanScope();
	WrapFrictionJoint* that = new WrapFrictionJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_friction_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_friction_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, SetMaxForce,
{
	that->m_friction_joint->SetMaxForce((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, GetMaxForce,
{
	NanReturnValue(NanNew<Number>(that->m_friction_joint->GetMaxForce()));
})

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, SetMaxTorque,
{
	that->m_friction_joint->SetMaxTorque((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapFrictionJoint, GetMaxTorque,
{
	NanReturnValue(NanNew<Number>(that->m_friction_joint->GetMaxTorque()));
})

//// b2RopeJointDef

class WrapRopeJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2RopeJointDef m_rope_jd;
	Persistent<Object> m_rope_jd_localAnchorA; // m_rope_jd.localAnchorA
	Persistent<Object> m_rope_jd_localAnchorB; // m_rope_jd.localAnchorB

private:
	WrapRopeJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_rope_jd_localAnchorA, WrapVec2::NewInstance(m_rope_jd.localAnchorA));
		NanAssignPersistent(m_rope_jd_localAnchorB, WrapVec2::NewInstance(m_rope_jd.localAnchorB));
	}
	~WrapRopeJointDef()
	{
		NanDisposePersistent(m_rope_jd_localAnchorA);
		NanDisposePersistent(m_rope_jd_localAnchorB);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_rope_jd.localAnchorA = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_rope_jd_localAnchorA))->GetVec2(); // struct copy
		m_rope_jd.localAnchorB = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_rope_jd_localAnchorB))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_rope_jd_localAnchorA))->SetVec2(m_rope_jd.localAnchorA);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_rope_jd_localAnchorB))->SetVec2(m_rope_jd.localAnchorB);
	}

	virtual b2JointDef& GetJointDef() { return m_rope_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localAnchorA)
	CLASS_MEMBER_DECLARE(localAnchorB)
	CLASS_MEMBER_DECLARE(maxLength)
};

Persistent<Function> WrapRopeJointDef::g_constructor;

void WrapRopeJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplRopeJointDef = NanNew<FunctionTemplate>(New);
	tplRopeJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplRopeJointDef->SetClassName(NanNew<String>("b2RopeJointDef"));
	tplRopeJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplRopeJointDef, localAnchorA)
	CLASS_MEMBER_APPLY(tplRopeJointDef, localAnchorB)
	CLASS_MEMBER_APPLY(tplRopeJointDef, maxLength)
	NanAssignPersistent(g_constructor, tplRopeJointDef->GetFunction());
	exports->Set(NanNew<String>("b2RopeJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapRopeJointDef::New)
{
	NanScope();
	WrapRopeJointDef* that = new WrapRopeJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapRopeJointDef, m_rope_jd, localAnchorA			) // m_rope_jd_localAnchorA
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapRopeJointDef, m_rope_jd, localAnchorB			) // m_rope_jd_localAnchorB
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapRopeJointDef, m_rope_jd, float32, maxLength	)

//// b2RopeJoint

class WrapRopeJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2RopeJoint* m_rope_joint;

private:
	WrapRopeJoint() : m_rope_joint(NULL) {}
	~WrapRopeJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_rope_joint; } // override WrapJoint

	void SetupObject(WrapRopeJointDef* wrap_rope_jd, b2RopeJoint* rope_joint)
	{
		m_rope_joint = rope_joint;

		WrapJoint::SetupObject(wrap_rope_jd);
	}

	b2RopeJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2RopeJoint* rope_joint = m_rope_joint;
		m_rope_joint = NULL;
		return rope_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(GetLocalAnchorA)
	CLASS_METHOD_DECLARE(GetLocalAnchorB)
	CLASS_METHOD_DECLARE(SetMaxLength)
	CLASS_METHOD_DECLARE(GetMaxLength)
	CLASS_METHOD_DECLARE(GetLimitState)
///	void Dump();
};

Persistent<Function> WrapRopeJoint::g_constructor;

void WrapRopeJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplRopeJoint = NanNew<FunctionTemplate>(New);
	tplRopeJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplRopeJoint->SetClassName(NanNew<String>("b2RopeJoint"));
	tplRopeJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplRopeJoint, GetLocalAnchorA)
	CLASS_METHOD_APPLY(tplRopeJoint, GetLocalAnchorB)
	CLASS_METHOD_APPLY(tplRopeJoint, SetMaxLength)
	CLASS_METHOD_APPLY(tplRopeJoint, GetMaxLength)
	CLASS_METHOD_APPLY(tplRopeJoint, GetLimitState)
	NanAssignPersistent(g_constructor, tplRopeJoint->GetFunction());
	exports->Set(NanNew<String>("b2RopeJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapRopeJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapRopeJoint::New)
{
	NanScope();
	WrapRopeJoint* that = new WrapRopeJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapRopeJoint, GetLocalAnchorA,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_rope_joint->GetLocalAnchorA());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapRopeJoint, GetLocalAnchorB,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_rope_joint->GetLocalAnchorB());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapRopeJoint, SetMaxLength,
{
	that->m_rope_joint->SetMaxLength((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapRopeJoint, GetMaxLength,
{
	NanReturnValue(NanNew<Number>(that->m_rope_joint->GetMaxLength()));
})

CLASS_METHOD_IMPLEMENT(WrapRopeJoint, GetLimitState,
{
	NanReturnValue(NanNew<Integer>(that->m_rope_joint->GetLimitState()));
})

//// b2MotorJointDef

class WrapMotorJointDef : public WrapJointDef
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2MotorJointDef m_motor_jd;
	Persistent<Object> m_motor_jd_linearOffset; // m_motor_jd.linearOffset

private:
	WrapMotorJointDef()
	{
		// create javascript objects
		NanAssignPersistent(m_motor_jd_linearOffset, WrapVec2::NewInstance(m_motor_jd.linearOffset));
	}
	~WrapMotorJointDef()
	{
		NanDisposePersistent(m_motor_jd_linearOffset);
	}

public:
	virtual void SyncPull() // override WrapJointDef
	{
		// sync: pull data from javascript objects
		WrapJointDef::SyncPull();
		m_motor_jd.linearOffset = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_motor_jd_linearOffset))->GetVec2(); // struct copy
	}

	virtual void SyncPush() // override WrapJointDef
	{
		// sync: push data into javascript objects
		WrapJointDef::SyncPush();
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_motor_jd_linearOffset))->SetVec2(m_motor_jd.linearOffset);
	}

	virtual b2JointDef& GetJointDef() { return m_motor_jd; } // override WrapJointDef

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(linearOffset)
	CLASS_MEMBER_DECLARE(angularOffset)
	CLASS_MEMBER_DECLARE(maxForce)
	CLASS_MEMBER_DECLARE(maxTorque)
	CLASS_MEMBER_DECLARE(correctionFactor)

	CLASS_METHOD_DECLARE(Initialize)
};

Persistent<Function> WrapMotorJointDef::g_constructor;

void WrapMotorJointDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplMotorJointDef = NanNew<FunctionTemplate>(New);
	tplMotorJointDef->Inherit(NanNew<FunctionTemplate>(WrapJointDef::g_constructor_template));
	tplMotorJointDef->SetClassName(NanNew<String>("b2MotorJointDef"));
	tplMotorJointDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplMotorJointDef, linearOffset)
	CLASS_MEMBER_APPLY(tplMotorJointDef, angularOffset)
	CLASS_MEMBER_APPLY(tplMotorJointDef, maxForce)
	CLASS_MEMBER_APPLY(tplMotorJointDef, maxTorque)
	CLASS_MEMBER_APPLY(tplMotorJointDef, correctionFactor)
	CLASS_METHOD_APPLY(tplMotorJointDef, Initialize)
	NanAssignPersistent(g_constructor, tplMotorJointDef->GetFunction());
	exports->Set(NanNew<String>("b2MotorJointDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapMotorJointDef::New)
{
	NanScope();
	WrapMotorJointDef* that = new WrapMotorJointDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapMotorJointDef, m_motor_jd, linearOffset				) // m_motor_jd_linearOffset
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMotorJointDef, m_motor_jd, float32, angularOffset		)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMotorJointDef, m_motor_jd, float32, maxForce			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMotorJointDef, m_motor_jd, float32, maxTorque			)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapMotorJointDef, m_motor_jd, float32, correctionFactor	)

CLASS_METHOD_IMPLEMENT(WrapMotorJointDef, Initialize,
{
	WrapBody* wrap_bodyA = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[0]));
	WrapBody* wrap_bodyB = node::ObjectWrap::Unwrap<WrapBody>(Local<Object>::Cast(args[1]));
	that->SyncPull();
	that->m_motor_jd.Initialize(wrap_bodyA->GetBody(), wrap_bodyB->GetBody());
	that->SyncPush();
	NanReturnUndefined();
})

//// b2MotorJoint

class WrapMotorJoint : public WrapJoint
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2MotorJoint* m_motor_joint;

private:
	WrapMotorJoint() : m_motor_joint(NULL) {}
	~WrapMotorJoint() {}

public:
	virtual b2Joint* GetJoint() { return m_motor_joint; } // override WrapJoint

	void SetupObject(WrapMotorJointDef* wrap_motor_jd, b2MotorJoint* motor_joint)
	{
		m_motor_joint = motor_joint;

		WrapJoint::SetupObject(wrap_motor_jd);
	}

	b2MotorJoint* ResetObject()
	{
		WrapJoint::ResetObject();

		b2MotorJoint* motor_joint = m_motor_joint;
		m_motor_joint = NULL;
		return motor_joint;
	}

private:
	static NAN_METHOD(New);

///	b2Vec2 GetAnchorA() const;
///	b2Vec2 GetAnchorB() const;
///	b2Vec2 GetReactionForce(float32 inv_dt) const;
///	float32 GetReactionTorque(float32 inv_dt) const;
	CLASS_METHOD_DECLARE(SetLinearOffset)
	CLASS_METHOD_DECLARE(GetLinearOffset)
	CLASS_METHOD_DECLARE(SetAngularOffset)
	CLASS_METHOD_DECLARE(GetAngularOffset)
	CLASS_METHOD_DECLARE(SetMaxForce)
	CLASS_METHOD_DECLARE(GetMaxForce)
	CLASS_METHOD_DECLARE(SetMaxTorque)
	CLASS_METHOD_DECLARE(GetMaxTorque)
	CLASS_METHOD_DECLARE(SetCorrectionFactor)
	CLASS_METHOD_DECLARE(GetCorrectionFactor)
///	void Dump();
};

Persistent<Function> WrapMotorJoint::g_constructor;

void WrapMotorJoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplMotorJoint = NanNew<FunctionTemplate>(New);
	tplMotorJoint->Inherit(NanNew<FunctionTemplate>(WrapJoint::g_constructor_template));
	tplMotorJoint->SetClassName(NanNew<String>("b2MotorJoint"));
	tplMotorJoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplMotorJoint, SetLinearOffset)
	CLASS_METHOD_APPLY(tplMotorJoint, GetLinearOffset)
	CLASS_METHOD_APPLY(tplMotorJoint, SetAngularOffset)
	CLASS_METHOD_APPLY(tplMotorJoint, GetAngularOffset)
	CLASS_METHOD_APPLY(tplMotorJoint, SetMaxForce)
	CLASS_METHOD_APPLY(tplMotorJoint, GetMaxForce)
	CLASS_METHOD_APPLY(tplMotorJoint, SetMaxTorque)
	CLASS_METHOD_APPLY(tplMotorJoint, GetMaxTorque)
	CLASS_METHOD_APPLY(tplMotorJoint, SetCorrectionFactor)
	CLASS_METHOD_APPLY(tplMotorJoint, GetCorrectionFactor)
	NanAssignPersistent(g_constructor, tplMotorJoint->GetFunction());
	exports->Set(NanNew<String>("b2MotorJoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapMotorJoint::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapMotorJoint::New)
{
	NanScope();
	WrapMotorJoint* that = new WrapMotorJoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, SetLinearOffset,
{
	that->m_motor_joint->SetLinearOffset(node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]))->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, GetLinearOffset,
{
	Local<Object> out = args[0]->IsUndefined() ? NanNew<Object>(WrapVec2::NewInstance()) : Local<Object>::Cast(args[0]);
	node::ObjectWrap::Unwrap<WrapVec2>(out)->SetVec2(that->m_motor_joint->GetLinearOffset());
	NanReturnValue(out);
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, SetAngularOffset,
{
	that->m_motor_joint->SetAngularOffset((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, GetAngularOffset,
{
	NanReturnValue(NanNew<Number>(that->m_motor_joint->GetAngularOffset()));
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, SetMaxForce,
{
	that->m_motor_joint->SetMaxForce((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, GetMaxForce,
{
	NanReturnValue(NanNew<Number>(that->m_motor_joint->GetMaxForce()));
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, SetMaxTorque,
{
	that->m_motor_joint->SetMaxTorque((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, GetMaxTorque,
{
	NanReturnValue(NanNew<Number>(that->m_motor_joint->GetMaxTorque()));
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, SetCorrectionFactor,
{
	that->m_motor_joint->SetCorrectionFactor((float32) args[0]->NumberValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapMotorJoint, GetCorrectionFactor,
{
	NanReturnValue(NanNew<Number>(that->m_motor_joint->GetCorrectionFactor()));
})

//// b2ContactID

class WrapContactID : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance(const b2ContactID& contact_id);

public:
	b2ContactID m_contact_id;

private:
	WrapContactID() {}
	~WrapContactID() {}

private:
	static NAN_METHOD(New);

//	CLASS_MEMBER_DECLARE(cf) // TODO
	CLASS_MEMBER_DECLARE(key)
};

Persistent<Function> WrapContactID::g_constructor;

void WrapContactID::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplContactID = NanNew<FunctionTemplate>(New);
	tplContactID->SetClassName(NanNew<String>("b2ContactID"));
	tplContactID->InstanceTemplate()->SetInternalFieldCount(1);
//	CLASS_MEMBER_APPLY(tplContactID, cf) // TODO
	CLASS_MEMBER_APPLY(tplContactID, key)
	NanAssignPersistent(g_constructor, tplContactID->GetFunction());
	exports->Set(NanNew<String>("b2ContactID"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapContactID::NewInstance(const b2ContactID& contact_id)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapContactID* wrap_contact_id = node::ObjectWrap::Unwrap<WrapContactID>(instance);
	wrap_contact_id->m_contact_id = contact_id;
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapContactID::New)
{
	NanScope();
	WrapContactID* that = new WrapContactID();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

//CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapContactID, m_contact_id, cf			) // TODO
CLASS_MEMBER_IMPLEMENT_UINT32	(WrapContactID, m_contact_id, uint32, key	)

//// b2ManifoldPoint

class WrapManifoldPoint : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance(const b2ManifoldPoint& manifold_point);

public:
	b2ManifoldPoint m_manifold_point;
	Persistent<Object> m_manifold_point_localPoint;	// m_manifold_point.localPoint
	Persistent<Object> m_manifold_point_id;			// m_manifold_point.id

private:
	WrapManifoldPoint()
	{
		NanAssignPersistent(m_manifold_point_localPoint, WrapVec2::NewInstance(m_manifold_point.localPoint));
		NanAssignPersistent(m_manifold_point_id, WrapContactID::NewInstance(m_manifold_point.id));
	}
	~WrapManifoldPoint()
	{
		NanDisposePersistent(m_manifold_point_localPoint);
		NanDisposePersistent(m_manifold_point_id);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_manifold_point.localPoint = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_point_localPoint))->GetVec2(); // struct copy
		m_manifold_point.id = node::ObjectWrap::Unwrap<WrapContactID>(NanNew<Object>(m_manifold_point_id))->m_contact_id;
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_point_localPoint))->SetVec2(m_manifold_point.localPoint);
		node::ObjectWrap::Unwrap<WrapContactID>(NanNew<Object>(m_manifold_point_id))->m_contact_id = m_manifold_point.id;
	}

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(localPoint)
	CLASS_MEMBER_DECLARE(normalImpulse)
	CLASS_MEMBER_DECLARE(tangentImpulse)
	CLASS_MEMBER_DECLARE(id)
};

Persistent<Function> WrapManifoldPoint::g_constructor;

void WrapManifoldPoint::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplManifoldPoint = NanNew<FunctionTemplate>(New);
	tplManifoldPoint->SetClassName(NanNew<String>("b2ManifoldPoint"));
	tplManifoldPoint->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplManifoldPoint, localPoint)
	CLASS_MEMBER_APPLY(tplManifoldPoint, normalImpulse)
	CLASS_MEMBER_APPLY(tplManifoldPoint, tangentImpulse)
	CLASS_MEMBER_APPLY(tplManifoldPoint, id)
	NanAssignPersistent(g_constructor, tplManifoldPoint->GetFunction());
	exports->Set(NanNew<String>("b2ManifoldPoint"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapManifoldPoint::NewInstance(const b2ManifoldPoint& manifold_point)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapManifoldPoint* wrap_manifold_point = node::ObjectWrap::Unwrap<WrapManifoldPoint>(instance);
	wrap_manifold_point->m_manifold_point = manifold_point; // struct copy
	wrap_manifold_point->SyncPush();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapManifoldPoint::New)
{
	NanScope();
	WrapManifoldPoint* that = new WrapManifoldPoint();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapManifoldPoint, m_manifold_point, localPoint				) // m_manifold_point_localPoint
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapManifoldPoint, m_manifold_point, float32, normalImpulse	)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapManifoldPoint, m_manifold_point, float32, tangentImpulse	)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapManifoldPoint, m_manifold_point, id						) // m_manifold_point_id

//// b2Manifold

class WrapManifold : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Manifold& manifold);

public:
	b2Manifold m_manifold;
	Persistent<Array> m_manifold_points;		// m_manifold.points
	Persistent<Object> m_manifold_localNormal;	// m_manifold.localNormal
	Persistent<Object> m_manifold_localPoint;	// m_manifold.localPoint

private:
	WrapManifold()
	{
		NanScope();

		NanAssignPersistent(m_manifold_points, NanNew<Array>(b2_maxManifoldPoints));
		NanAssignPersistent(m_manifold_localNormal, WrapVec2::NewInstance(m_manifold.localNormal));
		NanAssignPersistent(m_manifold_localPoint, WrapVec2::NewInstance(m_manifold.localPoint));

		Local<Array> manifold_points = NanNew<Array>(m_manifold_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			manifold_points->Set(i, WrapManifoldPoint::NewInstance(m_manifold.points[i]));
		}
	}
	~WrapManifold()
	{
		NanDisposePersistent(m_manifold_points);
		NanDisposePersistent(m_manifold_localNormal);
		NanDisposePersistent(m_manifold_localPoint);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		NanScope();
		Local<Array> manifold_points = NanNew<Array>(m_manifold_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			WrapManifoldPoint* wrap_manifold_point = node::ObjectWrap::Unwrap<WrapManifoldPoint>(Local<Object>::Cast(manifold_points->Get(i)));
			m_manifold.points[i] = wrap_manifold_point->m_manifold_point; // struct copy
			wrap_manifold_point->SyncPull();
		}
		m_manifold.localNormal = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_localNormal))->GetVec2(); // struct copy
		m_manifold.localPoint = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_localPoint))->GetVec2(); // struct copy
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		NanScope();
		Local<Array> manifold_points = NanNew<Array>(m_manifold_points);
		for (uint32_t i = 0; i < manifold_points->Length(); ++i)
		{
			WrapManifoldPoint* wrap_manifold_point = node::ObjectWrap::Unwrap<WrapManifoldPoint>(Local<Object>::Cast(manifold_points->Get(i)));
			wrap_manifold_point->m_manifold_point = m_manifold.points[i]; // struct copy
			wrap_manifold_point->SyncPush();
		}
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_localNormal))->SetVec2(m_manifold.localNormal);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_manifold_localPoint))->SetVec2(m_manifold.localPoint);
	}

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(points)
	CLASS_MEMBER_DECLARE(localNormal)
	CLASS_MEMBER_DECLARE(localPoint)
	CLASS_MEMBER_DECLARE(type)
	CLASS_MEMBER_DECLARE(pointCount)
};

Persistent<Function> WrapManifold::g_constructor;

void WrapManifold::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplManifold = NanNew<FunctionTemplate>(New);
	tplManifold->SetClassName(NanNew<String>("b2Manifold"));
	tplManifold->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplManifold, points)
	CLASS_MEMBER_APPLY(tplManifold, localNormal)
	CLASS_MEMBER_APPLY(tplManifold, localPoint)
	CLASS_MEMBER_APPLY(tplManifold, type)
	CLASS_MEMBER_APPLY(tplManifold, pointCount)
	NanAssignPersistent(g_constructor, tplManifold->GetFunction());
	exports->Set(NanNew<String>("b2Manifold"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapManifold::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapManifold* wrap_manifold = node::ObjectWrap::Unwrap<WrapManifold>(instance);
	wrap_manifold->SyncPush();
	return NanEscapeScope(instance);
}

Handle<Object> WrapManifold::NewInstance(const b2Manifold& manifold)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapManifold* wrap_manifold = node::ObjectWrap::Unwrap<WrapManifold>(instance);
	wrap_manifold->m_manifold = manifold; // struct copy
	wrap_manifold->SyncPush();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapManifold::New)
{
	NanScope();
	WrapManifold* that = new WrapManifold();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapManifold, m_manifold, points					) // m_manifold_points
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapManifold, m_manifold, localNormal				) // m_manifold_localNormal
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapManifold, m_manifold, localPoint				) // m_manifold_localPoint
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapManifold, m_manifold, b2Manifold::Type, type	)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapManifold, m_manifold, int32, pointCount		)

//// b2WorldManifold

class WrapWorldManifold : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2WorldManifold& world_manifold);

public:
	b2WorldManifold m_world_manifold;
	Persistent<Object> m_world_manifold_normal;		// m_world_manifold.normal
	Persistent<Array> m_world_manifold_points;		// m_world_manifold.points
	Persistent<Array> m_world_manifold_separations;	// m_world_manifold.separations

private:
	WrapWorldManifold()
	{
		NanScope();

		NanAssignPersistent(m_world_manifold_normal, WrapVec2::NewInstance(m_world_manifold.normal));
		NanAssignPersistent(m_world_manifold_points, NanNew<Array>(b2_maxManifoldPoints));
		NanAssignPersistent(m_world_manifold_separations, NanNew<Array>(b2_maxManifoldPoints));

		Local<Array> world_manifold_points = NanNew<Array>(m_world_manifold_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			world_manifold_points->Set(i, WrapVec2::NewInstance(m_world_manifold.points[i]));
		}
		Local<Array> world_manifold_separations = NanNew<Array>(m_world_manifold_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			world_manifold_separations->Set(i, NanNew<Number>(m_world_manifold.separations[i]));
		}
	}
	~WrapWorldManifold()
	{
		NanDisposePersistent(m_world_manifold_normal);
		NanDisposePersistent(m_world_manifold_points);
		NanDisposePersistent(m_world_manifold_separations);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		NanScope();
		m_world_manifold.normal = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_world_manifold_normal))->GetVec2(); // struct copy
		Local<Array> world_manifold_points = NanNew<Array>(m_world_manifold_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			m_world_manifold.points[i] = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(world_manifold_points->Get(i)))->GetVec2(); // struct copy
		}
		Local<Array> world_manifold_separations = NanNew<Array>(m_world_manifold_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			m_world_manifold.separations[i] = (float32) world_manifold_separations->Get(i)->NumberValue();
		}
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_world_manifold_normal))->SetVec2(m_world_manifold.normal);
		Local<Array> world_manifold_points = NanNew<Array>(m_world_manifold_points);
		for (uint32_t i = 0; i < world_manifold_points->Length(); ++i)
		{
			node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(world_manifold_points->Get(i)))->SetVec2(m_world_manifold.points[i]);
		}
		Local<Array> world_manifold_separations = NanNew<Array>(m_world_manifold_separations);
		for (uint32_t i = 0; i < world_manifold_separations->Length(); ++i)
		{
			world_manifold_separations->Set(i, NanNew<Number>(m_world_manifold.separations[i]));
		}
	}

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(normal)
	CLASS_MEMBER_DECLARE(points)
	CLASS_MEMBER_DECLARE(separations)
};

Persistent<Function> WrapWorldManifold::g_constructor;

void WrapWorldManifold::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWorldManifold = NanNew<FunctionTemplate>(New);
	tplWorldManifold->SetClassName(NanNew<String>("b2WorldManifold"));
	tplWorldManifold->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplWorldManifold, normal)
	CLASS_MEMBER_APPLY(tplWorldManifold, points)
	CLASS_MEMBER_APPLY(tplWorldManifold, separations)
	NanAssignPersistent(g_constructor, tplWorldManifold->GetFunction());
	exports->Set(NanNew<String>("b2WorldManifold"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapWorldManifold::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapWorldManifold* wrap_world_manifold = node::ObjectWrap::Unwrap<WrapWorldManifold>(instance);
	wrap_world_manifold->SyncPush();
	return NanEscapeScope(instance);
}

Handle<Object> WrapWorldManifold::NewInstance(const b2WorldManifold& world_manifold)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapWorldManifold* wrap_world_manifold = node::ObjectWrap::Unwrap<WrapWorldManifold>(instance);
	wrap_world_manifold->m_world_manifold = world_manifold; // struct copy
	wrap_world_manifold->SyncPush();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapWorldManifold::New)
{
	NanScope();
	WrapWorldManifold* that = new WrapWorldManifold();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapWorldManifold, m_world_manifold, normal		) // m_world_manifold_normal
CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapWorldManifold, m_world_manifold, points		) // m_world_manifold_points
CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapWorldManifold, m_world_manifold, separations	) // m_world_manifold_separations

//// b2Contact

class WrapContact : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance(b2Contact* contact);

public:
	b2Contact* m_contact;

private:
	WrapContact() : m_contact(NULL) {}
	~WrapContact() { m_contact = NULL; }

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(GetManifold)
	CLASS_METHOD_DECLARE(GetWorldManifold)
	CLASS_METHOD_DECLARE(IsTouching)
	CLASS_METHOD_DECLARE(SetEnabled)
	CLASS_METHOD_DECLARE(IsEnabled)
	CLASS_METHOD_DECLARE(GetNext)
	CLASS_METHOD_DECLARE(GetFixtureA)
	CLASS_METHOD_DECLARE(GetChildIndexA)
	CLASS_METHOD_DECLARE(GetFixtureB)
	CLASS_METHOD_DECLARE(GetChildIndexB)
	CLASS_METHOD_DECLARE(SetFriction)
	CLASS_METHOD_DECLARE(GetFriction)
	CLASS_METHOD_DECLARE(ResetFriction)
	CLASS_METHOD_DECLARE(SetRestitution)
	CLASS_METHOD_DECLARE(GetRestitution)
	CLASS_METHOD_DECLARE(ResetRestitution)
	CLASS_METHOD_DECLARE(SetTangentSpeed)
	CLASS_METHOD_DECLARE(GetTangentSpeed)
//	virtual void Evaluate(b2Manifold* manifold, const b2Transform& xfA, const b2Transform& xfB) = 0;
};

Persistent<Function> WrapContact::g_constructor;

void WrapContact::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplContact = NanNew<FunctionTemplate>(New);
	tplContact->SetClassName(NanNew<String>("b2Contact"));
	tplContact->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplContact, GetManifold)
	CLASS_METHOD_APPLY(tplContact, GetWorldManifold)
	CLASS_METHOD_APPLY(tplContact, IsTouching)
	CLASS_METHOD_APPLY(tplContact, SetEnabled)
	CLASS_METHOD_APPLY(tplContact, IsEnabled)
	CLASS_METHOD_APPLY(tplContact, GetNext)
	CLASS_METHOD_APPLY(tplContact, GetFixtureA)
	CLASS_METHOD_APPLY(tplContact, GetChildIndexA)
	CLASS_METHOD_APPLY(tplContact, GetFixtureB)
	CLASS_METHOD_APPLY(tplContact, GetChildIndexB)
	CLASS_METHOD_APPLY(tplContact, SetFriction)
	CLASS_METHOD_APPLY(tplContact, GetFriction)
	CLASS_METHOD_APPLY(tplContact, ResetFriction)
	CLASS_METHOD_APPLY(tplContact, SetRestitution)
	CLASS_METHOD_APPLY(tplContact, GetRestitution)
	CLASS_METHOD_APPLY(tplContact, ResetRestitution)
	CLASS_METHOD_APPLY(tplContact, SetTangentSpeed)
	CLASS_METHOD_APPLY(tplContact, GetTangentSpeed)
	NanAssignPersistent(g_constructor, tplContact->GetFunction());
	exports->Set(NanNew<String>("b2Contact"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapContact::NewInstance(b2Contact* contact)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapContact* wrap_contact = node::ObjectWrap::Unwrap<WrapContact>(instance);
	wrap_contact->m_contact = contact;
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapContact::New)
{
	NanScope();
	WrapContact* that = new WrapContact();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapContact, GetManifold,
{
	const b2Manifold* manifold = that->m_contact->GetManifold();
	NanReturnValue(WrapManifold::NewInstance(*manifold));
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetWorldManifold,
{
	WrapWorldManifold* wrap_world_manifold = node::ObjectWrap::Unwrap<WrapWorldManifold>(Local<Object>::Cast(args[0]));
	that->m_contact->GetWorldManifold(&wrap_world_manifold->m_world_manifold);
	wrap_world_manifold->SyncPush();
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapContact, IsTouching,
{
	NanReturnValue(NanNew<Boolean>(that->m_contact->IsTouching()));
})

CLASS_METHOD_IMPLEMENT(WrapContact, SetEnabled,
{
	that->m_contact->SetEnabled(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapContact, IsEnabled,
{
	NanReturnValue(NanNew<Boolean>(that->m_contact->IsEnabled()));
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetNext,
{
	b2Contact* contact = that->m_contact->GetNext();
	if (contact)
	{
		NanReturnValue(WrapContact::NewInstance(contact));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetFixtureA,
{
	b2Fixture* fixture = that->m_contact->GetFixtureA();
	// get fixture internal data
	WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
	NanReturnValue(NanObjectWrapHandle(wrap_fixture));
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetChildIndexA,
{
	NanReturnValue(NanNew<Integer>(that->m_contact->GetChildIndexA()));
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetFixtureB,
{
	b2Fixture* fixture = that->m_contact->GetFixtureB();
	// get fixture internal data
	WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
	NanReturnValue(NanObjectWrapHandle(wrap_fixture));
})

CLASS_METHOD_IMPLEMENT(WrapContact, GetChildIndexB,
{
	NanReturnValue(NanNew<Integer>(that->m_contact->GetChildIndexB()));
})

CLASS_METHOD_IMPLEMENT(WrapContact, SetFriction, that->m_contact->SetFriction((float32) args[0]->NumberValue()); NanReturnUndefined(); )
CLASS_METHOD_IMPLEMENT(WrapContact, GetFriction, NanReturnValue(NanNew<Number>(that->m_contact->GetFriction())); )
CLASS_METHOD_IMPLEMENT(WrapContact, ResetFriction, that->m_contact->ResetFriction(); NanReturnUndefined(); )

CLASS_METHOD_IMPLEMENT(WrapContact, SetRestitution, that->m_contact->SetRestitution((float32) args[0]->NumberValue()); NanReturnUndefined(); )
CLASS_METHOD_IMPLEMENT(WrapContact, GetRestitution, NanReturnValue(NanNew<Number>(that->m_contact->GetRestitution())); )
CLASS_METHOD_IMPLEMENT(WrapContact, ResetRestitution, that->m_contact->ResetRestitution(); NanReturnUndefined(); )

CLASS_METHOD_IMPLEMENT(WrapContact, SetTangentSpeed, that->m_contact->SetTangentSpeed((float32) args[0]->NumberValue()); NanReturnUndefined(); )
CLASS_METHOD_IMPLEMENT(WrapContact, GetTangentSpeed, NanReturnValue(NanNew<Number>(that->m_contact->GetTangentSpeed())); )

//// b2ContactImpulse

class WrapContactImpulse : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance(const b2ContactImpulse& contact_impulse);

public:
	b2ContactImpulse m_contact_impulse;
	Persistent<Array> m_contact_impulse_normalImpulses;		// m_contact_impulse.normalImpulses
	Persistent<Array> m_contact_impulse_tangentImpulses;	// m_contact_impulse.tangentImpulses

private:
	WrapContactImpulse()
	{
		NanAssignPersistent(m_contact_impulse_normalImpulses, NanNew<Array>(b2_maxManifoldPoints));
		NanAssignPersistent(m_contact_impulse_tangentImpulses, NanNew<Array>(b2_maxManifoldPoints));
	}
	~WrapContactImpulse()
	{
		NanDisposePersistent(m_contact_impulse_normalImpulses);
		NanDisposePersistent(m_contact_impulse_tangentImpulses);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		NanScope();
		Local<Array> contact_impulse_normalImpulses = NanNew<Array>(m_contact_impulse_normalImpulses);
		for (uint32_t i = 0; i < contact_impulse_normalImpulses->Length(); ++i)
		{
			m_contact_impulse.normalImpulses[i] = (float32) contact_impulse_normalImpulses->Get(i)->NumberValue();
		}
		Local<Array> contact_impulse_tangentImpulses = NanNew<Array>(m_contact_impulse_tangentImpulses);
		for (uint32_t i = 0; i < contact_impulse_tangentImpulses->Length(); ++i)
		{
			m_contact_impulse.tangentImpulses[i] = (float32) contact_impulse_tangentImpulses->Get(i)->NumberValue();
		}
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		NanScope();
		Local<Array> contact_impulse_normalImpulses = NanNew<Array>(m_contact_impulse_normalImpulses);
		for (uint32_t i = 0; i < contact_impulse_normalImpulses->Length(); ++i)
		{
			contact_impulse_normalImpulses->Set(i, NanNew<Number>(m_contact_impulse.normalImpulses[i]));
		}
		Local<Array> contact_impulse_tangentImpulses = NanNew<Array>(m_contact_impulse_tangentImpulses);
		for (uint32_t i = 0; i < contact_impulse_tangentImpulses->Length(); ++i)
		{
			contact_impulse_tangentImpulses->Set(i, NanNew<Number>(m_contact_impulse.tangentImpulses[i]));
		}
	}

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(normalImpulses)
	CLASS_MEMBER_DECLARE(tangentImpulses)
	CLASS_MEMBER_DECLARE(count)
};

Persistent<Function> WrapContactImpulse::g_constructor;

void WrapContactImpulse::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplContactImpulse = NanNew<FunctionTemplate>(New);
	tplContactImpulse->SetClassName(NanNew<String>("b2ContactImpulse"));
	tplContactImpulse->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplContactImpulse, normalImpulses)
	CLASS_MEMBER_APPLY(tplContactImpulse, tangentImpulses)
	CLASS_MEMBER_APPLY(tplContactImpulse, count)
	NanAssignPersistent(g_constructor, tplContactImpulse->GetFunction());
	exports->Set(NanNew<String>("b2ContactImpulse"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapContactImpulse::NewInstance(const b2ContactImpulse& contact_impulse)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapContactImpulse* wrap_contact_impulse = node::ObjectWrap::Unwrap<WrapContactImpulse>(instance);
	wrap_contact_impulse->m_contact_impulse = contact_impulse; // struct copy
	wrap_contact_impulse->SyncPush();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapContactImpulse::New)
{
	NanScope();
	WrapContactImpulse* that = new WrapContactImpulse();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapContactImpulse, m_contact_impulse, normalImpulses	) // m_contact_impulse_normalImpulses
CLASS_MEMBER_IMPLEMENT_ARRAY	(WrapContactImpulse, m_contact_impulse, tangentImpulses	) // m_contact_impulse_tangentImpulses
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapContactImpulse, m_contact_impulse, int32, count	)

//// b2Color

class WrapColor : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2Color& o);

private:
	b2Color m_color;

private:
	WrapColor(float32 r, float32 g, float32 b) : m_color(r, g, b) {}
	~WrapColor() {}

public:
	const b2Color& GetColor() const { return m_color; }
	void SetColor(const b2Color& color) { m_color = color; } // struct copy

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(r)
	CLASS_MEMBER_DECLARE(g)
	CLASS_MEMBER_DECLARE(b)
};

Persistent<Function> WrapColor::g_constructor;

void WrapColor::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplColor = NanNew<FunctionTemplate>(New);
	tplColor->SetClassName(NanNew<String>("b2Color"));
	tplColor->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplColor, r)
	CLASS_MEMBER_APPLY(tplColor, g)
	CLASS_MEMBER_APPLY(tplColor, b)
	NanAssignPersistent(g_constructor, tplColor->GetFunction());
	exports->Set(NanNew<String>("b2Color"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapColor::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapColor::NewInstance(const b2Color& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapColor* that = node::ObjectWrap::Unwrap<WrapColor>(instance);
	that->SetColor(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapColor::New)
{
	NanScope();
	float32 r = (args.Length() > 0) ? (float32) args[0]->NumberValue() : 0.5f;
	float32 g = (args.Length() > 1) ? (float32) args[1]->NumberValue() : 0.5f;
	float32 b = (args.Length() > 2) ? (float32) args[2]->NumberValue() : 0.5f;
	WrapColor* that = new WrapColor(r, g, b);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapColor, m_color, float32, r)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapColor, m_color, float32, g)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapColor, m_color, float32, b)

//// b2Draw

#if 1 // B2_ENABLE_PARTICLE

//// b2Particle

//// b2ParticleColor

class WrapParticleColor : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();
	static Handle<Object> NewInstance(const b2ParticleColor& o);

private:
	b2ParticleColor m_particle_color;

private:
	WrapParticleColor(uint8 r, uint8 g, uint8 b, uint8 a) : m_particle_color(r, g, b, a) {}
	~WrapParticleColor() {}

public:
	const b2ParticleColor& GetParticleColor() const { return m_particle_color; }
	void SetParticleColor(const b2ParticleColor& particle_color) { m_particle_color = particle_color; } // struct copy

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(r)
	CLASS_MEMBER_DECLARE(g)
	CLASS_MEMBER_DECLARE(b)
	CLASS_MEMBER_DECLARE(a)
};

Persistent<Function> WrapParticleColor::g_constructor;

void WrapParticleColor::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleColor = NanNew<FunctionTemplate>(New);
	tplParticleColor->SetClassName(NanNew<String>("b2ParticleColor"));
	tplParticleColor->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplParticleColor, r)
	CLASS_MEMBER_APPLY(tplParticleColor, g)
	CLASS_MEMBER_APPLY(tplParticleColor, b)
	CLASS_MEMBER_APPLY(tplParticleColor, a)
	NanAssignPersistent(g_constructor, tplParticleColor->GetFunction());
	exports->Set(NanNew<String>("b2ParticleColor"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapParticleColor::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

Handle<Object> WrapParticleColor::NewInstance(const b2ParticleColor& o)
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	WrapParticleColor* that = node::ObjectWrap::Unwrap<WrapParticleColor>(instance);
	that->SetParticleColor(o);
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapParticleColor::New)
{
	NanScope();
	uint8 r = (args.Length() > 0) ? (uint8) args[0]->NumberValue() : 0;
	uint8 g = (args.Length() > 1) ? (uint8) args[1]->NumberValue() : 0;
	uint8 b = (args.Length() > 2) ? (uint8) args[2]->NumberValue() : 0;
	uint8 a = (args.Length() > 3) ? (uint8) args[3]->NumberValue() : 0;
	WrapParticleColor* that = new WrapParticleColor(r, g, b, a);
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleColor, m_particle_color, uint8, r)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleColor, m_particle_color, uint8, g)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleColor, m_particle_color, uint8, b)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleColor, m_particle_color, uint8, a)

//// b2ParticleDef

class WrapParticleDef : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2ParticleDef m_pd;
	Persistent<Object> m_pd_position;	// m_pd.position
	Persistent<Object> m_pd_velocity;	// m_pd.velocity
	Persistent<Object> m_pd_color;		// m_pd.color
	Persistent<Value> m_pd_userData;	// m_pd.userData

private:
	WrapParticleDef()
	{
		// create javascript objects
		NanAssignPersistent(m_pd_position, WrapVec2::NewInstance(m_pd.position));
		NanAssignPersistent(m_pd_velocity, WrapVec2::NewInstance(m_pd.velocity));
		NanAssignPersistent(m_pd_color, WrapParticleColor::NewInstance(m_pd.color));
	}
	~WrapParticleDef()
	{
		NanDisposePersistent(m_pd_position);
		NanDisposePersistent(m_pd_velocity);
		NanDisposePersistent(m_pd_color);
		NanDisposePersistent(m_pd_userData);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_pd.position = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pd_position))->GetVec2(); // struct copy
		m_pd.velocity = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pd_velocity))->GetVec2(); // struct copy
		m_pd.color = node::ObjectWrap::Unwrap<WrapParticleColor>(NanNew<Object>(m_pd_color))->GetParticleColor(); // struct copy
		//m_pd.userData; // not used
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pd_position))->SetVec2(m_pd.position);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pd_velocity))->SetVec2(m_pd.velocity);
		node::ObjectWrap::Unwrap<WrapParticleColor>(NanNew<Object>(m_pd_color))->SetParticleColor(m_pd.color);
		//m_pd.userData; // not used
	}

	b2ParticleDef& GetParticleDef() { return m_pd; }

	const b2ParticleDef& UseParticleDef() { SyncPull(); return GetParticleDef(); }

	Handle<Value> GetUserDataHandle() { return NanNew<Value>(m_pd_userData); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(flags)
	CLASS_MEMBER_DECLARE(position)
	CLASS_MEMBER_DECLARE(velocity)
	CLASS_MEMBER_DECLARE(color)
	CLASS_MEMBER_DECLARE(lifetime)
	CLASS_MEMBER_DECLARE(userData)
	//b2ParticleGroup* group;
};

Persistent<Function> WrapParticleDef::g_constructor;

void WrapParticleDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleDef = NanNew<FunctionTemplate>(New);
	tplParticleDef->SetClassName(NanNew<String>("b2ParticleDef"));
	tplParticleDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplParticleDef, flags)
	CLASS_MEMBER_APPLY(tplParticleDef, position)
	CLASS_MEMBER_APPLY(tplParticleDef, velocity)
	CLASS_MEMBER_APPLY(tplParticleDef, color)
	CLASS_MEMBER_APPLY(tplParticleDef, lifetime)
	CLASS_MEMBER_APPLY(tplParticleDef, userData)
	//b2ParticleGroup* group;
	NanAssignPersistent(g_constructor, tplParticleDef->GetFunction());
	exports->Set(NanNew<String>("b2ParticleDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapParticleDef::New)
{
	NanScope();
	WrapParticleDef* that = new WrapParticleDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapParticleDef, m_pd, uint32, flags				)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleDef, m_pd, position					) // m_pd_position
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleDef, m_pd, velocity					) // m_pd_velocity
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleDef, m_pd, color						) // m_pd_color
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleDef, m_pd, float32, lifetime			)
CLASS_MEMBER_IMPLEMENT_VALUE	(WrapParticleDef, m_pd, userData					) // m_pd_userData
//b2ParticleGroup* group;

//// b2ParticleHandle

class WrapParticleHandle : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

private:
	b2ParticleHandle m_particle_handle;

private:
	WrapParticleHandle() {}
	~WrapParticleHandle() {}

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(GetIndex)
};

Persistent<Function> WrapParticleHandle::g_constructor;

void WrapParticleHandle::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleHandle = NanNew<FunctionTemplate>(New);
	tplParticleHandle->SetClassName(NanNew<String>("b2ParticleHandle"));
	tplParticleHandle->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplParticleHandle, GetIndex)
	NanAssignPersistent(g_constructor, tplParticleHandle->GetFunction());
	exports->Set(NanNew<String>("b2ParticleHandle"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapParticleHandle::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapParticleHandle::New)
{
	NanScope();
	WrapParticleHandle* that = new WrapParticleHandle();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapParticleHandle, GetIndex, { NanReturnValue(NanNew<Number>(that->m_particle_handle.GetIndex())); })

//// b2ParticleGroupDef

class WrapParticleGroupDef : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2ParticleGroupDef m_pgd;
	Persistent<Object> m_pgd_position;			// m_pgd.position
	Persistent<Object> m_pgd_linearVelocity;	// m_pgd.linearVelocity
	Persistent<Object> m_pgd_color;				// m_pgd.color
	Persistent<Object> m_pgd_shape;				// m_pgd.shape
	Persistent<Value> m_pgd_userData;			// m_pgd.userData

private:
	WrapParticleGroupDef()
	{
		// create javascript objects
		NanAssignPersistent(m_pgd_position, WrapVec2::NewInstance(m_pgd.position));
		NanAssignPersistent(m_pgd_linearVelocity, WrapVec2::NewInstance(m_pgd.linearVelocity));
		NanAssignPersistent(m_pgd_color, WrapParticleColor::NewInstance(m_pgd.color));
	}
	~WrapParticleGroupDef()
	{
		NanDisposePersistent(m_pgd_position);
		NanDisposePersistent(m_pgd_linearVelocity);
		NanDisposePersistent(m_pgd_color);
		NanDisposePersistent(m_pgd_shape);
		NanDisposePersistent(m_pgd_userData);
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
		m_pgd.position = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pgd_position))->GetVec2(); // struct copy
		m_pgd.linearVelocity = node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pgd_linearVelocity))->GetVec2(); // struct copy
		m_pgd.color = node::ObjectWrap::Unwrap<WrapParticleColor>(NanNew<Object>(m_pgd_color))->GetParticleColor(); // struct copy
		if (!m_pgd_shape.IsEmpty())
		{
			WrapShape* wrap_shape = node::ObjectWrap::Unwrap<WrapShape>(NanNew<Object>(m_pgd_shape));
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
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pgd_position))->SetVec2(m_pgd.position);
		node::ObjectWrap::Unwrap<WrapVec2>(NanNew<Object>(m_pgd_linearVelocity))->SetVec2(m_pgd.linearVelocity);
		node::ObjectWrap::Unwrap<WrapParticleColor>(NanNew<Object>(m_pgd_color))->SetParticleColor(m_pgd.color);
		if (m_pgd.shape)
		{
			// TODO: NanAssignPersistent(m_pgd_shape, NanObjectWrapHandle(WrapShape::GetWrap(m_pgd.shape)));
		}
		else
		{
			NanDisposePersistent(m_pgd_shape);
		}
		//m_pgd.userData; // not used
	}

	b2ParticleGroupDef& GetParticleGroupDef() { return m_pgd; }

	const b2ParticleGroupDef& UseParticleGroupDef() { SyncPull(); return GetParticleGroupDef(); }

	Handle<Value> GetUserDataHandle() { return NanNew<Value>(m_pgd_userData); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(flags)
	CLASS_MEMBER_DECLARE(groupFlags)
	CLASS_MEMBER_DECLARE(position)
	CLASS_MEMBER_DECLARE(angle)
	CLASS_MEMBER_DECLARE(linearVelocity)
	CLASS_MEMBER_DECLARE(angularVelocity)
	CLASS_MEMBER_DECLARE(color)
	CLASS_MEMBER_DECLARE(strength)
	CLASS_MEMBER_DECLARE(shape)
	//const b2Shape* const* shapes;
	//int32 shapeCount;
	//float32 stride;
	//int32 particleCount;
	//const b2Vec2* positionData;
	CLASS_MEMBER_DECLARE(lifetime)
	CLASS_MEMBER_DECLARE(userData)
	//b2ParticleGroup* group;
};

Persistent<Function> WrapParticleGroupDef::g_constructor;

void WrapParticleGroupDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleGroupDef = NanNew<FunctionTemplate>(New);
	tplParticleGroupDef->SetClassName(NanNew<String>("b2ParticleGroupDef"));
	tplParticleGroupDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplParticleGroupDef, flags)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, groupFlags)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, position)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, angle)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, linearVelocity)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, angularVelocity)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, color)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, strength)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, shape)
	//const b2Shape* const* shapes;
	//int32 shapeCount;
	//float32 stride;
	//int32 particleCount;
	//const b2Vec2* positionData;
	CLASS_MEMBER_APPLY(tplParticleGroupDef, lifetime)
	CLASS_MEMBER_APPLY(tplParticleGroupDef, userData)
	//b2ParticleGroup* group;
	NanAssignPersistent(g_constructor, tplParticleGroupDef->GetFunction());
	exports->Set(NanNew<String>("b2ParticleGroupDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapParticleGroupDef::New)
{
	NanScope();
	WrapParticleGroupDef* that = new WrapParticleGroupDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapParticleGroupDef, m_pgd, uint32, flags				)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapParticleGroupDef, m_pgd, uint32, groupFlags		)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleGroupDef, m_pgd, position					) // m_pgd_position
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleGroupDef, m_pgd, float32, angle			)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleGroupDef, m_pgd, linearVelocity			) // m_pgd_linearVelocity
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleGroupDef, m_pgd, float32, angularVelocity	)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleGroupDef, m_pgd, color						) // m_pgd_color
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleGroupDef, m_pgd, float32, strength			)
CLASS_MEMBER_IMPLEMENT_OBJECT	(WrapParticleGroupDef, m_pgd, shape						) // m_pgd_shape
//const b2Shape* const* shapes;
//int32 shapeCount;
//float32 stride;
//int32 particleCount;
//const b2Vec2* positionData;
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleGroupDef, m_pgd, float32, lifetime			)
CLASS_MEMBER_IMPLEMENT_VALUE	(WrapParticleGroupDef, m_pgd, userData					) // m_pgd_userData
//b2ParticleGroup* group;

//// b2ParticleGroup

class WrapParticleGroup : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

	static WrapParticleGroup* GetWrap(const b2ParticleGroup* particle_group)
	{
		return (WrapParticleGroup*) particle_group->GetUserData();
	}

	static void SetWrap(b2ParticleGroup* particle_group, WrapParticleGroup* wrap)
	{
		particle_group->SetUserData(wrap);
	}

private:
	b2ParticleGroup* m_particle_group;
	Persistent<Object> m_particle_group_particle_system;
	Persistent<Value> m_particle_group_userData;

private:
	WrapParticleGroup() :
		m_particle_group(NULL)
	{
		// create javascript objects
	}
	~WrapParticleGroup()
	{
		NanDisposePersistent(m_particle_group_particle_system);
		NanDisposePersistent(m_particle_group_userData);
	}

public:
	b2ParticleGroup* GetParticleGroup() { return m_particle_group; }

	void SetupObject(Handle<Object> h_particle_system, WrapParticleGroupDef* wrap_pgd, b2ParticleGroup* particle_group)
	{
		m_particle_group = particle_group;

		// set particle_group internal data
		WrapParticleGroup::SetWrap(m_particle_group, this);
		// set reference to this particle_group (prevent GC)
		Ref();

		// set reference to world object
		NanAssignPersistent(m_particle_group_particle_system, h_particle_system);
		// set reference to user data object
		NanAssignPersistent(m_particle_group_userData, wrap_pgd->GetUserDataHandle());
	}

	b2ParticleGroup* ResetObject()
	{
		// clear reference to world object
		NanDisposePersistent(m_particle_group_particle_system);
		// clear reference to user data object
		NanDisposePersistent(m_particle_group_userData);

		// clear reference to this particle_group (allow GC)
		Unref();
		// clear particle_group internal data
		WrapParticleGroup::SetWrap(m_particle_group, NULL);

		b2ParticleGroup* particle_group = m_particle_group;
		m_particle_group = NULL;
		return particle_group;
	}

private:
	static NAN_METHOD(New);

	//b2ParticleGroup* GetNext();
	//b2ParticleSystem* GetParticleSystem();
	CLASS_METHOD_DECLARE(GetParticleCount)
	CLASS_METHOD_DECLARE(GetBufferIndex)
	CLASS_METHOD_DECLARE(ContainsParticle)
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
	CLASS_METHOD_DECLARE(DestroyParticles)
};

Persistent<Function> WrapParticleGroup::g_constructor;

void WrapParticleGroup::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleGroup = NanNew<FunctionTemplate>(New);
	tplParticleGroup->SetClassName(NanNew<String>("b2ParticleGroup"));
	tplParticleGroup->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplParticleGroup, GetParticleCount)
	CLASS_METHOD_APPLY(tplParticleGroup, GetBufferIndex)
	CLASS_METHOD_APPLY(tplParticleGroup, ContainsParticle)
	CLASS_METHOD_APPLY(tplParticleGroup, DestroyParticles)
	NanAssignPersistent(g_constructor, tplParticleGroup->GetFunction());
	exports->Set(NanNew<String>("b2ParticleGroup"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapParticleGroup::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapParticleGroup::New)
{
	NanScope();
	WrapParticleGroup* that = new WrapParticleGroup();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapParticleGroup, GetParticleCount,
{
	NanScope();
	NanReturnValue(NanNew<Integer>(that->m_particle_group->GetParticleCount()));
})

CLASS_METHOD_IMPLEMENT(WrapParticleGroup, GetBufferIndex,
{
	NanScope();
	NanReturnValue(NanNew<Integer>(that->m_particle_group->GetBufferIndex()));
})

CLASS_METHOD_IMPLEMENT(WrapParticleGroup, ContainsParticle,
{
	NanScope();
	int32 index = (int32) args[0]->Int32Value();
	NanReturnValue(NanNew<Boolean>(that->m_particle_group->ContainsParticle(index)));
})

CLASS_METHOD_IMPLEMENT(WrapParticleGroup, DestroyParticles,
{
	NanScope();
	bool callDestructionListener = (args.Length() > 0) ? args[0]->BooleanValue() : false;
	that->m_particle_group->DestroyParticles(callDestructionListener);
	NanReturnUndefined();
})

//// b2ParticleSystemDef

class WrapParticleSystemDef : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);

private:
	b2ParticleSystemDef m_psd;

private:
	WrapParticleSystemDef()
	{
	}
	~WrapParticleSystemDef()
	{
	}

public:
	void SyncPull()
	{
		// sync: pull data from javascript objects
	}

	void SyncPush()
	{
		// sync: push data into javascript objects
	}

	b2ParticleSystemDef& GetParticleSystemDef() { return m_psd; }

	const b2ParticleSystemDef& UseParticleSystemDef() { SyncPull(); return GetParticleSystemDef(); }

private:
	static NAN_METHOD(New);

	CLASS_MEMBER_DECLARE(strictContactCheck)
	CLASS_MEMBER_DECLARE(density)
	CLASS_MEMBER_DECLARE(gravityScale)
	CLASS_MEMBER_DECLARE(radius)
	CLASS_MEMBER_DECLARE(maxCount)
	CLASS_MEMBER_DECLARE(pressureStrength)
	CLASS_MEMBER_DECLARE(dampingStrength)
	CLASS_MEMBER_DECLARE(elasticStrength)
	CLASS_MEMBER_DECLARE(springStrength)
	CLASS_MEMBER_DECLARE(viscousStrength)
	CLASS_MEMBER_DECLARE(surfaceTensionPressureStrength)
	CLASS_MEMBER_DECLARE(surfaceTensionNormalStrength)
	CLASS_MEMBER_DECLARE(repulsiveStrength)
	CLASS_MEMBER_DECLARE(powderStrength)
	CLASS_MEMBER_DECLARE(ejectionStrength)
	CLASS_MEMBER_DECLARE(staticPressureStrength)
	CLASS_MEMBER_DECLARE(staticPressureRelaxation)
	CLASS_MEMBER_DECLARE(staticPressureIterations)
	CLASS_MEMBER_DECLARE(colorMixingStrength)
	CLASS_MEMBER_DECLARE(destroyByAge)
	CLASS_MEMBER_DECLARE(lifetimeGranularity)
};

Persistent<Function> WrapParticleSystemDef::g_constructor;

void WrapParticleSystemDef::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleSystemDef = NanNew<FunctionTemplate>(New);
	tplParticleSystemDef->SetClassName(NanNew<String>("b2ParticleSystemDef"));
	tplParticleSystemDef->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_MEMBER_APPLY(tplParticleSystemDef, strictContactCheck)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, strictContactCheck)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, density)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, gravityScale)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, radius)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, maxCount)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, pressureStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, dampingStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, elasticStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, springStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, viscousStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, surfaceTensionPressureStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, surfaceTensionNormalStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, repulsiveStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, powderStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, ejectionStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, staticPressureStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, staticPressureRelaxation)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, staticPressureIterations)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, colorMixingStrength)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, destroyByAge)
	CLASS_MEMBER_APPLY(tplParticleSystemDef, lifetimeGranularity)
	NanAssignPersistent(g_constructor, tplParticleSystemDef->GetFunction());
	exports->Set(NanNew<String>("b2ParticleSystemDef"), NanNew<Function>(g_constructor));
}

NAN_METHOD(WrapParticleSystemDef::New)
{
	NanScope();
	WrapParticleSystemDef* that = new WrapParticleSystemDef();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapParticleSystemDef, m_psd, bool, strictContactCheck)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, density)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, gravityScale)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, radius)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapParticleSystemDef, m_psd, int32, maxCount)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, pressureStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, dampingStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, elasticStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, springStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, viscousStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, surfaceTensionPressureStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, surfaceTensionNormalStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, repulsiveStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, powderStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, ejectionStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, staticPressureStrength)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, staticPressureRelaxation)
CLASS_MEMBER_IMPLEMENT_INTEGER	(WrapParticleSystemDef, m_psd, int32, staticPressureIterations)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, colorMixingStrength)
CLASS_MEMBER_IMPLEMENT_BOOLEAN	(WrapParticleSystemDef, m_psd, bool, destroyByAge)
CLASS_MEMBER_IMPLEMENT_NUMBER	(WrapParticleSystemDef, m_psd, float32, lifetimeGranularity)

//// b2ParticleSystem

class WrapParticleSystem : public node::ObjectWrap
{
private:
	static Persistent<Function> g_constructor;

public:
	static void Init(Handle<Object> exports);
	static Handle<Object> NewInstance();

	static WrapParticleSystem* GetWrap(const b2ParticleSystem* particle_system)
	{
		//return (WrapParticleSystem*) particle_system->GetUserData();
		return NULL;
	}

	static void SetWrap(b2ParticleSystem* particle_system, WrapParticleSystem* wrap)
	{
		//particle_system->SetUserData(wrap);
	}

private:
	b2ParticleSystem* m_particle_system;
	Persistent<Object> m_particle_system_world;

private:
	WrapParticleSystem() :
		m_particle_system(NULL)
	{
		// create javascript objects
	}
	~WrapParticleSystem()
	{
		NanDisposePersistent(m_particle_system_world);
	}

public:
	b2ParticleSystem* GetParticleSystem() { return m_particle_system; }

	void SetupObject(Handle<Object> h_world, WrapParticleSystemDef* wrap_psd, b2ParticleSystem* particle_system)
	{
		m_particle_system = particle_system;

		// set reference to this particle_system (prevent GC)
		Ref();

		// set reference to world object
		NanAssignPersistent(m_particle_system_world, h_world);
	}

	b2ParticleSystem* ResetObject()
	{
		// clear reference to world object
		NanDisposePersistent(m_particle_system_world);

		// clear reference to this particle_system (allow GC)
		Unref();

		b2ParticleSystem* particle_system = m_particle_system;
		m_particle_system = NULL;
		return particle_system;
	}

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(CreateParticle)
//	const b2ParticleHandle* GetParticleHandleFromIndex(const int32 index);
	CLASS_METHOD_DECLARE(DestroyParticle)
	CLASS_METHOD_DECLARE(DestroyOldestParticle)
//	int32 DestroyParticlesInShape(const b2Shape& shape, const b2Transform& xf);
//	int32 DestroyParticlesInShape(const b2Shape& shape, const b2Transform& xf, bool callDestructionListener);
	CLASS_METHOD_DECLARE(CreateParticleGroup)
//	void JoinParticleGroups(b2ParticleGroup* groupA, b2ParticleGroup* groupB);
//	void SplitParticleGroup(b2ParticleGroup* group);
//	b2ParticleGroup* GetParticleGroupList();
//	int32 GetParticleGroupCount() const;
//	int32 GetParticleCount() const;
	CLASS_METHOD_DECLARE(GetParticleCount)
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
	CLASS_METHOD_DECLARE(GetRadius)
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
	CLASS_METHOD_DECLARE(SetPositionBuffer)
//	void SetVelocityBuffer(b2Vec2* buffer, int32 capacity);
	CLASS_METHOD_DECLARE(SetVelocityBuffer)
//	void SetColorBuffer(b2ParticleColor* buffer, int32 capacity);
	CLASS_METHOD_DECLARE(SetColorBuffer)
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
//	void QueryAABB(b2QueryCallback* callback, const b2AABB& aabb) const;
//	void QueryShapeAABB(b2QueryCallback* callback, const b2Shape& shape, const b2Transform& xf) const;
//	void RayCast(b2RayCastCallback* callback, const b2Vec2& point1, const b2Vec2& point2) const;
//	void ComputeAABB(b2AABB* const aabb) const;
};

Persistent<Function> WrapParticleSystem::g_constructor;

void WrapParticleSystem::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplParticleSystem = NanNew<FunctionTemplate>(New);
	tplParticleSystem->SetClassName(NanNew<String>("b2ParticleSystem"));
	tplParticleSystem->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplParticleSystem, CreateParticle)
	CLASS_METHOD_APPLY(tplParticleSystem, DestroyParticle)
	CLASS_METHOD_APPLY(tplParticleSystem, DestroyOldestParticle)
	CLASS_METHOD_APPLY(tplParticleSystem, CreateParticleGroup)
	CLASS_METHOD_APPLY(tplParticleSystem, GetParticleCount)
	CLASS_METHOD_APPLY(tplParticleSystem, GetRadius)
	CLASS_METHOD_APPLY(tplParticleSystem, SetPositionBuffer)
	CLASS_METHOD_APPLY(tplParticleSystem, SetVelocityBuffer)
	CLASS_METHOD_APPLY(tplParticleSystem, SetColorBuffer)
	NanAssignPersistent(g_constructor, tplParticleSystem->GetFunction());
	exports->Set(NanNew<String>("b2ParticleSystem"), NanNew<Function>(g_constructor));
}

Handle<Object> WrapParticleSystem::NewInstance()
{
	NanEscapableScope();
	Local<Object> instance = NanNew<Function>(g_constructor)->NewInstance();
	return NanEscapeScope(instance);
}

NAN_METHOD(WrapParticleSystem::New)
{
	NanScope();
	WrapParticleSystem* that = new WrapParticleSystem();
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, CreateParticle,
{
	WrapParticleDef* wrap_pd = node::ObjectWrap::Unwrap<WrapParticleDef>(Local<Object>::Cast(args[0]));

	// create box2d particle
	int32 particle_index = that->m_particle_system->CreateParticle(wrap_pd->UseParticleDef());

	NanReturnValue(NanNew<Integer>(particle_index));
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, DestroyParticle,
{
	int32 particle_index = (int32) args[0]->Int32Value();
	bool callDestructionListener = (args.Length() > 1) ? args[1]->BooleanValue() : false;

	// destroy box2d particle
	that->m_particle_system->DestroyParticle(particle_index, callDestructionListener);

	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, DestroyOldestParticle,
{
	int32 particle_index = (int32) args[0]->Int32Value();
	bool callDestructionListener = (args.Length() > 1) ? args[1]->BooleanValue() : false;

	// destroy box2d particle
	that->m_particle_system->DestroyOldestParticle(particle_index, callDestructionListener);

	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, CreateParticleGroup,
{
	WrapParticleGroupDef* wrap_pgd = node::ObjectWrap::Unwrap<WrapParticleGroupDef>(Local<Object>::Cast(args[0]));

	// create box2d particle group
	b2ParticleGroup* particle_group = that->m_particle_system->CreateParticleGroup(wrap_pgd->UseParticleGroupDef());

	// create javascript particle group object
	Local<Object> h_particle_group = NanNew<Object>(WrapParticleGroup::NewInstance());
	WrapParticleGroup* wrap_particle_group = node::ObjectWrap::Unwrap<WrapParticleGroup>(h_particle_group);

	// set up javascript particle group object
	wrap_particle_group->SetupObject(args.This(), wrap_pgd, particle_group);

	NanReturnValue(h_particle_group);
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, GetParticleCount, { NanReturnValue(NanNew<Integer>(that->m_particle_system->GetParticleCount())); })

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, GetRadius, { NanReturnValue(NanNew<Number>(that->m_particle_system->GetRadius())); })
	
CLASS_METHOD_IMPLEMENT(WrapParticleSystem, SetPositionBuffer, 
{
	Handle<Object> _buffer = Handle<Object>::Cast(args[0]);
	int32 capacity = (int32) args[1]->Int32Value();
	b2Vec2* buffer = (b2Vec2*) _buffer->GetIndexedPropertiesExternalArrayData();
	that->m_particle_system->SetPositionBuffer(buffer, capacity);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, SetVelocityBuffer, 
{
	Handle<Object> _buffer = Handle<Object>::Cast(args[0]);
	int32 capacity = (int32) args[1]->Int32Value();
	b2Vec2* buffer = (b2Vec2*) _buffer->GetIndexedPropertiesExternalArrayData();
	that->m_particle_system->SetVelocityBuffer(buffer, capacity);
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapParticleSystem, SetColorBuffer, 
{
	Handle<Object> _buffer = Handle<Object>::Cast(args[0]);
	int32 capacity = (int32) args[1]->Int32Value();
	b2ParticleColor* buffer = (b2ParticleColor*) _buffer->GetIndexedPropertiesExternalArrayData();
	that->m_particle_system->SetColorBuffer(buffer, capacity);
	NanReturnUndefined();
})

#endif

//// b2World

class WrapWorld : public node::ObjectWrap
{
private:
	class WrapDestructionListener : public b2DestructionListener
	{
	public:
		WrapWorld* m_that;
	public:
		WrapDestructionListener(WrapWorld* that) : m_that(that) {}
		~WrapDestructionListener() { m_that = NULL; }
	public:
		virtual void SayGoodbye(b2Joint* joint);
		virtual void SayGoodbye(b2Fixture* fixture);
		#if 1 // B2_ENABLE_PARTICLE
		virtual void SayGoodbye(b2ParticleGroup* group);
		virtual void SayGoodbye(b2ParticleSystem* particleSystem, int32 index);
		#endif
	};

private:
	class WrapContactFilter : public b2ContactFilter
	{
	public:
		WrapWorld* m_that;
	public:
		WrapContactFilter(WrapWorld* that) : m_that(that) {}
		~WrapContactFilter() { m_that = NULL; }
	public:
		virtual bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB);
		#if 1 // B2_ENABLE_PARTICLE
		virtual bool ShouldCollide(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 particleIndex);
		virtual bool ShouldCollide(b2ParticleSystem* particleSystem, int32 particleIndexA, int32 particleIndexB);
		#endif
	};

private:
	class WrapContactListener : public b2ContactListener
	{
	public:
		WrapWorld* m_that;
	public:
		WrapContactListener(WrapWorld* that) : m_that(that) {}
		~WrapContactListener() { m_that = NULL; }
	public:
		virtual void BeginContact(b2Contact* contact);
		virtual void EndContact(b2Contact* contact);
		#if 1 // B2_ENABLE_PARTICLE
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
		WrapWorld* m_that;
	public:
		WrapDraw(WrapWorld* that) : m_that(that) {}
		~WrapDraw() { m_that = NULL; }
	public:
		virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
		virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
		virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
		#if 1 // B2_ENABLE_PARTICLE
		virtual void DrawParticles(const b2Vec2 *centers, float32 radius, const b2ParticleColor *colors, int32 count);
		#endif
		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
		virtual void DrawTransform(const b2Transform& xf);
	};

public:
	static void Init(Handle<Object> exports);

	static WrapWorld* GetWrap(const b2World* world)
	{
		//return (WrapWorld*) world->GetUserData();
		return NULL;
	}

	static void SetWrap(b2World* world, WrapWorld* wrap)
	{
		//world->SetUserData(wrap);
	}

private:
	b2World m_world;
	Persistent<Object> m_destruction_listener;
	WrapDestructionListener m_wrap_destruction_listener;
	Persistent<Object> m_contact_filter;
	WrapContactFilter m_wrap_contact_filter;
	Persistent<Object> m_contact_listener;
	WrapContactListener m_wrap_contact_listener;
	Persistent<Object> m_draw;
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

		NanDisposePersistent(m_destruction_listener);
		NanDisposePersistent(m_contact_filter);
		NanDisposePersistent(m_contact_listener);
		NanDisposePersistent(m_draw);
	}

private:
	static NAN_METHOD(New);

	CLASS_METHOD_DECLARE(SetDestructionListener)
	CLASS_METHOD_DECLARE(SetContactFilter)
	CLASS_METHOD_DECLARE(SetContactListener)
	CLASS_METHOD_DECLARE(SetDebugDraw)
	CLASS_METHOD_DECLARE(CreateBody)
	CLASS_METHOD_DECLARE(DestroyBody)
	CLASS_METHOD_DECLARE(CreateJoint)
	CLASS_METHOD_DECLARE(DestroyJoint)
	CLASS_METHOD_DECLARE(Step)
	CLASS_METHOD_DECLARE(ClearForces)
	CLASS_METHOD_DECLARE(DrawDebugData)
	CLASS_METHOD_DECLARE(QueryAABB)
	#if 1 // B2_ENABLE_PARTICLE
	CLASS_METHOD_DECLARE(QueryShapeAABB)
	#endif
	CLASS_METHOD_DECLARE(RayCast)
	CLASS_METHOD_DECLARE(GetBodyList)
	CLASS_METHOD_DECLARE(GetJointList)
	CLASS_METHOD_DECLARE(GetContactList)
	CLASS_METHOD_DECLARE(SetAllowSleeping)
	CLASS_METHOD_DECLARE(GetAllowSleeping)
	CLASS_METHOD_DECLARE(SetWarmStarting)
	CLASS_METHOD_DECLARE(GetWarmStarting)
	CLASS_METHOD_DECLARE(SetContinuousPhysics)
	CLASS_METHOD_DECLARE(GetContinuousPhysics)
	CLASS_METHOD_DECLARE(SetSubStepping)
	CLASS_METHOD_DECLARE(GetSubStepping)
//	int32 GetProxyCount() const;
	CLASS_METHOD_DECLARE(GetBodyCount)
	CLASS_METHOD_DECLARE(GetJointCount)
	CLASS_METHOD_DECLARE(GetContactCount)
//	int32 GetTreeHeight() const;
//	int32 GetTreeBalance() const;
//	float32 GetTreeQuality() const;
	CLASS_METHOD_DECLARE(SetGravity)
	CLASS_METHOD_DECLARE(GetGravity)
	CLASS_METHOD_DECLARE(IsLocked)
	CLASS_METHOD_DECLARE(SetAutoClearForces)
	CLASS_METHOD_DECLARE(GetAutoClearForces)
//	void ShiftOrigin(const b2Vec2& newOrigin);
///	const b2ContactManager& GetContactManager() const;
///	const b2Profile& GetProfile() const;
///	void Dump();

	#if 1 // B2_ENABLE_PARTICLE
	CLASS_METHOD_DECLARE(CreateParticleSystem)
	CLASS_METHOD_DECLARE(DestroyParticleSystem)
	CLASS_METHOD_DECLARE(CalculateReasonableParticleIterations)
	#endif
};

void WrapWorld::WrapDestructionListener::SayGoodbye(b2Joint* joint)
{
	NanScope();
	if (!m_that->m_destruction_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_destruction_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("SayGoodbyeJoint")));
		// get joint internal data
		WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
		Local<Object> h_joint = NanObjectWrapHandle(wrap_joint);
		Handle<Value> argv[] = { h_joint };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDestructionListener::SayGoodbye(b2Fixture* fixture)
{
	NanScope();
	if (!m_that->m_destruction_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_destruction_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("SayGoodbyeFixture")));
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		Local<Object> h_fixture = NanObjectWrapHandle(wrap_fixture);
		Handle<Value> argv[] = { h_fixture };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if 1 // B2_ENABLE_PARTICLE

void WrapWorld::WrapDestructionListener::SayGoodbye(b2ParticleGroup* group)
{
	NanScope();
	if (!m_that->m_destruction_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_destruction_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("SayGoodbyeParticleGroup")));
		// get particle group internal data
		WrapParticleGroup* wrap_group = WrapParticleGroup::GetWrap(group);
		Local<Object> h_group = NanObjectWrapHandle(wrap_group);
		Handle<Value> argv[] = { h_group };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDestructionListener::SayGoodbye(b2ParticleSystem* particleSystem, int32 index)
{
	NanScope();
	if (!m_that->m_destruction_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_destruction_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("SayGoodbyeParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = (WrapParticleSystem*) particleSystem->GetUserData();
		///	Local<Object> h_system = NanObjectWrapHandle(wrap_system);
		///	Handle<Value> argv[] = { h_system, NanNew<Integer>(index) };
		///	NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#endif

bool WrapWorld::WrapContactFilter::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
	NanScope();
	if (!m_that->m_contact_filter.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_filter);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("ShouldCollide")));
		// get fixture internal data
		WrapFixture* wrap_fixtureA = WrapFixture::GetWrap(fixtureA);
		WrapFixture* wrap_fixtureB = WrapFixture::GetWrap(fixtureB);
		Local<Object> h_fixtureA = NanObjectWrapHandle(wrap_fixtureA);
		Local<Object> h_fixtureB = NanObjectWrapHandle(wrap_fixtureB);
		Handle<Value> argv[] = { h_fixtureA, h_fixtureB };
		return NanMakeCallback(h_that, h_method, countof(argv), argv)->BooleanValue();
	}
	return b2ContactFilter::ShouldCollide(fixtureA, fixtureB);
}

#if 1 // B2_ENABLE_PARTICLE

bool WrapWorld::WrapContactFilter::ShouldCollide(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 particleIndex)
{
	NanScope();
	if (!m_that->m_contact_filter.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_filter);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("ShouldCollideFixtureParticle")));
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		Local<Object> h_fixture = NanObjectWrapHandle(wrap_fixture);
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = (WrapParticleSystem*) particleSystem->GetUserData();
		///	Local<Object> h_system = NanObjectWrapHandle(wrap_system);
		///	Handle<Value> argv[] = { h_fixture, h_system, NanNew<Integer>(particleIndex) };
		///	return NanMakeCallback(h_that, h_method, countof(argv), argv)->BooleanValue();
	}
	return b2ContactFilter::ShouldCollide(fixture, particleSystem, particleIndex);
}

bool WrapWorld::WrapContactFilter::ShouldCollide(b2ParticleSystem* particleSystem, int32 particleIndexA, int32 particleIndexB)
{
	NanScope();
	if (!m_that->m_contact_filter.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_filter);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("ShouldCollideParticleParticle")));
		// TODO: get particle system internal data
		///	WrapParticleSystem* wrap_system = (WrapParticleSystem*) particleSystem->GetUserData();
		///	Local<Object> h_system = NanObjectWrapHandle(wrap_system);
		///	Handle<Value> argv[] = { h_system, NanNew<Integer>(particleIndexA), NanNew<Integer>(particleIndexB) };
		///	return NanMakeCallback(h_that, h_method, countof(argv), argv)->BooleanValue();
	}
	return b2ContactFilter::ShouldCollide(particleSystem, particleIndexA, particleIndexB);
}

#endif

void WrapWorld::WrapContactListener::BeginContact(b2Contact* contact)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("BeginContact")));
		Local<Object> h_contact = NanNew<Object>(WrapContact::NewInstance(contact));
		Handle<Value> argv[] = { h_contact };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2Contact* contact)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("EndContact")));
		Local<Object> h_contact = NanNew<Object>(WrapContact::NewInstance(contact));
		Handle<Value> argv[] = { h_contact };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if 1 // B2_ENABLE_PARTICLE

void WrapWorld::WrapContactListener::BeginContact(b2ParticleSystem* particleSystem, b2ParticleBodyContact* particleBodyContact)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("BeginContactFixtureParticle")));
		// TODO: get particle system internal data, wrap b2ParticleBodyContact
		///	Handle<Value> argv[] = {};
		///	NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2Fixture* fixture, b2ParticleSystem* particleSystem, int32 index)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("EndContactFixtureParticle")));
		// TODO: get particle system internal data
		///	Handle<Value> argv[] = {};
		///	NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::BeginContact(b2ParticleSystem* particleSystem, b2ParticleContact* particleContact)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("BeginContactParticleParticle")));
		// TODO: get particle system internal data, wrap b2ParticleContact
		///	Handle<Value> argv[] = {};
		///	NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::EndContact(b2ParticleSystem* particleSystem, int32 indexA, int32 indexB)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("EndContactParticleParticle")));
		// TODO: get particle system internal data
		///	Handle<Value> argv[] = {};
		///	NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#endif

void WrapWorld::WrapContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("PreSolve")));
		Local<Object> h_contact = NanNew<Object>(WrapContact::NewInstance(contact));
		Local<Object> h_oldManifold = NanNew<Object>(WrapManifold::NewInstance(*oldManifold));
		Handle<Value> argv[] = { h_contact, h_oldManifold };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
	NanScope();
	if (!m_that->m_contact_listener.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_contact_listener);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("PostSolve")));
		Local<Object> h_contact = NanNew<Object>(WrapContact::NewInstance(contact));
		Local<Object> h_impulse = NanNew<Object>(WrapContactImpulse::NewInstance(*impulse));
		Handle<Value> argv[] = { h_contact, h_impulse };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawPolygon")));
		Local<Array> h_vertices = NanNew<Array>(vertexCount);
		for (int i = 0; i < vertexCount; ++i)
		{
			h_vertices->Set(i, NanNew<Object>(WrapVec2::NewInstance(vertices[i])));
		}
		Local<Integer> h_vertexCount = NanNew<Integer>(vertexCount);
		Local<Object> h_color = NanNew<Object>(WrapColor::NewInstance(color));
		Handle<Value> argv[] = { h_vertices, h_vertexCount, h_color };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawSolidPolygon")));
		Local<Array> h_vertices = NanNew<Array>(vertexCount);
		for (int i = 0; i < vertexCount; ++i)
		{
			h_vertices->Set(i, NanNew<Object>(WrapVec2::NewInstance(vertices[i])));
		}
		Local<Integer> h_vertexCount = NanNew<Integer>(vertexCount);
		Local<Object> h_color = NanNew<Object>(WrapColor::NewInstance(color));
		Handle<Value> argv[] = { h_vertices, h_vertexCount, h_color };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawCircle")));
		Local<Object> h_center = NanNew<Object>(WrapVec2::NewInstance(center));
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		Local<Number> h_radius = NanNew<Number>(radius);
		Local<Object> h_color = NanNew<Object>(WrapColor::NewInstance(color));
		Handle<Value> argv[] = { h_center, h_radius, h_color };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawSolidCircle")));
		Local<Object> h_center = NanNew<Object>(WrapVec2::NewInstance(center));
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		Local<Number> h_radius = NanNew<Number>(radius);
		Local<Object> h_axis = NanNew<Object>(WrapVec2::NewInstance(axis));
		Local<Object> h_color = NanNew<Object>(WrapColor::NewInstance(color));
		Handle<Value> argv[] = { h_center, h_radius, h_axis, h_color };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

#if 1 // B2_ENABLE_PARTICLE
void WrapWorld::WrapDraw::DrawParticles(const b2Vec2 *centers, float32 radius, const b2ParticleColor *colors, int32 count)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawParticles")));
		Local<Array> h_centers = NanNew<Array>(count);
		for (int i = 0; i < count; ++i)
		{
			h_centers->Set(i, NanNew<Object>(WrapVec2::NewInstance(centers[i])));
		}
		#if defined(_WIN32)
		if (radius <= 0) { radius = 0.05f; } // hack: bug in Windows release build
		#endif
		Local<Number> h_radius = NanNew<Number>(radius);
		Local<Integer> h_count = NanNew<Integer>(count);
		if (colors != NULL)
		{
			Local<Array> h_colors = NanNew<Array>(count);
			for (int i = 0; i < count; ++i)
			{
				h_colors->Set(i, NanNew<Object>(WrapParticleColor::NewInstance(colors[i])));
			}
			Handle<Value> argv[] = { h_centers, h_radius, h_colors, h_count };
			NanMakeCallback(h_that, h_method, countof(argv), argv);
		}
		else
		{
			Handle<Value> argv[] = { h_centers, h_radius, NanNull(), h_count };
			NanMakeCallback(h_that, h_method, countof(argv), argv);
		}
	}
}
#endif

void WrapWorld::WrapDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawSegment")));
		Local<Object> h_p1 = NanNew<Object>(WrapVec2::NewInstance(p1));
		Local<Object> h_p2 = NanNew<Object>(WrapVec2::NewInstance(p2));
		Local<Object> h_color = NanNew<Object>(WrapColor::NewInstance(color));
		Handle<Value> argv[] = { h_p1, h_p2, h_color };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::WrapDraw::DrawTransform(const b2Transform& xf)
{
	NanScope();
	if (!m_that->m_draw.IsEmpty())
	{
		Local<Object> h_that = NanNew<Object>(m_that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_that->Get(NanNew<String>("DrawTransform")));
		Local<Object> h_xf = NanNew<Object>(WrapTransform::NewInstance(xf));
		Handle<Value> argv[] = { h_xf };
		NanMakeCallback(h_that, h_method, countof(argv), argv);
	}
}

void WrapWorld::Init(Handle<Object> exports)
{
	Local<FunctionTemplate> tplWorld = NanNew<FunctionTemplate>(New);
	tplWorld->SetClassName(NanNew<String>("b2World"));
	tplWorld->InstanceTemplate()->SetInternalFieldCount(1);
	CLASS_METHOD_APPLY(tplWorld, SetDestructionListener)
	CLASS_METHOD_APPLY(tplWorld, SetContactFilter)
	CLASS_METHOD_APPLY(tplWorld, SetContactListener)
	CLASS_METHOD_APPLY(tplWorld, SetDebugDraw)
	CLASS_METHOD_APPLY(tplWorld, CreateBody)
	CLASS_METHOD_APPLY(tplWorld, DestroyBody)
	CLASS_METHOD_APPLY(tplWorld, CreateJoint)
	CLASS_METHOD_APPLY(tplWorld, DestroyJoint)
	CLASS_METHOD_APPLY(tplWorld, Step)
	CLASS_METHOD_APPLY(tplWorld, ClearForces)
	CLASS_METHOD_APPLY(tplWorld, DrawDebugData)
	CLASS_METHOD_APPLY(tplWorld, QueryAABB)
	#if 1 // B2_ENABLE_PARTICLE
	CLASS_METHOD_APPLY(tplWorld, QueryShapeAABB)
	#endif
	CLASS_METHOD_APPLY(tplWorld, RayCast)
	CLASS_METHOD_APPLY(tplWorld, GetBodyList)
	CLASS_METHOD_APPLY(tplWorld, GetJointList)
	CLASS_METHOD_APPLY(tplWorld, GetContactList)
	CLASS_METHOD_APPLY(tplWorld, SetAllowSleeping)
	CLASS_METHOD_APPLY(tplWorld, GetAllowSleeping)
	CLASS_METHOD_APPLY(tplWorld, SetWarmStarting)
	CLASS_METHOD_APPLY(tplWorld, GetWarmStarting)
	CLASS_METHOD_APPLY(tplWorld, SetContinuousPhysics)
	CLASS_METHOD_APPLY(tplWorld, GetContinuousPhysics)
	CLASS_METHOD_APPLY(tplWorld, SetSubStepping)
	CLASS_METHOD_APPLY(tplWorld, GetSubStepping)
	CLASS_METHOD_APPLY(tplWorld, GetBodyCount)
	CLASS_METHOD_APPLY(tplWorld, GetJointCount)
	CLASS_METHOD_APPLY(tplWorld, GetContactCount)
	CLASS_METHOD_APPLY(tplWorld, SetGravity)
	CLASS_METHOD_APPLY(tplWorld, GetGravity)
	CLASS_METHOD_APPLY(tplWorld, IsLocked)
	CLASS_METHOD_APPLY(tplWorld, SetAutoClearForces)
	CLASS_METHOD_APPLY(tplWorld, GetAutoClearForces)
	#if 1 // B2_ENABLE_PARTICLE
	CLASS_METHOD_APPLY(tplWorld, CreateParticleSystem)
	CLASS_METHOD_APPLY(tplWorld, DestroyParticleSystem)
	CLASS_METHOD_APPLY(tplWorld, CalculateReasonableParticleIterations)
	#endif
	exports->Set(NanNew<String>("b2World"), tplWorld->GetFunction());
}

NAN_METHOD(WrapWorld::New)
{
	NanScope();
	WrapVec2* wrap_gravity = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapWorld* that = new WrapWorld(wrap_gravity->GetVec2());
	that->Wrap(args.This());
	NanReturnValue(args.This());
}

CLASS_METHOD_IMPLEMENT(WrapWorld, SetDestructionListener,
{
	if (args[0]->IsObject())
	{
		NanAssignPersistent(that->m_destruction_listener, args[0].As<Object>());
	}
	else
	{
		NanDisposePersistent(that->m_destruction_listener);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetContactFilter,
{
	if (args[0]->IsObject())
	{
		NanAssignPersistent(that->m_contact_filter, args[0].As<Object>());
	}
	else
	{
		NanDisposePersistent(that->m_contact_filter);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetContactListener,
{
	if (args[0]->IsObject())
	{
		NanAssignPersistent(that->m_contact_listener, args[0].As<Object>());
	}
	else
	{
		NanDisposePersistent(that->m_contact_listener);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetDebugDraw,
{
	if (args[0]->IsObject())
	{
		NanAssignPersistent(that->m_draw, args[0].As<Object>());
	}
	else
	{
		NanDisposePersistent(that->m_draw);
	}
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, CreateBody,
{
	WrapBodyDef* wrap_bd = node::ObjectWrap::Unwrap<WrapBodyDef>(Local<Object>::Cast(args[0]));

	// create box2d body
	b2Body* body = that->m_world.CreateBody(&wrap_bd->UseBodyDef());

	// create javascript body object
	Local<Object> h_body = NanNew<Object>(WrapBody::NewInstance());
	WrapBody* wrap_body = node::ObjectWrap::Unwrap<WrapBody>(h_body);

	// set up javascript body object
	wrap_body->SetupObject(args.This(), wrap_bd, body);

	NanReturnValue(h_body);
})

CLASS_METHOD_IMPLEMENT(WrapWorld, DestroyBody,
{
	Local<Object> h_body = Local<Object>::Cast(args[0]);
	WrapBody* wrap_body = node::ObjectWrap::Unwrap<WrapBody>(h_body);

	// delete box2d body
	that->m_world.DestroyBody(wrap_body->GetBody());

	// reset javascript body object
	wrap_body->ResetObject();

	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, CreateJoint,
{
	WrapJointDef* wrap_jd = node::ObjectWrap::Unwrap<WrapJointDef>(Local<Object>::Cast(args[0]));

	switch (wrap_jd->GetJointDef().type)
	{
	case e_unknownJoint:
		break;
	case e_revoluteJoint:
	{
		WrapRevoluteJointDef* wrap_revolute_jd = node::ObjectWrap::Unwrap<WrapRevoluteJointDef>(Local<Object>::Cast(args[0]));

		// create box2d revolute joint
		b2RevoluteJoint* revolute_joint = (b2RevoluteJoint*) that->m_world.CreateJoint(&wrap_revolute_jd->UseJointDef());

		// create javascript revolute joint object
		Local<Object> h_revolute_joint = NanNew<Object>(WrapRevoluteJoint::NewInstance());
		WrapRevoluteJoint* wrap_revolute_joint = node::ObjectWrap::Unwrap<WrapRevoluteJoint>(h_revolute_joint);

		// set up javascript revolute joint object
		wrap_revolute_joint->SetupObject(wrap_revolute_jd, revolute_joint);

		NanReturnValue(h_revolute_joint);
		break;
	}
	case e_prismaticJoint:
	{
		WrapPrismaticJointDef* wrap_prismatic_jd = node::ObjectWrap::Unwrap<WrapPrismaticJointDef>(Local<Object>::Cast(args[0]));

		// create box2d prismatic joint
		b2PrismaticJoint* prismatic_joint = (b2PrismaticJoint*) that->m_world.CreateJoint(&wrap_prismatic_jd->UseJointDef());

		// create javascript prismatic joint object
		Local<Object> h_prismatic_joint = NanNew<Object>(WrapPrismaticJoint::NewInstance());
		WrapPrismaticJoint* wrap_prismatic_joint = node::ObjectWrap::Unwrap<WrapPrismaticJoint>(h_prismatic_joint);

		// set up javascript prismatic joint object
		wrap_prismatic_joint->SetupObject(wrap_prismatic_jd, prismatic_joint);

		NanReturnValue(h_prismatic_joint);
		break;
	}
	case e_distanceJoint:
	{
		WrapDistanceJointDef* wrap_distance_jd = node::ObjectWrap::Unwrap<WrapDistanceJointDef>(Local<Object>::Cast(args[0]));

		// create box2d distance joint
		b2DistanceJoint* distance_joint = (b2DistanceJoint*) that->m_world.CreateJoint(&wrap_distance_jd->UseJointDef());

		// create javascript distance joint object
		Local<Object> h_distance_joint = NanNew<Object>(WrapDistanceJoint::NewInstance());
		WrapDistanceJoint* wrap_distance_joint = node::ObjectWrap::Unwrap<WrapDistanceJoint>(h_distance_joint);

		// set up javascript distance joint object
		wrap_distance_joint->SetupObject(wrap_distance_jd, distance_joint);

		NanReturnValue(h_distance_joint);
		break;
	}
	case e_pulleyJoint:
	{
		WrapPulleyJointDef* wrap_pulley_jd = node::ObjectWrap::Unwrap<WrapPulleyJointDef>(Local<Object>::Cast(args[0]));

		// create box2d pulley joint
		b2PulleyJoint* pulley_joint = (b2PulleyJoint*) that->m_world.CreateJoint(&wrap_pulley_jd->UseJointDef());

		// create javascript pulley joint object
		Local<Object> h_pulley_joint = NanNew<Object>(WrapPulleyJoint::NewInstance());
		WrapPulleyJoint* wrap_pulley_joint = node::ObjectWrap::Unwrap<WrapPulleyJoint>(h_pulley_joint);

		// set up javascript pulley joint object
		wrap_pulley_joint->SetupObject(wrap_pulley_jd, pulley_joint);

		NanReturnValue(h_pulley_joint);
		break;
	}
	case e_mouseJoint:
	{
		WrapMouseJointDef* wrap_mouse_jd = node::ObjectWrap::Unwrap<WrapMouseJointDef>(Local<Object>::Cast(args[0]));

		// create box2d mouse joint
		b2MouseJoint* mouse_joint = (b2MouseJoint*) that->m_world.CreateJoint(&wrap_mouse_jd->UseJointDef());

		// create javascript mouse joint object
		Local<Object> h_mouse_joint = NanNew<Object>(WrapMouseJoint::NewInstance());
		WrapMouseJoint* wrap_mouse_joint = node::ObjectWrap::Unwrap<WrapMouseJoint>(h_mouse_joint);

		// set up javascript mouse joint object
		wrap_mouse_joint->SetupObject(wrap_mouse_jd, mouse_joint);

		NanReturnValue(h_mouse_joint);
		break;
	}
	case e_gearJoint:
	{
		WrapGearJointDef* wrap_gear_jd = node::ObjectWrap::Unwrap<WrapGearJointDef>(Local<Object>::Cast(args[0]));

		// create box2d gear joint
		b2GearJoint* gear_joint = (b2GearJoint*) that->m_world.CreateJoint(&wrap_gear_jd->UseJointDef());

		// create javascript gear joint object
		Local<Object> h_gear_joint = NanNew<Object>(WrapGearJoint::NewInstance());
		WrapGearJoint* wrap_gear_joint = node::ObjectWrap::Unwrap<WrapGearJoint>(h_gear_joint);

		// set up javascript gear joint object
		wrap_gear_joint->SetupObject(wrap_gear_jd, gear_joint);

		NanReturnValue(h_gear_joint);
		break;
	}
	case e_wheelJoint:
	{
		WrapWheelJointDef* wrap_wheel_jd = node::ObjectWrap::Unwrap<WrapWheelJointDef>(Local<Object>::Cast(args[0]));

		// create box2d wheel joint
		b2WheelJoint* wheel_joint = (b2WheelJoint*) that->m_world.CreateJoint(&wrap_wheel_jd->UseJointDef());

		// create javascript wheel joint object
		Local<Object> h_wheel_joint = NanNew<Object>(WrapWheelJoint::NewInstance());
		WrapWheelJoint* wrap_wheel_joint = node::ObjectWrap::Unwrap<WrapWheelJoint>(h_wheel_joint);

		// set up javascript wheel joint object
		wrap_wheel_joint->SetupObject(wrap_wheel_jd, wheel_joint);

		NanReturnValue(h_wheel_joint);
		break;
	}
    case e_weldJoint:
	{
		WrapWeldJointDef* wrap_weld_jd = node::ObjectWrap::Unwrap<WrapWeldJointDef>(Local<Object>::Cast(args[0]));

		// create box2d weld joint
		b2WeldJoint* weld_joint = (b2WeldJoint*) that->m_world.CreateJoint(&wrap_weld_jd->UseJointDef());

		// create javascript weld joint object
		Local<Object> h_weld_joint = NanNew<Object>(WrapWeldJoint::NewInstance());
		WrapWeldJoint* wrap_weld_joint = node::ObjectWrap::Unwrap<WrapWeldJoint>(h_weld_joint);

		// set up javascript weld joint object
		wrap_weld_joint->SetupObject(wrap_weld_jd, weld_joint);

		NanReturnValue(h_weld_joint);
		break;
	}
	case e_frictionJoint:
	{
		WrapFrictionJointDef* wrap_friction_jd = node::ObjectWrap::Unwrap<WrapFrictionJointDef>(Local<Object>::Cast(args[0]));

		// create box2d friction joint
		b2FrictionJoint* friction_joint = (b2FrictionJoint*) that->m_world.CreateJoint(&wrap_friction_jd->UseJointDef());

		// create javascript friction joint object
		Local<Object> h_friction_joint = NanNew<Object>(WrapFrictionJoint::NewInstance());
		WrapFrictionJoint* wrap_friction_joint = node::ObjectWrap::Unwrap<WrapFrictionJoint>(h_friction_joint);

		// set up javascript friction joint object
		wrap_friction_joint->SetupObject(wrap_friction_jd, friction_joint);

		NanReturnValue(h_friction_joint);
		break;
	}
	case e_ropeJoint:
	{
		WrapRopeJointDef* wrap_rope_jd = node::ObjectWrap::Unwrap<WrapRopeJointDef>(Local<Object>::Cast(args[0]));

		// create box2d rope joint
		b2RopeJoint* rope_joint = (b2RopeJoint*) that->m_world.CreateJoint(&wrap_rope_jd->UseJointDef());

		// create javascript rope joint object
		Local<Object> h_rope_joint = NanNew<Object>(WrapRopeJoint::NewInstance());
		WrapRopeJoint* wrap_rope_joint = node::ObjectWrap::Unwrap<WrapRopeJoint>(h_rope_joint);

		// set up javascript rope joint object
		wrap_rope_joint->SetupObject(wrap_rope_jd, rope_joint);

		NanReturnValue(h_rope_joint);
		break;
	}
	case e_motorJoint:
	{
		WrapMotorJointDef* wrap_motor_jd = node::ObjectWrap::Unwrap<WrapMotorJointDef>(Local<Object>::Cast(args[0]));

		// create box2d motor joint
		b2MotorJoint* motor_joint = (b2MotorJoint*) that->m_world.CreateJoint(&wrap_motor_jd->UseJointDef());

		// create javascript motor joint object
		Local<Object> h_motor_joint = NanNew<Object>(WrapMotorJoint::NewInstance());
		WrapMotorJoint* wrap_motor_joint = node::ObjectWrap::Unwrap<WrapMotorJoint>(h_motor_joint);

		// set up javascript motor joint object
		wrap_motor_joint->SetupObject(wrap_motor_jd, motor_joint);

		NanReturnValue(h_motor_joint);
		break;
	}
	default:
		break;
	}

	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, DestroyJoint,
{
	Local<Object> h_joint = Local<Object>::Cast(args[0]);
	WrapJoint* wrap_joint = node::ObjectWrap::Unwrap<WrapJoint>(h_joint);

	switch (wrap_joint->GetJoint()->GetType())
	{
	case e_unknownJoint:
		break;
	case e_revoluteJoint:
	{
		Local<Object> h_revolute_joint = Local<Object>::Cast(args[0]);
		WrapRevoluteJoint* wrap_revolute_joint = node::ObjectWrap::Unwrap<WrapRevoluteJoint>(h_revolute_joint);

		// delete box2d revolute joint
		that->m_world.DestroyJoint(wrap_revolute_joint->GetJoint());

		// reset javascript revolute joint object
		wrap_revolute_joint->ResetObject();

		break;
	}
	case e_prismaticJoint:
	{
		Local<Object> h_prismatic_joint = Local<Object>::Cast(args[0]);
		WrapPrismaticJoint* wrap_prismatic_joint = node::ObjectWrap::Unwrap<WrapPrismaticJoint>(h_prismatic_joint);

		// delete box2d prismatic joint
		that->m_world.DestroyJoint(wrap_prismatic_joint->GetJoint());

		// reset javascript prismatic joint object
		wrap_prismatic_joint->ResetObject();
		break;
	}
	case e_distanceJoint:
	{
		Local<Object> h_distance_joint = Local<Object>::Cast(args[0]);
		WrapDistanceJoint* wrap_distance_joint = node::ObjectWrap::Unwrap<WrapDistanceJoint>(h_distance_joint);

		// delete box2d distance joint
		that->m_world.DestroyJoint(wrap_distance_joint->GetJoint());

		// reset javascript distance joint object
		wrap_distance_joint->ResetObject();

		break;
	}
	case e_pulleyJoint:
	{
		Local<Object> h_pulley_joint = Local<Object>::Cast(args[0]);
		WrapPulleyJoint* wrap_pulley_joint = node::ObjectWrap::Unwrap<WrapPulleyJoint>(h_pulley_joint);

		// delete box2d pulley joint
		that->m_world.DestroyJoint(wrap_pulley_joint->GetJoint());

		// reset javascript pulley joint object
		wrap_pulley_joint->ResetObject();

		break;
	}
	case e_mouseJoint:
	{
		Local<Object> h_mouse_joint = Local<Object>::Cast(args[0]);
		WrapMouseJoint* wrap_mouse_joint = node::ObjectWrap::Unwrap<WrapMouseJoint>(h_mouse_joint);

		// delete box2d mouse joint
		that->m_world.DestroyJoint(wrap_mouse_joint->GetJoint());

		// reset javascript mouse joint object
		wrap_mouse_joint->ResetObject();
		break;
	}
	case e_gearJoint:
	{
		Local<Object> h_gear_joint = Local<Object>::Cast(args[0]);
		WrapGearJoint* wrap_gear_joint = node::ObjectWrap::Unwrap<WrapGearJoint>(h_gear_joint);

		// delete box2d gear joint
		that->m_world.DestroyJoint(wrap_gear_joint->GetJoint());

		// reset javascript gear joint object
		wrap_gear_joint->ResetObject();

		break;
	}
	case e_wheelJoint:
	{
		Local<Object> h_wheel_joint = Local<Object>::Cast(args[0]);
		WrapWheelJoint* wrap_wheel_joint = node::ObjectWrap::Unwrap<WrapWheelJoint>(h_wheel_joint);

		// delete box2d wheel joint
		that->m_world.DestroyJoint(wrap_wheel_joint->GetJoint());

		// reset javascript wheel joint object
		wrap_wheel_joint->ResetObject();

		break;
	}
    case e_weldJoint:
	{
		Local<Object> h_weld_joint = Local<Object>::Cast(args[0]);
		WrapWeldJoint* wrap_weld_joint = node::ObjectWrap::Unwrap<WrapWeldJoint>(h_weld_joint);

		// delete box2d weld joint
		that->m_world.DestroyJoint(wrap_weld_joint->GetJoint());

		// reset javascript weld joint object
		wrap_weld_joint->ResetObject();
		break;
	}
	case e_frictionJoint:
	{
		Local<Object> h_friction_joint = Local<Object>::Cast(args[0]);
		WrapFrictionJoint* wrap_friction_joint = node::ObjectWrap::Unwrap<WrapFrictionJoint>(h_friction_joint);

		// delete box2d friction joint
		that->m_world.DestroyJoint(wrap_friction_joint->GetJoint());

		// reset javascript friction joint object
		wrap_friction_joint->ResetObject();

		break;
	}
	case e_ropeJoint:
	{
		Local<Object> h_rope_joint = Local<Object>::Cast(args[0]);
		WrapRopeJoint* wrap_rope_joint = node::ObjectWrap::Unwrap<WrapRopeJoint>(h_rope_joint);

		// delete box2d rope joint
		that->m_world.DestroyJoint(wrap_rope_joint->GetJoint());

		// reset javascript rope joint object
		wrap_rope_joint->ResetObject();

		break;
	}
	case e_motorJoint:
	{
		Local<Object> h_motor_joint = Local<Object>::Cast(args[0]);
		WrapMotorJoint* wrap_motor_joint = node::ObjectWrap::Unwrap<WrapMotorJoint>(h_motor_joint);

		// delete box2d motor joint
		that->m_world.DestroyJoint(wrap_motor_joint->GetJoint());

		// reset javascript motor joint object
		wrap_motor_joint->ResetObject();

		break;
	}
	default:
		break;
	}

	NanReturnUndefined();
})

#if 1 // B2_ENABLE_PARTICLE
CLASS_METHOD_IMPLEMENT(WrapWorld, Step,
{
	float32 timeStep = (float32) args[0]->NumberValue();
	int32 velocityIterations = (int32) args[1]->Int32Value();
	int32 positionIterations = (int32) args[2]->Int32Value();
	int32 particleIterations = (args.Length() > 3)?((int32) args[3]->Int32Value()):(that->m_world.CalculateReasonableParticleIterations(timeStep));
	that->m_world.Step(timeStep, velocityIterations, positionIterations, particleIterations);
	NanReturnUndefined();
})
#else
CLASS_METHOD_IMPLEMENT(WrapWorld, Step,
{
	float32 timeStep = (float32) args[0]->NumberValue();
	int32 velocityIterations = (int32) args[1]->Int32Value();
	int32 positionIterations = (int32) args[2]->Int32Value();
	that->m_world.Step(timeStep, velocityIterations, positionIterations);
	NanReturnUndefined();
})
#endif

CLASS_METHOD_IMPLEMENT(WrapWorld, ClearForces,
{
	that->m_world.ClearForces();
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, DrawDebugData,
{
	if (!that->m_draw.IsEmpty())
	{
//		Local<Object> h_draw = NanNew<Object>(that->m_draw);
//		WrapDraw* wrap_draw = node::ObjectWrap::Unwrap<WrapDraw>(h_draw);
//		that->m_world.SetDebugDraw(&wrap_draw->m_wrap_draw);
//		that->m_world.DrawDebugData();
//		that->m_world.SetDebugDraw(NULL);

		Local<Object> h_draw = NanNew<Object>(that->m_draw);
		Local<Function> h_method = Local<Function>::Cast(h_draw->Get(NanNew<String>("GetFlags")));
		uint32 flags = NanMakeCallback(h_draw, h_method, 0, NULL)->Uint32Value();
		that->m_wrap_draw.SetFlags(flags);
		that->m_world.DrawDebugData();
	}

	NanReturnUndefined();
})

class WrapQueryCallback : public b2QueryCallback
{
private:
	Handle<Function> m_callback;

public:
	WrapQueryCallback(Handle<Function> callback) : m_callback(callback) {}

	bool ReportFixture(b2Fixture* fixture)
	{
		NanScope();
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		Local<Object> h_fixture = NanObjectWrapHandle(wrap_fixture);
		Handle<Value> argv[] = { h_fixture };
		return NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv)->BooleanValue();
	}

	#if 1 // B2_ENABLE_PARTICLE

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

CLASS_METHOD_IMPLEMENT(WrapWorld, QueryAABB,
{
	Local<Function> callback = Local<Function>::Cast(args[0]);
	WrapAABB* wrap_aabb = node::ObjectWrap::Unwrap<WrapAABB>(Local<Object>::Cast(args[1]));
	WrapQueryCallback wrap_callback(callback);
	that->m_world.QueryAABB(&wrap_callback, wrap_aabb->GetAABB());
	NanReturnUndefined();
})

#if 1 // B2_ENABLE_PARTICLE
CLASS_METHOD_IMPLEMENT(WrapWorld, QueryShapeAABB,
{
	Local<Function> callback = Local<Function>::Cast(args[0]);
	WrapShape* wrap_shape = node::ObjectWrap::Unwrap<WrapShape>(Local<Object>::Cast(args[1]));
    WrapTransform* wrap_transform = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[2]));
	WrapQueryCallback wrap_callback(callback);
	that->m_world.QueryShapeAABB(&wrap_callback, wrap_shape->GetShape(), wrap_transform->GetTransform());
	NanReturnUndefined();
})
#endif

class WrapRayCastCallback : public b2RayCastCallback
{
private:
	Handle<Function> m_callback;

public:
	WrapRayCastCallback(Handle<Function> callback) : m_callback(callback) {}

	float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
	{
		NanScope();
		// get fixture internal data
		WrapFixture* wrap_fixture = WrapFixture::GetWrap(fixture);
		Local<Object> h_fixture = NanObjectWrapHandle(wrap_fixture);
		Local<Object> h_point = NanNew<Object>(WrapVec2::NewInstance(point));
		Local<Object> h_normal = NanNew<Object>(WrapVec2::NewInstance(normal));
		Local<Number> h_fraction = NanNew<Number>(fraction);
		Handle<Value> argv[] = { h_fixture, h_point, h_normal, h_fraction };
		return (float32) NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv)->NumberValue();
	}

	#if 1 // B2_ENABLE_PARTICLE

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

CLASS_METHOD_IMPLEMENT(WrapWorld, RayCast,
{
	Local<Function> callback = Local<Function>::Cast(args[0]);
	WrapVec2* point1 = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	WrapVec2* point2 = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	WrapRayCastCallback wrap_callback(callback);
	that->m_world.RayCast(&wrap_callback, point1->GetVec2(), point2->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetBodyList,
{
	b2Body* body = that->m_world.GetBodyList();
	if (body)
	{
		// get body internal data
		WrapBody* wrap_body = WrapBody::GetWrap(body);
		NanReturnValue(NanObjectWrapHandle(wrap_body));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetJointList,
{
	b2Joint* joint = that->m_world.GetJointList();
	if (joint)
	{
		// get joint internal data
		WrapJoint* wrap_joint = WrapJoint::GetWrap(joint);
		NanReturnValue(NanObjectWrapHandle(wrap_joint));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetContactList,
{
	b2Contact* contact = that->m_world.GetContactList();
	if (contact)
	{
		NanReturnValue(WrapContact::NewInstance(contact));
	}
	NanReturnNull();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetAllowSleeping,
{
	that->m_world.SetAllowSleeping(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetAllowSleeping,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.GetAllowSleeping()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetWarmStarting,
{
	that->m_world.SetWarmStarting(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetWarmStarting,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.GetWarmStarting()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetContinuousPhysics,
{
	that->m_world.SetContinuousPhysics(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetContinuousPhysics,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.GetContinuousPhysics()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetSubStepping,
{
	that->m_world.SetSubStepping(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetSubStepping,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.GetSubStepping()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetBodyCount,
{
	NanReturnValue(NanNew<Integer>(that->m_world.GetBodyCount()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetJointCount,
{
	NanReturnValue(NanNew<Integer>(that->m_world.GetJointCount()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetContactCount,
{
	NanReturnValue(NanNew<Integer>(that->m_world.GetContactCount()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetGravity,
{
	WrapVec2* wrap_gravity = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	that->m_world.SetGravity(wrap_gravity->GetVec2());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetGravity,
{
	NanReturnValue(WrapVec2::NewInstance(that->m_world.GetGravity()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, IsLocked,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.IsLocked()));
})

CLASS_METHOD_IMPLEMENT(WrapWorld, SetAutoClearForces,
{
	that->m_world.SetAutoClearForces(args[0]->BooleanValue());
	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, GetAutoClearForces,
{
	NanReturnValue(NanNew<Boolean>(that->m_world.GetAutoClearForces()));
})

#if 1 // B2_ENABLE_PARTICLE

CLASS_METHOD_IMPLEMENT(WrapWorld, CreateParticleSystem,
{
	WrapParticleSystemDef* wrap_psd = node::ObjectWrap::Unwrap<WrapParticleSystemDef>(Local<Object>::Cast(args[0]));

	// create box2d particle system
	b2ParticleSystem* system = that->m_world.CreateParticleSystem(&wrap_psd->UseParticleSystemDef());

	// create javascript particle system object
	Local<Object> h_particle_system = NanNew<Object>(WrapParticleSystem::NewInstance());
	WrapParticleSystem* wrap_particle_system = node::ObjectWrap::Unwrap<WrapParticleSystem>(h_particle_system);

	// set up javascript particle system object
	wrap_particle_system->SetupObject(args.This(), wrap_psd, system);

	NanReturnValue(h_particle_system);
})

CLASS_METHOD_IMPLEMENT(WrapWorld, DestroyParticleSystem,
{
	Local<Object> h_particle_system = Local<Object>::Cast(args[0]);
	WrapParticleSystem* wrap_particle_system = node::ObjectWrap::Unwrap<WrapParticleSystem>(h_particle_system);

	// delete box2d particle system
	that->m_world.DestroyParticleSystem(wrap_particle_system->GetParticleSystem());

	// reset javascript system object
	wrap_particle_system->ResetObject();

	NanReturnUndefined();
})

CLASS_METHOD_IMPLEMENT(WrapWorld, CalculateReasonableParticleIterations,
{
	float32 timeStep = (float32) args[0]->NumberValue();
	NanReturnValue(NanNew<Integer>(that->m_world.CalculateReasonableParticleIterations(timeStep)));
})

#endif

////

MODULE_EXPORT_IMPLEMENT(b2Distance)
{
	NanScope();
	WrapVec2* a = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* b = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	NanReturnValue(NanNew<Number>(b2Distance(a->GetVec2(), b->GetVec2())));
}

MODULE_EXPORT_IMPLEMENT(b2DistanceSquared)
{
	NanScope();
	WrapVec2* a = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* b = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	NanReturnValue(NanNew<Number>(b2DistanceSquared(a->GetVec2(), b->GetVec2())));
}

MODULE_EXPORT_IMPLEMENT(b2Dot_V2_V2)
{
	NanScope();
	WrapVec2* a = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* b = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	NanReturnValue(NanNew<Number>(b2Dot(a->GetVec2(), b->GetVec2())));
}

MODULE_EXPORT_IMPLEMENT(b2Add_V2_V2)
{
	NanScope();
	WrapVec2* a = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* b = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	WrapVec2* out = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	out->SetVec2(a->GetVec2() + b->GetVec2());
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2Sub_V2_V2)
{
	NanScope();
	WrapVec2* a = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[0]));
	WrapVec2* b = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	WrapVec2* out = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	out->SetVec2(a->GetVec2() - b->GetVec2());
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2Mul_X_V2)
{
	NanScope();
	WrapTransform* T = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[0]));
	WrapVec2* v = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	WrapVec2* out = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	out->SetVec2(b2Mul(T->GetTransform(), v->GetVec2()));
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2MulT_X_V2)
{
	NanScope();
	WrapTransform* T = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[0]));
	WrapVec2* v = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[1]));
	WrapVec2* out = node::ObjectWrap::Unwrap<WrapVec2>(Local<Object>::Cast(args[2]));
	out->SetVec2(b2MulT(T->GetTransform(), v->GetVec2()));
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2Mul_X_X)
{
	NanScope();
	WrapTransform* A = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[0]));
	WrapTransform* B = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[1]));
	WrapTransform* out = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[2]));
	out->SetTransform(b2Mul(A->GetTransform(), B->GetTransform()));
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2MulT_X_X)
{
	NanScope();
	WrapTransform* A = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[0]));
	WrapTransform* B = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[1]));
	WrapTransform* out = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[2]));
	out->SetTransform(b2MulT(A->GetTransform(), B->GetTransform()));
	NanReturnValue(args[2]);
}

MODULE_EXPORT_IMPLEMENT(b2GetPointStates)
{
	NanScope();
	Local<Array> wrap_state1 = Local<Array>::Cast(args[0]);
	Local<Array> wrap_state2 = Local<Array>::Cast(args[1]);
	WrapManifold* wrap_manifold1 = node::ObjectWrap::Unwrap<WrapManifold>(Local<Object>::Cast(args[2]));
	WrapManifold* wrap_manifold2 = node::ObjectWrap::Unwrap<WrapManifold>(Local<Object>::Cast(args[3]));
	b2PointState state1[b2_maxManifoldPoints];
	b2PointState state2[b2_maxManifoldPoints];
	b2GetPointStates(state1, state2, &wrap_manifold1->m_manifold, &wrap_manifold2->m_manifold);
	for (int i = 0; i < b2_maxManifoldPoints; ++i)
	{
		wrap_state1->Set(i, NanNew<Integer>(state1[i]));
		wrap_state2->Set(i, NanNew<Integer>(state2[i]));
	}
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(b2TestOverlap_AABB)
{
	NanReturnValue(NanNew<Boolean>(false));
}

MODULE_EXPORT_IMPLEMENT(b2TestOverlap_Shape)
{
	WrapShape* wrap_shapeA = node::ObjectWrap::Unwrap<WrapShape>(Local<Object>::Cast(args[0]));
    int32 indexA = (int32) args[1]->Int32Value();
	WrapShape* wrap_shapeB = node::ObjectWrap::Unwrap<WrapShape>(Local<Object>::Cast(args[2]));
    int32 indexB = (int32) args[3]->Int32Value();
    WrapTransform* wrap_transformA = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[4]));
    WrapTransform* wrap_transformB = node::ObjectWrap::Unwrap<WrapTransform>(Local<Object>::Cast(args[5]));
    const bool overlap = b2TestOverlap(&wrap_shapeA->GetShape(), indexA, &wrap_shapeB->GetShape(), indexB, wrap_transformA->GetTransform(), wrap_transformB->GetTransform());
	NanReturnValue(NanNew<Boolean>(overlap));
}

#if 1 // B2_ENABLE_PARTICLE

MODULE_EXPORT_IMPLEMENT(b2CalculateParticleIterations)
{
	NanScope();
	float32 gravity = (float32) args[0]->NumberValue();
	float32 radius = (float32) args[1]->NumberValue();
	float32 timeStep = (float32) args[2]->NumberValue();
	NanReturnValue(NanNew<Integer>(b2CalculateParticleIterations(gravity, radius, timeStep)));
}

#endif

////

#if NODE_VERSION_AT_LEAST(0,11,0)
void init(Handle<Object> exports, Handle<Value> module, Handle<Context> context)
#else
void init(Handle<Object> exports/*, Handle<Value> module*/)
#endif
{
	NanScope();

	MODULE_CONSTANT(exports, b2_maxFloat);
	MODULE_CONSTANT(exports, b2_epsilon);
	MODULE_CONSTANT(exports, b2_pi);
	MODULE_CONSTANT(exports, b2_maxManifoldPoints);
	MODULE_CONSTANT(exports, b2_maxPolygonVertices);
	MODULE_CONSTANT(exports, b2_aabbExtension);
	MODULE_CONSTANT(exports, b2_aabbMultiplier);
	MODULE_CONSTANT(exports, b2_linearSlop);
	MODULE_CONSTANT(exports, b2_angularSlop);
	MODULE_CONSTANT(exports, b2_polygonRadius);
	MODULE_CONSTANT(exports, b2_maxSubSteps);
	MODULE_CONSTANT(exports, b2_maxTOIContacts);
	MODULE_CONSTANT(exports, b2_velocityThreshold);
	MODULE_CONSTANT(exports, b2_maxLinearCorrection);
	MODULE_CONSTANT(exports, b2_maxAngularCorrection);
	MODULE_CONSTANT(exports, b2_maxTranslation);
	MODULE_CONSTANT(exports, b2_maxTranslationSquared);
	MODULE_CONSTANT(exports, b2_maxRotation);
	MODULE_CONSTANT(exports, b2_maxRotationSquared);
	MODULE_CONSTANT(exports, b2_baumgarte);
	MODULE_CONSTANT(exports, b2_toiBaugarte);
	MODULE_CONSTANT(exports, b2_timeToSleep);
	MODULE_CONSTANT(exports, b2_linearSleepTolerance);
	MODULE_CONSTANT(exports, b2_angularSleepTolerance);

	Local<Object> version = NanNew<Object>();
	exports->Set(NanNew<String>("b2_version"), version);
	version->Set(NanNew<String>("major"), NanNew<Integer>(b2_version.major));
	version->Set(NanNew<String>("minor"), NanNew<Integer>(b2_version.minor));
	version->Set(NanNew<String>("revision"), NanNew<Integer>(b2_version.revision));

	Local<Object> WrapShapeType = NanNew<Object>();
	exports->Set(NanNew<String>("b2ShapeType"), WrapShapeType);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_unknownShape,	-1);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_circleShape,		b2Shape::e_circle);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_edgeShape,		b2Shape::e_edge);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_polygonShape,		b2Shape::e_polygon);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_chainShape,		b2Shape::e_chain);
	MODULE_CONSTANT_VALUE(WrapShapeType, e_shapeTypeCount,	b2Shape::e_typeCount);

	Local<Object> WrapBodyType = NanNew<Object>();
	exports->Set(NanNew<String>("b2BodyType"), WrapBodyType);
	MODULE_CONSTANT(WrapBodyType, b2_staticBody);
	MODULE_CONSTANT(WrapBodyType, b2_kinematicBody);
	MODULE_CONSTANT(WrapBodyType, b2_dynamicBody);

	Local<Object> WrapJointType = NanNew<Object>();
	exports->Set(NanNew<String>("b2JointType"), WrapJointType);
	MODULE_CONSTANT(WrapJointType, e_unknownJoint);
	MODULE_CONSTANT(WrapJointType, e_revoluteJoint);
	MODULE_CONSTANT(WrapJointType, e_prismaticJoint);
	MODULE_CONSTANT(WrapJointType, e_distanceJoint);
	MODULE_CONSTANT(WrapJointType, e_pulleyJoint);
	MODULE_CONSTANT(WrapJointType, e_mouseJoint);
	MODULE_CONSTANT(WrapJointType, e_gearJoint);
	MODULE_CONSTANT(WrapJointType, e_wheelJoint);
	MODULE_CONSTANT(WrapJointType, e_weldJoint);
	MODULE_CONSTANT(WrapJointType, e_frictionJoint);
	MODULE_CONSTANT(WrapJointType, e_ropeJoint);
	MODULE_CONSTANT(WrapJointType, e_motorJoint);

	Local<Object> WrapLimitState = NanNew<Object>();
	exports->Set(NanNew<String>("b2LimitState"), WrapLimitState);
	MODULE_CONSTANT(WrapLimitState, e_inactiveLimit);
	MODULE_CONSTANT(WrapLimitState, e_atLowerLimit);
	MODULE_CONSTANT(WrapLimitState, e_atUpperLimit);
	MODULE_CONSTANT(WrapLimitState, e_equalLimits);

	Local<Object> WrapManifoldType = NanNew<Object>();
	exports->Set(NanNew<String>("b2ManifoldType"), WrapManifoldType);
	MODULE_CONSTANT_VALUE(WrapManifoldType, e_circles,	b2Manifold::e_circles);
	MODULE_CONSTANT_VALUE(WrapManifoldType, e_faceA,	b2Manifold::e_faceA);
	MODULE_CONSTANT_VALUE(WrapManifoldType, e_faceB,	b2Manifold::e_faceB);

	Local<Object> WrapPointState = NanNew<Object>();
	exports->Set(NanNew<String>("b2PointState"), WrapPointState);
	MODULE_CONSTANT(WrapPointState, b2_nullState);
	MODULE_CONSTANT(WrapPointState, b2_addState);
	MODULE_CONSTANT(WrapPointState, b2_persistState);
	MODULE_CONSTANT(WrapPointState, b2_removeState);

	Local<Object> WrapDrawFlags = NanNew<Object>();
	exports->Set(NanNew<String>("b2DrawFlags"), WrapDrawFlags);
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_shapeBit,		b2Draw::e_shapeBit);
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_jointBit,		b2Draw::e_jointBit);
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_aabbBit,			b2Draw::e_aabbBit);
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_pairBit,			b2Draw::e_pairBit);
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_centerOfMassBit,	b2Draw::e_centerOfMassBit);
	#if 1 // B2_ENABLE_PARTICLE
	MODULE_CONSTANT_VALUE(WrapDrawFlags, e_particleBit,		b2Draw::e_particleBit);
	#endif

	#if 1 // B2_ENABLE_PARTICLE

	Local<Object> WrapParticleFlag = NanNew<Object>();
	exports->Set(NanNew<String>("b2ParticleFlag"), WrapParticleFlag);
	MODULE_CONSTANT(WrapParticleFlag, b2_waterParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_zombieParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_wallParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_springParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_elasticParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_viscousParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_powderParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_tensileParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_colorMixingParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_destructionListenerParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_barrierParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_staticPressureParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_reactiveParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_repulsiveParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_fixtureContactListenerParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_particleContactListenerParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_fixtureContactFilterParticle);
	MODULE_CONSTANT(WrapParticleFlag, b2_particleContactFilterParticle);

	Local<Object> WrapParticleGroupFlag = NanNew<Object>();
	exports->Set(NanNew<String>("b2ParticleGroupFlag"), WrapParticleGroupFlag);
	MODULE_CONSTANT(WrapParticleGroupFlag, b2_solidParticleGroup);
	MODULE_CONSTANT(WrapParticleGroupFlag, b2_rigidParticleGroup);
	MODULE_CONSTANT(WrapParticleGroupFlag, b2_particleGroupCanBeEmpty);
	MODULE_CONSTANT(WrapParticleGroupFlag, b2_particleGroupWillBeDestroyed);
	MODULE_CONSTANT(WrapParticleGroupFlag, b2_particleGroupNeedsUpdateDepth);

	#endif

	WrapVec2::Init(exports);
	WrapRot::Init(exports);
	WrapTransform::Init(exports);
	WrapAABB::Init(exports);
	WrapMassData::Init(exports);
	WrapShape::Init(exports);
	WrapCircleShape::Init(exports);
	WrapEdgeShape::Init(exports);
	WrapPolygonShape::Init(exports);
	WrapChainShape::Init(exports);
	WrapFilter::Init(exports);
	WrapFixtureDef::Init(exports);
	WrapFixture::Init(exports);
	WrapBodyDef::Init(exports);
	WrapBody::Init(exports);
	WrapJointDef::Init(exports);
	WrapJoint::Init(exports);
	WrapRevoluteJointDef::Init(exports);
	WrapRevoluteJoint::Init(exports);
	WrapPrismaticJointDef::Init(exports);
	WrapPrismaticJoint::Init(exports);
	WrapDistanceJointDef::Init(exports);
	WrapDistanceJoint::Init(exports);
	WrapPulleyJointDef::Init(exports);
	WrapPulleyJoint::Init(exports);
	WrapMouseJointDef::Init(exports);
	WrapMouseJoint::Init(exports);
	WrapGearJointDef::Init(exports);
	WrapGearJoint::Init(exports);
	WrapWheelJointDef::Init(exports);
	WrapWheelJoint::Init(exports);
	WrapWeldJointDef::Init(exports);
	WrapWeldJoint::Init(exports);
	WrapFrictionJointDef::Init(exports);
	WrapFrictionJoint::Init(exports);
	WrapRopeJointDef::Init(exports);
	WrapRopeJoint::Init(exports);
	WrapMotorJointDef::Init(exports);
	WrapMotorJoint::Init(exports);
	WrapContactID::Init(exports);
	WrapManifoldPoint::Init(exports);
	WrapManifold::Init(exports);
	WrapWorldManifold::Init(exports);
	WrapContact::Init(exports);
	WrapContactImpulse::Init(exports);
	WrapColor::Init(exports);
	#if 0
	WrapDraw::Init(exports);
	#endif
	#if 1 // B2_ENABLE_PARTICLE
	WrapParticleColor::Init(exports);
	WrapParticleDef::Init(exports);
	WrapParticleHandle::Init(exports);
	WrapParticleGroupDef::Init(exports);
	WrapParticleGroup::Init(exports);
	WrapParticleSystemDef::Init(exports);
	WrapParticleSystem::Init(exports);
	#endif
	WrapWorld::Init(exports);

	MODULE_EXPORT_APPLY(exports, b2Distance);
	MODULE_EXPORT_APPLY(exports, b2DistanceSquared);
	MODULE_EXPORT_APPLY(exports, b2Dot_V2_V2);
	MODULE_EXPORT_APPLY(exports, b2Add_V2_V2);
	MODULE_EXPORT_APPLY(exports, b2Sub_V2_V2);
	MODULE_EXPORT_APPLY(exports, b2Mul_X_V2);
	MODULE_EXPORT_APPLY(exports, b2MulT_X_V2);
	MODULE_EXPORT_APPLY(exports, b2Mul_X_X);
	MODULE_EXPORT_APPLY(exports, b2MulT_X_X);

	MODULE_EXPORT_APPLY(exports, b2GetPointStates);

	MODULE_EXPORT_APPLY(exports, b2TestOverlap_AABB);
	MODULE_EXPORT_APPLY(exports, b2TestOverlap_Shape);

	#if 1 // B2_ENABLE_PARTICLE
	MODULE_EXPORT_APPLY(exports, b2CalculateParticleIterations);
	#endif
}

////

} // namespace node_box2d

#if NODE_VERSION_AT_LEAST(0,11,0)
NODE_MODULE_CONTEXT_AWARE_BUILTIN(node_box2d, node_box2d::init);
#else
NODE_MODULE(node_box2d, node_box2d::init);
#endif

