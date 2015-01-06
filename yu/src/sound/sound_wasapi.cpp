#include "../core/log.h"
#include "../core/thread.h"
#include "../core/timer.h"
#include "../math/yu_math.h"
#include "sound.h"
#include <math.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
//#include <AudioPolicy.h>
#include <functiondiscoverykeys.h>
#include <Mmsystem.h>
#include <avrt.h>
#pragma comment(lib, "avrt.lib")
#define SAFE_RELEASE(p) \
				{\
		ULONG testRef = 0;\
		if(p)\
			testRef = p->Release();\
		p = 0;\
				}

namespace yu
{
struct SoundDeviceWasapi
{
	IMMDevice *immDevice;
	IAudioClient *audioClient;
	IAudioRenderClient *renderClient;

	HANDLE	audioSamplesReadyEvent;

	short	*soundBuffer;
	size_t	soundBufferSize;

	WAVEFORMATEX* mixFormat;

	int		latencyMs = 10;
	unsigned int framesToWrite;

	int		targetBitsPerSample = 16;
	int		targetFreq = 48000;

	bool	preferExclusive = false;
};
static SoundDeviceWasapi gSoundDevice;
SoundSamples gFrameSamples[2];

static Thread audioThread;
bool YuRunning();

#define Pi32 3.14159265359f
ThreadReturn ThreadCall AudioThreadFunc(ThreadContext context)
{
	Log("audio thread started\n");
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	timeBeginPeriod(1);
	if (FAILED(hr))
	{
		Log("Unable to initialize COM in render thread: %x\n", hr);
		return hr;
	}
	
	HANDLE mmcssHandle = NULL;
	DWORD mmcssTaskIndex = 0;

	mmcssHandle = AvSetMmThreadCharacteristicsA("Pro Audio", &mmcssTaskIndex);
	if (mmcssHandle == NULL)
	{
		Log("Unable to enable MMCSS on render thread: %d\n", GetLastError());
	}

	SoundDeviceWasapi* soundDevice = (SoundDeviceWasapi*)context;
	HANDLE waitArray[] = { gSoundDevice.audioSamplesReadyEvent };
	static float sinT = 0.f;
	float midTone = 440.f;


	soundDevice->soundBufferSize = soundDevice->framesToWrite * (soundDevice->mixFormat->wBitsPerSample / 8) * soundDevice->mixFormat->nChannels;
	soundDevice->soundBuffer = new short[soundDevice->framesToWrite * soundDevice->mixFormat->nChannels];
	

	hr = gSoundDevice.audioClient->Start();

	if (FAILED(hr))
	{
		Log("Unable to start render client: %x.\n", hr);
		return false;
	}
	UINT framesToWrite;
	soundDevice->audioClient->GetBufferSize(&framesToWrite);

	REFERENCE_TIME refTime;
	soundDevice->audioClient->GetStreamLatency(&refTime);

	UINT latencyMs = UINT(refTime * 100 / 1000000);
	UINT maxLatencyMs = max(latencyMs, UINT(gSoundDevice.latencyMs));
	UINT minLatencyMs = min(latencyMs, UINT(gSoundDevice.latencyMs));

	unsigned int Packet = (soundDevice->mixFormat->nSamplesPerSec * maxLatencyMs / 1000);
	while (YuRunning())
	{
		HRESULT hr;
		PerfTimer waitTimer;
		waitTimer.Start();
		DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, minLatencyMs / 2);
		waitTimer.Finish();

		//Log("audio frame wait time: %lf\n", waitTimer.DurationInMs());
		UINT numFramesPadding;

		BYTE* bufferToWrite;
		UINT framesAvailable;
		switch (waitResult) 
		{
		case WAIT_OBJECT_0:
			break;
		case WAIT_TIMEOUT:
	
			hr = soundDevice->audioClient->GetCurrentPadding(&numFramesPadding);
			if (FAILED(hr))
			{
				//Log("Failed to get padding: %x.\n", hr);
				continue;
			}
			//Log("audio buffer padding: %d\n", numFramesPadding);
			assert(framesToWrite >= numFramesPadding);
			framesAvailable = framesToWrite - numFramesPadding;

			if (framesAvailable < Packet)
				continue;

			hr = soundDevice->renderClient->GetBuffer(Packet, &bufferToWrite);


			if (SUCCEEDED(hr))
			{
				short* samples = (short*)bufferToWrite;
				for (unsigned int i = 0; i < Packet; i++)
				{
					float period = soundDevice->mixFormat->nSamplesPerSec / midTone;
					float sound = sinf(sinT);

					*samples++ = short(sound * 256.f);
					*samples++ = short(sound * 256.f);

					sinT += (2.f * Pi32 / (float)period);

					if (sinT > 2.f * Pi32)
					{
						sinT -= 2.f * Pi32;
					}
				}
				hr = soundDevice->renderClient->ReleaseBuffer(Packet, 0);
				if (!SUCCEEDED(hr))
				{
					Log("Unable to release buffer: %x\n", hr);
				}
			
			}
			else
			{
				hr = soundDevice->renderClient->ReleaseBuffer(Packet, 0);
				if (!SUCCEEDED(hr))
				{
					Log("Unable to release buffer: %x\n", hr);
				}
			}

			break;
		default:
			Log("error, audio thread wait failed\n");
			//assert(0);
			break;
		}

	}
	return 0;
}

bool ConvertToSupportFormat(SoundDeviceWasapi* soundDevice, WAVEFORMATEX* mixFormat, AUDCLNT_SHAREMODE shareMode)
{
	HRESULT hr;

	mixFormat->wBitsPerSample = soundDevice->targetBitsPerSample;
	mixFormat->nBlockAlign = (mixFormat->wBitsPerSample / 8) * mixFormat->nChannels;
	mixFormat->nAvgBytesPerSec = mixFormat->nBlockAlign * mixFormat->nSamplesPerSec;

	bool formatSupported = false;
	WAVEFORMATEX* closestMatchedFormat;
	hr = gSoundDevice.audioClient->IsFormatSupported(shareMode, mixFormat, &closestMatchedFormat);

	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
	{
		Log("Device does not natively support the mix format, converting to PCM.\n");
		//
		//  If the mix format is a float format, just try to convert the format to PCM.
		//
		if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		{
			mixFormat->wFormatTag = WAVE_FORMAT_PCM;
			mixFormat->wBitsPerSample = soundDevice->targetBitsPerSample;
			mixFormat->nBlockAlign = (mixFormat->wBitsPerSample / 8) * mixFormat->nChannels;
			mixFormat->nAvgBytesPerSec = mixFormat->nSamplesPerSec*mixFormat->nBlockAlign;
		}
		else if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			WAVEFORMATEXTENSIBLE *waveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mixFormat);
			waveFormatExtensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			waveFormatExtensible->Format.wBitsPerSample = soundDevice->targetBitsPerSample;
			waveFormatExtensible->Format.nBlockAlign = (mixFormat->wBitsPerSample / 8) * mixFormat->nChannels;
			waveFormatExtensible->Format.nAvgBytesPerSec = waveFormatExtensible->Format.nSamplesPerSec*waveFormatExtensible->Format.nBlockAlign;
			waveFormatExtensible->Samples.wValidBitsPerSample = soundDevice->targetBitsPerSample;
		}
		else
		{
			Log("Mix format is not a floating point format.\n");
		}

		hr = gSoundDevice.audioClient->IsFormatSupported(shareMode, mixFormat, &closestMatchedFormat);

		if (SUCCEEDED(hr))
		{
			formatSupported = true;
			Log("Successfully convert to supported mix format\n");
		}
		else
		{
			Log("Format is not supported \n");
		}
	}
	else if (SUCCEEDED(hr))
	{
		formatSupported = true;
		Log("Mix format is supported\n");
	}
	else
	{
		Log("Format is not supported \n");
	}

	return formatSupported;
}

bool InitWasapiExclusive(WAVEFORMATEX *mixFormat)
{
	HRESULT hr;
	
	bool supportMixFormat = false;

	supportMixFormat = ConvertToSupportFormat(&gSoundDevice, mixFormat, AUDCLNT_SHAREMODE_EXCLUSIVE);

	if (supportMixFormat)
	{

		REFERENCE_TIME bufferDuration = gSoundDevice.latencyMs * 10000;
		REFERENCE_TIME periodicity = gSoundDevice.latencyMs * 10000;

		hr = gSoundDevice.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
			AUDCLNT_STREAMFLAGS_NOPERSIST,// | AUDCLNT_STREAMFLAGS_EVENTCALLBACK ,,
			bufferDuration,
			periodicity,
			mixFormat,
			NULL);
		//
		//  When rendering in exclusive mode event driven, the HDAudio specification requires that the buffers handed to the device must 
		//  be aligned on a 128 byte boundary.  When the buffer is initialized and the resulting buffer size would not be 128 byte aligned,
		//  we need to "swizzle" the periodicity of the engine to ensure that the buffers are properly aligned.
		//
		if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
		{
			UINT32 bufferSize;
			Log("Buffers not aligned. Aligning the buffers... \n");
			//
			//  Retrieve the buffer size for the audio client.  The buffer size returned is aligned to the nearest 128 byte
			//  boundary given the input buffer duration.
			//
			hr = gSoundDevice.audioClient->GetBufferSize(&bufferSize);
			if (FAILED(hr))
			{
				Log("Unable to get audio client buffer: %x. \n", hr);
				return false;
			}

			//
			//  Calculate the new aligned periodicity.  We do that by taking the buffer size returned (which is in frames),
			//  multiplying it by the frames/second in the render format (which gets us seconds per buffer), then converting the 
			//  seconds/buffer calculation into a REFERENCE_TIME.
			//
			bufferDuration = (REFERENCE_TIME)(10000.0 *                         // (REFERENCE_TIME / ms) *
				1000 *                            // (ms / s) *
				bufferSize /                      // frames /
				mixFormat->nSamplesPerSec +      // (frames / s)
				0.5);                             // rounding

			//
			//  Now reactivate an IAudioClient object on our preferred endpoint and reinitialize AudioClient
			//
			hr = gSoundDevice.immDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&gSoundDevice.audioClient));
			if (FAILED(hr))
			{
				Log("Unable to activate audio client: %x.\n", hr);
				return false;
			}

			hr = gSoundDevice.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
				AUDCLNT_STREAMFLAGS_NOPERSIST,// | AUDCLNT_STREAMFLAGS_EVENTCALLBACK ,
				bufferDuration,
				0,
				mixFormat,
				NULL);
			if (FAILED(hr))
			{
				Log("Unable to reinitialize audio client: %x \n", hr);
				return false;
			}
		}
		else if (FAILED(hr))
		{
			Log("Unable to initialize audio client in exclusive mode: %x.\n", hr);
			return false;
		}

		UINT frameSize;
		hr = gSoundDevice.audioClient->GetBufferSize(&frameSize);
		if (FAILED(hr))
		{
			Log("Unable to get audio client buffer: %x. \n", hr);
			return false;
		}
		gSoundDevice.framesToWrite = frameSize;


		gSoundDevice.audioSamplesReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
		if (gSoundDevice.audioSamplesReadyEvent == NULL)
		{
			Log("Unable to create samples ready event: %d.\n", GetLastError());
			return false;
		}
		/*
		hr = gSoundDevice.audioClient->SetEventHandle(gSoundDevice.audioSamplesReadyEvent);
		if (FAILED(hr))
		{
		Log("Unable to get set event handle: %x.\n", hr);
		return false;
		}
		*/

		hr = gSoundDevice.audioClient->GetService(IID_PPV_ARGS(&gSoundDevice.renderClient));
		if (FAILED(hr))
		{
			Log("Unable to get new render client: %x.\n", hr);
			return false;
		}

		return true;

	}

	return false;
}

bool InitWasapiShared(WAVEFORMATEX *mixFormat)
{
	HRESULT hr;
	bool supportMixFormat = ConvertToSupportFormat(&gSoundDevice, mixFormat, AUDCLNT_SHAREMODE_SHARED);

	if (supportMixFormat)
	{
		REFERENCE_TIME bufferDuration = gSoundDevice.latencyMs * 10000;
		REFERENCE_TIME periodicity = gSoundDevice.latencyMs * 10000;
		hr = gSoundDevice.audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_NOPERSIST,
			bufferDuration,
			periodicity,
			mixFormat,
			NULL);

		if (FAILED(hr))
		{
			Log("Unable to initialize audio client : %x.\n", hr);
			return false;
		}

		hr = gSoundDevice.audioClient->GetBufferSize(&gSoundDevice.framesToWrite);
		if (FAILED(hr))
		{
			Log("Unable to get audio client buffer: %x. \n", hr);
			return false;
		}

		hr = gSoundDevice.audioClient->GetService(IID_PPV_ARGS(&gSoundDevice.renderClient));
		if (FAILED(hr))
		{
			Log("Unable to get new render client: %x.\n", hr);
			return false;
		}

		gSoundDevice.audioSamplesReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
		if (gSoundDevice.audioSamplesReadyEvent == NULL)
		{
			Log("Unable to create samples ready event: %d.\n", GetLastError());
			return false;
		}

		return true;
	}
	return false;
}

bool InitSound(float desiredFrameTime)
{
	//
	//  A GUI application should use COINIT_APARTMENTTHREADED instead of COINIT_MULTITHREADED.
	//
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
	{
		Log("Unable to initialize COM: %x\n", hr);
		return false;
	}
	
	IMMDeviceEnumerator *deviceEnumerator = nullptr;
	IMMDeviceCollection *deviceCollection = nullptr;

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
	if (FAILED(hr))
	{
		Log("Unable to instantiate device enumerator: %x\n", hr);
		return false;
	}

	hr = deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
	if (FAILED(hr))
	{
		Log("Unable to retrieve device collection: %x\n", hr);
		return false;
	}

	UINT deviceCount;
	hr = deviceCollection->GetCount(&deviceCount);
	if (FAILED(hr))
	{
		Log("Unable to get device collection length: %x\n", hr);
		return false;
	}

	for (UINT i = 0; i < deviceCount; i++)
	{
		hr = deviceCollection->Item(i, &gSoundDevice.immDevice);
		if (FAILED(hr))
		{
			Log("Unable to get imm device %d: %x\n", i, hr);
			return false;
		}
		else
		{
			IPropertyStore *propertyStore;
			hr = gSoundDevice.immDevice->OpenPropertyStore(STGM_READ, &propertyStore);
			if (FAILED(hr))
			{
				Log("Unable to open device %d property store: %x\n", i, hr);
				return false;
			}

			PROPVARIANT friendlyName;
			PropVariantInit(&friendlyName);
			hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
			SAFE_RELEASE(propertyStore);

			if (FAILED(hr))
			{
				Log("Unable to retrieve friendly name for device %d : %x\n", i, hr);
				return false;
			}
			Log("audio device: %S\n", friendlyName.pwszVal);
			PropVariantClear(&friendlyName);

			break;
		}
	}

	hr = gSoundDevice.immDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, NULL, reinterpret_cast<void **>(&gSoundDevice.audioClient));
	if (FAILED(hr))
	{
		Log("Unable to activate audio client: %x.\n", hr);
		return false;
	}
	WAVEFORMATEX *mixFormat;
	hr = gSoundDevice.audioClient->GetMixFormat(&mixFormat);
	if (FAILED(hr))
	{
		Log("Unable to get mix format on audio client: %x.\n", hr);
		return false;
	}
	gSoundDevice.mixFormat = mixFormat;

	bool initSucceed = false;
	if (gSoundDevice.preferExclusive)
	{
		initSucceed = InitWasapiExclusive(mixFormat);
	}
	
	if (!initSucceed)
	{
		initSucceed = InitWasapiShared(mixFormat);
	}

	if (initSucceed)
		audioThread = CreateThread(AudioThreadFunc, &gSoundDevice);

	return initSucceed;



}

}