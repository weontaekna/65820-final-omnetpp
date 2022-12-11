#include <stdio.h>
#include <string.h>
#include <omnetpp.h>

using namespace omnetpp;

#include "lb_message_m.h"

#define REQ  0 
#define RESP 1

class OutsideNode: public cSimpleModule
{
  private:
    int limit;
    int ComputeNodeSize;
    int RackLBNodeSize;
    int NetworkLBNodeSize;
    int OutsideNodeSize;
    double OutsideNodeDelay;
    cMessage *event = nullptr;  
    LBMessage *msgStore = nullptr;  

  public:
    virtual ~OutsideNode();

  protected:
    virtual LBMessage *generateMessage();
    virtual void forwardMessage(LBMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(OutsideNode);

void OutsideNode::initialize()
{
    event = new cMessage("event");
    msgStore = nullptr;
    limit = par("limit");
    ComputeNodeSize = par("ComputeNodeSize");
    OutsideNodeDelay = par("OutsideNodeDelay");

    EV << "Scheduling first send to t=5.0s\n";
    msgStore = generateMessage();
    scheduleAt(0.0, event);
}

OutsideNode::~OutsideNode()
{
    //cancelAndDelete(event);
    delete msgStore;
}

LBMessage *OutsideNode::generateMessage()
{
    int dest = intuniform(0, ComputeNodeSize-1);

    char msgname[20];
    sprintf(msgname, "req-to-worker%d", dest);

    // Create message object and set source and destination field.
    LBMessage *msg = new LBMessage(msgname);
    msg->setType(REQ);
    msg->setDestination(dest);
    msg->setHopCount(0);
    return msg;
}

void OutsideNode::forwardMessage(LBMessage *msg)
{
    // Increment hop count.
    msg->setHopCount(msg->getHopCount()+1);

    EV << "Forwarding message " << msg << " on the only gate\n";
    send(msg, "gate$o", 0);
}

void OutsideNode::handleMessage(cMessage *msg)
{
    // scheduled event to send a new message.
    if (msg == event) {
        limit--;
        // The self-message arrived, so we can send out msgStore and nullptr out
        // its pointer so that it doesn't confuse us later.
        EV << "Wait period is over, sending back message\n";
        forwardMessage(msgStore);
        msgStore = nullptr;
        
        if(limit==0) {
            EV << getName() << "'s counter reached zero, deleting message\n";
            delete msg;
        }
        else {
            EV << "Sending another message, starting to wait 1 sec...\n";
            // generate new message.
            LBMessage *newMsg = generateMessage();
            msgStore = newMsg;
            scheduleAt(simTime()+OutsideNodeDelay, event);
        }
    }
    // interrupted event due to arriving response message.
    else {
        LBMessage *lbMsg = check_and_cast<LBMessage*>(msg);
        delete lbMsg; 
    }
}

//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

class NetworkLBNode: public cSimpleModule
{
  private:
    int limit;
    int ComputeNodeSize;
    int RackLBNodeSize;
    int NetworkLBNodeSize;
    int OutsideNodeSize;
    cMessage *event = nullptr;  
    LBMessage *msgStore = nullptr;  

  public:
    virtual ~NetworkLBNode();

  protected:
    //virtual LBMessage *generateMessage();
    //virtual void forwardMessage(LBMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(NetworkLBNode);

void NetworkLBNode::initialize()
{
    event = nullptr;
    msgStore = nullptr;
    ComputeNodeSize = par("ComputeNodeSize");
    RackLBNodeSize = par("RackLBNodeSize");
}

NetworkLBNode::~NetworkLBNode()
{
    cancelAndDelete(event);
    delete msgStore;
}



void NetworkLBNode::handleMessage(cMessage *msg)
{
    LBMessage *lbMsg = check_and_cast<LBMessage*>(msg);
    
    if(lbMsg->getType() == REQ) { // Request sent from outside node
        int currentWorkerDest = lbMsg->getDestination();
        int newWorkerDest = intuniform(0, ComputeNodeSize-1);
        lbMsg->setDestination(newWorkerDest);
        lbMsg->setHopCount(lbMsg->getHopCount()+1);

        int workersPerRack = ComputeNodeSize/RackLBNodeSize;
        int rackDest = newWorkerDest/workersPerRack;
        EV << "Forwarding message " << lbMsg << " on gate[" << rackDest 
           << "] (to worker " << newWorkerDest << " )\n";
        send(lbMsg, "gate$o", rackDest);
    } 
    else {
        lbMsg->setHopCount(lbMsg->getHopCount()+1);
        // gate 0, 1, 2 <-> RackLBNode, gate 3(=RackLBNodeSize) <-> OutsideNode
        EV << "Forwarding message " << lbMsg << " on gate[" << RackLBNodeSize 
           << "] (to OutsideNode )\n";
        send(lbMsg, "gate$o", RackLBNodeSize); 
    }
}

//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////


class RackLBNode : public cSimpleModule
{
  private:
    int limit;
    int ComputeNodeSize;
    int RackLBNodeSize;
    int NetworkLBNodeSize;
    int OutsideNodeSize;
    cMessage *event = nullptr;  
    LBMessage *msgStore = nullptr;  

  public:
    virtual ~RackLBNode();

  protected:
    //virtual LBMessage *generateMessage();
    //virtual void forwardMessage(LBMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

Define_Module(RackLBNode);

void RackLBNode::initialize()
{
    event = nullptr;
    msgStore = nullptr;
    ComputeNodeSize = par("ComputeNodeSize");
    RackLBNodeSize = par("RackLBNodeSize");
}


RackLBNode::~RackLBNode()
{
    cancelAndDelete(event);
    delete msgStore;
}


void RackLBNode::handleMessage(cMessage *msg)
{
    LBMessage *lbMsg = check_and_cast<LBMessage*>(msg);
    int workersPerRack = ComputeNodeSize/RackLBNodeSize;
    
    if(lbMsg->getType() == REQ) { // Request sent from NetworkLBNode 
        int currentWorkerDest = lbMsg->getDestination();
        int newWorkerDest = intuniform(0, ComputeNodeSize-1);
        lbMsg->setDestination(newWorkerDest);
        lbMsg->setHopCount(lbMsg->getHopCount()+1);

        int rackDest = newWorkerDest/workersPerRack;

        int workerDestWithinRack = newWorkerDest - rackDest * workersPerRack; 
        EV << "Forwarding message " << lbMsg << " on gate[" << workerDestWithinRack 
           << "] (to worker [" << newWorkerDest << "] )\n";
        send(lbMsg, "gate$o", workerDestWithinRack);
    } 
    else {
        lbMsg->setHopCount(lbMsg->getHopCount()+1);
        EV << "Forwarding message " << lbMsg << " on gate[" << workersPerRack
           << "] (to  networkLBNode\n";
        // gate 0, 1, 2, 3 <-> ComputeNode, gate 4(=workersPerRack) <-> OutsideNode
        send(lbMsg, "gate$o", workersPerRack); 
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class ComputeNode: public cSimpleModule
{
  private:
    int limit;
    int ComputeNodeSize;
    int RackLBNodeSize;
    int NetworkLBNodeSize;
    double OutsideNodeSize;
    double ComputeNodeDelay;
    cMessage *event;
    cQueue *msgQueue;
    cHistogram queueLengthStats;
    cOutVector queueLengthVector;

  public:
    virtual ~ComputeNode();
    static int myCompareFunc(cObject *a, cObject *b);

  protected:
    virtual LBMessage *generateResponse(LBMessage *req);
    virtual void forwardMessage(LBMessage *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

Define_Module(ComputeNode);

int ComputeNode::myCompareFunc(cObject *a, cObject *b)
{
    return 0;
}

void ComputeNode::initialize()
{
    msgQueue = new cQueue("msgQueue", myCompareFunc);
    event = new cMessage("event");

    ComputeNodeSize = par("ComputeNodeSize");
    RackLBNodeSize = par("RackLBNodeSize");
    ComputeNodeDelay = par("ComputeNodeDelay");

    queueLengthStats.setName("queueLengthStats");
    queueLengthVector.setName("queueLengthVector");
}

ComputeNode::~ComputeNode()
{
    cancelAndDelete(event);
    delete msgQueue;
}

LBMessage *ComputeNode::generateResponse(LBMessage *req)
{
    char msgname[20];
    sprintf(msgname, "resp-from-worker%d", req->getDestination());

    // Create message object and set source and destination field.
    LBMessage *msg = new LBMessage(msgname);
    msg->setType(RESP);
    msg->setSource(req->getDestination());
    msg->setHopCount(0);
    return msg;
}

void ComputeNode::forwardMessage(LBMessage *msg)
{
    // Increment hop count.
    msg->setHopCount(msg->getHopCount()+1);

    EV << "Forwarding message " << msg << " on the only gate\n";
    send(msg, "gate$o", 0);
}

void ComputeNode::handleMessage(cMessage *msg)
{

    queueLengthVector.record(msgQueue->getLength());
    queueLengthStats.collect(msgQueue->getLength());

    if (msg == event) {
        // The self-message arrived, so we can send out lbMsg and nullptr out
        // its pointer so that it doesn't confuse us later.
        EV << "Wait period is over, sending back message\n";
        cObject *tempMsg = msgQueue->pop();
        LBMessage *respMsg = check_and_cast<LBMessage*>(tempMsg);
        forwardMessage(respMsg);

        if(msgQueue->isEmpty()) {}
        else {
            scheduleAt(simTime()+ComputeNodeDelay, event);
        }
    }
    // interrupted event due to arriving request message.
    else {
        LBMessage *lbMsg = check_and_cast<LBMessage*>(msg);
        EV << "Message arrived, starting to wait 1 sec...\n";
        LBMessage *newResp = generateResponse(lbMsg);

        if(msgQueue->isEmpty()) {
            scheduleAt(simTime()+ComputeNodeDelay, event);
        }
        msgQueue->insert(newResp);
    }
}

void ComputeNode::finish()
{
    queueLengthStats.recordAs("msg queue length");
}

