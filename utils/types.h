#pragma once

#include <algorithm>

template<typename T>
struct TPoint
{
	T x, y;

	explicit TPoint(T x_=0, T y_=0) : x(x_), y(y_) {}
	
	bool operator==(const TPoint& other) const
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const TPoint& other) const
	{
		return !(*this == other);
	}

	TPoint operator+(const TPoint& other) const
	{
		return TPoint(x + other.x, y + other.y);
	}

	TPoint operator-(const TPoint& other) const
	{
		return TPoint(x - other.x, y - other.y);
	}

	TPoint operator*(T scalar) const
	{
		return TPoint(x * scalar, y * scalar);
	}

	TPoint operator/(T scalar) const
	{
		return TPoint(x / scalar, y / scalar);
	}

	TPoint& operator+=(const TPoint& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
};

using Point = TPoint<int>;
using Pointd = TPoint<double>;

template<typename T>
struct TRect
{
	T x, y, width, height;

	explicit TRect(T x_=0, T y_=0, T width_=0, T height_=0)
		: x(x_), y(y_), width(width_), height(height_)
	{}

	T bottom() const
	{
		return y + height;
	}

	T right() const
	{
		return x + width;
	}

	bool operator==(const TRect& other) const
	{
		return x == other.x && y == other.y && width == other.width && height == other.height;
	}

	bool operator!=(const TRect& other) const
	{
		return !(*this == other);
	}

	void inflate(T dx, T dy)
	{
		x -= dx;
		y -= dy;
		width += 2 * dx;
		height += 2 * dy;
	}

	bool contains(const TPoint<T>& point) const
	{
		return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
	}

	bool intersects(const TRect& other) const
	{
		return !(x + width <= other.x || x >= other.x + other.width ||
				 y + height <= other.y || y >= other.y + other.height);
	}

	TRect intersection(const TRect& other) const
	{
		if (!intersects(other)) return TRect();
		return TRect(
			std::max(x, other.x),
			std::max(y, other.y),
			std::min(x + width, other.x + other.width) - std::max(x, other.x),
			std::min(y + height, other.y + other.height) - std::max(y, other.y)
		);
	}

	TRect union_rect(const TRect& other) const
	{
		return TRect(
			std::min(x, other.x),
			std::min(y, other.y),
			std::max(x + width, other.x + other.width) - std::min(x, other.x),
			std::max(y + height, other.y + other.height) - std::min(y, other.y)
		);
	}
};

using Rect = TRect<int>;
