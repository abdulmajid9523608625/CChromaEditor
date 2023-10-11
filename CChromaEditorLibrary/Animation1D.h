#pragma once

#include "ChromaSDKPluginTypes.h"
#include "AnimationBase.h"

namespace ChromaSDK
{
	class Animation1D : public AnimationBase
	{
	public:
		Animation1D();
		~Animation1D();
		Animation1D& operator=(const Animation1D& rhs);
		void Reset();
		EChromaSDKDeviceTypeEnum GetDeviceType();
		EChromaSDKDevice1DEnum GetDevice();
		bool SetDevice(EChromaSDKDevice1DEnum device);
		int GetDeviceId();
		std::vector<FChromaSDKColorFrame1D>& GetFrames();
		int GetFrameCount();
		float GetDuration(unsigned int index);
		void Load();
		void Unload();
		void Play(bool loop);
		void Pause();
		void Resume(bool loop);
		void Stop();
		void Update(float deltaTime);
		void ClearFrames();
		void ResetFrames();
		int Save(const char* path);

		// Support idle animation
		void InternalUpdate(float deltaTime);

		// Handle preload and immediate mode
		void InternalShowFrame();
	private:
		EChromaSDKDevice1DEnum _mDevice;
		std::vector<FChromaSDKColorFrame1D> _mFrames;
	};
}
