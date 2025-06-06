#include "CurveAndDrag.hpp"
#include "CurveAndDragWidget.hpp"

// Register the model with the plugin
Model* modelCurveAndDrag = createModel<CurveAndDrag::CurveAndDragModule, CurveAndDrag::CurveAndDragWidget>("CurveAndDrag");
