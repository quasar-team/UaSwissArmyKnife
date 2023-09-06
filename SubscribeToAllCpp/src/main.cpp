// @author pnikiel
// based on UASDK C++ demos

#include <iostream>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <iomanip>

#include <sys/time.h>
using namespace std;
/* C++ UA Client API */
#include "uaclientsdk.h"
using namespace UaClientSdk;
#include "uasession.h"
#include "uaplatformlayer.h"

/* low level and stack */
#include "opcua_core.h"
#include <time.h>

#include <boost/accumulators/accumulators.hpp>
using namespace boost::accumulators;

#include <boost/accumulators/statistics.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <chrono>

using boost::bind;
#include <boost/ref.hpp>

using boost::ref;

#include <list>
#include <iostream>

using namespace std;

class Callback;

/* Globals */
UaSession*                              g_pUaSession;
Callback*                               g_pCallback;
UaObjectArray<UaNodeId>                 g_VariableNodeIds;
UaObjectArray<UaNodeId>                 g_WriteVariableNodeIds;
bool exitFlag = false;

/* Connect / disconnect */
void connect(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo);
void disconnect();

#ifdef WIN32
#define kbhit _kbhit
#define getch _getch
#else
#include <unistd.h>
void Sleep(int ms)
{
    usleep(1000 * ms);
}
#endif


struct Options
{
    std::string endpoint_url ;
    bool        print_timestamps ;
    int			publishing_interval;
    int 	    sampling_interval;
};

Options options;

std::map<unsigned int,std::string> g_itemIdToName;

class Callback:
    public UaSessionCallback,
    public UaSubscriptionCallback
{
public:
    Callback() {};
    virtual ~Callback() {};

    /** Send changed status. */
    virtual void connectionStatusChanged(
        OpcUa_UInt32              clientConnectionId,
        UaClient::ServerStatus serverStatus)
    {
        printf("-- Event connectionStatusChanged ----------------------\n");
        printf("clientConnectionId %d \n", clientConnectionId);
        printf("serverStatus %d \n", serverStatus);
        printf("-------------------------------------------------------\n");
    };

    /** Send changed data. */
    virtual void dataChange(
        OpcUa_UInt32               clientSubscriptionHandle,
        const UaDataNotifications& dataNotifications,
        const UaDiagnosticInfos&   /*diagnosticInfos*/)
    {
        OpcUa_UInt32 i = 0;
        OpcUa_UInt32 v = 0;
        timespec t;
        clock_gettime( CLOCK_REALTIME, &t );

        using std::chrono::system_clock;


		time_t now = time(0 );
		tm* tm = localtime(&now);
		char buff[200];
		strftime(buff, sizeof buff, "%Y-%m-%d %H:%M:%S", tm	);

		std::cout << "@" << buff << " received notification of " << dataNotifications.length() << " elements. Contents:" <<  std::endl;

        for ( i=0; i<dataNotifications.length(); i++ )
        {
	    const OpcUa_MonitoredItemNotification &notif = dataNotifications[i];



	    std::string item;
	    std::map<unsigned int,std::string>::iterator it = g_itemIdToName.find(notif.ClientHandle);
	    if (it != g_itemIdToName.end())
	    {
	    	item = it->second;
	    }
	    else
	    {
	    	item = "<? " + boost::lexical_cast<std::string>(notif.ClientHandle) + "> ";
	    }
	    //= g_itemIdToName[ notif.ClientHandle ];

	    std::cout << std::setw(5) <<  i << " node=" << std::setw(40) <<  item << " ";

	    // by ClientHandle find the address
	    
            if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
            {
                UaVariant v ( dataNotifications[i].Value.Value );
                std::string s ( v.toString().toUtf8() );

		        std::cout << " value=" << setw(40) << s;

		        if (options.print_timestamps)
		        {
		        	OpcUa_DateTime dt ( dataNotifications[i].Value.SourceTimestamp );
		        	UaDateTime highLevelDt (dt);

		        	time_t t = highLevelDt.toTime_t();
		    		tm = localtime(&t);
		    		strftime(buff, sizeof buff, "%Y-%m-%d %H:%M:%S", tm	);
		    		std::cout << " ts_source=" << buff<<"."<<std::setw(3)<<std::setfill('0')<<highLevelDt.msec()<<std::setfill(' ') << " ";
		        	highLevelDt = dataNotifications[i].Value.ServerTimestamp;
		        	std::cout << " ts_server=" ;
		        	t = highLevelDt.toTime_t();
		        	tm = localtime(&t);
		        	strftime(buff, sizeof buff, "%Y-%m-%d %H:%M:%S", tm	);
		        	std::cout << " " << buff<<"."<<std::setw(3)<<std::setfill('0')<<highLevelDt.msec()<<std::setfill(' ') << " ";


		        }

		        std::cout << std::endl;

            }
            else
            {
		std::cout << "not_good" << std::endl;
            }
        }

    };

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      /*clientSubscriptionHandle*/,
        const UaStatus&   /*status*/) {};

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
    };

private:
    OpcUa_UInt32 lastValue;
};





void subscribe(list<UaNodeId>& nodeIds)
{
    printf("\n\n****************************************************************\n");
    printf("** Try to create a subscribe\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus             status;
    UaSubscription* pUaSubscription         = NULL;
    SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = options.publishing_interval;
    ServiceSettings      serviceSettings;

    /*********************************************************************
     Create a Subscription
    **********************************************************************/
    status = g_pUaSession->createSubscription(
                 serviceSettings,        // Use default settings
                 g_pCallback,            // Callback object
                 0,                      // We have only one subscription, handle is not needed
                 subscriptionSettings,   // general settings
                 OpcUa_True,             // Publishing enabled
                 &pUaSubscription);      // Returned Subscription instance
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** Error: UaSession::createdSubscription failed [ret=%s] *********\n", status.toString().toUtf8());
    }
    else
    {
        std::cout << "Agreed publishingInterval is: " << pUaSubscription->publishingInterval() << std::endl;

        printf("****************************************************************\n");
        printf("** Try to create monitored items\n");

        OpcUa_UInt32                  i;
        OpcUa_UInt32                  count;
        UaMonitoredItemCreateRequests monitoredItemCreateRequests;
        UaMonitoredItemCreateResults  monitoredItemCreateResults;






        list<UaNodeId> ns2Items;

        int ix =0;
        for( list<UaNodeId>::iterator it = nodeIds.begin(); it != nodeIds.end(); ++it )
        {
            //g_VariableNodeIds[ix++] = *it;
            if ((*it).namespaceIndex() == 2)
                ns2Items.push_back( *it );
        }


        g_VariableNodeIds.create ( ns2Items.size() );

        for( list<UaNodeId>::iterator it = ns2Items.begin(); it != ns2Items.end(); ++it )
        {
            g_VariableNodeIds[ix++] = *it;

        }


        count = g_VariableNodeIds.length();
        monitoredItemCreateRequests.create(count+1); // We create also an event monitored item
        for ( i=0; i<count; i++ )
        {

            g_VariableNodeIds[i].copyTo(&monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
            monitoredItemCreateRequests[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
            monitoredItemCreateRequests[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
            monitoredItemCreateRequests[i].RequestedParameters.ClientHandle = i+1;
            monitoredItemCreateRequests[i].RequestedParameters.SamplingInterval = options.sampling_interval;
            monitoredItemCreateRequests[i].RequestedParameters.QueueSize = 100;
        }

        /*********************************************************************
             Call createMonitoredItems service
        **********************************************************************/
        status = pUaSubscription->createMonitoredItems(
                     serviceSettings,               // Use default settings
                     OpcUa_TimestampsToReturn_Both, // Select time stamps to return
                     monitoredItemCreateRequests,   // monitored items to create
                     monitoredItemCreateResults);   // Returned monitored items create result
        /*********************************************************************/
        if ( status.isBad() )
        {
            printf("** Error: UaSession::createMonitoredItems failed [ret=%s] *********\n", status.toString().toUtf8());
            printf("****************************************************************\n");
            return;
        }
        else
        {
            printf("** UaSession::createMonitoredItems result **********************\n");
            for ( i=0; i<count; i++ )
            {
                UaNodeId node(monitoredItemCreateRequests[i].ItemToMonitor.NodeId);
                if ( OpcUa_IsGood(monitoredItemCreateResults[i].StatusCode) )
                {
                    printf("** Variable %s MonitoredItemId = %d\n", node.toString().toUtf8(), monitoredItemCreateResults[i].MonitoredItemId);
		    g_itemIdToName.insert( std::pair<unsigned int,std::string>( monitoredItemCreateResults[i].MonitoredItemId, node.toString().toUtf8() ) );
		    
                }
                else
                {
                    printf("** Variable %s failed!\n", node.toString().toUtf8());
                }
            }
            if ( OpcUa_IsGood(monitoredItemCreateResults[count].StatusCode) )
            {
                printf("** Event MonitoredItemId = %d\n", monitoredItemCreateResults[count].MonitoredItemId);
            }
            else
            {
                printf("** Event MonitoredItem for Server Object failed!\n");
            }
            printf("****************************************************************\n");
        }
    }

    printf("\n*******************************************************\n");
    printf("*******************************************************\n");
#ifdef _WIN32_WCE
    printf("**        Press ESC to stop subscription        *******\n");
#else
    printf("**        Press x to stop subscription          *******\n");
#endif
    printf("*******************************************************\n");
    printf("*******************************************************\n");


    while (! exitFlag)
    {
        sleep (1);
    }

    int action=-1;

    /*********************************************************************
     Delete Subscription
    **********************************************************************/
    status = g_pUaSession->deleteSubscription(
                 serviceSettings,    // Use default settings
                 &pUaSubscription);  // Subcription
    /*********************************************************************/
    if ( status.isBad() )
    {
        printf("** UaSession::deleteSubscription failed! **********************\n");
        printf("****************************************************************\n");
    }
    else
    {
        pUaSubscription = NULL;
        printf("** UaSession::deleteSubscription succeeded!\n");
        printf("****************************************************************\n");
    }
}


double getTimeDiff (timeval *t0, timeval *t1)
{
    time_t diffSec = t1->tv_sec - t0->tv_sec;
    suseconds_t diffUsec = t1->tv_usec - t0->tv_usec;

    diffUsec += diffSec*1000000;
    return (double)diffUsec / 1.0E6;
}

void browseRecurrentlyForVariables( const UaNodeId& startPoint, std::list<UaNodeId>& out  )
{
    ServiceSettings ss;
    BrowseContext bc;
    UaByteString contPoint;
    UaReferenceDescriptions referenceDescriptions;

    OpcUa_ReferenceDescription r;

    UaStatus s = g_pUaSession->browse(
                     ss,
                     startPoint,
                     bc,
                     contPoint,
                     referenceDescriptions
                 );
    // DFS:
    for (int i=0; i<referenceDescriptions.length(); ++i)
    {
        r = referenceDescriptions[i];
        switch (r.NodeClass)
        {
        case OpcUa_NodeClass_Object:
        {
            browseRecurrentlyForVariables( r.NodeId.NodeId, out );
            break;
        };

        case OpcUa_NodeClass_Variable:
        {
            out.push_back( r.NodeId.NodeId );
            break;
        }
        default:
        {
            //cout << "object type not handled" << endl;
        }

        }
    }

}

std::list<UaNodeId> browseAllVars()
{
    std::list<UaNodeId> out;
    UaNodeId rootNode(OpcUaId_RootFolder);
    browseRecurrentlyForVariables( rootNode, out );
    return out;
}



void sig(int i)
{
    exitFlag = true;
}



Options parse_program_options(int argc, char** argv)
{
    namespace po = boost::program_options;
    
    Options options;

    po::options_description desc ("Usage");
    desc.add_options()
	("help",             "help")
	("endpoint_url",     po::value<std::string>(&options.endpoint_url), "the endpoint should be printed by a server at startup")
	("print_timestamps", po::bool_switch(&options.print_timestamps)->default_value(false), "print timestamps")
	("pub_interval",     po::value<int>(&options.publishing_interval)->default_value(500), "the requested publishing interval [ms]")
	("samp_interval",    po::value<int>(&options.sampling_interval)->default_value(100), "the requested sampling interval (same for all items) [ms]");
    

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << desc << endl;
	exit(0);
    }


    return options;
    
}

/*============================================================================
 * main
 *===========================================================================*/
#ifdef _WIN32_WCE
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPWSTR, int)
#else
int main(int argc, char* argv[])
#endif
{

    options = parse_program_options(argc, argv);

    if( options.endpoint_url == "")
    {
        std::cout << "Sorry, you must specify endpoint. Run with --help" << std::endl;
        exit(1);
    }

    signal( SIGINT, sig );

    g_pUaSession = NULL;
    g_pCallback  = NULL;

    // Initialize the UA Stack platform layer
    UaPlatformLayer::init();

    UaStatus status;

    UaString sUrl( options.endpoint_url.c_str() );
    SessionSecurityInfo sessionSecurityInfo;
    SessionConnectInfo sessionConnectInfo;

    timeval told, tnew;
    connect(sUrl, sessionSecurityInfo);

    cout << "Found following vars:" << endl;
    list<UaNodeId> allVariables = browseAllVars ();
    for (list<UaNodeId>::iterator it = allVariables.begin(); it != allVariables.end(); ++it )
    {
        cout << (*it).toFullString().toUtf8() << endl;
    }
    subscribe ( allVariables );

    int failCtr=0;

    gettimeofday (&told, 0);
    while (! exitFlag)
    {
        sleep (1);
    }
    disconnect();

 
    return 0;
}

/*============================================================================
 * connect - Connect to OPC UA Server
 *===========================================================================*/
void connect(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus status;

    printf("\n\n****************************************************************\n");
    printf("** Try to connect to selected server\n");
    if ( g_pUaSession )
    {
        disconnect();
    }

    g_pUaSession = new UaSession();
    g_pCallback  = new Callback;

    SessionConnectInfo sessionConnectInfo;
    sessionConnectInfo.sApplicationName = "Client_Cpp_SDK@myComputer";
    sessionConnectInfo.sApplicationUri  = "Client_Cpp_SDK@myComputer";
    sessionConnectInfo.sProductUri      = "Client_Cpp_SDK";

    /*********************************************************************
     Connect to OPC UA Server
     **********************************************************************/
    status = g_pUaSession->connect(
                 sUrl,                // URL of the Endpoint - from discovery or config
                 sessionConnectInfo,  // General settings for connection
                 sessionSecurityInfo, // Security settings
                 g_pCallback);        // Callback interface
    /*********************************************************************/
    if ( status.isBad() )
    {
        delete g_pUaSession;
        g_pUaSession = NULL;
        printf("** Error: UaSession::connect failed [ret=%s] *********\n", status.toString().toUtf8());
        printf("****************************************************************\n");
        return;
    }
    else
    {
        printf("** Connected to Server %s ****\n", sUrl.toUtf8());
        printf("****************************************************************\n");
    }
}

/*============================================================================
 * disconnect - Disconnect from OPC UA Server
 *===========================================================================*/
void disconnect()
{



    printf("\n\n****************************************************************\n");
    printf("** Disconnect from Server\n");
    if ( g_pUaSession == NULL )
    {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    ServiceSettings serviceSettings;

    /*********************************************************************
     Disconnect from OPC UA Server
     **********************************************************************/
    g_pUaSession->disconnect(
        serviceSettings, // Use default settings
        OpcUa_True);       // Delete subscriptions
    /*********************************************************************/
    printf("****************************************************************\n");

    delete g_pUaSession;
    g_pUaSession = NULL;

    g_VariableNodeIds.create(1);
    g_WriteVariableNodeIds.clear();



}
