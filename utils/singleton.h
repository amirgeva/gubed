#pragma once

template<typename T>
class Singleton
{
public:
	static T& Instance()
	{
		static T instance; // Guaranteed to be destroyed, instantiated on first use
		return instance;
	}
};
