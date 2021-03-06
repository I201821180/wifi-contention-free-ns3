#include "ns3/core-module.h"
#include "ns3/simulator.h"
#include "ns3/applications-module.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/random-variable-stream.h"

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED (PhyNode);

PhyNode::PhyNode ()
{
  // we must provide a public default constructor,
  // implicit or explicit, but never private.
}

PhyNode::~PhyNode ()
{
}

TypeId
PhyNode::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PhyNode")
    .SetParent<Object> ()
    .AddConstructor<PhyNode> ()
		.AddAttribute ("WiFiTxMode", "WifiMode for transmission",
					   StringValue ("OfdmRate6Mbps"),
					   MakeStringAccessor (&PhyNode::m_txMode),
					   MakeStringChecker())
    .AddAttribute ("TXPowerLevel", "The transmission power level",
  					 UintegerValue (0),
  					 MakeUintegerAccessor (&PhyNode::m_txPowerLevel),
  					 MakeUintegerChecker<uint8_t> (1))
    .AddAttribute ("NodeID", "The identifier of PhyNode",
					   UintegerValue (0),
					   MakeUintegerAccessor (&PhyNode::m_nodeId),
					   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("IntfErrorRateDL", "Interference error rate in downlink",
            DoubleValue (0.000005),
            MakeDoubleAccessor (&PhyNode::m_intfErrorRateDL),
            MakeDoubleChecker<double> ())
    .AddAttribute ("IntfErrorRateUL", "Interference error rate in uplink",
            DoubleValue (0.000005),
            MakeDoubleAccessor (&PhyNode::m_intfErrorRateUL),
            MakeDoubleChecker<double> ())
  ;
  return tid;
}

PhyNode::PhyNode (uint32_t nodeId, std::string txMode, uint8_t txPowerLevel,
                  Vector loc)
  : m_loc (loc),
    m_txMode (txMode),
    m_txPowerLevel(txPowerLevel),
    m_nodeId(nodeId)
{
   m_position->SetPosition (loc);

   WifiMode mode = WifiMode (txMode);
   WifiTxVector txVector;
   txVector.SetTxPowerLevel (txPowerLevel);
   txVector.SetMode (mode);
   txVector.SetPreambleType (WIFI_PREAMBLE_LONG);

   m_datarate = mode.GetDataRate(txVector);
}

void PhyNode::PhyDownlinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0)
{
  m_dlChannelNumber = channelNumber;
  m_dl->SetChannel(channel);
  m_dl->SetErrorRateModel (error);
  m_dl->SetChannelNumber(channelNumber);
  m_dl->SetMobility(m_position);

  m_dl->ConfigureStandard(standard);
}

void PhyNode::PhyUplinkSetup(WifiPhyStandard standard, Ptr<YansWifiChannel> channel,
                Ptr<ErrorRateModel> error, uint8_t channelNumber = 0, bool multiband = false)
{
  if(multiband)
  {
    m_ul->ConfigureStandard(standard);
    m_ul->SetChannel(channel);
    m_ul->SetChannelNumber(channelNumber);

    m_ul->SetErrorRateModel (error);
    m_ul->SetMobility(m_position);
  }
  else
  {
    m_ulChannelNumber = m_dlChannelNumber;
    m_ul = m_dl;
  }

}

void PhyNode::InterferenceDLSetup(double rate)
{
  m_rvDL->SetAttribute ("Min", DoubleValue (0));
  m_rvDL->SetAttribute ("Max", DoubleValue (1));
  m_intfErrorRateDL = rate;
  m_remDL->SetRandomVariable (m_rvDL);
  m_remDL->SetRate (m_intfErrorRateDL);
  m_remDL->SetAttribute("ErrorUnit",EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
}

void PhyNode::InterferenceULSetup(double rate)
{
  m_rvUL->SetAttribute ("Min", DoubleValue (0));
  m_rvUL->SetAttribute ("Max", DoubleValue (1));
  m_intfErrorRateUL = rate;
  m_remUL->SetRandomVariable (m_rvUL);
  m_remUL->SetRate (m_intfErrorRateUL);
  m_remUL->SetAttribute("ErrorUnit",EnumValue(RateErrorModel::ERROR_UNIT_PACKET));
}

void
PhyNode::Send (Ptr<YansWifiPhy> m_tx, uint32_t nodeId, uint32_t packetSize)
{
  Ptr<Packet> p = Create<Packet>(packetSize);

  BasicHeader sourceHeader;
  sourceHeader.SetData (nodeId);
  p->AddHeader (sourceHeader);

  WifiMode mode = WifiMode (m_txMode);
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetMode (mode);
  txVector.SetPreambleType (WIFI_PREAMBLE_LONG);

  m_tx->SendPacket (p, txVector);

  Time txDuration = m_tx -> CalculateTxDuration(packetSize, txVector,
                              m_tx->GetFrequency());
  //std::cout << "TX duration: " << txDuration.GetMicroSeconds() << std::endl;
}

void
PhyNode::Send (Ptr<YansWifiPhy> m_tx, uint32_t nodeId, uint32_t packetSize,
              std::string msg)
{
  std::stringstream msgx;
  Ptr<Packet> packet;

  msgx << msg;
  uint32_t msgSize = msg.length();
  //std::cout << "Message size = " << msgSize << std::endl;
  packet = Create<Packet>((uint8_t*) msgx.str().c_str(), packetSize);
  if(msgSize < packetSize)
  {
    packet->AddPaddingAtEnd(packetSize - msgSize);
  }

  BasicHeader sourceHeader;
  sourceHeader.SetData (nodeId);
  packet->AddHeader (sourceHeader);

  WifiMode mode = WifiMode (m_txMode);
  WifiTxVector txVector;
  txVector.SetTxPowerLevel (m_txPowerLevel);
  txVector.SetMode (mode);
  txVector.SetPreambleType (WIFI_PREAMBLE_SHORT);

  m_tx->SendPacket (packet, txVector);

  Time txDuration = m_tx -> CalculateTxDuration(packetSize, txVector,
                              m_tx->GetFrequency());
  //std::cout << "TX duration: " << txDuration.GetMicroSeconds() << std::endl;
}

void PhyNode::GetChannelNumbers()
{
  std::cout << "Downlink Channel Number = "
  << (double) (m_dl -> GetChannelNumber())
  << " and Uplink Channel Number = "
  << (double) (m_ul -> GetChannelNumber())
  << std::endl;
}
