#include "Template.h"
#include "Value.h"

bool TemplateInstantiation::HasTemplatedType() const
{
	for (uint32 i = 0; i < args.size(); i++)
		if (args[i].value == (uint32)ValueType::TEMPLATE_TYPE)
			return true;

	return false;
}
