simple OutsideNode
{
    parameters:
        @display("i=block/routing, red");
        int limit = default(10);
        int ComputeNodeSize = default(12);
        int RackLBNodeSize = default(3);
        int NetworkLBNodeSize = default(1);
        int OutsideNodeSize = default(1);
        double OutsideNodeDelay = default(1.0);
        double ComputeNodeDelay = default(1.0);
    gates:
        inout gate[];
}

simple ComputeNode
{
    parameters:
        @display("i=block/routing");
        int limit = default(10);
        int ComputeNodeSize = default(12);
        int RackLBNodeSize = default(3);
        int NetworkLBNodeSize = default(1);
        int OutsideNodeSize = default(1);
        double OutsideNodeDelay = default(1.0);
        double ComputeNodeDelay = default(1.0);
    gates:
        inout gate[];
}

simple RackLBNode
{
    parameters:
        @display("i=block/routing, cyan");
        int limit = default(10);
        int ComputeNodeSize = default(12);
        int RackLBNodeSize = default(3);
        int NetworkLBNodeSize = default(1);
        int OutsideNodeSize = default(1);
    gates:
        inout gate[];
}

simple NetworkLBNode
{
    parameters:
        @display("i=block/routing, gold");
        int limit = default(10);
        int ComputeNodeSize = default(12);
        int RackLBNodeSize = default(3);
        int NetworkLBNodeSize = default(1);
        int OutsideNodeSize = default(1);
    gates:
        inout gate[];
}

network lb_network
{
    types:
        channel NetworkChannel extends ned.DelayChannel
        {
            delay = 100ms;
        }
        channel RackChannel extends ned.DelayChannel
        {
            delay = 10ms;
        }
    submodules:
        worker_0[4]: ComputeNode;
        worker_1[4]: ComputeNode;
        worker_2[4]: ComputeNode;
        rack_lb[3]: RackLBNode {
            @display("p=189,84");
        }
        network_lb: NetworkLBNode;
        outsider: OutsideNode {
            @display("p=326,178");
        }
    connections:
        for i=0..3 {
            worker_0[i].gate++ <--> RackChannel <--> rack_lb[0].gate++;
            worker_1[i].gate++ <--> RackChannel <--> rack_lb[1].gate++;
            worker_2[i].gate++ <--> RackChannel <--> rack_lb[2].gate++;
        }
        for i=0..2 {
            rack_lb[i].gate++ <--> NetworkChannel <--> network_lb.gate++;
        }
        outsider.gate++ <--> NetworkChannel <--> network_lb.gate++;
}

