// Single-player integration does not use the SDK's optional ICMP/DNS benchmark.
void Trace_StartTrace(int* result,int* finished,const char*){*result=-1;*finished=1;}
void Trace_Think(){}
