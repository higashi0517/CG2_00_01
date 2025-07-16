#pragma once

#include <xaudio2.h>
#pragma comment (lib,"xaudio2.lib")
#include<fstream>
#include <cassert>

class Sound
{

public:

	// チャンクヘッダ
	struct ChunkHeader {
		char id[4];   // チャンクID
		int32_t size; // チャンクのサイズ
	};

	// RIFFヘッダチャンク
	struct RiffHeader {
		ChunkHeader chunk; // "RIFF"
		char type[4];      // "WAVE"
	};

	// FMTチャンク
	struct FormatChunk {
		ChunkHeader chunk; // "fmt "
		WAVEFORMATEX fmt;  // 波形データのフォーマット
	};

	// 音声データ
	struct SoundData {
		WAVEFORMATEX wfex; // 波形データのフォーマット
		BYTE* pBuffer;     // バッファの先着アドレス
		unsigned int bufferSize; // バッファのサイズ
	};

	Sound();
	~Sound();

	SoundData LoadWave(const char* filePath);

	void Unload(SoundData* soundData);

	void PlayWave(const SoundData& soundData);



private:
	IXAudio2* xAudio2_ = nullptr;
	IXAudio2MasteringVoice* masterVoice_ = nullptr;

};

