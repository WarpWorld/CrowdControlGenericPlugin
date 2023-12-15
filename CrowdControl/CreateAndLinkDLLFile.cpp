#pragma once

#include "string.h"
#include "CreateAndLinkDLLFile.h"
#include "CrowdControlRunner.hpp"

//Exported method that invertes a given boolean.
bool getInvertedBool(bool boolState)
{
	return bool(!boolState);
}

//Exported method that iterates a given int value.
int getIntPlusPlus(int lastInt)
{
	return int(++lastInt);
}

//Exported method that calculates the are of a circle by a given radius.
float getCircleArea(float radius)
{
	return float(3.1416f * (radius * radius));
}


void AddNewBasicEffect(char* name, char* desc, int price, int retries, float retryDelay, float pendingDelay, bool sellable, bool visible, bool nonPoolable, int morality, int orderliness, char** categoriesArray)
{
	CrowdControlRunner::AddBasicEffect(name, desc, price, retries, retryDelay, pendingDelay, sellable, visible, nonPoolable, morality, orderliness, categoriesArray);
}

//Exported method that adds a vector4 to a given vector4 and returns the sum.
float *getVector4(float x, float y, float z, float w)
{
	float* modifiedVector4 = new float[4];

	modifiedVector4[0] = x + 1.0F;
	modifiedVector4[1] = y + 2.0F;
	modifiedVector4[2] = z + 3.0F;
	modifiedVector4[3] = w + 4.0F;

	return (float*)modifiedVector4;
}
