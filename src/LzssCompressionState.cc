#include "LzssCompressionState.h"

LzssCompressionState::LzssCompressionState()
{
	for (int i = 0; i < 4096; i ++)
		text[i] = 0;
}
