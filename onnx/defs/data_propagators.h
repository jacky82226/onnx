#include "onnx/defs/shape_inference.h"

namespace ONNX_NAMESPACE {

inline void PropagateShapeDataFromInputToOutput(DataPropagationContext& ctx, int idx) {
  // propogate input data
  const auto input_data = ctx.getGeneratedShapeData(idx);
  if (input_data != nullptr) {
    TensorShapeProto tp;
    tp.CopyFrom(*input_data);
    ctx.addGeneratedShapeData(0, std::move(tp));
  }
}

// Data propagation function for Shape op
// Propagates input shape to output shape
inline void ShapeOpDataPropagator(DataPropagationContext& ctx) {
  if (!hasNInputShapes(ctx, 1)) {
    return;
  }

  if (ctx.getInputType(0)->tensor_type().has_shape()) {
    auto input_shape = ctx.getInputType(0)->tensor_type().shape();

    TensorShapeProto tsp;
    tsp.CopyFrom(input_shape);
    ctx.addGeneratedShapeData(0, std::move(tsp));
  }
}

}