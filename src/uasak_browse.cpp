#include <iostream>

#include <boost/program_options.hpp>

#include <LogIt.h>

/* From open62541-compat or UA-SDK */
#include <uaclient/uaclientsdk.h>

using namespace UaClientSdk;

struct Options
{
    std::string endpoint_url ;
    bool        print_timestamps ;
    int         publishing_interval;
    int         sampling_interval;
};


Options parse_program_options(int argc, char* argv[])
{
    namespace po = boost::program_options;

    Options options;

    po::options_description desc ("Usage");
    desc.add_options()
      ("help,h",           "help")
      ("endpoint_url",     po::value<std::string>(&options.endpoint_url)->default_value("opc.tcp://127.0.0.1:4841"), "If you are looking for the endpoint address, note it is often printed by OPC-UA servers when they start up");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(0);
    }

    return options;

}

void browse_recurse(UaNodeId n0)
{
  
}

int main (int argc, char* argv[])
{
  Log::initializeLogging();

  Options options (parse_program_options(argc, argv));
  std::cout << "will connect to the following endpoint: " << options.endpoint_url << std::endl;

  SessionConnectInfo      connectInfo;
  SessionSecurityInfo     securityInfo;

  UaSession session;
  UaStatus status = session.connect(
    options.endpoint_url.c_str(),
    connectInfo,
    securityInfo,
    nullptr /*callback*/);
  if (!status.isGood())
  {
    LOG(Log::ERR) << status.toString().toUtf8();
    return 1;
  }

  LOG(Log::INF) << "Session opened ;-)";



}
