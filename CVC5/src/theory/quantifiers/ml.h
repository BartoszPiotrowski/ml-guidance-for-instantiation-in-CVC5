#ifndef CVC4__ML
#define CVC4__ML

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "base/check.h"
#include "expr/node.h"
#include "lightgbm.h"
class tcp_client_t;

namespace cvc5 {
class TCPClient
{
 public:
  static TCPClient s_client;
  virtual ~TCPClient();
  static int sopen() { return s_client.open(); }
  static void sclose() { return s_client.close(); }
  static int send(const char* message)
  {
    return s_client.send_internal(message);
  }
  static std::string receive() { return s_client.receive_internal(); }

 private:
  TCPClient();
  const unsigned short d_port = 8080;
  const char d_server[255] = "127.0.0.1";  // server host name or IP
  bool d_isOpen = false;
  std::unique_ptr<tcp_client_t> d_client;
  int open();
  void close();
  int send_internal(const char* message);
  std::string receive_internal();
};

class TCPReader
{
 public:
  TCPReader() {}
  virtual ~TCPReader() {}
  int predict(const char* message);
  const std::vector<std::vector<double> >& predictions() const
  {
    return d_predictions;
  }

 private:
  std::vector<std::vector<double> > d_predictions;
  int receive();
};

class PredictorInterface
{
 public:
  virtual ~PredictorInterface() {}
  virtual double predict(const float* features) = 0;
  virtual size_t numberOfFeatures() const = 0;
};

class Sigmoid : public PredictorInterface
{
 public:
  Sigmoid(const char* modelFile);
  virtual ~Sigmoid() {}

  inline double sigmoid(double x)
  {
    if (x < 0)
    {
      const double expx = std::exp(x);
      return expx / (1 + expx);
    }
    return 1 / (1 + std::exp(-x));
  }

  virtual double predict(const float* features) override
  {
    double exponent =
        d_coefficients[0];  //  assuming intercept on the first position
    for (size_t i = 1; i < d_coefficients.size(); i++)
    {
      exponent += d_coefficients[i] * features[i - 1];
    }
    return sigmoid(exponent);
  }

  virtual size_t numberOfFeatures() const override
  {
    return d_coefficients.size() - 1;
  }

 protected:
  std::vector<double> d_coefficients;
};

class LightGBMWrapper : public PredictorInterface
{
 public:
  LightGBMWrapper(const char* modelFile);
  virtual double predict(const float* features) override;
  virtual ~LightGBMWrapper();

  virtual size_t numberOfFeatures() const override;

 protected:
  BoosterHandle d_handle;
  int d_numIterations;
};
}  // namespace cvc5

#endif
