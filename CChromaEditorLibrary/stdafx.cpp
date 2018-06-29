// stdafx.cpp : source file that includes just the standard includes
// CChromaEditorLibrary.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "CChromaEditorLibrary.h"
#include "ChromaThread.h"
#include <map>
#include <sstream>
#include <thread>

using namespace ChromaSDK;
using namespace std;

/* Setup log mechanism */
static DebugLogPtr _gDebugLogPtr;
void LogDebug(const char* format, ...)
{
	if (NULL == _gDebugLogPtr)
	{
		va_list args;
		va_start(args, format);
		vfprintf_s(stdout, format, args);
		va_end(args);
	}
	else if (NULL == format)
	{
		_gDebugLogPtr("");
	}
	else
	{
		char buffer[1024] = { 0 };
		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, format, args);
		va_end(args);
		_gDebugLogPtr(&buffer[0]);
	}
}
void LogError(const char* format, ...)
{
	if (NULL == _gDebugLogPtr)
	{
		va_list args;
		va_start(args, format);
		vfprintf_s(stderr, format, args);
		va_end(args);
	}
	else if (NULL == format)
	{
		_gDebugLogPtr("");
	}
	else
	{
		char buffer[1024] = { 0 };
		va_list args;
		va_start(args, format);
		vsprintf_s(buffer, format, args);
		va_end(args);
		_gDebugLogPtr(&buffer[0]);
	}
}
/* End of setup log mechanism */

bool _gDialogIsOpen = false;
string _gPath = "";
int _gAnimationId = 0;
map<string, int> _gAnimationMapID;
map<int, AnimationBase*> _gAnimations;
map<EChromaSDKDevice1DEnum, int> _gPlayMap1D;
map<EChromaSDKDevice2DEnum, int> _gPlayMap2D;

void SetupChromaThread()
{
	ChromaThread::Instance()->Start();
}

void StopChromaThread()
{
	ChromaThread::Instance()->Stop();
}

void ThreadOpenEditorDialog(bool playOnOpen)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// normal function body here

	//LogError("CChromaEditorLibrary::ThreadOpenEditorDialog %s\r\n", _gPath.c_str());

	// dialog instance
	CMainViewDlg mainViewDlg;

	mainViewDlg.OpenOrCreateAnimation(_gPath);

	if (playOnOpen)
	{
		mainViewDlg.PlayAnimationOnOpen();
	}

	// keep dialog focused
	mainViewDlg.DoModal();

	// dialog is closed
	_gDialogIsOpen = false;
}

extern "C"
{
	EXPORT_API void PluginSetLogDelegate(DebugLogPtr fp)
	{
		_gDebugLogPtr = fp;
		//LogDebug("PluginSetLogDelegate:");
	}

	EXPORT_API bool PluginIsPlatformSupported()
	{
		return true;
	}

	EXPORT_API double PluginIsPlatformSupportedD()
	{
		if (PluginIsPlatformSupported())
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API bool PluginIsInitialized()
	{
		// Chroma thread plays animations
		SetupChromaThread();

		return ChromaSDKPlugin::GetInstance()->IsInitialized();
	}

	EXPORT_API double PluginIsInitializedD()
	{
		if (PluginIsInitialized())
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API bool PluginIsDialogOpen()
	{
		return _gDialogIsOpen;
	}

	EXPORT_API double PluginIsDialogOpenD()
	{
		if (PluginIsDialogOpen())
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API int PluginOpenEditorDialog(const char* path)
	{
		PluginIsInitialized();

		//LogDebug("PluginOpenEditorDialog %s\r\n", path);

		if (_gDialogIsOpen)
		{
			LogError("PluginOpenEditorDialog: Dialog is already open!\r\n");
			return -1;
		}

		_gDialogIsOpen = true;
		_gPath = path;
		thread newThread(ThreadOpenEditorDialog, false);
		newThread.detach();

		return 0;
	}

	EXPORT_API double PluginOpenEditorDialogD(const char* path)
	{
		return PluginOpenEditorDialog(path);
	}

	EXPORT_API int PluginOpenEditorDialogAndPlay(const char* path)
	{
		PluginIsInitialized();

		//LogDebug("PluginOpenEditorDialogAndPlay %s\r\n", path);

		if (_gDialogIsOpen)
		{
			LogError("PluginOpenEditorDialogAndPlay: Dialog is already open!\r\n");
			return -1;
		}

		_gDialogIsOpen = true;
		_gPath = path;
		thread newThread(ThreadOpenEditorDialog, true);
		newThread.detach();

		return 0;
	}

	EXPORT_API double PluginOpenEditorDialogAndPlayD(const char* path)
	{
		return PluginOpenEditorDialogAndPlay(path);
	}

	EXPORT_API const char* PluginGetAnimationName(int animationId)
	{
		if (animationId < 0)
		{
			return "";
		}
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (animation == nullptr)
		{
			return "";
		}
		return animation->GetName().c_str();
	}

	EXPORT_API void PluginStopAll()
	{
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_ChromaLink);
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_Headset);
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Keyboard);
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Keypad);
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Mouse);
		PluginStopAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_Mousepad);
	}

	EXPORT_API void PluginClearAnimationType(int deviceType, int device)
	{
		PluginStopAnimationType(deviceType, device);

		FChromaSDKEffectResult result;
		switch ((EChromaSDKDeviceTypeEnum)deviceType)
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
			result = ChromaSDKPlugin::GetInstance()->CreateEffectNone1D((EChromaSDKDevice1DEnum)device);
			if (result.Result == 0)
			{
				ChromaSDKPlugin::GetInstance()->SetEffect(result.EffectId);
				ChromaSDKPlugin::GetInstance()->DeleteEffect(result.EffectId);
			}
			break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
			result = ChromaSDKPlugin::GetInstance()->CreateEffectNone2D((EChromaSDKDevice2DEnum)device);
			if (result.Result == 0)
			{
				ChromaSDKPlugin::GetInstance()->SetEffect(result.EffectId);
				ChromaSDKPlugin::GetInstance()->DeleteEffect(result.EffectId);
			}
			break;
		}
	}

	EXPORT_API void PluginClearAll()
	{
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_ChromaLink);
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_Headset);
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Keyboard);
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Keypad);
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_2D, (int)EChromaSDKDevice2DEnum::DE_Mouse);
		PluginClearAnimationType((int)EChromaSDKDeviceTypeEnum::DE_1D, (int)EChromaSDKDevice1DEnum::DE_Mousepad);
	}

	EXPORT_API int PluginGetAnimationCount()
	{
		return _gAnimationMapID.size();
	}

	EXPORT_API int PluginGetAnimationId(int index)
	{
		int i = 0;
		for (std::map<string, int>::iterator it = _gAnimationMapID.begin(); it != _gAnimationMapID.end(); ++it)
		{
			if (index == i)
			{
				return (*it).second;
			}
			++i;
		}
		return -1;
	}

	EXPORT_API int PluginGetPlayingAnimationCount()
	{
		if (ChromaThread::Instance() == nullptr)
		{
			return 0;
		}
		return ChromaThread::Instance()->GetAnimationCount();
	}

	EXPORT_API int PluginGetPlayingAnimationId(int index)
	{
		if (ChromaThread::Instance() == nullptr)
		{
			return -1;
		}
		return ChromaThread::Instance()->GetAnimationId(index);
	}

	EXPORT_API int PluginOpenAnimation(const char* path)
	{
		try
		{
			//return animation id
			AnimationBase* animation = ChromaSDKPlugin::GetInstance()->OpenAnimation(path);
			if (animation == nullptr)
			{
				LogError("PluginOpenAnimation: Animation is null! name=%s\r\n", path);
				return -1;
			}
			else
			{
				animation->SetName(path);
				int id = _gAnimationId;
				_gAnimations[id] = animation;
				++_gAnimationId;
				_gAnimationMapID[path] = id;
				return id;
			}
		}
		catch (exception)
		{
			LogError("PluginOpenAnimation: Exception path=%s\r\n", path);
			return -1;
		}
	}

	EXPORT_API double PluginOpenAnimationD(const char* path)
	{
		return PluginOpenAnimation(path);
	}

	EXPORT_API int PluginLoadAnimation(int animationId)
	{
		try
		{
			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginLoadAnimation: Animation is null! id=%d", animationId);
					return -1;
				}
				animation->Load();
				return animationId;
			}
		}
		catch (exception)
		{
			LogError("PluginLoadAnimation: Exception animationId=%d\r\n", (int)animationId);
		}
		return -1;
	}

	EXPORT_API void PluginLoadAnimationName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginLoadAnimationName: Animation not found! %s", path);
			return;
		}
		PluginLoadAnimation(animationId);
	}

	EXPORT_API double PluginLoadAnimationD(double animationId)
	{
		return (double)PluginLoadAnimation((int)animationId);
	}

	EXPORT_API int PluginUnloadAnimation(int animationId)
	{
		try
		{
			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginUnloadAnimation: Animation is null! id=%d", animationId);
					return -1;
				}
				animation->Unload();
				return animationId;
			}
		}
		catch (exception)
		{
			LogError("PluginUnloadAnimation: Exception animationId=%d\r\n", (int)animationId);
		}
		return -1;
	}

	EXPORT_API void PluginUnloadAnimationName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginUnloadAnimationName: Animation not found! %s", path);
			return;
		}
		PluginUnloadAnimation(animationId);
	}

	EXPORT_API double PluginUnloadAnimationD(double animationId)
	{
		return (double)PluginUnloadAnimation((int)animationId);
	}

	EXPORT_API int PluginPlayAnimation(int animationId)
	{
		try
		{
			// Chroma thread plays animations
			SetupChromaThread();

			if (!PluginIsInitialized())
			{
				LogError("PluginPlayAnimation: Plugin is not initialized!\r\n");
				return -1;
			}

			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginPlayAnimation: Animation is null! id=%d", animationId);
					return -1;
				}
				PluginStopAnimationType(animation->GetDeviceTypeId(), animation->GetDeviceId());
				switch (animation->GetDeviceType())
				{
				case EChromaSDKDeviceTypeEnum::DE_1D:
					_gPlayMap1D[(EChromaSDKDevice1DEnum)animation->GetDeviceId()] = animationId;
					break;
				case EChromaSDKDeviceTypeEnum::DE_2D:
					_gPlayMap2D[(EChromaSDKDevice2DEnum)animation->GetDeviceId()] = animationId;
					break;
				}
				animation->Play(false);
				return animationId;
			}
		}
		catch (exception)
		{
			LogError("PluginPlayAnimation: Exception animationId=%d\r\n", (int)animationId);
		}
		return -1;
	}

	EXPORT_API double PluginPlayAnimationD(double animationId)
	{
		return (double)PluginPlayAnimation((int)animationId);
	}

	EXPORT_API bool PluginIsPlaying(int animationId)
	{
		try
		{
			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginIsPlaying: Animation is null! id=%d", animationId);
					return false;
				}
				return animation->IsPlaying();
			}
		}
		catch (exception)
		{
			LogError("PluginIsPlaying: Exception animationId=%d\r\n", (int)animationId);
		}
		return false;
	}

	EXPORT_API double PluginIsPlayingD(double animationId)
	{
		if (PluginIsPlaying((int)animationId))
		{
			return 1.0;
		}
		else
		{
			return 0.0;
		}
	}

	EXPORT_API int PluginStopAnimation(int animationId)
	{
		try
		{
			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginStopAnimation: Animation is null! id=%d", animationId);
					return -1;
				}
				animation->Stop();
				return animationId;
			}
		}
		catch (exception)
		{
			LogError("PluginStopAnimation: Exception animationId=%d\r\n", (int)animationId);
		}
		return -1;
	}

	EXPORT_API double PluginStopAnimationD(double animationId)
	{
		return (double)PluginStopAnimation((int)animationId);
	}

	EXPORT_API int PluginCloseAnimation(int animationId)
	{
		try
		{
			if (_gAnimations.find(animationId) != _gAnimations.end())
			{
				AnimationBase* animation = _gAnimations[animationId];
				if (animation == nullptr)
				{
					LogError("PluginCloseAnimation: Animation is null! id=%d", animationId);
					return -1;
				}
				animation->Stop();
				string animationName = animation->GetName();
				if (_gAnimationMapID.find(animationName) != _gAnimationMapID.end())
				{
					_gAnimationMapID.erase(animationName);
				}
				delete _gAnimations[animationId];
				_gAnimations.erase(animationId);
				return animationId;
			}
		}
		catch (exception)
		{
			LogError("PluginCloseAnimation: Exception animationId=%d\r\n", (int)animationId);
		}
		return -1;
	}

	EXPORT_API double PluginCloseAnimationD(double animationId)
	{
		return (double)PluginCloseAnimation((int)animationId);
	}

	EXPORT_API int PluginInit()
	{
		// Chroma thread plays animations
		SetupChromaThread();

		_gAnimationId = 0;
		_gAnimationMapID.clear();
		_gAnimations.clear();
		_gPlayMap1D.clear();
		_gPlayMap2D.clear();

		return ChromaSDKPlugin::GetInstance()->ChromaSDKInit();
	}

	EXPORT_API double PluginInitD()
	{
		return (double)PluginInit();
	}

	EXPORT_API int PluginUninit()
	{
		// Chroma thread plays animations
		StopChromaThread();

		int result = ChromaSDKPlugin::GetInstance()->ChromaSDKUnInit();
		if (PluginIsInitialized())
		{
			for (auto iter = _gAnimations.begin(); iter != _gAnimations.end(); ++iter)
			{
				PluginCloseAnimation(iter->first);
			}
		}
		_gAnimationId = 0;
		_gAnimationMapID.clear();
		_gAnimations.clear();
		_gPlayMap1D.clear();
		_gPlayMap2D.clear();
		return result;
	}

	EXPORT_API double PluginUninitD()
	{
		return (double)PluginUninit();
	}

	EXPORT_API int PluginCreateAnimationInMemory(int deviceType, int device)
	{
		switch ((EChromaSDKDeviceTypeEnum)deviceType)
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
			{
				Animation1D* animation1D = new Animation1D();
				animation1D->SetDevice((EChromaSDKDevice1DEnum)device);
				vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
				frames.clear();
				FChromaSDKColorFrame1D frame = FChromaSDKColorFrame1D();
				frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors1D((EChromaSDKDevice1DEnum)device);
				frames.push_back(frame);

				int id = _gAnimationId;
				_gAnimations[id] = animation1D;
				++_gAnimationId;
				return id;
			}
			break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
			{
				Animation2D* animation2D = new Animation2D();
				animation2D->SetDevice((EChromaSDKDevice2DEnum)device);
				vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
				frames.clear();
				FChromaSDKColorFrame2D frame = FChromaSDKColorFrame2D();
				frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors2D((EChromaSDKDevice2DEnum)device);
				frames.push_back(frame);
				
				int id = _gAnimationId;
				_gAnimations[id] = animation2D;
				++_gAnimationId;
				return id;
			}
			break;
		}
		return -1;
	}

	EXPORT_API int PluginCreateAnimation(const char* path, int deviceType, int device)
	{
		switch ((EChromaSDKDeviceTypeEnum)deviceType)
		{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D animation1D = Animation1D();
					animation1D.SetDevice((EChromaSDKDevice1DEnum)device);
					vector<FChromaSDKColorFrame1D>& frames = animation1D.GetFrames();
					frames.clear();
					FChromaSDKColorFrame1D frame = FChromaSDKColorFrame1D();
					frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors1D((EChromaSDKDevice1DEnum)device);
					frames.push_back(frame);
					animation1D.Save(path);
					return PluginOpenAnimation(path);
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D animation2D = Animation2D();
					animation2D.SetDevice((EChromaSDKDevice2DEnum)device);
					vector<FChromaSDKColorFrame2D>& frames = animation2D.GetFrames();
					frames.clear();
					FChromaSDKColorFrame2D frame = FChromaSDKColorFrame2D();
					frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors2D((EChromaSDKDevice2DEnum)device);
					frames.push_back(frame);
					animation2D.Save(path);
					return PluginOpenAnimation(path);
				}
				break;
		}
		return -1;
	}

	EXPORT_API int PluginSaveAnimation(int animationId, const char* path)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginSaveAnimation: Animation is null! id=%d", animationId);
				return -1;
			}
			animation->Save(path);
			return animationId;
		}
		return -1;
	}

	EXPORT_API int PluginResetAnimation(int animationId)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginResetAnimation: Animation is null! id=%d", animationId);
				return -1;
			}
			animation->ResetFrames();
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginGetFrameCount(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			LogError("PluginGetFrameCount: Animation is null! id=%d", animationId);
			return -1;
		}
		return animation->GetFrameCount();
	}

	EXPORT_API int PluginGetFrameCountName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGetFrameCountName: Animation not found! %s", path);
			return -1;
		}
		return PluginGetFrameCount(animationId);
	}

	EXPORT_API double PluginGetFrameCountNameD(const char* path)
	{
		return (double)PluginGetFrameCountName(path);
	}

	EXPORT_API int PluginGetCurrentFrame(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			LogError("PluginGetCurrentFrame: Animation is null! id=%d", animationId);
			return -1;
		}
		return animation->GetCurrentFrame();
	}

	EXPORT_API int PluginGetCurrentFrameName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGetCurrentFrameName: Animation not found! %s", path);
			return -1;
		}
		return PluginGetCurrentFrame(animationId);
	}

	EXPORT_API double PluginGetCurrentFrameNameD(const char* path)
	{
		return (double)PluginGetCurrentFrameName(path);
	}

	EXPORT_API int PluginGetDeviceType(int animationId)
	{
		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginGetDeviceType: Animation is null! id=%d", animationId);
				return -1;
			}
			return (int)animation->GetDeviceType();
		}

		return -1;
	}

	EXPORT_API int PluginGetDeviceTypeName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGetDeviceTypeName: Animation not found! %s", path);
			return -1;
		}
		return PluginGetDeviceType(animationId);
	}

	EXPORT_API double PluginGetDeviceTypeNameD(const char* path)
	{
		return (double)PluginGetDeviceTypeName(path);
	}

	EXPORT_API int PluginGetDevice(int animationId)
	{
		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginGetDevice: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					return (int)animation1D->GetDevice();
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					return (int)animation2D->GetDevice();
				}
				break;
			}
		}

		return -1;
	}

	EXPORT_API int PluginGetDeviceName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGetDeviceName: Animation not found! %s", path);
			return -1;
		}
		return PluginGetDevice(animationId);
	}

	EXPORT_API double PluginGetDeviceNameD(const char* path)
	{
		return (double)PluginGetDeviceName(path);
	}

	EXPORT_API int PluginSetDevice(int animationId, int deviceType, int device)
	{
		PluginStopAnimation(animationId);

		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			LogError("PluginSetDevice: Animation is null! id=%d", animationId);
			return -1;
		}
		string path = animation->GetName();
		PluginCloseAnimation(animationId);
		return PluginCreateAnimation(path.c_str(), deviceType, device);
	}

	EXPORT_API int PluginGetMaxLeds(int device)
	{
		return ChromaSDKPlugin::GetInstance()->GetMaxLeds((EChromaSDKDevice1DEnum)device);
	}

	EXPORT_API double PluginGetMaxLedsD(double device)
	{
		return (double)PluginGetMaxLeds((int)device);
	}

	EXPORT_API int PluginGetMaxRow(int device)
	{
		return ChromaSDKPlugin::GetInstance()->GetMaxRow((EChromaSDKDevice2DEnum)device);
	}

	EXPORT_API double PluginGetMaxRowD(double device)
	{
		return (double)PluginGetMaxRow((int)device);
	}

	EXPORT_API int PluginGetMaxColumn(int device)
	{
		return ChromaSDKPlugin::GetInstance()->GetMaxColumn((EChromaSDKDevice2DEnum)device);
	}

	EXPORT_API double PluginGetMaxColumnD(double device)
	{
		return (double)PluginGetMaxColumn((int)device);
	}

	EXPORT_API int PluginAddFrame(int animationId, float duration, int* colors, int length)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginAddFrame: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					FChromaSDKColorFrame1D frame = FChromaSDKColorFrame1D();
					if (duration < 0.033f)
					{
						duration = 0.033f;
					}
					frame.Duration = duration;
					vector<COLORREF> newColors = ChromaSDKPlugin::GetInstance()->CreateColors1D(animation1D->GetDevice());
					for (int i = 0; i < maxLeds && i < length; ++i)
					{
						newColors[i] = colors[i];
					}
					frame.Colors = newColors;
					frames.push_back(frame);
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					FChromaSDKColorFrame2D frame = FChromaSDKColorFrame2D();
					if (duration < 0.033f)
					{
						duration = 0.033f;
					}
					frame.Duration = duration;
					vector<FChromaSDKColors> newColors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
					int index = 0;
					for (int i = 0; i < maxRow && index < length; ++i)
					{
						std::vector<COLORREF>& row = newColors[i].Colors;
						for (int j = 0; j < maxColumn && index < length; ++j)
						{
							row[j] = colors[index];
							++index;
						}
					}
					frame.Colors = newColors;
					frames.push_back(frame);
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginUpdateFrame(int animationId, int frameIndex, float duration, int* colors, int length)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginUpdateFrame: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginUpdateFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame1D& frame = frames[frameIndex];
					if(duration < 0.033f)
					{
						duration = 0.033f;
					}
					frame.Duration = duration;
					vector<COLORREF> newColors = ChromaSDKPlugin::GetInstance()->CreateColors1D(animation1D->GetDevice());
					for (int i = 0; i < maxLeds && i < length; ++i)
					{
						newColors[i] = colors[i];
					}
					frame.Colors = newColors;
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginUpdateFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame2D& frame = frames[frameIndex];
					if (duration < 0.033f)
					{
						duration = 0.033f;
					}
					frame.Duration = duration;
					vector<FChromaSDKColors> newColors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
					int index = 0;
					for (int i = 0; i < maxRow && index < length; ++i)
					{
						std::vector<COLORREF>& row = newColors[i].Colors;
						for (int j = 0; j < maxColumn && index < length; ++j)
						{
							row[j] = colors[index];
							++index;
						}
					}
					frame.Colors = newColors;
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginGetFrame(int animationId, int frameIndex, float* duration, int* colors, int length)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginGetFrame: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginGetFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame1D& frame = frames[frameIndex];
					*duration = frame.Duration;
					vector<COLORREF>& frameColors = frame.Colors;
					for (int i = 0; i < maxLeds && i < length; ++i)
					{
						colors[i] = frameColors[i];
					}
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginGetFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame2D& frame = frames[frameIndex];
					*duration = frame.Duration;
					vector<FChromaSDKColors>& frameColors = frame.Colors;
					int index = 0;
					for (int i = 0; i < maxRow && index < length; ++i)
					{
						std::vector<COLORREF>& row = frameColors[i].Colors;
						for (int j = 0; j < maxColumn && index < length; ++j)
						{
							colors[index] = row[j];
							++index;
						}
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginPreviewFrame(int animationId, int frameIndex)
	{
		//LogDebug("PluginPreviewFrame: animationId=%d frameIndex=%d\r\n", animationId, frameIndex);

		PluginStopAnimation(animationId);

		// Chroma thread plays animations
		SetupChromaThread();

		if (!PluginIsInitialized())
		{
			LogError("PluginPreviewFrame: Plugin is not initialized!\r\n");
			return -1;
		}

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginPreviewFrame: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginPreviewFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame1D frame = frames[frameIndex];
					vector<COLORREF>& colors = frame.Colors;
					FChromaSDKEffectResult result = ChromaSDKPlugin::GetInstance()->CreateEffectCustom1D(animation1D->GetDevice(), colors);
					if (result.Result == 0)
					{
						ChromaSDKPlugin::GetInstance()->SetEffect(result.EffectId);
						ChromaSDKPlugin::GetInstance()->DeleteEffect(result.EffectId);
					}
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					if (frameIndex < 0 || frameIndex >= int(frames.size()))
					{
						LogError("PluginPreviewFrame: frame index is invalid! %d of %d", frameIndex, int(frames.size()));
						return -1;
					}
					FChromaSDKColorFrame2D frame = frames[frameIndex];
					vector<FChromaSDKColors>& colors = frame.Colors;
					FChromaSDKEffectResult result;
					if (animation2D->UseChromaCustom())
					{
						result = ChromaSDKPlugin::GetInstance()->CreateEffectKeyboardCustom2D(colors);
					}
					else
					{
						result = ChromaSDKPlugin::GetInstance()->CreateEffectCustom2D(animation2D->GetDevice(), colors);
					}
					if (result.Result == 0)
					{
						ChromaSDKPlugin::GetInstance()->SetEffect(result.EffectId);
						ChromaSDKPlugin::GetInstance()->DeleteEffect(result.EffectId);
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API void PluginPreviewFrameName(const char* path, int frameIndex)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginPreviewFrameName: Animation not found! %s", path);
			return;
		}
		PluginPreviewFrame(animationId, frameIndex);
	}

	EXPORT_API double PluginPreviewFrameD(double animationId, double frameIndex)
	{
		return (int)PluginPreviewFrame((int)animationId, (int)frameIndex);
	}

	EXPORT_API int PluginOverrideFrameDuration(int animationId, float duration)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginOverrideFrameDuration: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					for (int i = 0; i < int(frames.size()); ++i)
					{
						FChromaSDKColorFrame1D& frame = frames[i];
						if (duration < 0.033f)
						{
							duration = 0.033f;
						}
						frame.Duration = duration;
					}
				}
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					for (int i = 0; i < int(frames.size()); ++i)
					{
						FChromaSDKColorFrame2D& frame = frames[i];
						if (duration < 0.033f)
						{
							duration = 0.033f;
						}
						frame.Duration = duration;
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API double PluginOverrideFrameDurationD(double animationId, double duration)
	{
		return (double)PluginOverrideFrameDuration((int)animationId, (float)duration);
	}

	EXPORT_API int PluginReverse(int animationId)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginReverse: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
				case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					vector<FChromaSDKColorFrame1D> copy = vector<FChromaSDKColorFrame1D>();
					for (int i = int(frames.size()) - 1; i >= 0; --i)
					{
						FChromaSDKColorFrame1D& frame = frames[i];
						copy.push_back(frame);
					}
					frames.clear();
					for (int i = 0; i < int(copy.size()); ++i)
					{
						FChromaSDKColorFrame1D& frame = copy[i];
						frames.push_back(frame);
					}
				}
				break;
				case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					vector<FChromaSDKColorFrame2D> copy = vector<FChromaSDKColorFrame2D>();
					for (int i = int(frames.size()) - 1; i >= 0; --i)
					{
						FChromaSDKColorFrame2D& frame = frames[i];
						copy.push_back(frame);
					}
					frames.clear();
					for (int i = 0; i < int(copy.size()); ++i)
					{
						FChromaSDKColorFrame2D& frame = copy[i];
						frames.push_back(frame);
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginMirrorHorizontally(int animationId)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginMirrorHorizontally: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
				case EChromaSDKDeviceTypeEnum::DE_1D:
				{
					Animation1D* animation1D = dynamic_cast<Animation1D*>(animation);
					vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
					int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
					for (int index = 0; index < int(frames.size()); ++index)
					{
						FChromaSDKColorFrame1D& frame = frames[index];
						std::vector<COLORREF>& colors = frame.Colors;
						std::vector<COLORREF> newColors = ChromaSDKPlugin::GetInstance()->CreateColors1D(animation1D->GetDevice());
						for (int i = 0; i < maxLeds; ++i)
						{
							int reverse = maxLeds - 1 - i;
							newColors[i] = colors[reverse];
						}
						frame.Colors = newColors;
					}
				}
				break;
				case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
					for (int index = 0; index < int(frames.size()); ++index)
					{
						FChromaSDKColorFrame2D& frame = frames[index];
						std::vector<FChromaSDKColors>& colors = frame.Colors;
						std::vector<FChromaSDKColors> newColors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
						for (int i = 0; i < maxRow; ++i)
						{
							std::vector<COLORREF>& row = colors[i].Colors;
							std::vector<COLORREF>& newRow = newColors[i].Colors;
							for (int j = 0; j < maxColumn; ++j)
							{
								int reverse = maxColumn - 1 - j;
								newRow[j] = row[reverse];
							}
						}
						frame.Colors = newColors;
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	EXPORT_API int PluginMirrorVertically(int animationId)
	{
		PluginStopAnimation(animationId);

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginMirrorVertically: Animation is null! id=%d", animationId);
				return -1;
			}
			switch (animation->GetDeviceType())
			{
				case EChromaSDKDeviceTypeEnum::DE_1D:
				//skip, only 1 high
				break;
				case EChromaSDKDeviceTypeEnum::DE_2D:
				{
					Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
					vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
					int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
					for (int index = 0; index < int(frames.size()); ++index)
					{
						FChromaSDKColorFrame2D& frame = frames[index];
						std::vector<FChromaSDKColors>& colors = frame.Colors;
						std::vector<FChromaSDKColors> newColors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
						for (int i = 0; i < maxRow; ++i)
						{
							int reverse = maxRow - 1 - i;
							newColors[reverse].Colors = colors[i].Colors;
						}
						frame.Colors = newColors;
					}
				}
				break;
			}
			return animationId;
		}

		return -1;
	}

	int PluginGetAnimationIdFromInstance(AnimationBase* animation)
	{
		if (animation == nullptr)
		{
			LogError("PluginGetAnimationIdFromInstance: Invalid animation!\r\n");
			return -1;
		}
		for (int index = 0; index < int(_gAnimations.size()); ++index)
		{
			if (_gAnimations[index] == animation)
			{
				return index;
			}
		}
		return -1;
	}

	AnimationBase* GetAnimationInstance(int animationId)
	{
		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			return _gAnimations[animationId];
		}
		return nullptr;
	}

	EXPORT_API int PluginGetAnimation(const char* name)
	{
		for (std::map<string, int>::iterator it = _gAnimationMapID.begin(); it != _gAnimationMapID.end(); ++it)
		{
			const string& item = (*it).first;
			if (item.compare(name) == 0) {
				return (*it).second;
			}
		}
		return PluginOpenAnimation(name);
	}

	EXPORT_API double PluginGetAnimationD(const char* name)
	{
		return PluginGetAnimation(name);
	}

	EXPORT_API void PluginCloseAnimationName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginCloseAnimationName: Animation not found! %s", path);
			return;
		}
		PluginCloseAnimation(animationId);
	}

	EXPORT_API double PluginCloseAnimationNameD(const char* path)
	{
		PluginCloseAnimationName(path);
		return 0;
	}

	EXPORT_API void PluginCloseAll()
	{
		while (PluginGetAnimationCount() > 0)
		{
			int animationId = PluginGetAnimationId(0);
			PluginCloseAnimation(animationId);
		}
	}

	EXPORT_API void PluginPlayAnimationLoop(int animationId, bool loop)
	{
		// Chroma thread plays animations
		SetupChromaThread();

		if (!PluginIsInitialized())
		{
			LogError("PluginPlayAnimationLoop: Plugin is not initialized!\r\n");
			return;
		}

		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginPlayAnimationLoop: Animation is null! id=%d\r\n", animationId);
				return;
			}
			PluginStopAnimationType(animation->GetDeviceType(), animation->GetDeviceId());
			//LogDebug("PluginPlayAnimationLoop: %s\r\n", animation->GetName().c_str());
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				_gPlayMap1D[(EChromaSDKDevice1DEnum)animation->GetDeviceId()] = animationId;
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				_gPlayMap2D[(EChromaSDKDevice2DEnum)animation->GetDeviceId()] = animationId;
				break;
			}
			animation->Play(loop);
		}
	}

	EXPORT_API void PluginPlayAnimationName(const char* path, bool loop)
	{

		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginPlayAnimationName: Animation not found! %s", path);
			return;
		}
		PluginPlayAnimationLoop(animationId, loop);
	}

	EXPORT_API double PluginPlayAnimationNameD(const char* path, double loop)
	{
		if (loop == 0)
		{
			PluginPlayAnimationName(path, false);
		}
		else
		{
			PluginPlayAnimationName(path, true);
		}
		return 0;
	}

	EXPORT_API void PluginPlayAnimationFrame(int animationId, int frameId, bool loop)
	{
		if (_gAnimations.find(animationId) != _gAnimations.end())
		{
			AnimationBase* animation = _gAnimations[animationId];
			if (animation == nullptr)
			{
				LogError("PluginPlayAnimationFrame: Animation is null! id=%d\r\n", animationId);
				return;
			}
			PluginStopAnimationType(animation->GetDeviceType(), animation->GetDeviceId());
			//LogDebug("PluginPlayAnimationFrame: %s\r\n", animation->GetName().c_str());
			animation->SetCurrentFrame(frameId);
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
				_gPlayMap1D[(EChromaSDKDevice1DEnum)animation->GetDeviceId()] = animationId;
				break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
				_gPlayMap2D[(EChromaSDKDevice2DEnum)animation->GetDeviceId()] = animationId;
				break;
			}
			animation->Play(loop);
		}
	}

	EXPORT_API void PluginPlayAnimationFrameName(const char* path, int frameId, bool loop)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginPlayAnimationFrameName: Animation not found! %s", path);
			return;
		}
		PluginPlayAnimationFrame(animationId, frameId, loop);
	}

	EXPORT_API double PluginPlayAnimationFrameNameD(const char* path, double frameId, double loop)
	{
		if (loop == 0)
		{
			PluginPlayAnimationFrameName(path, (int)frameId, false);
		}
		else
		{
			PluginPlayAnimationFrameName(path, (int)frameId, true);
		}
		return 0;
	}

	EXPORT_API void PluginStopAnimationName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginStopAnimationName: Animation not found! %s", path);
			return;
		}
		PluginStopAnimation(animationId);
	}

	EXPORT_API double PluginStopAnimationNameD(const char* path)
	{
		PluginStopAnimationName(path);
		return 0;
	}

	EXPORT_API void PluginStopAnimationType(int deviceType, int device)
	{
		switch ((EChromaSDKDeviceTypeEnum)deviceType)
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
			{
				if (_gPlayMap1D.find((EChromaSDKDevice1DEnum)device) != _gPlayMap1D.end())
				{
					int prevAnimation = _gPlayMap1D[(EChromaSDKDevice1DEnum)device];
					if (prevAnimation != -1)
					{
						PluginStopAnimation(prevAnimation);
						_gPlayMap1D[(EChromaSDKDevice1DEnum)device] = -1;
					}
				}
			}
			break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
			{
				if (_gPlayMap2D.find((EChromaSDKDevice2DEnum)device) != _gPlayMap2D.end())
				{
					int prevAnimation = _gPlayMap2D[(EChromaSDKDevice2DEnum)device];
					if (prevAnimation != -1)
					{
						PluginStopAnimation(prevAnimation);
						_gPlayMap2D[(EChromaSDKDevice2DEnum)device] = -1;
					}
				}
			}
			break;
		}
	}

	EXPORT_API double PluginStopAnimationTypeD(double deviceType, double device)
	{
		PluginStopAnimationType((int)deviceType, (int)device);
		return 0;
	}

	EXPORT_API bool PluginIsPlayingName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginIsPlayingName: Animation not found! %s", path);
			return false;
		}
		return PluginIsPlaying(animationId);
	}

	EXPORT_API double PluginIsPlayingNameD(const char* path)
	{
		if (PluginIsPlayingName(path))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API bool PluginIsPlayingType(int deviceType, int device)
	{
		switch ((EChromaSDKDeviceTypeEnum)deviceType)
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
			{
				if (_gPlayMap1D.find((EChromaSDKDevice1DEnum)device) != _gPlayMap1D.end())
				{
					int prevAnimation = _gPlayMap1D[(EChromaSDKDevice1DEnum)device];
					if (prevAnimation != -1)
					{
						return PluginIsPlaying(prevAnimation);
					}
				}
			}
			break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
			{
				if (_gPlayMap2D.find((EChromaSDKDevice2DEnum)device) != _gPlayMap2D.end())
				{
					int prevAnimation = _gPlayMap2D[(EChromaSDKDevice2DEnum)device];
					if (prevAnimation != -1)
					{
						return PluginIsPlaying(prevAnimation);
					}
				}
			}
			break;
		}
		return false;
	}

	EXPORT_API double PluginIsPlayingTypeD(double deviceType, double device)
	{
		if (PluginIsPlayingType((int)deviceType, (int)device))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API void PluginLoadComposite(const char* name)
	{
		string baseName = name;
		PluginLoadAnimationName((baseName + "_ChromaLink.chroma").c_str());
		PluginLoadAnimationName((baseName + "_Headset.chroma").c_str());
		PluginLoadAnimationName((baseName + "_Keyboard.chroma").c_str());
		PluginLoadAnimationName((baseName + "_Keypad.chroma").c_str());
		PluginLoadAnimationName((baseName + "_Mouse.chroma").c_str());
		PluginLoadAnimationName((baseName + "_Mousepad.chroma").c_str());
	}

	EXPORT_API void PluginUnloadComposite(const char* name)
	{
		string baseName = name;
		PluginUnloadAnimationName((baseName + "_ChromaLink.chroma").c_str());
		PluginUnloadAnimationName((baseName + "_Headset.chroma").c_str());
		PluginUnloadAnimationName((baseName + "_Keyboard.chroma").c_str());
		PluginUnloadAnimationName((baseName + "_Keypad.chroma").c_str());
		PluginUnloadAnimationName((baseName + "_Mouse.chroma").c_str());
		PluginUnloadAnimationName((baseName + "_Mousepad.chroma").c_str());
	}

	EXPORT_API void PluginPlayComposite(const char* name, bool loop)
	{
		string baseName = name;
		PluginPlayAnimationName((baseName + "_ChromaLink.chroma").c_str(), loop);
		PluginPlayAnimationName((baseName + "_Headset.chroma").c_str(), loop);
		PluginPlayAnimationName((baseName + "_Keyboard.chroma").c_str(), loop);
		PluginPlayAnimationName((baseName + "_Keypad.chroma").c_str(), loop);
		PluginPlayAnimationName((baseName + "_Mouse.chroma").c_str(), loop);
		PluginPlayAnimationName((baseName + "_Mousepad.chroma").c_str(), loop);
	}

	EXPORT_API double PluginPlayCompositeD(const char* name, double loop)
	{
		if (loop == 0)
		{
			PluginPlayComposite(name, false);
		}
		else
		{
			PluginPlayComposite(name, true);
		}
		return 0;
	}

	EXPORT_API void PluginStopComposite(const char* name)
	{
		string baseName = name;
		PluginStopAnimationName((baseName + "_ChromaLink.chroma").c_str());
		PluginStopAnimationName((baseName + "_Headset.chroma").c_str());
		PluginStopAnimationName((baseName + "_Keyboard.chroma").c_str());
		PluginStopAnimationName((baseName + "_Keypad.chroma").c_str());
		PluginStopAnimationName((baseName + "_Mouse.chroma").c_str());
		PluginStopAnimationName((baseName + "_Mousepad.chroma").c_str());
	}

	EXPORT_API double PluginStopCompositeD(const char* name)
	{
		PluginStopComposite(name);
		return 0;
	}

	EXPORT_API void PluginCloseComposite(const char* name)
	{
		string baseName = name;
		PluginCloseAnimationName((baseName + "_ChromaLink.chroma").c_str());
		PluginCloseAnimationName((baseName + "_Headset.chroma").c_str());
		PluginCloseAnimationName((baseName + "_Keyboard.chroma").c_str());
		PluginCloseAnimationName((baseName + "_Keypad.chroma").c_str());
		PluginCloseAnimationName((baseName + "_Mouse.chroma").c_str());
		PluginCloseAnimationName((baseName + "_Mousepad.chroma").c_str());
	}

	EXPORT_API double PluginCloseCompositeD(const char* name)
	{
		PluginCloseComposite(name);
		return 0;
	}

	EXPORT_API int PluginGetKeyColor(int animationId, int frameId, int rzkey)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return 0;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				return frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)];
			}
		}
		return 0;
	}

	EXPORT_API int PluginGetKeyColorName(const char* path, int frameId, int rzkey)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGetKeyColorName: Animation not found! %s", path);
			return 0;
		}
		return PluginGetKeyColor(animationId, frameId, rzkey);
	}

	EXPORT_API double PluginGetKeyColorD(const char* path, double frameId, double rzkey)
	{
		return (double)PluginGetKeyColorName(path, (int)frameId, (int)rzkey);
	}

	EXPORT_API int PluginGet1DColor(int animationId, int frameId, int led)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return 0;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_1D)
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				if (led >= 0 &&
					led < int(frame.Colors.size()))
				{
					return frame.Colors[led];
				}
			}
		}
		return 0;
	}

	EXPORT_API int PluginGet1DColorName(const char* path, int frameId, int index)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGet1DColorName: Animation not found! %s", path);
			return 0;
		}
		return PluginGet1DColor(animationId, frameId, index);
	}

	EXPORT_API double PluginGet1DColorNameD(const char* path, double frameId, double index)
	{
		return (double)PluginGet1DColorName(path, (int)frameId, (int)index);
	}

	EXPORT_API int PluginGet2DColor(int animationId, int frameId, int row, int column)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return 0;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				if (row >= 0 &&
					row < int(frame.Colors.size()))
				{
					FChromaSDKColors& items = frame.Colors[row];
					if (column >= 0 &&
						column < int(items.Colors.size()))
					{
						return items.Colors[column];
					}
				}
			}
		}
		return 0;
	}

	EXPORT_API int PluginGet2DColorName(const char* path, int frameId, int row, int column)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginGet2DColorName: Animation not found! %s", path);
			return 0;
		}
		return PluginGet2DColor(animationId, frameId, row, column);
	}

	EXPORT_API double PluginGet2DColorNameD(const char* path, double frameId, double row, double column)
	{
		return (double)PluginGet2DColorName(path, (int)frameId, (int)row, (int)column);
	}

	EXPORT_API void PluginSetKeyColor(int animationId, int frameId, int rzkey, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
			}
		}
	}

	EXPORT_API void PluginSetKeyColorName(const char* path, int frameId, int rzkey, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeyColor(animationId, frameId, rzkey, color);
	}

	EXPORT_API double PluginSetKeyColorNameD(const char* path, double frameId, double rzkey, double color)
	{
		PluginSetKeyColorName(path, (int)frameId, (int)rzkey, (int)color);
		return 0;
	}


	EXPORT_API void PluginSetKeyColorAllFrames(int animationId, int rzkey, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			for (int frameId = 0; frameId < int(frames.size()); ++frameId)
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
			}
		}
	}

	EXPORT_API void PluginSetKeyColorAllFramesName(const char* path, int rzkey, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginSetKeyColorAllFrames(animationId, rzkey, color);
	}

	EXPORT_API double PluginSetKeyColorAllFramesNameD(const char* path, double rzkey, double color)
	{
		PluginSetKeyColorAllFramesName(path, (int)rzkey, (int)color);
		return 0;
	}


	EXPORT_API void PluginSetKeysColor(int animationId, int frameId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysColorName(const char* path, int frameId, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysColor(animationId, frameId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSetKeysColorAllFrames(int animationId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			for (int frameId = 0; frameId < int(frames.size()); ++frameId)
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysColorAllFramesName(const char* path, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeysColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysColorAllFrames(animationId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSetKeyNonZeroColor(int animationId, int frameId, int rzkey, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				if (frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] != 0)
				{
					frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSetKeyNonZeroColorName(const char* path, int frameId, int rzkey, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyNonZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeyNonZeroColor(animationId, frameId, rzkey, color);
	}

	EXPORT_API double PluginSetKeyNonZeroColorNameD(const char* path, double frameId, double rzkey, double color)
	{
		PluginSetKeyNonZeroColorName(path, (int)frameId, (int)rzkey, (int)color);
		return 0;
	}


	EXPORT_API void PluginSetKeyZeroColor(int animationId, int frameId, int rzkey, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				if (frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] == 0)
				{
					frame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSetKeyZeroColorName(const char* path, int frameId, int rzkey, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeyZeroColor(animationId, frameId, rzkey, color);
	}

	EXPORT_API double PluginSetKeyZeroColorNameD(const char* path, double frameId, double rzkey, double color)
	{
		PluginSetKeyZeroColorName(path, (int)frameId, (int)rzkey, (int)color);
		return 0;
	}


	EXPORT_API void PluginSetKeysNonZeroColor(int animationId, int frameId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					if (frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] != 0)
					{
						frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysNonZeroColorName(const char* path, int frameId, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyNonZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysNonZeroColor(animationId, frameId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSetKeysZeroColor(int animationId, int frameId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					if (frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] == 0)
					{
						frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysZeroColorName(const char* path, int frameId, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeyZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysZeroColor(animationId, frameId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSetKeysNonZeroColorAllFrames(int animationId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			for (int frameId = 0; frameId < int(frames.size()); ++frameId)
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					if (frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] != 0)
					{
						frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysNonZeroColorAllFramesName(const char* path, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeysNonZeroColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysNonZeroColorAllFrames(animationId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSetKeysZeroColorAllFrames(int animationId, const int* rzkeys, int keyCount, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D &&
			animation->GetDeviceId() == (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			for (int frameId = 0; frameId < int(frames.size()); ++frameId)
			{
				for (int index = 0; index < keyCount; ++index)
				{
					const int* rzkey = &rzkeys[index];
					FChromaSDKColorFrame2D& frame = frames[frameId];
					if (frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] == 0)
					{
						frame.Colors[HIBYTE(*rzkey)].Colors[LOBYTE(*rzkey)] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginSetKeysZeroColorAllFramesName(const char* path, const int* rzkeys, int keyCount, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetKeysZeroColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginSetKeysZeroColorAllFrames(animationId, rzkeys, keyCount, color);
	}


	EXPORT_API void PluginSet1DColor(int animationId, int frameId, int led, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_1D)
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				if (led >= 0 &&
					led < int(frame.Colors.size()))
				{
					frame.Colors[led] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSet1DColorName(const char* path, int frameId, int index, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSet1DColorName: Animation not found! %s", path);
			return;
		}
		PluginSet1DColor(animationId, frameId, index, color);
	}

	EXPORT_API double PluginSet1DColorNameD(const char* path, double frameId, double index, double color)
	{
		PluginSet1DColorName(path, (int)frameId, (int)index, (int)color);
		return 0;
	}

	EXPORT_API void PluginSet2DColor(int animationId, int frameId, int row, int column, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() == EChromaSDKDeviceTypeEnum::DE_2D)
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < int(frames.size()))
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				if (row >= 0 &&
					row < int(frame.Colors.size()))
				{
					FChromaSDKColors& items = frame.Colors[row];
					if (column >= 0 &&
						column < int(items.Colors.size()))
					{
						items.Colors[column] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginSet2DColorName(const char* path, int frameId, int row, int column, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSet2DColorName: Animation not found! %s", path);
			return;
		}
		PluginSet2DColor(animationId, frameId, row, column, color);
	}

	// GMS only allows 4 params when string datatype is used
	EXPORT_API double PluginSet2DColorNameD(const char* path, double frameId, double rowColumnIndex, double color)
	{
		int device = PluginGetDeviceName(path);
		if (device == -1)
		{
			return 0;
		}
		int maxColumn = PluginGetMaxColumn(device);
		int index = (int)rowColumnIndex;
		int row = index / maxColumn;
		int column = index - (row * maxColumn);
		PluginSet2DColorName(path, (int)frameId, row, column, (int)color);
		return 0;
	}

	EXPORT_API void PluginCopyKeyColor(int sourceAnimationId, int targetAnimationId, int frameId, int rzkey)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (frameId < 0)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		if (frameId < int(targetFrames.size()))
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			targetFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = sourceFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)];
		}
	}

	EXPORT_API void PluginCopyKeyColorName(const char* sourceAnimation, const char* targetAnimation, int frameId, int rzkey)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyKeyColorName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyKeyColorName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyKeyColor(sourceAnimationId, targetAnimationId, frameId, rzkey);
	}

	EXPORT_API double PluginCopyKeyColorNameD(const char* sourceAnimation, const char* targetAnimation, double frameId, double rzkey)
	{
		PluginCopyKeyColorName(sourceAnimation, targetAnimation, (int)frameId, (int)rzkey);
		return 0;
	}


	EXPORT_API void PluginCopyNonZeroAllKeysAllFrames(int sourceAnimationId, int targetAnimationId)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(targetFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color != 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyNonZeroAllKeysAllFramesName(const char* sourceAnimation, const char* targetAnimation)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyNonZeroAllKeysAllFramesName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyNonZeroAllKeysAllFramesName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyNonZeroAllKeysAllFrames(sourceAnimationId, targetAnimationId);
	}

	EXPORT_API double PluginCopyNonZeroAllKeysAllFramesNameD(const char* sourceAnimation, const char* targetAnimation)
	{
		PluginCopyNonZeroAllKeysAllFramesName(sourceAnimation, targetAnimation);
		return 0;
	}


	EXPORT_API void PluginCopyNonZeroAllKeysAllFramesOffset(int sourceAnimationId, int targetAnimationId, int offset)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(sourceFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[(frameId+offset) % targetFrames.size()];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color != 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyNonZeroAllKeysAllFramesOffsetName(const char* sourceAnimation, const char* targetAnimation, int offset)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyNonZeroAllKeysAllFramesOffsetName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyNonZeroAllKeysAllFramesOffsetName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyNonZeroAllKeysAllFramesOffset(sourceAnimationId, targetAnimationId, offset);
	}

	EXPORT_API double PluginCopyNonZeroAllKeysAllFramesOffsetNameD(const char* sourceAnimation, const char* targetAnimation, double offset)
	{
		PluginCopyNonZeroAllKeysAllFramesOffsetName(sourceAnimation, targetAnimation, (int)offset);
		return 0;
	}


	EXPORT_API void PluginCopyZeroAllKeysAllFrames(int sourceAnimationId, int targetAnimationId)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(targetFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color == 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyZeroAllKeysAllFramesName(const char* sourceAnimation, const char* targetAnimation)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyZeroAllKeysAllFramesName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyZeroAllKeysAllFramesName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyZeroAllKeysAllFrames(sourceAnimationId, targetAnimationId);
	}

	EXPORT_API double PluginCopyZeroAllKeysAllFramesNameD(const char* sourceAnimation, const char* targetAnimation)
	{
		PluginCopyZeroAllKeysAllFramesName(sourceAnimation, targetAnimation);
		return 0;
	}


	EXPORT_API void PluginCopyZeroAllKeysAllFramesOffset(int sourceAnimationId, int targetAnimationId, int offset)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(sourceFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[(frameId + offset) % targetFrames.size()];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color == 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyZeroAllKeysAllFramesOffsetName(const char* sourceAnimation, const char* targetAnimation, int offset)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyZeroAllKeysAllFramesOffsetName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyZeroAllKeysAllFramesOffsetName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyZeroAllKeysAllFramesOffset(sourceAnimationId, targetAnimationId, offset);
	}

	EXPORT_API double PluginCopyZeroAllKeysAllFramesOffsetNameD(const char* sourceAnimation, const char* targetAnimation, double offset)
	{
		PluginCopyZeroAllKeysAllFramesOffsetName(sourceAnimation, targetAnimation, (int)offset);
		return 0;
	}


	EXPORT_API void PluginCopyNonZeroTargetAllKeysAllFrames(int sourceAnimationId, int targetAnimationId)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(targetFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color != 0 &&
						targetFrame.Colors[i].Colors[j] != 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyNonZeroTargetAllKeysAllFramesName(const char* sourceAnimation, const char* targetAnimation)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyNonZeroTargetAllKeysAllFramesName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyNonZeroTargetAllKeysAllFramesName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyNonZeroTargetAllKeysAllFrames(sourceAnimationId, targetAnimationId);
	}

	EXPORT_API double PluginCopyNonZeroTargetAllKeysAllFramesNameD(const char* sourceAnimation, const char* targetAnimation)
	{
		PluginCopyNonZeroTargetAllKeysAllFramesName(sourceAnimation, targetAnimation);
		return 0;
	}


	EXPORT_API void PluginCopyZeroTargetAllKeysAllFrames(int sourceAnimationId, int targetAnimationId)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		int maxRow = PluginGetMaxRow(EChromaSDKDevice2DEnum::DE_Keyboard);
		int maxColumn = PluginGetMaxColumn(EChromaSDKDevice2DEnum::DE_Keyboard);
		for (int frameId = 0; frameId < int(targetFrames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			for (int i = 0; i < maxRow; ++i)
			{
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = sourceFrame.Colors[i].Colors[j];
					if (color != 0 &&
						targetFrame.Colors[i].Colors[j] == 0)
					{
						targetFrame.Colors[i].Colors[j] = color;
					}
				}
			}
		}
	}

	EXPORT_API void PluginCopyZeroTargetAllKeysAllFramesName(const char* sourceAnimation, const char* targetAnimation)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyZeroTargetAllKeysAllFramesName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyZeroTargetAllKeysAllFramesName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyZeroTargetAllKeysAllFrames(sourceAnimationId, targetAnimationId);
	}

	EXPORT_API double PluginCopyZeroTargetAllKeysAllFramesNameD(const char* sourceAnimation, const char* targetAnimation)
	{
		PluginCopyZeroTargetAllKeysAllFramesName(sourceAnimation, targetAnimation);
		return 0;
	}


	EXPORT_API void PluginCopyNonZeroKeyColor(int sourceAnimationId, int targetAnimationId, int frameId, int rzkey)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (frameId < 0)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		if (frameId < int(targetFrames.size()))
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			int color = sourceFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)];
			if (color != 0)
			{
				targetFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
			}
		}
	}

	EXPORT_API void PluginCopyNonZeroKeyColorName(const char* sourceAnimation, const char* targetAnimation, int frameId, int rzkey)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyNonZeroKeyColorName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyNonZeroKeyColorName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyNonZeroKeyColor(sourceAnimationId, targetAnimationId, frameId, rzkey);
	}

	EXPORT_API double PluginCopyNonZeroKeyColorNameD(const char* sourceAnimation, const char* targetAnimation, double frameId, double rzkey)
	{
		PluginCopyNonZeroKeyColorName(sourceAnimation, targetAnimation, (int)frameId, (int)rzkey);
		return 0;
	}


	EXPORT_API void PluginCopyZeroKeyColor(int sourceAnimationId, int targetAnimationId, int frameId, int rzkey)
	{
		PluginStopAnimation(targetAnimationId);
		AnimationBase* sourceAnimation = GetAnimationInstance(sourceAnimationId);
		if (nullptr == sourceAnimation)
		{
			return;
		}
		AnimationBase* targetAnimation = GetAnimationInstance(targetAnimationId);
		if (nullptr == targetAnimation)
		{
			return;
		}
		if (sourceAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			sourceAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (targetAnimation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D ||
			targetAnimation->GetDeviceId() != (int)EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		if (frameId < 0)
		{
			return;
		}
		Animation2D* sourceAnimation2D = (Animation2D*)(sourceAnimation);
		Animation2D* targetAnimation2D = (Animation2D*)(targetAnimation);
		vector<FChromaSDKColorFrame2D>& sourceFrames = sourceAnimation2D->GetFrames();
		vector<FChromaSDKColorFrame2D>& targetFrames = targetAnimation2D->GetFrames();
		if (sourceFrames.size() == 0)
		{
			return;
		}
		if (targetFrames.size() == 0)
		{
			return;
		}
		if (frameId < int(targetFrames.size()))
		{
			FChromaSDKColorFrame2D& sourceFrame = sourceFrames[frameId % sourceFrames.size()];
			FChromaSDKColorFrame2D& targetFrame = targetFrames[frameId];
			int color = sourceFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)];
			if (color == 0)
			{
				targetFrame.Colors[HIBYTE(rzkey)].Colors[LOBYTE(rzkey)] = color;
			}
		}
	}

	EXPORT_API void PluginCopyZeroKeyColorName(const char* sourceAnimation, const char* targetAnimation, int frameId, int rzkey)
	{
		int sourceAnimationId = PluginGetAnimation(sourceAnimation);
		if (sourceAnimationId < 0)
		{
			LogError("PluginCopyZeroKeyColorName: Source Animation not found! %s", sourceAnimation);
			return;
		}

		int targetAnimationId = PluginGetAnimation(targetAnimation);
		if (targetAnimationId < 0)
		{
			LogError("PluginCopyZeroKeyColorName: Target Animation not found! %s", targetAnimation);
			return;
		}

		PluginCopyZeroKeyColor(sourceAnimationId, targetAnimationId, frameId, rzkey);
	}

	EXPORT_API double PluginCopyZeroKeyColorNameD(const char* sourceAnimation, const char* targetAnimation, double frameId, double rzkey)
	{
		PluginCopyZeroKeyColorName(sourceAnimation, targetAnimation, (int)frameId, (int)rzkey);
		return 0;
	}


	EXPORT_API void PluginFillColor(int animationId, int frameId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					colors[i] = color;
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						row.Colors[j] = color;
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginFillColorName(const char* path, int frameId, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillColorName: Animation not found! %s", path);
			return;
		}
		PluginFillColor(animationId, frameId, color);
	}

	EXPORT_API double PluginFillColorNameD(const char* path, double frameId, double color)
	{
		PluginFillColorName(path, (int)frameId, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillColorRGB(int animationId, int frameId, int red, int green, int blue)
	{
		//clamp values
		red = max(0, min(255, red));
		green = max(0, min(255, green));
		blue = max(0, min(255, blue));
		int color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
		PluginFillColor(animationId, frameId, color);
	}

	EXPORT_API void PluginFillColorRGBName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillColorRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillColorRGB(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginFillColorRGBNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginFillColorRGBName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillColorAllFrames(int animationId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillColor(animationId, frameId, color);
		}
	}

	EXPORT_API void PluginFillColorAllFramesName(const char* path, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginFillColorAllFrames(animationId, color);
	}

	EXPORT_API double PluginFillColorAllFramesNameD(const char* path, double color)
	{
		PluginFillColorAllFramesName(path, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillColorAllFramesRGB(int animationId, int red, int green, int blue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillColorRGB(animationId, frameId, red, green, blue);
		}
	}

	EXPORT_API void PluginFillColorAllFramesRGBName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillColorAllFramesRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillColorAllFramesRGB(animationId, red, green, blue);
	}

	EXPORT_API double PluginFillColorAllFramesRGBNameD(const char* path, double red, double green, double blue)
	{
		PluginFillColorAllFramesRGBName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillNonZeroColor(int animationId, int frameId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					if (colors[i] != 0)
					{
						colors[i] = color;
					}
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						if (row.Colors[j] != 0)
						{
							row.Colors[j] = color;
						}
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginFillNonZeroColorName(const char* path, int frameId, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillNonZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginFillNonZeroColor(animationId, frameId, color);
	}

	EXPORT_API double PluginFillNonZeroColorNameD(const char* path, double frameId, double color)
	{
		PluginFillNonZeroColorName(path, (int)frameId, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillNonZeroColorRGB(int animationId, int frameId, int red, int green, int blue)
	{
		//clamp values
		red = max(0, min(255, red));
		green = max(0, min(255, green));
		blue = max(0, min(255, blue));
		int color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);

		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					if (colors[i] != 0)
					{
						colors[i] = color;
					}
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						if (row.Colors[j] != 0)
						{
							row.Colors[j] = color;
						}
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginFillNonZeroColorRGBName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillNonZeroColorRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillNonZeroColorRGB(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginFillNonZeroColorRGBNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginFillNonZeroColorRGBName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillZeroColor(int animationId, int frameId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					if (colors[i] == 0)
					{
						colors[i] = color;
					}
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						if (row.Colors[j] == 0)
						{
							row.Colors[j] = color;
						}
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginFillZeroColorName(const char* path, int frameId, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillZeroColorName: Animation not found! %s", path);
			return;
		}
		PluginFillZeroColor(animationId, frameId, color);
	}

	EXPORT_API double PluginFillZeroColorNameD(const char* path, double frameId, double color)
	{
		PluginFillZeroColorName(path, (int)frameId, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillZeroColorRGB(int animationId, int frameId, int red, int green, int blue)
	{
		//clamp values
		red = max(0, min(255, red));
		green = max(0, min(255, green));
		blue = max(0, min(255, blue));
		int color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);

		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					if (colors[i] == 0)
					{
						colors[i] = color;
					}
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						if (row.Colors[j] == 0)
						{
							row.Colors[j] = color;
						}
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginFillZeroColorRGBName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillZeroColorRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillZeroColorRGB(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginFillZeroColorRGBNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginFillZeroColorRGBName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillNonZeroColorAllFrames(int animationId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillNonZeroColor(animationId, frameId, color);
		}
	}

	EXPORT_API void PluginFillNonZeroColorAllFramesName(const char* path, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillNonZeroColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginFillNonZeroColorAllFrames(animationId, color);
	}

	EXPORT_API double PluginFillNonZeroColorAllFramesNameD(const char* path, double color)
	{
		PluginFillNonZeroColorAllFramesName(path, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillNonZeroColorAllFramesRGB(int animationId, int red, int green, int blue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillNonZeroColorRGB(animationId, frameId, red, green, blue);
		}
	}

	EXPORT_API void PluginFillNonZeroColorAllFramesRGBName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillNonZeroColorAllFramesRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillNonZeroColorAllFramesRGB(animationId, red, green, blue);
	}

	EXPORT_API double PluginFillNonZeroColorAllFramesRGBNameD(const char* path, double red, double green, double blue)
	{
		PluginFillNonZeroColorAllFramesRGBName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillZeroColorAllFrames(int animationId, int color)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillZeroColor(animationId, frameId, color);
		}
	}

	EXPORT_API void PluginFillZeroColorAllFramesName(const char* path, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillZeroColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginFillZeroColorAllFrames(animationId, color);
	}

	EXPORT_API double PluginFillZeroColorAllFramesNameD(const char* path, double color)
	{
		PluginFillZeroColorAllFramesName(path, (int)color);
		return 0;
	}


	EXPORT_API void PluginFillZeroColorAllFramesRGB(int animationId, int red, int green, int blue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginFillZeroColorRGB(animationId, frameId, red, green, blue);
		}
	}

	EXPORT_API void PluginFillZeroColorAllFramesRGBName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillZeroColorAllFramesRGBName: Animation not found! %s", path);
			return;
		}
		PluginFillZeroColorAllFramesRGB(animationId, red, green, blue);
	}

	EXPORT_API double PluginFillZeroColorAllFramesRGBNameD(const char* path, double red, double green, double blue)
	{
		PluginFillZeroColorAllFramesRGBName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginFillRandomColorsBlackAndWhite(int animationId, int frameId)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			switch (animation->GetDeviceType())
			{
			case EChromaSDKDeviceTypeEnum::DE_1D:
			{
				Animation1D* animation1D = (Animation1D*)(animation);
				vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
				if (frameId >= 0 &&
					frameId < frames.size())
				{
					FChromaSDKColorFrame1D& frame = frames[frameId];
					frame.Colors = ChromaSDKPlugin::GetInstance()->CreateRandomColorsBlackAndWhite1D(animation1D->GetDevice());
				}
			}
			break;
			case EChromaSDKDeviceTypeEnum::DE_2D:
			{
				Animation2D* animation2D = (Animation2D*)(animation);
				vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
				if (frameId >= 0 &&
					frameId < frames.size())
				{
					FChromaSDKColorFrame2D& frame = frames[frameId];
					frame.Colors = ChromaSDKPlugin::GetInstance()->CreateRandomColorsBlackAndWhite2D(animation2D->GetDevice());
				}
			}
			break;
			}
		}
	}

	EXPORT_API void PluginFillRandomColorsBlackAndWhiteName(const char* path, int frameId)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginFillRandomColorsBlackAndWhiteName: Animation not found! %s", path);
			return;
		}
		PluginFillRandomColorsBlackAndWhite(animationId, frameId);
	}

	EXPORT_API double PluginFillRandomColorsBlackAndWhiteNameD(const char* path, double frameId)
	{
		PluginFillRandomColorsBlackAndWhiteName(path, (int)frameId);
		return 0;
	}


	EXPORT_API void PluginOffsetColors(int animationId, int frameId, int offsetRed, int offsetGreen, int offsetBlue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					int color = colors[i];
					int red = (color & 0xFF);
					int green = (color & 0xFF00) >> 8;
					int blue = (color & 0xFF0000) >> 16;
					red = max(0, min(255, red + offsetRed));
					green = max(0, min(255, green + offsetGreen));
					blue = max(0, min(255, blue + offsetBlue));
					color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
					colors[i] = color;
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						int color = row.Colors[j];
						int red = (color & 0xFF);
						int green = (color & 0xFF00) >> 8;
						int blue = (color & 0xFF0000) >> 16;
						red = max(0, min(255, red + offsetRed));
						green = max(0, min(255, green + offsetGreen));
						blue = max(0, min(255, blue + offsetBlue));
						color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
						row.Colors[j] = color;
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginOffsetColorsName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginOffsetColorsName: Animation not found! %s", path);
			return;
		}
		PluginOffsetColors(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginOffsetColorsNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginOffsetColorsName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginOffsetColorsAllFrames(int animationId, int offsetRed, int offsetGreen, int offsetBlue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginOffsetColors(animationId, frameId, offsetRed, offsetGreen, offsetBlue);
		}
	}

	EXPORT_API void PluginOffsetColorsAllFramesName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginOffsetColorsAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginOffsetColorsAllFrames(animationId, red, green, blue);
	}

	EXPORT_API double PluginOffsetColorsAllFramesNameD(const char* path, double red, double green, double blue)
	{
		PluginOffsetColorsAllFramesName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginOffsetNonZeroColors(int animationId, int frameId, int offsetRed, int offsetGreen, int offsetBlue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					int color = colors[i];
					if (color != 0)
					{
						int red = (color & 0xFF);
						int green = (color & 0xFF00) >> 8;
						int blue = (color & 0xFF0000) >> 16;
						red = max(0, min(255, red + offsetRed));
						green = max(0, min(255, green + offsetGreen));
						blue = max(0, min(255, blue + offsetBlue));
						color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
						colors[i] = color;
					}
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						int color = row.Colors[j];
						if (color != 0)
						{
							int red = (color & 0xFF);
							int green = (color & 0xFF00) >> 8;
							int blue = (color & 0xFF0000) >> 16;
							red = max(0, min(255, red + offsetRed));
							green = max(0, min(255, green + offsetGreen));
							blue = max(0, min(255, blue + offsetBlue));
							color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
							row.Colors[j] = color;
						}
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginOffsetNonZeroColorsName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginOffsetNonZeroColorsName: Animation not found! %s", path);
			return;
		}
		PluginOffsetNonZeroColors(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginOffsetNonZeroColorsNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginOffsetNonZeroColorsName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginOffsetNonZeroColorsAllFrames(int animationId, int offsetRed, int offsetGreen, int offsetBlue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginOffsetNonZeroColors(animationId, frameId, offsetRed, offsetGreen, offsetBlue);
		}
	}

	EXPORT_API void PluginOffsetNonZeroColorsAllFramesName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginOffsetNonZeroColorsAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginOffsetNonZeroColorsAllFrames(animationId, red, green, blue);
	}

	EXPORT_API double PluginOffsetNonZeroColorsAllFramesNameD(const char* path, double red, double green, double blue)
	{
		PluginOffsetNonZeroColorsAllFramesName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginMultiplyIntensity(int animationId, int frameId, float intensity)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					int color = colors[i];
					int red = (color & 0xFF);
					int green = (color & 0xFF00) >> 8;
					int blue = (color & 0xFF0000) >> 16;
					red = max(0, min(255, red * intensity));
					green = max(0, min(255, green * intensity));
					blue = max(0, min(255, blue * intensity));
					color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
					colors[i] = color;
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						int color = row.Colors[j];
						int red = (color & 0xFF);
						int green = (color & 0xFF00) >> 8;
						int blue = (color & 0xFF0000) >> 16;
						red = max(0, min(255, red * intensity));
						green = max(0, min(255, green * intensity));
						blue = max(0, min(255, blue * intensity));
						color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
						row.Colors[j] = color;
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginMultiplyIntensityName(const char* path, int frameId, float intensity)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMultiplyIntensityName: Animation not found! %s", path);
			return;
		}
		PluginMultiplyIntensity(animationId, frameId, intensity);
	}

	EXPORT_API double PluginMultiplyIntensityNameD(const char* path, double frameId, double intensity)
	{
		PluginMultiplyIntensityName(path, (int)frameId, (float)intensity);
		return 0;
	}


	EXPORT_API void PluginMultiplyIntensityRGB(int animationId, int frameId, int red, int green, int blue)
	{
		float redIntensity = red / 255.0f;
		float greenIntensity = green / 255.0f;
		float blueIntensity = blue / 255.0f;

		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame1D& frame = frames[frameId];
				int maxLeds = ChromaSDKPlugin::GetInstance()->GetMaxLeds(animation1D->GetDevice());
				vector<COLORREF>& colors = frame.Colors;
				for (int i = 0; i < maxLeds; ++i)
				{
					int color = colors[i];
					int red = (color & 0xFF);
					int green = (color & 0xFF00) >> 8;
					int blue = (color & 0xFF0000) >> 16;
					red = max(0, min(255, red * redIntensity));
					green = max(0, min(255, green * greenIntensity));
					blue = max(0, min(255, blue * blueIntensity));
					color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
					colors[i] = color;
				}
			}
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			if (frameId >= 0 &&
				frameId < frames.size())
			{
				FChromaSDKColorFrame2D& frame = frames[frameId];
				int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
				int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
				for (int i = 0; i < maxRow; ++i)
				{
					FChromaSDKColors& row = frame.Colors[i];
					for (int j = 0; j < maxColumn; ++j)
					{
						int color = row.Colors[j];
						int red = (color & 0xFF);
						int green = (color & 0xFF00) >> 8;
						int blue = (color & 0xFF0000) >> 16;
						red = max(0, min(255, red * redIntensity));
						green = max(0, min(255, green * greenIntensity));
						blue = max(0, min(255, blue * blueIntensity));
						color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
						row.Colors[j] = color;
					}
				}
			}
		}
		break;
		}
	}

	EXPORT_API void PluginMultiplyIntensityRGBName(const char* path, int frameId, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMultiplyIntensityRGBName: Animation not found! %s", path);
			return;
		}
		PluginMultiplyIntensityRGB(animationId, frameId, red, green, blue);
	}

	EXPORT_API double PluginMultiplyIntensityRGBNameD(const char* path, double frameId, double red, double green, double blue)
	{
		PluginMultiplyIntensityRGBName(path, (int)frameId, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginMultiplyIntensityAllFrames(int animationId, float intensity)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginMultiplyIntensity(animationId, frameId, intensity);
		}
	}

	EXPORT_API void PluginMultiplyIntensityAllFramesName(const char* path, float intensity)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMultiplyIntensityAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginMultiplyIntensityAllFrames(animationId, intensity);
	}

	EXPORT_API double PluginMultiplyIntensityAllFramesNameD(const char* path, double intensity)
	{
		PluginMultiplyIntensityAllFramesName(path, (float)intensity);
		return 0;
	}


	EXPORT_API void PluginMultiplyIntensityAllFramesRGB(int animationId, int red, int green, int blue)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			PluginMultiplyIntensityRGB(animationId, frameId, red, green, blue);
		}
	}

	EXPORT_API void PluginMultiplyIntensityAllFramesRGBName(const char* path, int red, int green, int blue)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMultiplyIntensityAllFramesRGBName: Animation not found! %s", path);
			return;
		}
		PluginMultiplyIntensityAllFramesRGB(animationId, red, green, blue);
	}

	EXPORT_API double PluginMultiplyIntensityAllFramesRGBNameD(const char* path, double red, double green, double blue)
	{
		PluginMultiplyIntensityAllFramesRGBName(path, (int)red, (int)green, (int)blue);
		return 0;
	}


	EXPORT_API void PluginReverseAllFrames(int animationId)
	{
		PluginStopAnimation(animationId);
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		int frameCount = PluginGetFrameCount(animationId);
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			
		}
		switch (animation->GetDeviceType())
		{
		case EChromaSDKDeviceTypeEnum::DE_1D:
		{
			Animation1D* animation1D = (Animation1D*)(animation);
			vector<FChromaSDKColorFrame1D>& frames = animation1D->GetFrames();
			reverse(frames.begin(), frames.end());
		}
		break;
		case EChromaSDKDeviceTypeEnum::DE_2D:
		{
			Animation2D* animation2D = (Animation2D*)(animation);
			vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
			reverse(frames.begin(), frames.end());
		}
		break;
		}
	}

	EXPORT_API void PluginReverseAllFramesName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginReverseAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginReverseAllFrames(animationId);
	}

	EXPORT_API double PluginReverseAllFramesNameD(const char* path)
	{
		PluginReverseAllFramesName(path);
		return 0;
	}


	EXPORT_API void PluginSetCurrentFrame(int animationId, int frameId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		animation->SetCurrentFrame(frameId);
	}

	EXPORT_API void PluginSetCurrentFrameName(const char* path, int frameId)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetCurrentFrameName: Animation not found! %s", path);
			return;
		}
		PluginSetCurrentFrame(animationId, frameId);
	}

	EXPORT_API double PluginSetCurrentFrameNameD(const char* path, double frameId)
	{
		PluginSetCurrentFrameName(path, (int)frameId);
		return 0;
	}

	EXPORT_API void PluginPauseAnimation(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		animation->Pause();
	}

	EXPORT_API void PluginPauseAnimationName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginPauseAnimationName: Animation not found! %s", path);
			return;
		}
		PluginPauseAnimation(animationId);
	}

	EXPORT_API double PluginPauseAnimationNameD(const char* path)
	{
		PluginPauseAnimationName(path);
		return 0;
	}

	EXPORT_API bool PluginIsAnimationPaused(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return false;
		}
		return animation->IsPaused();
	}

	EXPORT_API bool PluginIsAnimationPausedName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginIsAnimationPausedName: Animation not found! %s", path);
			return false;
		}
		return PluginIsAnimationPaused(animationId);
	}

	EXPORT_API double PluginIsAnimationPausedNameD(const char* path)
	{
		if (PluginIsAnimationPausedName(path))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API bool PluginHasAnimationLoop(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return false;
		}
		return animation->HasLoop();
	}

	EXPORT_API bool PluginHasAnimationLoopName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginHasAnimationLoopName: Animation not found! %s", path);
			return false;
		}
		return PluginHasAnimationLoop(animationId);
	}

	EXPORT_API double PluginHasAnimationLoopNameD(const char* path)
	{
		if (PluginHasAnimationLoopName(path))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	EXPORT_API void PluginResumeAnimation(int animationId, bool loop)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		animation->Resume(loop);
	}

	EXPORT_API void PluginResumeAnimationName(const char* path, bool loop)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginResumeAnimationName: Animation not found! %s", path);
			return;
		}
		PluginResumeAnimation(animationId, loop);
	}

	EXPORT_API double PluginResumeAnimationNameD(const char* path, double loop)
	{
		if (loop == 0)
		{
			PluginResumeAnimationName(path, false);
		}
		else
		{
			PluginResumeAnimationName(path, true);
		}
		return 0;
	}


	EXPORT_API void PluginSetChromaCustomFlag(int animationId, bool flag)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D)
		{
			return;
		}
		if (animation->GetDeviceId() != EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
		animation2D->SetChromaCustom(flag);
	}

	EXPORT_API void PluginSetChromaCustomFlagName(const char* path, bool flag)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginKeyboardUseChromaCustomName: Animation not found! %s", path);
			return;
		}
		PluginSetChromaCustomFlag(animationId, flag);
	}

	EXPORT_API double PluginSetChromaCustomFlagNameD(const char* path, double flag)
	{
		if (flag == 0)
		{
			PluginSetChromaCustomFlagName(path, false);
		}
		else
		{
			PluginSetChromaCustomFlagName(path, true);
		}
		return 0;
	}


	EXPORT_API void PluginSetChromaCustomColorAllFrames(int animationId)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D)
		{
			return;
		}
		if (animation->GetDeviceId() != EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
		vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
		for (int frameId = 0; frameId < int(frames.size()); ++frameId)
		{
			FChromaSDKColorFrame2D& frame = frames[frameId];
			int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
			int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
			for (int i = 0; i < maxRow; ++i)
			{
				FChromaSDKColors& row = frame.Colors[i];
				for (int j = 0; j < maxColumn; ++j)
				{
					int color = row.Colors[j];
					int customFlag = 0x1;
					int red = (color & 0xFF);
					int green = (color & 0xFF00) >> 8;
					int blue = (color & 0xFF0000) >> 16;
					color = (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16) | (customFlag << 24);
					row.Colors[j] = color;
				}
			}
		}
	}

	EXPORT_API void PluginSetChromaCustomColorAllFramesName(const char* path)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginSetChromaCustomColorAllFramesName: Animation not found! %s", path);
			return;
		}
		PluginSetChromaCustomColorAllFrames(animationId);
	}

	EXPORT_API double PluginSetChromaCustomColorAllFramesNameD(const char* path)
	{
		PluginSetChromaCustomColorAllFramesName(path);
		return 0;
	}


	EXPORT_API void PluginMakeBlankFrames(int animationId, int frameCount, float duration, int color)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D)
		{
			return;
		}
		if (animation->GetDeviceId() != EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		PluginStopAnimation(animationId);
		Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
		vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
		frames.clear();
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			FChromaSDKColorFrame2D& frame = FChromaSDKColorFrame2D();
			frame.Duration = duration;
			frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
			int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
			int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
			for (int i = 0; i < maxRow; ++i)
			{
				FChromaSDKColors& row = frame.Colors[i];
				for (int j = 0; j < maxColumn; ++j)
				{
					row.Colors[j] = color;
				}
			}
			frames.push_back(frame);
		}
	}

	EXPORT_API void PluginMakeBlankFramesName(const char* path, int frameCount, float duration, int color)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMakeBlankFramesName: Animation not found! %s", path);
			return;
		}
		PluginMakeBlankFrames(animationId, frameCount, duration, color);
	}

	EXPORT_API double PluginMakeBlankFramesNameD(const char* path, double frameCount, double duration, double color)
	{
		PluginMakeBlankFramesName(path, (int)frameCount, (int)duration, (int)color);
		return 0;
	}


	EXPORT_API void PluginMakeBlankFramesRandomBlackAndWhite(int animationId, int frameCount, float duration)
	{
		AnimationBase* animation = GetAnimationInstance(animationId);
		if (nullptr == animation)
		{
			return;
		}
		if (animation->GetDeviceType() != EChromaSDKDeviceTypeEnum::DE_2D)
		{
			return;
		}
		if (animation->GetDeviceId() != EChromaSDKDevice2DEnum::DE_Keyboard)
		{
			return;
		}
		PluginStopAnimation(animationId);
		Animation2D* animation2D = dynamic_cast<Animation2D*>(animation);
		vector<FChromaSDKColorFrame2D>& frames = animation2D->GetFrames();
		frames.clear();
		for (int frameId = 0; frameId < frameCount; ++frameId)
		{
			FChromaSDKColorFrame2D& frame = FChromaSDKColorFrame2D();
			frame.Duration = duration;
			frame.Colors = ChromaSDKPlugin::GetInstance()->CreateColors2D(animation2D->GetDevice());
			int maxRow = ChromaSDKPlugin::GetInstance()->GetMaxRow(animation2D->GetDevice());
			int maxColumn = ChromaSDKPlugin::GetInstance()->GetMaxColumn(animation2D->GetDevice());
			for (int i = 0; i < maxRow; ++i)
			{
				FChromaSDKColors& row = frame.Colors[i];
				for (int j = 0; j < maxColumn; ++j)
				{
					int gray = fastrand() % 256;
					COLORREF color = RGB(gray, gray, gray);
					row.Colors[j] = color;
				}
			}
			frames.push_back(frame);
		}
	}

	EXPORT_API void PluginMakeBlankFramesRandomBlackAndWhiteName(const char* path, int frameCount, float duration)
	{
		int animationId = PluginGetAnimation(path);
		if (animationId < 0)
		{
			LogError("PluginMakeBlankFramesRandomBlackAndWhiteName: Animation not found! %s", path);
			return;
		}
		PluginMakeBlankFramesRandomBlackAndWhite(animationId, frameCount, duration);
	}

	EXPORT_API double PluginMakeBlankFramesRandomBlackAndWhiteNameD(const char* path, double frameCount, double duration)
	{
		PluginMakeBlankFramesRandomBlackAndWhiteName(path, (int)frameCount, (int)duration);
		return 0;
	}


	EXPORT_API RZRESULT PluginCreateEffect(RZDEVICEID deviceId, EFFECT_TYPE effect, int* colors, int size, FChromaSDKGuid* effectId)
	{
		// Chroma thread plays animations
		SetupChromaThread();

		vector<FChromaSDKColors> newColors = vector<FChromaSDKColors>();

		int index = 0;
		for (int i = 0; i < MAX_ROW; i++)
		{
			FChromaSDKColors row = FChromaSDKColors();
			for (int j = 0; j < MAX_COLUMN; ++j)
			{
				if (index < size)
				{
					int color = colors[index];
					row.Colors.push_back(color);
				}
				++index;
			}
			newColors.push_back(row);
		}

		FChromaSDKEffectResult result = ChromaSDKPlugin::GetInstance()->CreateEffect(deviceId, effect, newColors);

		effectId->Data.Data1 = result.EffectId.Data.Data1;
		effectId->Data.Data2 = result.EffectId.Data.Data2;
		effectId->Data.Data3 = result.EffectId.Data.Data3;
		effectId->Data.Data4[0] = result.EffectId.Data.Data4[0];
		effectId->Data.Data4[1] = result.EffectId.Data.Data4[1];
		effectId->Data.Data4[2] = result.EffectId.Data.Data4[2];
		effectId->Data.Data4[3] = result.EffectId.Data.Data4[3];
		effectId->Data.Data4[4] = result.EffectId.Data.Data4[4];
		effectId->Data.Data4[5] = result.EffectId.Data.Data4[5];
		effectId->Data.Data4[6] = result.EffectId.Data.Data4[6];
		effectId->Data.Data4[7] = result.EffectId.Data.Data4[7];
		return result.Result;
	}

	EXPORT_API RZRESULT PluginSetEffect(const FChromaSDKGuid& effectId)
	{
		// Chroma thread plays animations
		SetupChromaThread();

		RZRESULT result = ChromaSDKPlugin::GetInstance()->SetEffect(effectId);
		return result;
	}

	EXPORT_API RZRESULT PluginDeleteEffect(const FChromaSDKGuid& effectId)
	{
		// Chroma thread plays animations
		SetupChromaThread();

		RZRESULT result = ChromaSDKPlugin::GetInstance()->DeleteEffect(effectId);
		return result;
	}
}
