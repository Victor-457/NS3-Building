#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/buildings-module.h"
#include <fstream>
#include "ns3/uniform-planar-array.h"
#include "ns3/three-gpp-spectrum-propagation-loss-model.h"
#include "ns3/three-gpp-v2v-propagation-loss-model.h"
#include "ns3/three-gpp-channel-model.h"



using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("Main");


static Vector
GetPosition (Ptr<Node> node)
 {
   Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
   return mobility->GetPosition ();
 }

int
main (int argc, char *argv[])
{
	uint32_t nAp = 4;
	uint32_t nWifi = 3;
	double x_min = 0.0;
	double x_max = 20.0;
	double y_min = 0.0;
	double y_max = 20.0;
	double z_min = 0.0;
	double z_max = 10.0;
	double P_X = 1.0;
	double P_Y = 1.0;


	WifiHelper wifi;
	MobilityHelper mobility;
	NodeContainer stas;
	NodeContainer ap;
	NetDeviceContainer staDevs;
	PacketSocketHelper packetSocket;
	NodeContainer::Iterator i;

	stas.Create (nWifi);
	ap.Create (nAp);

	// give packet socket powers to nodes.
	packetSocket.Install (stas);
	packetSocket.Install (ap);

	WifiMacHelper wifiMac;
  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  Ssid ssid = Ssid ("wifi-default");
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

  // setup ap.
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  wifi.Install (wifiPhy, wifiMac, ap);

  // mobility.
	for (i = ap.Begin (); i != ap.End (); ++i)
	{
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		"MinX", DoubleValue (P_X),
		"MinY", DoubleValue (P_Y),
		"DeltaX", DoubleValue (5.0),
		"DeltaY", DoubleValue (10.0),
		"GridWidth", UintegerValue (2),
		"LayoutType", StringValue ("RowFirst"));

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	 	mobility.Install ((*i));

	 	if(P_Y == 1.0 && P_X == 1.0){
	 		P_X = 11.0;
	 	}
	 	else if(P_Y == 1.0 && P_X == 11.0){
	 		P_Y = 11.0;	
	 	}
	 	else if(P_Y == 11.0 && P_X == 11.0){
	 		P_X = 1.0;	
	 	}
	}


	Ptr<Building> b = CreateObject <Building> ();

	b->SetBoundaries (Box(x_min, x_max, y_min, y_max, z_min, z_max));
	b->SetBuildingType (Building::Residential);
	b->SetExtWallsType (Building:: ConcreteWithWindows);
	b->SetNFloors (1);
	b->SetNRoomsX (2);
	b->SetNRoomsY (2);

	Vector pos_ap_1 = GetPosition (ap.Get(0));
	Vector pos_ap_2 = GetPosition (ap.Get(1));
	Vector pos_ap_3 = GetPosition (ap.Get(2));
	Vector pos_ap_4 = GetPosition (ap.Get(3));

	Vector3D sala1 = Vector3D( 1.0,1.0,0.0);	
	Vector3D sala2 = Vector3D( 11.0,1.0,0.0);	
	Vector3D sala3 = Vector3D( 11.0,11.0,0.0);	
	Vector3D sala4 = Vector3D( 1.0,11.0,0.0);	

	// initialize the output file
 	std::ofstream f;
  f.open ("Output.txt", std::ios::out);
  f << "Posicao APs:" <<
  		 "\nAP_1 " << pos_ap_1 <<
       "\nAP_2 " << pos_ap_2 <<
       "\nAP_3 " << pos_ap_3 <<
       "\nAP_4 " << pos_ap_4 <<
       "\n\nSalas:" <<
       "\nSala 1 " << b->GetRoomX(sala1)<<":" << b->GetRoomY(sala1) <<
       "\nSala 2 " << b->GetRoomX(sala2)<<":" << b->GetRoomY(sala2) <<
	     "\nSala 3 " << b->GetRoomX(sala3)<<":" << b->GetRoomY(sala3) <<
	     "\nSala 4 " << b->GetRoomX(sala4)<<":" << b->GetRoomY(sala4) <<  std::endl;
  f.close ();
  
  return 0;
}











