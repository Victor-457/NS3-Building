#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include <iostream>
#include <fstream>
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
            std::cout << "\n" <<txPower <<"\nTXTRACE-------------------------------" << context << std::endl;
            std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
        }
}
static void
SetPosition (Ptr<Node> node, Vector position)
{
    Ptr<ConstantPositionMobilityModel> mobility = node->GetObject<ConstantPositionMobilityModel> ();
    mobility->SetPosition (position);
}

void
SavingRXInfo(std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble)
{   std::string filename = "RXInfo.txt";
    std::ofstream outFile;
    outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::app);
    outFile << Simulator::Now().GetSeconds()
            << "\n"
            << context
            << "\nsnr=" << 10*log10(snr) 
            << "\n" << *packet
            << "\n"
            << std::endl;  
}

void
SavingTXInfo(std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{   std::string filename = "TXInfo.txt";
    std::ofstream outFile;
    outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::app);
    outFile << Simulator::Now().GetSeconds()
            << "\n"
            << context
            << "\n" << *packet
            << "\n"
            << std::endl;  
}


int main (int argc, char *argv[])
{
    Packet::EnablePrinting ();

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

    uint32_t nStas = 13;
    stas.Create (nStas);
    
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (stas);

    //Room1
    SetPosition(stas.Get(0), Vector(0.5, 0.5, 1.0));
    SetPosition(stas.Get(1), Vector(2.0, 2.0, 1.0));

    //Room 2
    SetPosition(stas.Get(2), Vector(0.5, 5.5, 1.0));
    SetPosition(stas.Get(3), Vector(0.5, 3.5, 1.0));
    SetPosition(stas.Get(4), Vector(1.5, 4.5, 1.0));
    SetPosition(stas.Get(5), Vector(2.5, 4.0, 1.0));

    //Room 3
    SetPosition(stas.Get(6), Vector(5.5, 5.5, 1.0));
    SetPosition(stas.Get(7), Vector(2.5, 5.5, 1.0));
    SetPosition(stas.Get(8), Vector(4.0, 5.0, 1.0));
    SetPosition(stas.Get(9), Vector(5.0, 3.5, 1.0));

    //Room 4
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

    BuildingsHelper::Install (stas);


    //Set g_verbose_RX equals true to show
    Config::Connect ("/NodeList/0/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));  
    Config::Connect ("/NodeList/2/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/6/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));        
    Config::Connect ("/NodeList/10/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));   

    //Set g_verbose_TX equals true to show
    Config::Connect ("/NodeList/0/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/2/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/6/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
    Config::Connect ("/NodeList/10/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));

    //Saves RX
    Config::Connect ("/NodeList/1/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/3/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/4/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/5/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/7/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/8/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/9/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/11/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
    Config::Connect ("/NodeList/12/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));  
          
    //Saves TX
    Config::Connect("/NodeList/0/DeviceList/*/Phy/State/Tx", MakeCallback (&SavingTXInfo));
    Config::Connect("/NodeList/2/DeviceList/*/Phy/State/Tx", MakeCallback (&SavingTXInfo));
    Config::Connect("/NodeList/6/DeviceList/*/Phy/State/Tx", MakeCallback (&SavingTXInfo));
    Config::Connect("/NodeList/10/DeviceList/*/Phy/State/Tx", MakeCallback (&SavingTXInfo));

    Simulator::Stop (Seconds (25.0));

    Simulator::Run ();

    Simulator::Destroy ();

    return 0;
}

