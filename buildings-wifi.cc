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
#include <ns3/object-base.h>
#include "ns3/internet-module.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-address.h"

using namespace ns3;

static bool verbose = false;
int nbeacons;
int nxrooms;
int nyrooms;
int nstasroom;
int run;
double xroomsize;
double yroomsize;
std::string outdir = "output";
std::string tracef;
std::ofstream outFile;
NetDeviceContainer apDevs;
NetDeviceContainer staDevs;

	static void
SetPosition (Ptr<Node> node, Vector position)
{
	Ptr<ConstantPositionMobilityModel> mobility = node->GetObject<ConstantPositionMobilityModel> ();
	mobility->SetPosition (position);
}

	void
SavingRXInfo(std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, WifiPreamble preamble)
{
	// Get Rx
	std::string s = context;
	s.erase(0,10);
	size_t pos = s.find("/");
	int rx = std::stoi(s.substr(0, pos));
	//std::cout << "RX: " << s.substr(0, pos) << std::endl;
	
	int naps = nxrooms*nyrooms;
	Mac48Address rxmac;
	if (rx < naps)
		rxmac = DynamicCast<WifiNetDevice>(apDevs.Get(rx))->GetMac()->GetAddress();
	else
		rxmac = DynamicCast<WifiNetDevice>(staDevs.Get(rx-naps))->GetMac()->GetAddress();

	// Get Tx
	WifiMacHeader head;
	packet->PeekHeader (head);
	Mac48Address src = head.GetAddr2 ();  
	//std::cout << "pkt: " << src << std::endl;

	//std::string filename = "Rgfhfo.txt";
	//std::ofstream outFile;
	//outFile.open (filename.c_str (), std::ios_base::out | std::ios_base::app);
	outFile << Simulator::Now().GetSeconds()
		<< "\t"	<< src   //TX
		<< "\t" << rxmac   //RX
		<< "\t" << 10*log10(snr) //SNR
		<< std::endl; 

	if (verbose)	
		std::cout << Simulator::Now().GetSeconds()
		<< "\t"	<< src   //TX
		<< "\t" << rxmac   //RX
		<< "\t" << 10*log10(snr) //SNR
		<< std::endl; 
}

	void
SavingTXInfo(std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{   
	std::string filename = "TXInfo.txt";
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
	nbeacons = 1;
	nxrooms = 2;
	nyrooms = 2;
	nstasroom = 1;
	xroomsize = 3.0;
	yroomsize = 3.0;
	run = 1;

	CommandLine cmd (__FILE__);
	cmd.AddValue ("verbose", "Print trace information if true", verbose);
	cmd.AddValue ("outdir", "Output directory name", outdir);
	cmd.AddValue ("nbeacons", "Number of beacons sent by each AP", nbeacons);
	cmd.AddValue ("nxrooms", "Number of rooms in x axis", nxrooms);
	cmd.AddValue ("nyrooms", "Number of rooms in y axis", nyrooms);
	cmd.AddValue ("nstasroom", "Number of STAs per room", nstasroom);
	cmd.AddValue ("xroomsize", "Size of room in x axis", xroomsize);
	cmd.AddValue ("yroomsize", "Size of room in y axis", yroomsize);
	cmd.AddValue ("run", "Run number", run);

	cmd.Parse (argc, argv);

	std::stringstream ss;
	ss << outdir << "/trace-" << nbeacons << "-" << nxrooms << "-" << nyrooms << "-" << nstasroom << "-" << xroomsize << "-" << yroomsize << "-" << run;
	tracef = ss.str();
	outFile.open (tracef.c_str ());

	RngSeedManager::SetSeed (10);
	RngSeedManager::SetRun (run);

	Packet::EnablePrinting ();

	//Building creation
	double x_min = 0.0;
	double x_max = nxrooms*xroomsize;
	double y_min = 0.0;
	double y_max = nyrooms*yroomsize;
	double z_min = 0.0;
	double z_max = 3.0;

	Ptr<Building> b = CreateObject <Building> ();

	b->SetBoundaries (Box(x_min, x_max, y_min, y_max, z_min, z_max));
	b->SetBuildingType (Building::Residential);
	b->SetExtWallsType (Building:: ConcreteWithWindows);
	b->SetNFloors (1);
	b->SetNRoomsX (nxrooms);
	b->SetNRoomsY (nyrooms);

	NodeContainer aps;
	int nrooms = nxrooms*nyrooms;
	aps.Create (nrooms);

	NodeContainer stas;
	int nstas = nstasroom*nrooms;
	stas.Create (nstas);

	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobility.Install (aps);
	mobility.Install (stas);

	//placeAPs(aps);
	Ptr<UniformRandomVariable> xrng = CreateObject<UniformRandomVariable> ();
	Ptr<UniformRandomVariable> yrng = CreateObject<UniformRandomVariable> ();
	int apid = 0;
	int staid = 0;
	for (int x = 0; x < nxrooms; x++) {
		xrng->SetAttribute ("Min", DoubleValue (x*xroomsize));
		xrng->SetAttribute ("Max", DoubleValue ((x+1)*xroomsize));
		for (int y = 0; y < nyrooms; y++) {
			yrng->SetAttribute ("Min", DoubleValue (y*yroomsize));
			yrng->SetAttribute ("Max", DoubleValue ((y+1)*yroomsize));
			double xpos = xrng->GetValue();
			double ypos = yrng->GetValue();
			SetPosition(aps.Get(apid), Vector(xpos, ypos, 1.0));
			if (verbose) std::cout << x << " " << y << " " << xpos << " " << ypos << std::endl;
			for (int n = 0; n < nstasroom; n++) {
				double xpos = xrng->GetValue();
				double ypos = yrng->GetValue();
				SetPosition(stas.Get(apid), Vector(xpos, ypos, 1.0));
				if (verbose) std::cout << x << " " << y << " " << n << " " << xpos << " " << ypos << std::endl;
				staid++;
			}
			apid++;
		}
	
	}

	WifiHelper wifi;

	WifiMacHelper wifiMac;
	YansWifiPhyHelper wifiPhy;
	YansWifiChannelHelper wifiChannel;

	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

	wifiChannel.AddPropagationLoss ("ns3::HybridBuildingsPropagationLossModel",
			"CitySize", StringValue("Small"),
			"ShadowSigmaOutdoor", DoubleValue (7.0),
			"ShadowSigmaIndoor", DoubleValue (8.0),
			"ShadowSigmaExtWalls", DoubleValue (5.0),
			"InternalWallLoss", DoubleValue (5.0),
			"Environment", StringValue("Urban"));

	wifiPhy.SetChannel (wifiChannel.Create ());
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager");

	Ssid ssid = Ssid ("wifi-default");
	wifiMac.SetType ("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid));
			//"BeaconInterval", TimeValue (MicroSeconds (25600000.0)));
	apDevs = wifi.Install (wifiPhy, wifiMac, aps);

	wifiMac.SetType ("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid),
			"BeaconGeneration", BooleanValue(false));
	staDevs = wifi.Install (wifiPhy, wifiMac, stas);

	BuildingsHelper::Install (aps);
	BuildingsHelper::Install (stas);

	if (verbose) {
		for (uint32_t i = 0; i < apDevs.GetN(); i++) {
			std::cout << "AP" << i << " " << DynamicCast<WifiNetDevice>(apDevs.Get(i))->GetMac()->GetAddress() << std::endl;
		}

		for (uint32_t i = 0; i < staDevs.GetN(); i++) {
			std::cout << "STA" << i << " " << DynamicCast<WifiNetDevice>(staDevs.Get(i))->GetMac()->GetAddress() << std::endl;
		}
	}

	//Saves RX
	Config::Connect("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback (&SavingRXInfo));        
	//Saves TX
	//Config::Connect("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&SavingTXInfo));


	Simulator::Stop (MicroSeconds(1.024e+05*nbeacons));
	//Simulator::Stop (Seconds (1.0));

	Simulator::Run ();

	Simulator::Destroy ();

	return 0;
}

