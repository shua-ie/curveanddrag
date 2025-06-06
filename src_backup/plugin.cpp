#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add all modules here
	p->addModel(modelCurveAndDrag);
}
