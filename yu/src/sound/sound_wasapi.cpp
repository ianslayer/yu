#include "../core/log.h"
#include "../core/thread.h"
#include <math.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include <functiondiscoverykeys.h>

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

	u8		*soundBuffer;
	size_t	soundBufferSize;

	int		desiredLatencyMs = 10;
	int		exclusiveModeBitsPerSample;
	int		exclusiveModeHertz;
	unsigned int exclusiveModeFramesToWrite;

	int		numChannels = 2;
	int		targetBitsPerSample = 16;
	int		targetHerz = 48000;
};
static SoundDeviceWasapi gSoundDevice;
static Thread audioThread;
bool YuRunning();

#define Pi32 3.14159265359f
ThreadReturn ThreadCall AudioThreadFunc(ThreadContext context)
{
	SoundDeviceWasapi* soundDevice = (SoundDeviceWasapi*)context;
	HANDLE waitArray[] = { gSoundDevice.audioSamplesReadyEvent };
	static float sinT = 0.f;
	float midTone = 440.f;

	soundDevice->soundBufferSize = soundDevice->exclusiveModeFramesToWrite * (soundDevice->exclusiveModeBitsPerSample / 8) * soundDevice->numChannels;
	soundDevice->soundBuffer = new u8[soundDevice->soundBufferSize];
	memset(soundDevice->soundBuffer, 0, soundDevice->soundBufferSize);


	while (YuRunning())
	{
		HRESULT hr;
		DWORD waitResult = WaitForMultipleObjects(1, waitArray, FALSE, INFINITE);
		BYTE* bufferToWrite;
		switch (waitResult) 
		{
		case WAIT_OBJECT_0:
			hr = soundDevice->renderClient->GetBuffer(soundDevice->exclusiveModeFramesToWrite, &bufferToWrite);
			if (FAILED(hr))
			{
				printf("Failed to get buffer: %x.\n", hr);
			}
			else
			{
				memcpy(bufferToWrite, soundDevice->soundBuffer, soundDevice->soundBufferSize);
				hr = soundDevice->renderClient->ReleaseBuffer(soundDevice->exclusiveModeFramesToWrite, 0);
				if (!SUCCEEDED(hr))
				{
					Log("Unable to release buffer: %x\n", hr);
				}
			}
			

			break;
		}

		short* samples = (short*)soundDevice->soundBuffer;
		for (unsigned int i = 0; i < soundDevice->exclusiveModeFramesToWrite; i++)
		{
			float period = soundDevice->exclusiveModeHertz / midTone;
			float sound = sinf(sinT);

			*samples++ = short(sound * 256.f);
			*samples++ = short(sound * 256.f);

			sinT += (2.f * Pi32 / (float)period);

			if (sinT > 2.f * Pi32)
			{
				sinT -= 2.f * Pi32;
			}
		}

		assert((u8*)samples == (soundDevice->soundBuffer + soundDevice->soundBufferSize));


	}
	return 0;
}

bool InitSound()
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
	}

	hr = gSoundDevice.audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mixFormat, NULL);
	bool supportExclusiveMode = false;
	if (hr == AUDCLNT_E_UNSUPPORTED_FORMAT)
	{
		Log("Device does not natively support the mix format, converting to PCM.\n");
		//
		//  If the mix format is a float format, just try to convert the format to PCM.
		//
		if (mixFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
		{
			mixFormat->wFormatTag = WAVE_FORMAT_PCM;
			mixFormat->wBitsPerSample = 16;
			mixFormat->nBlockAlign = (mixFormat->wBitsPerSample / 8) * mixFormat->nChannels;
			mixFormat->nAvgBytesPerSec = mixFormat->nSamplesPerSec*mixFormat->nBlockAlign;
		}
		else if (mixFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
			reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mixFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
		{
			WAVEFORMATEXTENSIBLE *waveFormatExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(mixFormat);
			waveFormatExtensible->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			waveFormatExtensible->Format.wBitsPerSample = 16;
			waveFormatExtensible->Format.nBlockAlign = (mixFormat->wBitsPerSample / 8) * mixFormat->nChannels;
			waveFormatExtensible->Format.nAvgBytesPerSec = waveFormatExtensible->Format.nSamplesPerSec*waveFormatExtensible->Format.nBlockAlign;
			waveFormatExtensible->Samples.wValidBitsPerSample = 16;
		}
		else
		{
			Log("Mix format is not a floating point format.\n");
		}

		hr = gSoundDevice.audioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, mixFormat, NULL);
		if (FAILED(hr))
		{
			Log("Format is not supported \n");
		}
		else
		{
			supportExclusiveMode = true;
			Log("obtained exclusive mode audio\n");
		}
	}
	else
	{
		supportExclusiveMode = true;
		Log("obtained exclusive mode audio\n");
	}
	REFERENCE_TIME bufferDuration = (REFERENCE_TIME)gSoundDevice.desiredLatencyMs * 10000;
	bool initExclusiveModeSuccess = false;
	if (supportExclusiveMode)
	{

		gSoundDevice.exclusiveModeHertz = mixFormat->nSamplesPerSec;
		gSoundDevice.exclusiveModeBitsPerSample = mixFormat->wBitsPerSample;

		hr = gSoundDevice.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
			AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
			bufferDuration,
			bufferDuration,
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
				goto exclusive_mode_failed;
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
				goto exclusive_mode_failed;
			}

			hr = gSoundDevice.audioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE,
				AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
				bufferDuration,
				bufferDuration,
				mixFormat,
				NULL);
			if (FAILED(hr))
			{
				Log("Unable to reinitialize audio client: %x \n", hr);
				goto exclusive_mode_failed;
			}
		}
		else if (FAILED(hr))
		{
			Log("Unable to initialize audio client in exclusive mode: %x.\n", hr);
			goto exclusive_mode_failed;
		}

		UINT bufferSize;
		hr = gSoundDevice.audioClient->GetBufferSize(&bufferSize);
		if (FAILED(hr))
		{
			Log("Unable to get audio client buffer: %x. \n", hr);
			goto exclusive_mode_failed;
		}
		initExclusiveModeSuccess = true;
		gSoundDevice.exclusiveModeFramesToWrite = bufferSize;

		gSoundDevice.audioSamplesReadyEvent = CreateEventEx(NULL, NULL, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
		if (gSoundDevice.audioSamplesReadyEvent == NULL)
		{
			Log("Unable to create samples ready event: %d.\n", GetLastError());
			return false;
		}

		hr = gSoundDevice.audioClient->SetEventHandle(gSoundDevice.audioSamplesReadyEvent);
		if (FAILED(hr))
		{
			Log("Unable to get set event handle: %x.\n", hr);
			return false;
		}

		hr = gSoundDevice.audioClient->GetService(IID_PPV_ARGS(&gSoundDevice.renderClient));
		if (FAILED(hr))
		{
			Log("Unable to get new render client: %x.\n", hr);
			return false;
		}

		hr = gSoundDevice.audioClient->Start();
		audioThread = CreateThread(AudioThreadFunc, &gSoundDevice);

		if (FAILED(hr))
		{
			printf("Unable to start render client: %x.\n", hr);
			return false;
		}

		return true;

	}
exclusive_mode_failed:
	//TODO: try shared mode

	return false;

}

}