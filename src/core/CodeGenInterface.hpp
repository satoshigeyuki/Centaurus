#pragma once

#include "BaseListener.hpp"
#include "Identifier.hpp"

namespace Centaurus
{
class IParser
{
public:
	IParser() {}
	virtual ~IParser() {}
	virtual const void *operator()(BaseListener *context, const void *input) = 0;
};

typedef void *(*ChaserFunc)(void *context, const void *input);

class IChaser
{
public:
	IChaser() {}
	virtual ~IChaser() {}
	virtual ChaserFunc operator[](const Identifier& id) const = 0;
	virtual ChaserFunc operator[](int id) const = 0;
};
}