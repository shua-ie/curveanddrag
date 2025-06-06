#include "plugin.hpp"

plugin::Plugin* pluginInstance;

// Declare the module model (defined in CurveAndDrag.cpp after widget include)
extern plugin::Model* modelCurveAndDrag;

void init(plugin::Plugin* p) {
	pluginInstance = p;

	// Add all modules here
	p->addModel(modelCurveAndDrag);
}
