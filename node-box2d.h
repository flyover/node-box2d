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

#ifndef _NODE_BOX2D_H_
#define _NODE_BOX2D_H_

//#include <v8.h>
//#include <node.h>
#include <nan.h>

namespace node_box2d {

#if NODE_VERSION_AT_LEAST(0,11,0)
void init(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> module, v8::Handle<v8::Context> context);
#else
void init(v8::Handle<v8::Object> exports/*, v8::Handle<v8::Value> module*/);
#endif

} // namespace node_box2d

#endif // _NODE_BOX2D_H_

