
#include "theory/quantifiers/ml.h"

#include <cctype>
#include <climits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
/* #include "torch/torch.h" */


#include "base/check.h"
#include "expr/dtype.h"
#include "expr/node_manager_attributes.h"
#include "expr/node_visitor.h"
#include "expr/sequence.h"
#include "options/bv_options.h"
#include "options/language.h"
#include "options/printer_options.h"
#include "options/quantifiers_options.h"
#include "options/smt_options.h"
#include "printer/let_binding.h"
#include "smt/command.h"
#include "smt/node_command.h"
#include "smt/smt_engine.h"
#include "smt_util/boolean_simplification.h"

namespace cvc5 {
LightGBMWrapper::LightGBMWrapper(const char* modelFile)
{
  const int ec =
      LGBM_BoosterCreateFromModelfile(modelFile, &d_numIterations, &d_handle);
  Trace("ml") << "Loaded LGBM model " << modelFile << " with "
              << d_numIterations << " iterations." << std::endl;
  AlwaysAssert(ec == 0);
}

double LightGBMWrapper::predict(const float* features)
{
  static_assert(CHAR_BIT * sizeof(float) == 32, "require 32-bit floats");
  double returnValue = 0;
  int64_t returnSize;
  const auto featureCount = numberOfFeatures();

  if (Trace.isOn("ml"))
  {
    Trace("ml") << "features:  [";
    for (size_t index = 0; index < featureCount; index++)
    {
      Trace("ml") << (index ? ", " : "") << features[index];
    }
    Trace("ml") << "]" << std::endl;
  }
  const int ec =
      LGBM_BoosterPredictForMatSingleRow(d_handle,
                                         features,
                                         C_API_DTYPE_FLOAT32,
                                         featureCount,
                                         1,
                                         C_API_PREDICT_NORMAL,
                                         /* C_API_PREDICT_RAW_SCORE, */
                                         0,
                                         -1,
                                         "early_stopping_rounds=100",
                                         &returnSize,
                                         &returnValue);
  AlwaysAssert(ec == 0);
  AlwaysAssert(returnSize == 1);
  Trace("ml") << "prediction :" << returnValue << std::endl;
  return returnValue;
}

size_t LightGBMWrapper::numberOfFeatures() const
{
  int sz = -1;
  const int ec = LGBM_BoosterGetNumFeature(d_handle, &sz);
  AlwaysAssert(ec == 0);
  return sz;
}

LightGBMWrapper::~LightGBMWrapper() {}

Sigmoid::Sigmoid(const char* modelFile)
{
  std::fstream fs(modelFile, std::fstream::in);
  double coefficient;
  while (fs >> coefficient) d_coefficients.push_back(coefficient);
}
}  // namespace cvc5

#ifndef LIB_SOCKET_H
#define LIB_SOCKET_H

#if defined(_MSC_VER)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>  //hostent
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cerrno>
#include <ctime>
#include <iostream>
#include <string>

// multi platform socket descriptor
#if _WIN32
typedef SOCKET socketfd_t;
#else
typedef int socketfd_t;
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////
// utils
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::string str_extract(const std::string& str_in);
std::string prt_time();
int set_daemon(const char* str_dir);
void wait(int nbr_secs);

/////////////////////////////////////////////////////////////////////////////////////////////////////
// socket_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

class socket_t
{
 public:
  socket_t();
  socket_t(socketfd_t sockfd, sockaddr_in sock_addr);
  void close();
  int write_all(const void* buf, int size_buf);
  int read_all(void* buf, int size_buf);
  int hostname_to_ip(const char* host_name, char* ip);

 public:
  socketfd_t m_sockfd;        // socket descriptor
  sockaddr_in m_sockaddr_in;  // client address (used to store return value of
                              // server accept())
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
// tcp_client_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

class tcp_client_t : public socket_t
{
 public:
  tcp_client_t();
  ~tcp_client_t();
  int connect();

  tcp_client_t(const char* host_name, const unsigned short server_port);
  int connect(const char* host_name, const unsigned short server_port);

 protected:
  std::string m_server_ip;
  unsigned short m_server_port;
};

#endif

#ifndef LIB_NETSOCKET_JSON_MESSAGE_H
#define LIB_NETSOCKET_JSON_MESSAGE_H

/* #include "socket.hh" */

/////////////////////////////////////////////////////////////////////////////////////////////////////
// custom TCP message:
// a header with size in bytes and # terminator
// JSON text
/////////////////////////////////////////////////////////////////////////////////////////////////////

int write_request(socket_t& socket, const char* buf_msg);
std::string read_response(socket_t& socket);

#endif

namespace cvc5 {
TCPClient TCPClient::s_client;
/* TCPClient::TCPClient() { open(); } */
TCPClient::TCPClient() {}
TCPClient::~TCPClient() { close(); }

int TCPReader::predict(const char* message)
{
  TCPClient::sopen();
  const auto sec = TCPClient::send(message);
  if (sec != 0)
  {
    return sec;
  }
  const auto rv = receive();
  TCPClient::sclose();
  return rv;
}

int TCPClient::open()
{
  d_client = std::make_unique<tcp_client_t>(d_server, d_port);
  if (options::tcpLearningVerb())
    std::cout << "client connecting to: " << d_server << ":" << d_port << " <"
              << d_client->m_sockfd << "> " << std::endl;

  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // create socket and open connection
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  if (d_client->connect() < 0)
  {
    AlwaysAssert(false) << "client connection error " << std::endl;
    return -1;
  }
  if (options::tcpLearningVerb())
  {
    std::cout << "client connected to: " << d_server << ":" << d_port << " <"
              << d_client->m_sockfd << "> " << std::endl;
  }
  d_isOpen = true;
  return 0;
}

int TCPClient::send_internal(const char* message)
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // write request
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  if (options::tcpLearningVerb())
    std::cout << "client write request " << std::endl;
  if (write_request(*d_client, message) < 0)
  {
    AlwaysAssert(false) << "client write error " << std::endl;
    return -1;
  }
  if (options::tcpLearningVerb() > 1)
  {
    std::cout << "client sent: ";
    std::cout << message << " " << d_server << ":" << d_port << " <"
              << d_client->m_sockfd << "> " << std::endl;
  }
  return 0;
}

static void skip(std::stringstream& s, char expected)
{
  const auto c = s.get();
  AlwaysAssert(c == expected)
      << c << " obtained instead of " << expected << std::endl;
}

std::string TCPClient::receive_internal()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  // read response
  /////////////////////////////////////////////////////////////////////////////////////////////////////

  std::string str_response = read_response(*d_client);
  if (options::tcpLearningVerb() > 1)
  {
    std::cout << "client received: ";
    std::cout << str_response << " " << d_server << ":" << d_port << " <"
              << d_client->m_sockfd << "> " << std::endl;
  }
  return str_response;
}

int TCPReader::receive()
{
  std::string str_response = TCPClient::receive();
  std::stringstream input(str_response);
  skip(input, '[');
  bool closed = true;
  while (!closed || input.peek() != ']')
  {
    AlwaysAssert(!(input.eof() || input.fail() || input.bad()))
        << " ERROR: something went wrong in input";
    if (isspace(input.peek()))
    {
      input.get();
      continue;
    }
    if (closed)
    {
      skip(input, '[');
      if (options::tcpLearningVerb())
        std::cout << "predicted variable: " << d_predictions.size()
                  << std::endl;
      d_predictions.resize(d_predictions.size() + 1);
      closed = false;
    }
    else if (input.peek() == ']')
    {
      skip(input, ']');
      closed = true;
    }
    else
    {
      double prediction;
      input >> prediction;
      if (options::tcpLearningVerb())
        std::cout << "prediction received: " << prediction << std::endl;
      d_predictions.back().push_back(prediction);
    }
  }

  return 0;
}
void TCPClient::close()
{
  if (!d_isOpen)
  {
    return;
  }
  if (options::tcpLearningVerb()) std::cout << "client closed" << std::endl;
  d_isOpen = false;
  return d_client->close();
}
}  // namespace cvc5

#define _CRT_NONSTDC_NO_DEPRECATE
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
/* #include "socket.hh" */

int write_request(socket_t& socket, const char* buf_msg)
{
  std::string buf;
  const size_t size_msg = strlen(buf_msg);
  const auto number_string = std::to_string(size_msg);
  const size_t buffer_size = size_msg + 1 + number_string.size() + 1;
  char* buffer = new char[buffer_size];
  memcpy(buffer, buf_msg, size_msg);
  strncpy(buffer + size_msg, number_string.c_str(), number_string.size() + 1);
  buffer[buffer_size - 1] = '\0';
  buf = std::to_string(static_cast<long long unsigned int>(size_msg));
  buf += "#";
  buf += std::string(buf_msg);

  const auto value = (socket.write_all(buf.data(), buf.size()));
  /* const auto value = socket.write_all(buffer, buffer_size); */
  delete[] buffer;
  return value;
}

std::string read_response(socket_t& socket)
{
  std::string str_header;
  // parse header one character at a time and look for #
  // assume size header length less than 20 digits
  str_header.reserve(20);
  for (size_t idx = 0; idx < 20; idx++)
  {
    char c;
    if (::recv(socket.m_sockfd, &c, 1, 0) == -1)
    {
      std::cout << "recv error: " << strerror(errno) << std::endl;
      return std::string();
    }
    if (c == '#')
    {
      break;
    }
    else
    {
      str_header += c;
    }
  }

  size_t size_msg;
  try
  {
    size_msg = static_cast<size_t>(std::stoi(str_header));
  }
  catch (const std::invalid_argument& ia)
  {
    std::cerr << "Error interpreting message header:" << str_header
              << std::endl;
    throw;
  }

  // read from socket with known size
  char* buf = new char[size_msg];
  if (socket.read_all(buf, size_msg) < 0)
  {
    std::cout << "recv error: " << strerror(errno) << std::endl;
    return std::string();
  }
  std::string str_msg(buf, size_msg);
  delete[] buf;
  return str_msg;
}

socket_t::socket_t() : m_sockfd(0)
{
  memset(&m_sockaddr_in, 0, sizeof(m_sockaddr_in));
}

socket_t::socket_t(socketfd_t sockfd, sockaddr_in sock_addr)
    : m_sockfd(sockfd), m_sockaddr_in(sock_addr)
{
}

void socket_t::close()
{
#if defined(_MSC_VER)
  ::closesocket(m_sockfd);
#else
  ::close(m_sockfd);
#endif
  // clear members
  memset(&m_sockaddr_in, 0, sizeof(m_sockaddr_in));
  m_sockfd = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// socket_t::write_all
//::send
// http://man7.org/linux/man-pages/man2/send.2.html
// The system calls send(), sendto(), and sendmsg() are used to transmit
// a message to another socket.
/////////////////////////////////////////////////////////////////////////////////////////////////////

int socket_t::write_all(const void* _buf, int size_buf)
{
  const char* buf =
      static_cast<const char*>(_buf);  // can't do pointer arithmetic on void*
  int sent_size;                       // size in bytes sent or -1 on error
  int size_left;                       // size in bytes left to send
  const int flags = 0;
  size_left = size_buf;
  while (size_left > 0)
  {
    sent_size = ::send(m_sockfd, buf, size_left, flags);
    if (-1 == sent_size)
    {
      std::cout << "send error: " << strerror(errno) << std::endl;
      return -1;
    }
    size_left -= sent_size;
    buf += sent_size;
  }
  return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// socket_t::read_all
// read SIZE_BUF bytes of data from m_sockfd into buffer BUF
// return total size read
// http://man7.org/linux/man-pages/man2/recv.2.html
// The recv(), recvfrom(), and recvmsg() calls are used to receive
// messages from a socket.
// NOTE: assumes : 1) blocking socket 2) socket closed , that makes ::recv
// return 0
/////////////////////////////////////////////////////////////////////////////////////////////////////

int socket_t::read_all(void* _buf, int size_buf)
{
  char* buf = static_cast<char*>(_buf);  // can't do pointer arithmetic on void*
  int recv_size;  // size in bytes received or -1 on error
  int size_left;  // size in bytes left to send
  const int flags = 0;
  int total_recv_size = 0;
  size_left = size_buf;
  while (size_left > 0)
  {
    recv_size = ::recv(m_sockfd, buf, size_left, flags);
    if (-1 == recv_size)
    {
      std::cout << "recv error: " << strerror(errno) << std::endl;
    }
    // everything received, exit
    if (0 == recv_size)
    {
      break;
    }
    size_left -= recv_size;
    buf += recv_size;
    total_recv_size += recv_size;
  }
  return total_recv_size;
}

///////////////////////////////////////////////////////////////////////////////////////
// socket_t::hostname_to_ip
// The getaddrinfo function provides protocol-independent translation from an
// ANSI host name to an address
///////////////////////////////////////////////////////////////////////////////////////

int socket_t::hostname_to_ip(const char* host_name, char* ip)
{
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in* h;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if ((rv = getaddrinfo(host_name, "http", &hints, &servinfo)) != 0)
  {
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    h = (struct sockaddr_in*)p->ai_addr;
    strcpy(ip, inet_ntoa(h->sin_addr));
  }

  freeaddrinfo(servinfo);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// tcp_client_t::tcp_client_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

tcp_client_t::tcp_client_t() : socket_t()
{
#if defined(_MSC_VER)
  WSADATA ws_data;
  if (WSAStartup(MAKEWORD(2, 0), &ws_data) != 0)
  {
    exit(1);
  }
#endif
}

tcp_client_t::tcp_client_t(const char* host_name,
                           const unsigned short server_port)
    : socket_t(), m_server_port(server_port)
{
#if defined(_MSC_VER)
  WSADATA ws_data;
  if (WSAStartup(MAKEWORD(2, 0), &ws_data) != 0)
  {
    exit(1);
  }
#endif

  char server_ip[100];

  // get ip address from hostname
  hostname_to_ip(host_name, server_ip);

  // store
  m_server_ip = server_ip;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// tcp_client_t::connect
//::accept will block until a socket is opened with ::connect
/////////////////////////////////////////////////////////////////////////////////////////////////////

int tcp_client_t::connect(const char* host_name,
                          const unsigned short server_port)
{
  struct sockaddr_in server_addr;  // server address
  char server_ip[100];

  // get ip address from hostname
  hostname_to_ip(host_name, server_ip);

  // store
  m_server_ip = server_ip;
  m_server_port = server_port;

  // create a stream socket using TCP
  if ((m_sockfd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    std::cout << "socket error: " << std::endl;
    return -1;
  }

  // construct the server address structure
  memset(&server_addr, 0, sizeof(server_addr));  // zero out structure
  server_addr.sin_family = AF_INET;              // internet address family
  if (inet_pton(AF_INET, m_server_ip.c_str(), &server_addr.sin_addr)
      <= 0)  // server IP address
  {
    std::cout << "inet_pton error: " << strerror(errno) << std::endl;
    return -1;
  }
  server_addr.sin_port = htons(m_server_port);  // server port

  // establish the connection to the server
  if (::connect(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))
      < 0)
  {
    std::cout << "connect error: " << strerror(errno) << std::endl;
    return -1;
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// tcp_client_t::connect
//::accept will block until a socket is opened with ::connect
/////////////////////////////////////////////////////////////////////////////////////////////////////

int tcp_client_t::connect()
{
  struct sockaddr_in server_addr;  // server address

  // create a stream socket using TCP
  if ((m_sockfd = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
  {
    std::cout << "socket error: " << std::endl;
    return -1;
  }

  // construct the server address structure
  memset(&server_addr, 0, sizeof(server_addr));  // zero out structure
  server_addr.sin_family = AF_INET;              // internet address family
  if (inet_pton(AF_INET, m_server_ip.c_str(), &server_addr.sin_addr)
      <= 0)  // server IP address
  {
    std::cout << "inet_pton error: " << strerror(errno) << std::endl;
    return -1;
  }
  server_addr.sin_port = htons(m_server_port);  // server port

  // establish the connection to the server
  if (::connect(m_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))
      < 0)
  {
    std::cout << "connect error: " << strerror(errno) << std::endl;
    return -1;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// tcp_client_t::~tcp_client_t
/////////////////////////////////////////////////////////////////////////////////////////////////////

tcp_client_t::~tcp_client_t()
{
#if defined(_MSC_VER)
  WSACleanup();
#endif
}

