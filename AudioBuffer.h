#pragma once
#include <cassert>
#include <cstdint>
#include <memory>

enum class OpusFrameDuration
{
  OFD_2_5_MS,
  OFD_5_MS,
  OFD_10_MS,
  OFD_20_MS,
  OFD_40_MS,
  OFD_60_MS
};

class AudioBuffer {
public:

  AudioBuffer(int samplesPerSecond, int channels, int bitsPerSample)
    :m_eFrameDurationMs(OpusFrameDuration::OFD_20_MS),
    m_inputFrameDurationMs(0.0),
    m_samplesPerSecond(samplesPerSecond),
    m_channels(channels),
    m_bitsPerSample(bitsPerSample),
    m_currentBufferSize(1536000), // size buffer to store one second of 16bps, 2 channels, 48K sampling rate
    m_currentSize(0),
    m_startPos(0),
    m_endPos(0),
    m_pDataBuffer(std::unique_ptr<uint8_t[]>(new uint8_t[m_currentBufferSize])),
    m_tStart(-1),
    m_tStop(0)
  {
    init();
  }

  ~AudioBuffer() {};

  OpusFrameDuration getFrameDurationMsEnum() { return m_eFrameDurationMs; }
  double getFrameDurationMs() { return m_inputFrameDurationMs; }
  int getBytesPerSecond() const { return m_bytesPerSecond; }
  int getBytesPerFrame() const { return m_bytesPerFrame; }

  /**
   *
   */
  int addAudioData(uint8_t* pData, uint32_t size, REFERENCE_TIME tStart, REFERENCE_TIME tStop)
  {
    // first version: we only read data if the buffer has enough space: in our case this should be sufficient
    if (freeSpace() < size) { return -1; }

    // re-position data in buffer: we are expecting one call of addAudioData to be followed by several 
    // readNextAudioFrame calls until there isn't enough data in the buffer to read an audio frame
    if (m_startPos > 0 && m_currentSize > 0)
    {
      memcpy(m_pDataBuffer.get(), m_pDataBuffer.get() + m_startPos, m_currentSize);
    }
    m_startPos = 0;
    m_endPos = m_currentSize;

    memcpy(m_pDataBuffer.get() + m_currentSize, pData, size);
    m_currentSize += size;

    if (m_tStart == -1) m_tStart = tStart;
    m_tStop = tStop;

    m_sampleBufferDurationInMs = m_currentSize * 1000 / m_bytesPerSecond;

    m_numberOfFrames = m_sampleBufferDurationInMs / m_inputFrameDurationMs;
    m_additionalDataInMs = std::fmod(m_sampleBufferDurationInMs, m_inputFrameDurationMs);
    return m_numberOfFrames;
  }

  bool readNextAudioFrame(REFERENCE_TIME& tStart, REFERENCE_TIME& tStop, uint8_t*& p)
  {
    if (m_numberOfFrames > 0)
    {
      p = m_pDataBuffer.get() + m_startPos;
      m_startPos += m_bytesPerFrame;
      assert(m_currentSize >= m_bytesPerFrame);
      m_currentSize -= m_bytesPerFrame;
      m_numberOfFrames--;
      tStart = m_tStart;
      m_tStart += getFrameDurationMs() * 10000;
      tStop = m_tStart;
      m_sampleBufferDurationInMs -= getFrameDurationMs();
      return true;
    }
    else {
      return false;
    }
  }

private:
  AudioBuffer(const AudioBuffer&) = delete;
  AudioBuffer& operator=(const AudioBuffer&) = delete;

  void init() {
    switch (m_eFrameDurationMs) {
    case OpusFrameDuration::OFD_2_5_MS:
    {
      m_inputFrameDurationMs = 2.5;
      break;
    }
    case OpusFrameDuration::OFD_5_MS:
    {
      m_inputFrameDurationMs = 5;
      break;
    }
    case OpusFrameDuration::OFD_10_MS:
    {
      m_inputFrameDurationMs = 10;
      break;
    }
    case OpusFrameDuration::OFD_20_MS:
    {
      m_inputFrameDurationMs = 20;
      break;
    }
    case OpusFrameDuration::OFD_40_MS:
    {
      m_inputFrameDurationMs = 40;
      break;
    }
    case OpusFrameDuration::OFD_60_MS:
    {
      m_inputFrameDurationMs = 60;
      break;
    }
    }

    // the number of these frames per second e.g. 50 20ms frames
    int iInputFrameCountPerSecond = 1000 / m_inputFrameDurationMs;
    // total bytes per second
    m_bytesPerSecond = m_samplesPerSecond * m_channels * (m_bitsPerSample / 8);
    // frame size in bytes
    m_bytesPerFrame = m_bytesPerSecond / iInputFrameCountPerSecond;

    //int iDataPerChannelInSample = lSourceSize / (pwfx->nChannels * pwfx->wBitsPerSample / 8);
    //int iSampleBufferDurationInMs = lSourceSize * 1000 / iBytesPerSecond;
  }

  int freeSpace() const { return m_currentBufferSize - m_currentSize; }

  double getFrameDurationMs() const
  {
    switch (m_eFrameDurationMs)
    {
    case OpusFrameDuration::OFD_2_5_MS:
    {
      return 2.5;
    }
    case OpusFrameDuration::OFD_5_MS:
    {
      return 5;
    }
    case OpusFrameDuration::OFD_10_MS:
    {
      return 10;
    }
    case OpusFrameDuration::OFD_20_MS:
    {
      return 20;
    }
    case OpusFrameDuration::OFD_40_MS:
    {
      return 40;
    }
    case OpusFrameDuration::OFD_60_MS:
    {
      return 60;
    }
    }
  }

  OpusFrameDuration m_eFrameDurationMs;
  double m_inputFrameDurationMs;

  int m_samplesPerSecond;
  int m_channels;
  int m_bitsPerSample;

  int m_bytesPerSecond;
  int m_bytesPerFrame;
  int m_sampleBufferDurationInMs;
  int m_numberOfFrames;
  double m_additionalDataInMs;

  //buffer we will use to manage the audio data
  int m_currentBufferSize;
  int m_currentSize;
  int m_startPos;
  int m_endPos;

  std::unique_ptr<uint8_t[]> m_pDataBuffer;
  // the time of the first sample in the buffer
  REFERENCE_TIME m_tStart;
  REFERENCE_TIME m_tStop;
};
