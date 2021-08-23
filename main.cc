#include "ns3/config.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-address.h"
#include <ns3/buildings-helper.h>
#include "ns3/buildings-module.h"
#include "ns3/mobility-building-info.h"
#include <ns3/hybrid-buildings-propagation-loss-model.h>
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-module.h"
#include "ns3/core-module.h"
#include <ns3/constant-position-mobility-model.h>
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Main");

static bool g_verbose = true;

void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble)
{ 
  if (g_verbose)
    {
      std::cout << "\n\nRXOK-------------------------------" << std::endl;
      std::cout << "PHYRXOK mode=" << mode << " snr=" << 10*log10(snr) << " " << *packet << std::endl;
  
    }
}

void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
  if (g_verbose)
    {
      std::cout << "\n\nTXTRACE-------------------------------" << std::endl;
      std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
    }
}

static void
SetPosition (Ptr<Node> node, Vector position)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  mobility->SetPosition (position);
}

int
main (int argc, char *argv[])
{
  Packet::EnablePrinting ();
  
  uint32_t nAp = 4;
  
  NodeContainer ap; 
  ap.Create (nAp);

  PacketSocketHelper packetSocket;
  packetSocket.Install (ap);
  
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ap);

  SetPosition(ap.Get(0), Vector(1.0, 1.0, 1.0));
  SetPosition(ap.Get(1), Vector(11.0, 1.0, 1.0));
  SetPosition(ap.Get(2), Vector(11.0, 11.0, 1.0));
  SetPosition(ap.Get(3), Vector(1.0, 11.0, 1.0));

  //Building creation
  double x_min = 0.0;
  double x_max = 20.0;
  double y_min = 0.0;
  double y_max = 20.0;
  double z_min = 0.0;
  double z_max = 10.0;
  
  Ptr<Building> b = CreateObject <Building> ();

  b->SetBoundaries (Box(x_min, x_max, y_min, y_max, z_min, z_max));
  b->SetBuildingType (Building::Residential);
  b->SetExtWallsType (Building:: ConcreteWithWindows);
  b->SetNFloors (1);
  b->SetNRoomsX (2);
  b->SetNRoomsY (2);


  BuildingsHelper::Install (ap);
    
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue ("OfdmRate6MbpsBW10MHz"));

  YansWifiChannelHelper wifiChannel= YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  Ptr<HybridBuildingsPropagationLossModel> propagationLossModel = CreateObject<HybridBuildingsPropagationLossModel> ();
  wifiChannel.AddPropagationLoss("ns3::HybridBuildingsPropagationLossModel",
                                 "CitySize", StringValue("Small"),
                                 "ShadowSigmaOutdoor", DoubleValue (10.0),
                                 "ShadowSigmaExtWalls", DoubleValue (10.0),
                                 "InternalWallLoss", DoubleValue (10.0),
                                 "Environment", StringValue("Urban"));

  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());

  WifiMacHelper wifiMac;
  Ssid ssid = Ssid ("wifi-default");
  wifiMac.SetType ("ns3::ApWifiMac",
      "Ssid", SsidValue (ssid),
      "BeaconInterval", TimeValue (MicroSeconds (2560000.0)));
    
  WifiHelper wifi;
  
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");

  wifi.Install (wifiPhy, wifiMac, ap);
  

  Simulator::Stop (Seconds (25.0));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}