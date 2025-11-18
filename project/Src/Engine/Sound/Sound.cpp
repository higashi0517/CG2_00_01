#include "Sound.h"

Sound::Sound() {

	HRESULT result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result) && xAudio2_);
	result = xAudio2_->CreateMasteringVoice(&masterVoice_);
	assert(SUCCEEDED(result) && masterVoice_);

}

Sound::~Sound() {

	if (masterVoice_) masterVoice_->DestroyVoice();
	if (xAudio2_) xAudio2_->Release();
}

// 音声の再生に必要なデータ
Sound::SoundData Sound::LoadWave(const char* filePath) {

	//ファイルを開く
	// ファイル入力ストリームのインスタンス
	std::ifstream file;
	// .wavファイルをバイナリモードで開く
	file.open(filePath, std::ios_base::binary);
	// ファイルオープン失敗を検出する
	assert(file.is_open());

	// .wavデータ読み込み
	// RIFFヘッダの読み込み
	Sound::RiffHeader riff;
	file.read((char*)&riff, sizeof(riff));
	// ファイルがRIFF形式であることを確認
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0) {
		assert(0);
	}
	// ファイルがWAVE形式であることを確認
	if (strncmp(riff.type, "WAVE", 4) != 0) {
		assert(0);
	}

	// Fmatチャンクの読み込み
	Sound::FormatChunk format = {};
	// チャンクヘッダーの確認
	file.read((char*)&format, sizeof(Sound::ChunkHeader));
	if (strncmp(format.chunk.id, "fmt ", 4) != 0) {
		assert(0);
	}
	// チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);


	// Dataチャンクの読み込み
	Sound::ChunkHeader data;
	file.read((char*)&data, sizeof(data));
	// JUNkチャンクの確認
	if (strncmp(data.id, "JUNK", 4) == 0) {
		// 読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);
		// 再読み込み
		file.read((char*)&data, sizeof(data));
	}
	if (strncmp(data.id, "data", 4) != 0) {
		assert(0);
	}

	// Dataチャンクのデータ部（波形データ）を読み込む
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	// Waveファイルを閉じる
	file.close();

	// 読み込んだデータをreturnする
	Sound::SoundData soundData = {};
	soundData.wfex = format.fmt; // 波形データのフォーマットを設定
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer); // バッファの先頭アドレスを設定
	soundData.bufferSize = data.size; // バッファのサイズを設定
	soundData.pSourceVoice = nullptr; // ソースボイスを初期化

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
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->bufferSize = 0;
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
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.bufferSize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	// 波形データの再生
	result = soundData.pSourceVoice->SubmitSourceBuffer(&buf);
	result = soundData.pSourceVoice->Start();
}