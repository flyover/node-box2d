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

var box2d = null;
try { box2d = box2d || require('./build/Release/node-box2d.node'); } catch (err) {}
try { box2d = box2d || process._linkedBinding('node_box2d'); } catch (err) {}
try { box2d = box2d || process.binding('node_box2d'); } catch (err) {}
module.exports = box2d;

box2d.b2Log = box2d.b2Log || console.log;

box2d.b2Clamp = function (n, lo, hi)
{
	return Math.min(Math.max(n, lo), hi);
}

box2d.b2Vec2.prototype.toString = function ()
{
	return "b2Vec2(" + this.x.toString() + "," + this.y.toString() + ")";
}

box2d.b2Vec2.prototype.toJSON = function ()
{
	return "{" + "x:" + this.x.toString() + "," + "y:" + this.y.toString() + "}";
}

box2d.b2Vec2.prototype.Clone = function ()
{
	return new box2d.b2Vec2(this.x, this.y);
}

box2d.b2Vec2.prototype.SelfRotate = function (c, s)
{
	var x = this.x, y = this.y;
	this.x = c * x - s * y;
	this.y = s * x + c * y;
	return this;
}

box2d.b2Vec2.prototype.SelfRotateAngle = function (a)
{
	return this.SelfRotate(Math.cos(a), Math.sin(a));
}

box2d.b2Vec2.s_t0 = new box2d.b2Vec2();
box2d.b2Vec2.s_t1 = new box2d.b2Vec2();
box2d.b2Vec2.s_t2 = new box2d.b2Vec2();
box2d.b2Vec2.s_t3 = new box2d.b2Vec2();

box2d.b2Rot.prototype.SetAngle = box2d.b2Rot.prototype.Set;

box2d.b2Body.prototype.SetTransform_X_Y_A = function (x, y, radians)
{
	this.SetTransform(new box2d.b2Vec2(x, y), radians);
}

box2d.b2Body.prototype.SetTransform_V2_A = function (poaition, radians)
{
	this.SetTransform(position, radians);
}

box2d.b2Body.prototype.SetTransform_X = function (xf)
{
	this.SetTransform(xf.p, xf.q.GetAngle());
}

box2d.b2Body.prototype.SetPosition = function (pos)
{
	this.SetTransform(pos, this.GetAngle());
}

box2d.b2Body.prototype._CreateFixture = box2d.b2Body.prototype.CreateFixture;

box2d.b2Body.prototype.CreateFixture = function (a0, a1)
{
	if (arguments.length === 2)
	{
		var fd = new box2d.b2FixtureDef();
		fd.shape = a0;
		fd.density = a1;
		return this._CreateFixture(fd);
	}
	else
	{
		return this._CreateFixture(a0);
	}
}

box2d.b2DestructionListener = function () {}
box2d.b2DestructionListener.prototype.SayGoodbyeJoint = function (joint) {}
box2d.b2DestructionListener.prototype.SayGoodbyeFixture = function (fixture) {}

box2d.b2ContactFilter = function () {}
box2d.b2ContactFilter.prototype.ShouldCollide = function (fixtureA, fixtureB)
{
	var bodyA = fixtureA.GetBody();
	var bodyB = fixtureB.GetBody();

	// At least one body should be dynamic or kinematic.
	if (bodyB.GetType() === box2d.b2BodyType.b2_staticBody && bodyA.GetType() === box2d.b2BodyType.b2_staticBody)
	{
		return false;
	}

	// Does a joint prevent collision?
	if (bodyB.ShouldCollideConnected(bodyA) === false)
	{
		return false;
	}

	var filterA = fixtureA.GetFilterData();
	var filterB = fixtureB.GetFilterData();

	if (filterA.groupIndex === filterB.groupIndex && filterA.groupIndex !== 0)
	{
		return (filterA.groupIndex > 0);
	}

	var collide = (((filterA.maskBits & filterB.categoryBits) !== 0) && ((filterA.categoryBits & filterB.maskBits) !== 0));
	return collide;
}

box2d.b2ContactListener = function () {}
box2d.b2ContactListener.prototype.BeginContact = function (contact) {}
box2d.b2ContactListener.prototype.EndContact = function (contact) {}
box2d.b2ContactListener.prototype.PreSolve = function (contact, oldManifold) {}
box2d.b2ContactListener.prototype.PostSolve = function (contact, impulse) {}

box2d.b2Draw = function () {}
box2d.b2Draw.prototype.m_flags = 0;
box2d.b2Draw.prototype.SetFlags = function (flags) { this.m_flags = flags; }
box2d.b2Draw.prototype.GetFlags = function () { return this.m_flags; }
box2d.b2Draw.prototype.AppendFlags = function (flags) { this.m_flags |= flags; }
box2d.b2Draw.prototype.ClearFlags = function (flags) { this.m_flags &= ~flags; }
box2d.b2Draw.prototype.DrawPolygon = function (vertices, vertexCount, color) {}
box2d.b2Draw.prototype.DrawSolidPolygon = function (vertices, vertexCount, color) {}
box2d.b2Draw.prototype.DrawCircle = function (center, radius, color) {}
box2d.b2Draw.prototype.DrawSolidCircle = function (center, radius, axis, color) {}
//#if B2_ENABLE_PARTICLE
box2d.b2Draw.prototype.DrawParticles = function (centers, radius, colors, count) {}
//#endif
box2d.b2Draw.prototype.DrawSegment = function (p1, p2, color) {}
box2d.b2Draw.prototype.DrawTransform = function (xf) {}

