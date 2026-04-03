#include "Sound.h"
#include <vector>
#include <StringUtility.h>
#include <string>
#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#pragma comment(lib,"mfplat.lib")
#include <mfreadwrite.h>
#pragma comment(lib,"mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")

Sound::Sound() {
	// XAudio2の初期化
	HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result) && xAudio2_);

	// マスターボイスの作成
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(result) && masterVoice_);

	// Media Foundationの初期化
	result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(result));
}

Sound::~Sound() {

	if (masterVoice_) masterVoice_->DestroyVoice();
	if (xAudio2_) xAudio2_->Release();
	MFShutdown();
}

// 音声の再生に必要なデータ
Sound::SoundData Sound::LoadFile(const char* filePath) {

	////ファイルを開く
	//// ファイル入力ストリームのインスタンス
	//std::ifstream file;
	//// .wavファイルをバイナリモードで開く
	//file.open(filePath, std::ios_base::binary);
	//// ファイルオープン失敗を検出する
	//assert(file.is_open());

	//// .wavデータ読み込み
	//// RIFFヘッダの読み込み
	//Sound::RiffHeader riff;
	//file.read((char*)&riff, sizeof(riff));
	//// ファイルがRIFF形式であることを確認
	//if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
	//	assert(0);
	//}
	//// ファイルがWAVE形式であることを確認
	//if (strncmp(riff.type, "WAVE", 4) != 0) {
	//	assert(0);
	//}

	//// Fmatチャンクの読み込み
	//Sound::FormatChunk format = {};
	//// チャンクヘッダーの確認
	//file.read((char*)&format, sizeof(Sound::ChunkHeader));
	//if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
	//	assert(0);
	//}
	//// チャンク本体の読み込み
	//assert(format.chunk.size <= sizeof(format.fmt));
	//file.read((char*)&format.fmt, format.chunk.size);


	//// Dataチャンクの読み込み
	//Sound::ChunkHeader data;
	//file.read((char*)&data, sizeof(data));
	//// JUNkチャンクの確認
	//if (strncmp(data.id, "JUNK", 4) == 0) {
	//	// 読み取り位置をJUNKチャンクの終わりまで進める
	//	file.seekg(data.size, std::ios_base::cur);
	//	// 再読み込み
	//	file.read((char*)&data, sizeof(data));
	//}
	//if (strncmp(data.id, "data", 4) != 0) {
	//	assert(0);
	//}

	//// Dataチャンクのデータ部（波形データ）を読み込む
	//std::vector<char> buffer(data.size);
	//file.read(buffer.data(), data.size);

	//file.close();

	// フルパスをワイド文字列に変換
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT result;

	// SourceReader作成
	Microsoft::WRL::ComPtr<IMFSourceReader> sourceReader;
	result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &sourceReader);
	assert(SUCCEEDED(result));

	// PCM形式で読み込むように設定
	Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
	MFCreateMediaType(&mediaType);
	mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	result = sourceReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType.Get());
	assert(SUCCEEDED(result));

	// 実際にセットされたメディアタイプを取得
	Microsoft::WRL::ComPtr<IMFMediaType> outType;
	sourceReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outType);

	// wavwフォーマットを取得
	WAVEFORMATEX* waveFormat = nullptr;
	MFCreateWaveFormatExFromMFMediaType(outType.Get(), &waveFormat, nullptr);

	// コンテナに格納する音声データ
	Sound::SoundData soundData = {};
	soundData.wfex = *waveFormat;

	// 生成したwaveフォーマットを解放
	CoTaskMemFree(waveFormat);

	// PCMデータのバッファを構築
	while (true) {
		Microsoft::WRL::ComPtr<IMFSample> sample;
		DWORD streamIndex, flags;
		LONGLONG llTimeStamp;
		// サンプルの読み込み
		result = sourceReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &streamIndex, &flags, &llTimeStamp, &sample);
		// ストリームの末尾に達したら抜ける
		if(flags& MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}
		if (sample) {
			Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
			// サンプルからバッファを取得
			sample->ConvertToContiguousBuffer(&buffer);

			BYTE* pData = nullptr;
			DWORD maxLength = 0, currentLength = 0;
			// バッファをロックしてデータへのポインタを取得
			buffer->Lock(&pData, &maxLength, &currentLength);
			soundData.pBuffer.insert(soundData.pBuffer.end(), pData, pData + currentLength);
			buffer->Unlock();
		}
	}

	/*soundData.pBuffer = reinterpret_cast<BYTE*>(new char[data.size]);
	std::memcpy(soundData.pBuffer, buffer.data(), data.size);
	soundData.bufferSize = data.size;*/
	soundData.pSourceVoice = nullptr;

	return soundData;
}

void Sound::Unload(SoundData* soundData) {

	// 再生中のソースボイスを停止して破棄
	if (soundData->pSourceVoice) {
		soundData->pSourceVoice->Stop();
		soundData->pSourceVoice->DestroyVoice();
		soundData->pSourceVoice = nullptr;
	}

	// バッファの解放
	soundData->pBuffer.clear();
	soundData->wfex = {};
}

void Sound::PlayWave(SoundData& soundData) {

	HRESULT result;

	// 既存のソースボイスがあれば停止して破棄
	if (soundData.pSourceVoice) {
		soundData.pSourceVoice->Stop();
		soundData.pSourceVoice->DestroyVoice();
		soundData.pSourceVoice = nullptr;
	}

	// 波形フォーマットを元にSOURCEVoiceを作成
	result = xAudio2_->CreateSourceVoice(&soundData.pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	// 再生する波形データの設定
	XAUDIO2_BUFFER buf = {};
	buf.pAudioData = soundData.pBuffer.data();
	buf.AudioBytes = (UINT32)soundData.pBuffer.size();
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	result = soundData.pSourceVoice->SubmitSourceBuffer(&buf);
	result = soundData.pSourceVoice->Start();
}