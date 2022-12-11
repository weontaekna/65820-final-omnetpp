message LBMessage
{
    int type; // 0 = Req, 1 = Resp
    int source;
    int destination;
    int hopCount = 0;
}
