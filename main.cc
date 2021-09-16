#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <iostream>
#include <ns3/buildings-module.h>
#include <ns3/buildings-helper.h>
#include <ns3/hybrid-buildings-propagation-loss-model.h>
#include <ns3/constant-position-mobility-model.h>
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include <../scratch/object-base.h>
#include "ns3/internet-module.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-address.h"

using namespace ns3;

static bool g_verbose_RX = false;
static bool g_verbose_TX = true;

void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble)
{   
    if (g_verbose_RX)
        {
            std::cout << "\n\nRXOK-------------------------------" << context << std::endl;
            std::cout << "PHYRXOK mode=" << mode << " snr=" << 10*log10(snr) << " " << *packet << std::endl;   
        }
}

void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
    if (g_verbose_TX)
        {
            std::cout << "\n\nTXTRACE-------------------------------" << context << std::endl;
            std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
        }
}
static void
SetPosition (Ptr<Node> node, Vector position)
{
    Ptr<ConstantPositionMobilityModel> mobility = node->GetObject<ConstantPositionMobilityModel> ();
    mobility->SetPosition (position);
}

int main (int argc, char *argv[])
{
    CommandLine cmd;

    cmd.Parse (argc, argv);

    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);


    //Building creation
    double x_min = 0.0;
    double x_max = 6.0;
    double y_min = 0.0;
    double y_max = 6.0;
    double z_min = 0.0;
    double z_max = 3.0;

    Ptr<Building> b = CreateObject <Building> ();

    b->SetBoundaries (Box(x_min, x_max, y_min, y_max, z_min, z_max));
    b->SetBuildingType (Building::Residential);
    b->SetExtWallsType (Building:: ConcreteWithWindows);
    b->SetNFloors (1);
    b->SetNRoomsX (2);
    b->SetNRoomsY (2);

    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
    Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));

    NodeContainer stas;

    Ipv4InterfaceContainer STAIfaceContainer;

    uint32_t nStas = 13;
    stas.Create (nStas);

    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (stas);

    //vermelho
    SetPosition(stas.Get(0), Vector(0.5, 0.5, 1.0));
    SetPosition(stas.Get(1), Vector(2.0, 2.0, 1.0));

    //azul
    SetPosition(stas.Get(2), Vector(0.5, 5.5, 1.0));
    SetPosition(stas.Get(3), Vector(0.5, 3.5, 1.0));
    SetPosition(stas.Get(4), Vector(1.5, 4.5, 1.0));
    SetPosition(stas.Get(5), Vector(2.5, 4.0, 1.0));

    //laranja
    SetPosition(stas.Get(6), Vector(5.5, 5.5, 1.0));
    SetPosition(stas.Get(7), Vector(2.5, 5.5, 1.0));
    SetPosition(stas.Get(8), Vector(4.0, 5.0, 1.0));
    SetPosition(stas.Get(9), Vector(5.0, 3.5, 1.0));

    //verde
    SetPosition(stas.Get(10), Vector(5.5, 0.5, 1.0));
    SetPosition(stas.Get(11), Vector(3.5, 1.5, 1.0));
    SetPosition(stas.Get(12), Vector(5.0, 2.5, 1.0));
    
    

    WifiHelper wifi;

    WifiMacHelper wifiMac;
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;

    wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

    wifiChannel.AddPropagationLoss ("ns3::HybridBuildingsPropagationLossModel",
                                    "CitySize", StringValue("Small"),
                                    "ShadowSigmaOutdoor", DoubleValue (7.0),
                                    "ShadowSigmaIndoor", DoubleValue (5.0),
                                    "ShadowSigmaExtWalls", DoubleValue (5.0),
                                    "InternalWallLoss", DoubleValue (5.0),
                                    "Environment", StringValue("Urban"));

    wifiPhy.SetChannel (wifiChannel.Create ());
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

    Ssid ssid = Ssid ("wifi-default");
    wifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "BeaconInterval", TimeValue (MicroSeconds (25600000.0)));

    NetDeviceContainer staDevs;
    staDevs = wifi.Install (wifiPhy, wifiMac, stas);

    InternetStackHelper internet;
    internet.Install (stas);


    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    STAIfaceContainer = ipv4.Assign (staDevs);

    BuildingsHelper::Install (stas);

    //BuildingsHelper::MakeMobilityModelConsistent();


    UdpServerHelper Server (9); 

    ApplicationContainer serverApps;
    ApplicationContainer clientApps;
    
    serverApps.Add (Server.Install (stas.Get (7)));

    UdpClientHelper Client0 (STAIfaceContainer.GetAddress (1), 9);

    Client0.SetAttribute ("MaxPackets", UintegerValue (20));
    Client0.SetAttribute ("Interval", TimeValue (Seconds (0.001)));
    Client0.SetAttribute ("PacketSize", UintegerValue (64)); 

    clientApps.Add(Client0.Install (stas.Get (6))); 

    serverApps.Add (Server.Install (stas.Get (4)));

    UdpClientHelper Client1 (STAIfaceContainer.GetAddress (2), 9);

    Client1.SetAttribute ("MaxPackets", UintegerValue (20));
    Client1.SetAttribute ("Interval", TimeValue (Seconds (0.001)));
    Client1.SetAttribute ("PacketSize", UintegerValue (64)); 

    clientApps.Add(Client1.Install (stas.Get (2))); 
    
    //Set g_verbose_RX equals true to show
    Config::Connect ("/NodeList/0/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));  
    Config::Connect ("/NodeList/1/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
    Config::Connect ("/NodeList/2/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/3/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
    Config::Connect ("/NodeList/4/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));       
    Config::Connect ("/NodeList/5/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/6/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/7/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
    Config::Connect ("/NodeList/8/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/9/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/10/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/11/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
    Config::Connect ("/NodeList/12/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));

    //Set g_verbose_TX equals true to show
    Config::Connect ("/NodeList/0/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/1/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/2/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/3/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/4/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/5/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/6/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/7/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/8/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/9/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/10/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/11/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/12/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));

    serverApps.Start (Seconds (0.5));
    serverApps.Stop (Seconds (25.0));

    serverApps.Get(0)->SetStartTime(Seconds(1));
    serverApps.Get(0)->SetStopTime(Seconds(10));

    clientApps.Start (Seconds (0.5));
    clientApps.Stop (Seconds (25.0));

    clientApps.Get(0)->SetStartTime(Seconds(0.5));
    clientApps.Get(0)->SetStopTime(Seconds(25.0));



    Simulator::Stop (Seconds (25.0));

    Simulator::Run ();

    Simulator::Destroy ();

    return 0;
}

