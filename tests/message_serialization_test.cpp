
#include "../include/mons.hpp"

#include <random>
#include <cstdlib>

template <typename T>
void RandomVal(T &v, T minV, T maxV)
{
  int ran = rand();
  v = minV + (maxV - minV) * ((double)ran / RAND_MAX);
}

template <typename T>
void RandomVal(T &v)
{
  T minV = std::numeric_limits<T>::min();
  T maxV = std::numeric_limits<T>::max();
  RandomVal(v, minV, maxV);
}

template <typename eT>
void RandTensor(arma::Row<eT> &tensor)
{
  int r;
  RandomVal(r, 5, 50);
  tensor.resize(r);
  tensor.randn();
}

template <typename eT>
void RandTensor(arma::Mat<eT> &tensor)
{
  int r, c;
  RandomVal(r, 5, 50);
  RandomVal(c, 5, 50);
  tensor.resize(r, c);
  tensor.randn();
}

template <typename eT>
void RandTensor(arma::Cube<eT> &tensor)
{
  int r, c, s;
  RandomVal(r, 5, 50);
  RandomVal(c, 5, 50);
  RandomVal(s, 5, 50);
  tensor.resize(r, c, s);
  tensor.randn();
}

template <typename T>
void ValidateArma(T &tensor1, T &tensor2, double eps)
{
  using eT = typename T::elem_type;
  T diff = tensor1 - tensor2;
  for (size_t i = 0; i < tensor1.n_elem; i++)
  {
    eT d = (diff[i] >= 0) ? diff[i] : -diff[i];
    assert(d < eps);
  }
}

// ============================== Write structs ==============================
void WriteData(mons::Message::Base::BaseDataStruct &data)
{
  RandomVal(data.id);
  RandomVal(data.messageType);
  RandomVal(data.reciever);
  RandomVal(data.responseTo);
  RandomVal(data.sender);
}

template <typename T>
void WriteData(mons::Message::Tensor::TensorDataStruct &data)
{
  T tensor;
  RandTensor(tensor);
  mons::Message::Tensor::SetTensor(tensor, data);
}

void WriteData(mons::Message::EvaluateWithGradient::EvaluateWithGradientDataStruct &data)
{
  RandomVal(data.batchSize);
  RandomVal(data.begin);
}

void WriteData(mons::Message::Gradient::GradientDataStruct &data)
{
  RandomVal(data.objective);
}

void WriteData(mons::Message::OperationStatus::OperationStatusStruct &data)
{
  RandomVal(data.status);
}

void WriteData(mons::Message::Cereal::CerealDataStruct &data)
{
  arma::mat obj;
  RandTensor(obj);
  mons::Message::Cereal::Cerealize(obj, data);
}

void WriteData(mons::Message::UpdateFunction::UpdateFunctionDataStruct &data)
{
  RandomVal(data.numDims, (uint16_t)5, (uint16_t)50);
  data.inputDimensions.resize(data.numDims);
  for (size_t i = 0; i < data.numDims; i++)
  {
    RandomVal(data.inputDimensions[i], (uint16_t)5, (uint16_t)50);
  }
}

void WriteData(mons::Message::ConnectInfo::ConnectInfoDataStruct &data)
{
  RandomVal(data.config);
}

#define VALIDATE(member) assert(data1.member == data2.member)
// ============================== Validate structs ==============================
void ValidateData(mons::Message::Base::BaseDataStruct &data1,
                  mons::Message::Base::BaseDataStruct &data2)
{
  VALIDATE(id);
  // These are set by `Serialize` so it won't match
  // VALIDATE(messageType);
  VALIDATE(reciever);
  VALIDATE(responseTo);
  VALIDATE(sender);
}

template <typename T>
void ValidateData(mons::Message::Tensor::TensorDataStruct &data1,
                  mons::Message::Tensor::TensorDataStruct &data2)
{
  T tensor1, tensor2;
  mons::Message::Tensor::GetTensor(tensor1, data1);
  mons::Message::Tensor::GetTensor(tensor2, data2);
  ValidateArma(tensor1, tensor2, 1e-3);
}

void ValidateData(mons::Message::EvaluateWithGradient::EvaluateWithGradientDataStruct &data1,
                  mons::Message::EvaluateWithGradient::EvaluateWithGradientDataStruct &data2)
{
  VALIDATE(batchSize);
  VALIDATE(begin);
}

void ValidateData(mons::Message::Gradient::GradientDataStruct &data1,
                  mons::Message::Gradient::GradientDataStruct &data2)
{
  VALIDATE(objective);
}

void ValidateData(mons::Message::OperationStatus::OperationStatusStruct &data1,
                  mons::Message::OperationStatus::OperationStatusStruct &data2)
{
  VALIDATE(status);
}

void ValidateData(mons::Message::Cereal::CerealDataStruct &data1,
                  mons::Message::Cereal::CerealDataStruct &data2)
{
  arma::mat obj1, obj2;
  mons::Message::Cereal::Decerealize(obj1, data1);
  mons::Message::Cereal::Decerealize(obj2, data2);
  ValidateArma(obj1, obj2, 1e-3);
}

void ValidateData(mons::Message::UpdateFunction::UpdateFunctionDataStruct &data1,
                  mons::Message::UpdateFunction::UpdateFunctionDataStruct &data2)
{
  VALIDATE(numDims);
  for (size_t i = 0; i < data1.numDims; i++)
  {
    VALIDATE(inputDimensions[i]);
  }
}

void ValidateData(mons::Message::ConnectInfo::ConnectInfoDataStruct &data1,
                  mons::Message::ConnectInfo::ConnectInfoDataStruct &data2)
{
  VALIDATE(config);
}

// Matches the deserializing method used in `Network`
// Returns bytes read
template <typename T>
size_t Deseralize(T &msg, mons::MessageBuffer buf)
{
  // Reset bytes written/read after serializing
  buf.processed = 0;
  // Read header
  size_t read = 0;
  mons::Message::Base::SerializeHeader(msg.BaseData, buf, false);
  // Read the rest
  read += msg.Deserialize(buf);
  return read;
}

#define VALIDATE_STRUCT(struct) ValidateData(i.struct, o.struct)
#define VALIDATE_STRUCT_TEMPLATE(struct, ...) ValidateData<__VA_ARGS__>(i.struct, o.struct)
// ============================== Test cases ==============================
void TestCaseUpdatePredictors()
{
  mons::Message::UpdatePredictors i, o;
  WriteData(i.BaseData);
  WriteData<MONS_PREDICTOR_TYPE>(i.TensorData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_PREDICTOR_TYPE);
}

void TestCaseUpdateResponses()
{
  mons::Message::UpdateResponses i, o;
  WriteData(i.BaseData);
  WriteData<MONS_RESPONSE_TYPE>(i.TensorData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_RESPONSE_TYPE);
}

void TestCaseUpdateWeights()
{
  mons::Message::UpdateWeights i, o;
  WriteData(i.BaseData);
  WriteData<MONS_WEIGHT_TYPE>(i.TensorData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_WEIGHT_TYPE);
}

void TestCaseShuffle()
{
  mons::Message::Shuffle i, o;
  WriteData(i.BaseData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
}

void TestCaseEvaluateWithGradient()
{
  mons::Message::EvaluateWithGradient i, o;
  WriteData(i.BaseData);
  WriteData<MONS_MAT_TYPE>(i.TensorData);
  WriteData(i.EvaluateWithGradientData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_MAT_TYPE);
  VALIDATE_STRUCT(EvaluateWithGradientData);
}

void TestCaseGradient()
{
  mons::Message::Gradient i, o;
  WriteData(i.BaseData);
  WriteData<MONS_MAT_TYPE>(i.TensorData);
  WriteData(i.GradientData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_MAT_TYPE);
  VALIDATE_STRUCT(GradientData);
}

void TestCaseOperationStatus()
{
  mons::Message::OperationStatus i, o;
  WriteData(i.BaseData);
  WriteData(i.OperationStatusData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT(OperationStatusData);
}

void TestCaseUpdateFunction()
{
  mons::Message::UpdateFunction i, o;
  WriteData(i.BaseData);
  WriteData(i.CerealData);
  WriteData(i.UpdateFunctionData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT(CerealData);
  VALIDATE_STRUCT(UpdateFunctionData);
}

void TestCaseUpdateParameters()
{
  mons::Message::UpdateParameters i, o;
  WriteData(i.BaseData);
  WriteData<MONS_MAT_TYPE>(i.TensorData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT_TEMPLATE(TensorData, MONS_MAT_TYPE);
}

void TestCaseConnectInfo()
{
  mons::Message::ConnectInfo i, o;
  WriteData(i.BaseData);
  WriteData(i.ConnectInfoData);
  mons::MessageBuffer buf = i.mons::Message::Base::Serialize();
  // Deserialize and check
  size_t read = Deseralize(o, buf);
  assert(read == buf.data.size());
  VALIDATE_STRUCT(BaseData);
  VALIDATE_STRUCT(ConnectInfoData);
}

#define Q(v) #v
#define QQ(v) Q(v)

int main()
{
  // Run each test 3 times
  for (size_t i = 0; i < 3; i++)
  {
#define REGISTER(Val)                             \
  mons::Log::Debug("Running test case " QQ(Val)); \
  TestCase##Val();
    MONS_REGISTER_MESSAGE_TYPES
#undef REGISTER
  }
  mons::Log::Status("All message serialization tests passed");
  return 0;
}
