#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "cfp-phy-null.h"
#include <cmath>
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/position-allocator.h"
#include "output.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-phy-cfp-class-test");

/*
static void GetChannelNumbers(ApNullPhyNode* node)
{
  node->GetChannelNumbers();
}
*/

static void StartCFP(CFPPhyNull *cfp)
{
  //std::cout << "Starting Contention Free Period at "
  //<< Simulator::Now().GetSeconds()
  //<< std::endl;
  cfp->StartCFP();
}

static void StopCFP(CFPPhyNull *cfp)
{
  //std::cout << "Stopping Contention Free Period at "
  //<< Simulator::Now().GetSeconds()
  //<< std::endl;
  cfp->StopCFP();
}

static void OutputCFP(CFPPhyNull *cfp)
{
  Output output = cfp->OutputCFP();
  std::cout << "Delay (ms): " << output.m_delayMean
  << ", Throughput (Mbps): " << output.m_throughput
  << ", Overhead (ms): " << output.m_overhead << std::endl;
  std::cout << "++++++++++++++++++++" << std::endl;
  std::cout << "++++++++++++++++++++" << std::endl;
  std::cout << "++++++++++++++++++++" << std::endl;
}

int
main (int argc, char *argv[])
{

  for(uint32_t run = 0; run < 10; run++)
  {

    uint32_t nSta = 25; //Number of stationary nodes
    double errorRate = 0.2;
    double simTime = 100;

    RngSeedManager::SetSeed (4);  // Changes seed from default of 1 to 3
    RngSeedManager::SetRun (3);   // Changes run number from default of 1 to 7

    // Enable the packet printing through Packet::Print command.
    Packet::EnablePrinting ();
    WifiPhyStandard standard = WIFI_PHY_STANDARD_80211a;
    uint8_t downlinkChannelNumber = 7;
    uint8_t uplinkChannelNumber = 7;

    Ptr<YansWifiChannel> dlChannel = CreateObject<YansWifiChannel> ();
    dlChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

    Ptr<YansWifiChannel> ulChannel = CreateObject<YansWifiChannel> ();
    ulChannel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());

    Ptr<LogDistancePropagationLossModel> log = CreateObject<LogDistancePropagationLossModel> ();
    dlChannel->SetPropagationLossModel (log);
    ulChannel->SetPropagationLossModel (log);
    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();

    ApNullPhyNode* apNode;
    apNode = new ApNullPhyNode(0, "OfdmRate18Mbps",0, Vector(0,0,0));
    apNode->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
    apNode->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
    apNode->InterferenceDLSetup(errorRate);
    apNode->InterferenceULSetup(errorRate);

    Ptr<RandomDiscPositionAllocator> location = CreateObject<RandomDiscPositionAllocator>();
    location -> SetAttribute("Rho", StringValue("ns3::UniformRandomVariable[Min=0.0|Max=25.0]"));

    std::vector<StaNullPhyNode*> staNodes;
    for(uint32_t i = 0; i < nSta; i++)
    {
      StaNullPhyNode* temp;
      Vector loc = location -> GetNext();
      //std::cout << loc << std::endl;
      temp = new StaNullPhyNode(apNode, i+1, "OfdmRate54Mbps", 0, loc);
      temp->PhyDownlinkSetup(standard, dlChannel, error,downlinkChannelNumber);
      temp->PhyUplinkSetup(standard, ulChannel, error,uplinkChannelNumber, false);
      temp->InterferenceDLSetup(errorRate);
      temp->InterferenceULSetup(errorRate);
      temp->m_ppbp->SetAttribute("MeanBurstArrivals",DoubleValue(200));
      staNodes.push_back(temp);
    }

    std::cout << "Run begins .." << std::endl;

    CFPPhyNull cfp(staNodes,apNode);

    Simulator::Schedule(MilliSeconds(10),&StartCFP, &cfp);
    Simulator::Schedule(MilliSeconds(simTime),&StopCFP, &cfp);
    Simulator::Schedule(MilliSeconds(simTime),&OutputCFP, &cfp);
    Simulator::Stop(MilliSeconds(simTime));
    Simulator::Run();
    Simulator::Destroy();
  }

  return 0;
}
