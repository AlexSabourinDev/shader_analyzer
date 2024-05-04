[[vk::binding(0, 0)]] StructuredBuffer<uint> Input;
[[vk::binding(1, 0)]] globallycoherent RWStructuredBuffer<uint> OutputHistogram;

groupshared uint SharedHistogram[256];
groupshared uint CommittedGroupCount;

static uint const ThreadGroupSize = 1024;
static uint const LimitedExpansion = 32;

[WaveSize(32)]
[numthreads(ThreadGroupSize, 1, 1)]
void CS(uint3 dispatchId : SV_DispatchThreadID,
		uint3 threadGroupId : SV_GroupThreadID,
		uint3 groupId : SV_GroupID)
{
	OutputHistogram[threadGroupId.x] = WaveActiveSum(Input[dispatchId.x]);
}