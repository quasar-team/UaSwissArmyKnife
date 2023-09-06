#include "uaclientsdk.h"
using namespace UaClientSdk;

class Callback:
    public UaSessionCallback,
    public UaSubscriptionCallback
{
public:
    Callback(){};
    virtual ~Callback(){};

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
   //     printf("-- Event dataChange -----------------------------------\n");
     //   printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
     	printf ("notification length: %d\n", dataNotifications.length());
        for ( i=0; i<dataNotifications.length(); i++ )
        {
            if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
            {
                UaVariant tempValue = dataNotifications[i].Value.Value;
		tempValue.toUInt32 (v);
            //    printf("Variable %d value = %s\n", dataNotifications[i].ClientHandle, tempValue.toString().toUtf8());


//                if (v != (lastValue+1))
//			printf ("Lost: previous=%u now=%u\n", lastValue, v);

	lastValue = v;
            }
            else
            {
                printf("Variable %d failed!\n", i);
            }
        }

    };

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      /*clientSubscriptionHandle*/,
        const UaStatus&   /*status*/){};

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
        OpcUa_UInt32 i = 0;
        printf("-- Event newEvents ------------------------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        for ( i=0; i<eventFieldList.length(); i++ )
        {
            UaVariant message    = eventFieldList[i].EventFields[0];
            UaVariant sourceName = eventFieldList[i].EventFields[1]; 
            UaVariant temperature = eventFieldList[i].EventFields[2]; 
            printf("Event[%d] Message = %s SourceName = %s Temperature = %s\n", 
                i, 
                message.toString().toUtf8(), 
                sourceName.toString().toUtf8(),
                temperature.toString().toUtf8());
        }
        printf("-------------------------------------------------------\n");
    };
	
	private:
		OpcUa_UInt32 lastValue;
};


