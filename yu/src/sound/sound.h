namespace yu
{

bool InitSound(float desiredFrameTime = 16.6f);

struct SoundSamples
{
	short*	samples;
	int		numChannels;
};

SoundSamples GetSoundSamples(u64 frame);
void CommitSamples(u64 frame);

}