#include <iostream>

#include <boost/program_options.hpp>

#include <UANodeSet.hxx>

#include <LogIt.h>

/* From open62541-compat or UA-SDK */
#include <uaclient/uaclientsdk.h>

using namespace UaClientSdk;

typedef std::list<UaNodeId> VisitedNodesType;

struct Options
{
    std::string endpoint_url ;
    bool        print_timestamps ;
    int         publishing_interval;
    int         sampling_interval;
    bool        skip_ns0;
};


Options parse_program_options(int argc, char* argv[])
{
    namespace po = boost::program_options;

    Options options;

    po::options_description desc ("Usage");
    desc.add_options()
    ("help,h",           "help")
    ("endpoint_url",     po::value<std::string>(&options.endpoint_url)->default_value("opc.tcp://127.0.0.1:4841"), "If you are looking for the endpoint address, note it is often printed by OPC-UA servers when they start up")
    ("skip_ns0",         po::value<bool>(&options.skip_ns0)->default_value(true), "Don't follow references to ns0");

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

std::string stringifyNodeId (const UaNodeId& id)
{
	switch (id.identifierType())
	{
	case IdentifierType::OpcUa_IdentifierType_Numeric:
		return "i=" + std::to_string(id.identifierNumeric());
	case IdentifierType::OpcUa_IdentifierType_String:
		return "s=" + id.identifierString().toUtf8();
	default:
		throw std::runtime_error("This identifier type is not yet implemented -- fixme!");
	}
}

void browse_recurse(
    Options    options,
    UaSession& session,
    ServiceSettings serviceSettings,
    UaNodeId n0,
    VisitedNodesType& visitedNodes,
	UANodeSet::UANodeSet* document)
{

    visitedNodes.push_back(n0);
    BrowseContext browseContext; // should come from the parent, actually.
    UaByteString continuationPoint; // hope won't use it ;-)
    UaReferenceDescriptions referenceDescriptions;

    UaStatus status = session.browse(
                          serviceSettings,
                          n0,
                          browseContext,
                          continuationPoint,
                          referenceDescriptions);

    if (! status.isGood())
    {
        LOG(Log::ERR) << "Browse on node: " << n0.toString().toUtf8() << " failed because: " << status.toString().toUtf8();
        return;
    }

    for (unsigned int i=0; i < referenceDescriptions.size(); i++)
    {
    	ReferenceDescription& rd (referenceDescriptions[i]);
        UaNodeId targetNodeId = rd.NodeId.nodeId();
        if (options.skip_ns0 && targetNodeId.namespaceIndex() == 0)
        {
        	LOG(Log::DBG) << "The target node: " << targetNodeId.toFullString().toUtf8() << " is in the NS0 and it was decided to skip NS0";
            continue;
        }

    	UANodeSet::NodeId xmlNodeId (stringifyNodeId(rd.NodeId.nodeId()));
    	UANodeSet::QualifiedName browseName (rd.BrowseName.toUtf8());
    	document->UAObject().push_back(UANodeSet::UAObject(xmlNodeId, browseName));


        if (std::find(std::begin(visitedNodes), std::end(visitedNodes), targetNodeId) == std::end(visitedNodes))
        {
            browse_recurse(options, session, serviceSettings, referenceDescriptions[i].NodeId.nodeId(), visitedNodes, document);
        }
    }

}

int main (int argc, char* argv[])
{
	xml_schema::namespace_infomap nsMap;
	nsMap[""].name = "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd";
	nsMap[""].schema = "../Configuration/Configuration.xsd";

	UANodeSet::UANodeSet nodeSetDocument;

	UANodeSet::NodeId n ("i=1000;ns=2");
	UANodeSet::QualifiedName browseName ("bingo!");



	//nodeSetDocument.UAObject().push_back(UANodeSet::UAObject(n, browseName));





    Log::initializeLogging(Log::DBG);

    Options options (parse_program_options(argc, argv));
    std::cout << "will connect to the following endpoint: " << options.endpoint_url << std::endl;

    SessionConnectInfo      connectInfo;
    SessionSecurityInfo     securityInfo;
    ServiceSettings         defaultServiceSettings;

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

    UaNodeId objectsFolder (85, 0);

    VisitedNodesType visitedNodes;

    browse_recurse(options, session, defaultServiceSettings, objectsFolder, visitedNodes, &nodeSetDocument);

    LOG(Log::INF) << "In total visited " << visitedNodes.size() << " nodes";

	UANodeSet::UANodeSet_ (std::cout, nodeSetDocument, nsMap);

}
