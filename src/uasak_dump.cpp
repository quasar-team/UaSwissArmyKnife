#include <iostream>
#include <fstream>

#include <boost/program_options.hpp>

#include <UANodeSet.hxx>

#include <LogIt.h>

#include "AddressSpaceVisitor.hxx"
#include "uasak_version.h"

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
    std::string fileName;
};


Options parse_program_options(int argc, char* argv[])
{
    namespace po = boost::program_options;

    Options options;

    po::options_description desc ("Usage");
    desc.add_options()
    ("help,h",           "help")
    ("version",          "version")
    ("endpoint_url",     po::value<std::string>(&options.endpoint_url)->default_value("opc.tcp://127.0.0.1:4841"), "If you are looking for the endpoint address, note it is often printed by OPC-UA servers when they start up")
    ("skip_ns0",         po::value<bool>(&options.skip_ns0)->default_value(true), "Don't follow references to ns0")
    ("out,o",            po::value<std::string>(&options.fileName)->default_value("dump.xml"), "File name to which the dump should be saved");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(const boost::exception& e)
    {
        std::cerr << "Unable to parse arguments; run with --help" << '\n';
        exit(1);
    }


    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(0);
    }
    if (vm.count("version"))
    {
        std::cout << UASAK_VERSION << std::endl;
        exit(0);
    }

    return options;

}

std::string stringifyNodeId (const UaNodeId& id)
{
	switch (id.identifierType())
	{
	case IdentifierType::OpcUa_IdentifierType_Numeric:
		return "ns=" + std::to_string(id.namespaceIndex()) + ";i=" + std::to_string(id.identifierNumeric());
	case IdentifierType::OpcUa_IdentifierType_String:
		return "ns=" + std::to_string(id.namespaceIndex()) + ";s=" + id.identifierString().toUtf8();
	default:
		throw std::runtime_error("This identifier type is not yet implemented -- fixme!");
	}
}


class XmlDumpingVisitor : public AddressSpaceVisitor
{
public:
	XmlDumpingVisitor(UANodeSet::UANodeSet& xmlDom) :
		m_xmlDom(xmlDom)
	{}

	void visitingObject (
			const UaExpandedNodeId& id,
			const UaString&         browseName,
            const std::list<ForwardReference> refs) override
	{
		LOG(Log::INF) << "Visiting object, the id is: " << id.nodeId().toFullString().toUtf8();
    	UANodeSet::NodeId xmlNodeId (stringifyNodeId(id.nodeId()));
    	UANodeSet::QualifiedName xmlBrowseName (browseName.toUtf8());
    	m_xmlDom.UAObject().push_back(UANodeSet::UAObject(xmlNodeId, xmlBrowseName));
        if (refs.size() > 0)
        {
            UANodeSet::ListOfReferences listOfRefs;
            for (const ForwardReference& reference : refs)
            {   
                UANodeSet::Reference uans_reference (stringifyNodeId(reference.to), /* reftype*/ stringifyNodeId(reference.type) );
                listOfRefs.Reference().push_back(uans_reference);
            }
            m_xmlDom.UAObject().back().References().set(listOfRefs);
        }

	}

	virtual void visitingVariable (
			const UaExpandedNodeId& id,
			const UaString&         browseName,
            const std::list<ForwardReference> refs) override
	{
		LOG(Log::INF) << "Visiting variable, the id is: " << id.nodeId().toFullString().toUtf8();
		UANodeSet::NodeId xmlNodeId (stringifyNodeId(id.nodeId()));
		UANodeSet::QualifiedName xmlBrowseName (browseName.toUtf8());
		m_xmlDom.UAVariable().push_back(UANodeSet::UAVariable(xmlNodeId, xmlBrowseName));
	}
private:
	UANodeSet::UANodeSet& m_xmlDom;
};

std::list<ForwardReference> browseReferencesFrom (
    UaSession&      session,
    UaNodeId        startFrom,
    ServiceSettings serviceSettings
    )
{
    std::list<ForwardReference> result;

    BrowseContext browseContext; // should come from the parent, actually.
    UaByteString continuationPoint; // hope won't use it ;-)
    UaReferenceDescriptions referenceDescriptions;

    UaStatus status = session.browse(
                        serviceSettings,
                        startFrom,
                        browseContext,
                        continuationPoint,
                        referenceDescriptions);

    for (unsigned int i=0; i < referenceDescriptions.size(); i++)
    {
    	ReferenceDescription& rd (referenceDescriptions[i]);
        UaNodeId targetNodeId = rd.NodeId.nodeId();
        ForwardReference fwdref;
        fwdref.to = targetNodeId;
        fwdref.type = rd.ReferenceTypeId;
        result.push_back(fwdref);
        
    }

    return result;
}

void browse_recurse(
    Options    options,
    UaSession& session,
    ServiceSettings serviceSettings,
    UaNodeId n0,
    VisitedNodesType& visitedNodes,
	AddressSpaceVisitor& visitor)
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

        switch (rd.NodeClass)
        {
			case OpcUa_NodeClass_Object:
			{
				visitor.visitingObject(rd.NodeId, rd.BrowseName, browseReferencesFrom(session, rd.NodeId.nodeId(), serviceSettings));
			};
			break;
			case OpcUa_NodeClass_Variable:
			{
				visitor.visitingVariable(rd.NodeId, rd.BrowseName, browseReferencesFrom(session, rd.NodeId.nodeId(), serviceSettings));
			}
			break;
            case OpcUa_NodeClass_Method:
            {
                LOG(Log::WRN) << "Methods not yet supported!";
            }
            break;
            case OpcUa_NodeClass_ObjectType:
            {
                LOG(Log::WRN) << "ObjectTypes not yet supported!";
            }
            default: LOG(Log::INF) << "This node class is not supported: " << rd.NodeClass;
        }

        if (std::find(std::begin(visitedNodes), std::end(visitedNodes), targetNodeId) == std::end(visitedNodes))
        {
            browse_recurse(options, session, serviceSettings, referenceDescriptions[i].NodeId.nodeId(), visitedNodes, visitor);
        }
    }

}

int main (int argc, char* argv[])
{
	xml_schema::namespace_infomap nsMap;
	nsMap[""].name = "http://opcfoundation.org/UA/2011/03/UANodeSet.xsd";
	nsMap[""].schema = "../Configuration/Configuration.xsd";

	UANodeSet::UANodeSet nodeSetDocument;

    Log::initializeLogging(Log::INF);

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

    XmlDumpingVisitor visitor (nodeSetDocument);

    browse_recurse(options, session, defaultServiceSettings, objectsFolder, visitedNodes, visitor);
    session.disconnect(defaultServiceSettings, OpcUa_True);

    LOG(Log::INF) << "In total visited " << visitedNodes.size() << " nodes";

    std::ofstream xml_file (options.fileName);
	UANodeSet::UANodeSet_ (xml_file, nodeSetDocument, nsMap);

    LOG(Log::INF) << "The nodeset file was dumped (name: " << options.fileName << ")";

}
