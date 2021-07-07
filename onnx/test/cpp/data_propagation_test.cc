/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include "gtest/gtest.h"
#include "onnx/checker.h"
#include "onnx/defs/schema.h"
#include "onnx/defs/shape_inference.h"
#include "onnx/onnx_pb.h"

#include "onnx/shape_inference/implementation.h"

using namespace ONNX_NAMESPACE::shape_inference;

namespace ONNX_NAMESPACE {

namespace Test {


inline bool CompareShape(const TensorShapeProto* A, const TensorShapeProto* B) {
    if (A == nullptr ||  B == nullptr) {
        fail_check("The compared shapes should not be nullptr.");
        return false;
    }
    if (A->dim_size() != B->dim_size()) {
        fail_check("The compared sizes of dim are different");
        return false;
    }
    for (int i = 0; i < A->dim_size() ; ++i) {
        if (A->dim(i).has_dim_value() != B->dim(i).has_dim_value() || 
                A->dim(i).dim_value() != B->dim(i).dim_value()) {
            fail_check("The compared dim values are different");
            return false;
        }
    }
    return true;
}

void TestPropagateShapeDataFromInputToOutput(std::string opsetName) {
  auto* schemaRegistry = OpSchemaRegistry::Instance();
  GraphProto subgraph;
  // simple tensor with shape info
  TypeProto simpleTensor;
  int domain_version = 15;
  simpleTensor.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);


  simpleTensor.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(7);
  simpleTensor.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(4);
  simpleTensor.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(1);
  const auto simpleShape = simpleTensor.tensor_type().shape();

  std::string shape_input_name = "shape_input";
  std::string shape_output_name = "shape_output";
  std::string output_name = "output";

  // Constructs Shape node
  NodeProto shape_node;
  shape_node.set_name("shape");
  shape_node.set_domain(ONNX_DOMAIN);
  shape_node.set_op_type("Shape");
  shape_node.add_input(shape_input_name);
  shape_node.add_output(shape_output_name);

  // Constructs final node

  NodeProto final_node;
  final_node.set_name(opsetName);
  final_node.set_op_type(opsetName);

  final_node.set_domain(ONNX_DOMAIN);
  final_node.add_input(shape_output_name);
  final_node.add_output(output_name);

  if (opsetName == "Unsqueeze") {
     // add a dummy input for axes
    std::string axes_name = "axes";
  }    
  
  // Constructs graph
  ValueInfoProto graph_input;  
  graph_input.set_name(shape_input_name);
  *graph_input.mutable_type() = simpleTensor;
  *subgraph.add_input() = graph_input;
  *subgraph.add_node() = shape_node;
  *subgraph.add_node() = final_node;
  std::unordered_map<std::string, int> opset_imports;
  opset_imports[ONNX_DOMAIN] = domain_version;

  const std::unordered_map<std::string, TypeProto*> outer_scope_value_types;
  SymbolTableImpl symbolTable;
  symbolTable.addFromGraph(subgraph);
  GraphInferenceContext graphInfCtx(outer_scope_value_types, opset_imports, symbolTable);

  GraphInferencerImpl graphInferencer(subgraph, graphInfCtx);

  std::vector<const TypeProto*> subgraphInputTypes = {&simpleTensor};

  std::unordered_map<std::string, TypeProto*> valueTypesByName;
  valueTypesByName[shape_input_name] = &simpleTensor;

  std::unordered_map<std::string, TensorShapeProto> generatedShapeDataByName;

  const TensorShapeProto* propagatedShape;
  int i = 0;
  for (auto n: subgraph.node()) {
    DataPropagationContextImpl dataPropagationCtx(
        n, valueTypesByName, {}, generatedShapeDataByName, &graphInfCtx);

    const auto schema = schemaRegistry->GetSchema(n.op_type(), domain_version, n.domain());
    //if (i == 1) break;
    EXPECT_TRUE(schema->has_data_propagation_function());
    schema->GetDataPropagationFunction()(dataPropagationCtx);
    propagatedShape = dataPropagationCtx.getGeneratedShapeDataFromName(output_name);
    ++i;
  }
  //EXPECT_TRUE(CompareShape(propagatedShape, &simpleShape));
}

TEST(DataPropagationImplTest, ShapeTest) {
  auto* schemaRegistry = OpSchemaRegistry::Instance();
  GraphProto subgraph;
  // simple tensor with shape info
  TypeProto simpleTensor;
  int domain_version = 15;
  simpleTensor.mutable_tensor_type()->set_elem_type(TensorProto_DataType_FLOAT);


  simpleTensor.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(2);
  simpleTensor.mutable_tensor_type()->mutable_shape()->add_dim()->set_dim_value(5);
  const auto simpleShape = simpleTensor.tensor_type().shape();

  std::string input_name = "shape_input";
  std::string output_name = "shape_output";

  // Constructs Shape node
  NodeProto shape_node;
  shape_node.set_name("shape");
  shape_node.set_domain(ONNX_DOMAIN);
  shape_node.set_op_type("Shape");
  shape_node.add_input(input_name);
  shape_node.add_output(output_name);
  
  // Constructs graph
  ValueInfoProto graph_input;  
  graph_input.set_name(input_name);
  *graph_input.mutable_type() = simpleTensor;
  *subgraph.add_input() = graph_input;
  *subgraph.add_node() = shape_node;
  std::unordered_map<std::string, int> opset_imports;
  opset_imports[ONNX_DOMAIN] = domain_version;

  const std::unordered_map<std::string, TypeProto*> outer_scope_value_types;
  SymbolTableImpl symbolTable;
  symbolTable.addFromGraph(subgraph);
  GraphInferenceContext graphInfCtx(outer_scope_value_types, opset_imports, symbolTable);

  GraphInferencerImpl graphInferencer(subgraph, graphInfCtx);

  std::vector<const TypeProto*> subgraphInputTypes = {&simpleTensor};

  std::unordered_map<std::string, TypeProto*> valueTypesByName;
  valueTypesByName[input_name] = &simpleTensor;

  std::unordered_map<std::string, TensorShapeProto> generatedShapeDataByName;
  DataPropagationContextImpl dataPropagationCtx(
    shape_node, valueTypesByName, {}, generatedShapeDataByName, &graphInfCtx);

  const auto schema = schemaRegistry->GetSchema(shape_node.op_type(), domain_version, shape_node.domain());
  EXPECT_TRUE(schema->has_data_propagation_function());

  schema->GetDataPropagationFunction()(dataPropagationCtx);
  const auto* propagatedShape = dataPropagationCtx.getGeneratedShapeDataFromName(output_name);

  EXPECT_TRUE(CompareShape(propagatedShape, &simpleShape));
}

TEST(DataPropagationImplTest, SqueezeTest) {
  TestPropagateShapeDataFromInputToOutput("Squeeze");
}

TEST(DataPropagationImplTest, UnsqueezeTest) {
  TestPropagateShapeDataFromInputToOutput("Unsqueeze");
}

} // namespace Test
} // namespace ONNX_NAMESPACE
