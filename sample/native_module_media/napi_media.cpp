// Copyright (c) 2026 Huawei Technologies Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <thread>

#include "native_api.h"

namespace media {

constexpr int DEFAULT_DURATION = 180;
constexpr int DEFAULT_WIDTH = 1920;
constexpr int DEFAULT_HEIGHT = 1080;
constexpr int DEFAULT_BITRATE = 5000;
constexpr int DEFAULT_VIDEO_BITRATE = 4000;
constexpr int DEFAULT_AUDIO_BITRATE = 128;
constexpr int DEFAULT_FRAME_RATE = 30;
constexpr int DEFAULT_AUDIO_SAMPLE_RATE = 44100;
constexpr int DEFAULT_AUDIO_CHANNELS = 2;
constexpr int MAX_PATH_LENGTH = 1024;
constexpr int THUMBNAIL_DELAY_MS = 500;
constexpr int SEEK_DELAY_MS = 100;
constexpr int SET_SOURCE_DELAY_MS = 500;
constexpr int DEFAULT_VOLUME = 1;
constexpr int MIN_VOLUME = 0;
constexpr int MAX_VOLUME = 1;

enum class MediaFormat {
    MP4,
    AVI,
    MKV,
    MP3,
    WAV,
    FLAC,
    JPEG,
    PNG,
    GIF,
    WEBP
};

struct MediaMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int duration = 0;
    int width = 0;
    int height = 0;
    int bitrate = 0;
    MediaFormat format = MediaFormat::MP4;
    std::string codec;
    std::string creationDate;
    std::string lastModified;
};

struct MediaTrack {
    enum class Type {
        AUDIO,
        VIDEO,
        SUBTITLE
    };
    Type type;
    std::string codec;
    int bitrate = 0;
    int channels = 0;
    int sampleRate = 0;
    int width = 0;
    int height = 0;
    int frameRate = 0;
    std::string language;
};

enum class PlayerState {
    IDLE,
    PREPARING,
    READY,
    PLAYING,
    PAUSED,
    COMPLETED,
    ERROR
};

struct EncoderSettings {
    MediaFormat outputFormat = MediaFormat::MP4;
    int videoBitrate = 2000;
    int audioBitrate = 128;
    int frameRate = 30;
    int width = 1920;
    int height = 1080;
    int audioSampleRate = 44100;
    int audioChannels = 2;
    std::string videoCodec = "h264";
    std::string audioCodec = "aac";
};

class MediaProcessor {
public:
    MediaProcessor() {
        std::random_device rd;
        gen_ = std::mt19937(rd());
        dist_ = std::uniform_real_distribution<>(0.0, 1.0);
    }

    MediaMetadata AnalyzeMedia(const std::string& filePath) {
        MediaMetadata metadata;
        metadata.title = "Sample Media";
        metadata.artist = "Unknown Artist";
        metadata.album = "Unknown Album";
        metadata.duration = DEFAULT_DURATION;
        metadata.width = DEFAULT_WIDTH;
        metadata.height = DEFAULT_HEIGHT;
        metadata.bitrate = DEFAULT_BITRATE;
        metadata.format = MediaFormat::MP4;
        metadata.codec = "h264/aac";
        metadata.creationDate = "2026-01-01";
        metadata.lastModified = "2026-01-01";
        return metadata;
    }

    std::vector<MediaTrack> GetMediaTracks(const std::string& filePath) {
        std::vector<MediaTrack> tracks;

        MediaTrack videoTrack;
        videoTrack.type = MediaTrack::Type::VIDEO;
        videoTrack.codec = "h264";
        videoTrack.bitrate = DEFAULT_VIDEO_BITRATE;
        videoTrack.width = DEFAULT_WIDTH;
        videoTrack.height = DEFAULT_HEIGHT;
        videoTrack.frameRate = DEFAULT_FRAME_RATE;
        tracks.push_back(videoTrack);

        MediaTrack audioTrack;
        audioTrack.type = MediaTrack::Type::AUDIO;
        audioTrack.codec = "aac";
        audioTrack.bitrate = DEFAULT_AUDIO_BITRATE;
        audioTrack.channels = DEFAULT_AUDIO_CHANNELS;
        audioTrack.sampleRate = DEFAULT_AUDIO_SAMPLE_RATE;
        tracks.push_back(audioTrack);

        return tracks;
    }

    bool TranscodeMedia(const std::string& inputPath, const std::string& outputPath, const EncoderSettings& settings) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return true;
    }

    bool ExtractAudio(const std::string& videoPath, const std::string& audioPath) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }

    bool MergeAudioVideo(const std::string& videoPath, const std::string& audioPath, const std::string& outputPath) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return true;
    }

    bool GenerateThumbnail(const std::string& videoPath, const std::string& thumbnailPath, int timeMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(THUMBNAIL_DELAY_MS));
        return true;
    }

    std::string GetMediaInfoJson(const std::string& filePath) {
        MediaMetadata metadata = AnalyzeMedia(filePath);
        std::vector<MediaTrack> tracks = GetMediaTracks(filePath);

        std::string json = "{";
        json += "\"title\":\"" + metadata.title + "\",";
        json += "\"artist\":\"" + metadata.artist + "\",";
        json += "\"album\":\"" + metadata.album + "\",";
        json += "\"duration\":" + std::to_string(metadata.duration) + ",";
        json += "\"width\":" + std::to_string(metadata.width) + ",";
        json += "\"height\":" + std::to_string(metadata.height) + ",";
        json += "\"bitrate\":" + std::to_string(metadata.bitrate) + ",";
        json += "\"format\":\"MP4\",";
        json += "\"codec\":\"" + metadata.codec + "\",";
        json += "\"creationDate\":\"" + metadata.creationDate + "\",";
        json += "\"lastModified\":\"" + metadata.lastModified + "\",";
        json += "\"tracks\":[";

        for (size_t i = 0; i < tracks.size(); ++i) {
            if (i > 0) {
                json += ",";
            }
            json += "{";
            json += "\"type\":\"" + (tracks[i].type == MediaTrack::Type::VIDEO ? "video" :
                                         tracks[i].type == MediaTrack::Type::AUDIO ? "audio" : "subtitle") + "\",";
            json += "\"codec\":\"" + tracks[i].codec + "\",";
            json += "\"bitrate\":" + std::to_string(tracks[i].bitrate) + ",";
            if (tracks[i].type == MediaTrack::Type::AUDIO) {
                json += "\"channels\":" + std::to_string(tracks[i].channels) + ",";
                json += "\"sampleRate\":" + std::to_string(tracks[i].sampleRate);
            } else if (tracks[i].type == MediaTrack::Type::VIDEO) {
                json += "\"width\":" + std::to_string(tracks[i].width) + ",";
                json += "\"height\":" + std::to_string(tracks[i].height) + ",";
                json += "\"frameRate\":" + std::to_string(tracks[i].frameRate);
            }
            json += "}";
        }

        json += "]";
        json += "}";
        return json;
    }

private:
    std::mt19937 gen_;
    std::uniform_real_distribution<> dist_;
};

class MediaPlayer {
public:
    MediaPlayer() : state_(PlayerState::IDLE) {}

    bool SetSource(const std::string& filePath) {
        state_ = PlayerState::PREPARING;
        std::this_thread::sleep_for(std::chrono::milliseconds(SET_SOURCE_DELAY_MS));
        state_ = PlayerState::READY;
        return true;
    }

    bool Play() {
        if (state_ == PlayerState::READY || state_ == PlayerState::PAUSED) {
            state_ = PlayerState::PLAYING;
            return true;
        }
        return false;
    }

    bool Pause() {
        if (state_ == PlayerState::PLAYING) {
            state_ = PlayerState::PAUSED;
            return true;
        }
        return false;
    }

    bool Stop() {
        if (state_ != PlayerState::IDLE && state_ != PlayerState::ERROR) {
            state_ = PlayerState::IDLE;
            return true;
        }
        return false;
    }

    bool Seek(int positionMs) {
        if (state_ == PlayerState::PLAYING || state_ == PlayerState::PAUSED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SEEK_DELAY_MS));
            return true;
        }
        return false;
    }

    bool SetVolume(float volume) {
        if (volume >= MIN_VOLUME && volume <= MAX_VOLUME) {
            volume_ = volume;
            return true;
        }
        return false;
    }

    PlayerState GetState() const {
        return state_;
    }

    int GetCurrentPosition() const {
        return 0;
    }

    int GetDuration() const {
        return DEFAULT_DURATION * 1000;
    }

private:
    PlayerState state_;
    float volume_ = DEFAULT_VOLUME;
};

class MediaUtils {
public:
    static std::string FormatToString(MediaFormat format) {
        switch (format) {
            case MediaFormat::MP4: return "MP4";
            case MediaFormat::AVI: return "AVI";
            case MediaFormat::MKV: return "MKV";
            case MediaFormat::MP3: return "MP3";
            case MediaFormat::WAV: return "WAV";
            case MediaFormat::FLAC: return "FLAC";
            case MediaFormat::JPEG: return "JPEG";
            case MediaFormat::PNG: return "PNG";
            case MediaFormat::GIF: return "GIF";
            case MediaFormat::WEBP: return "WEBP";
            default: return "Unknown";
        }
    }

    static MediaFormat StringToFormat(const std::string& formatStr) {
        if (formatStr == "MP4") return MediaFormat::MP4;
        if (formatStr == "AVI") return MediaFormat::AVI;
        if (formatStr == "MKV") return MediaFormat::MKV;
        if (formatStr == "MP3") return MediaFormat::MP3;
        if (formatStr == "WAV") return MediaFormat::WAV;
        if (formatStr == "FLAC") return MediaFormat::FLAC;
        if (formatStr == "JPEG" || formatStr == "jpg") return MediaFormat::JPEG;
        if (formatStr == "PNG" || formatStr == "png") return MediaFormat::PNG;
        if (formatStr == "GIF" || formatStr == "gif") return MediaFormat::GIF;
        if (formatStr == "WEBP" || formatStr == "webp") return MediaFormat::WEBP;
        return MediaFormat::MP4;
    }

    static bool ValidateMediaFile(const std::string& filePath) {
        return true;
    }
};

} // namespace media

namespace {

std::shared_ptr<media::MediaProcessor> g_mediaProcessor = std::make_shared<media::MediaProcessor>();
std::unordered_map<napi_env, std::shared_ptr<media::MediaPlayer>> g_players;

napi_value AnalyzeMedia(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[MAX_PATH_LENGTH];
    size_t filePathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    media::MediaMetadata metadata = g_mediaProcessor->AnalyzeMedia(filePath);

    napi_value result;
    status = napi_create_object(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create object");
        return nullptr;
    }

    napi_value title;
    status = napi_create_string_utf8(env, metadata.title.c_str(), NAPI_AUTO_LENGTH, &title);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "title", title);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value artist;
    status = napi_create_string_utf8(env, metadata.artist.c_str(), NAPI_AUTO_LENGTH, &artist);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "artist", artist);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value album;
    status = napi_create_string_utf8(env, metadata.album.c_str(), NAPI_AUTO_LENGTH, &album);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "album", album);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value duration;
    status = napi_create_int32(env, metadata.duration, &duration);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "duration", duration);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value width;
    status = napi_create_int32(env, metadata.width, &width);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "width", width);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value height;
    status = napi_create_int32(env, metadata.height, &height);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "height", height);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value bitrate;
    status = napi_create_int32(env, metadata.bitrate, &bitrate);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "bitrate", bitrate);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value format;
    status = napi_create_string_utf8(env, media::MediaUtils::FormatToString(metadata.format).c_str(), NAPI_AUTO_LENGTH, &format);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "format", format);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value codec;
    status = napi_create_string_utf8(env, metadata.codec.c_str(), NAPI_AUTO_LENGTH, &codec);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "codec", codec);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value creationDate;
    status = napi_create_string_utf8(env, metadata.creationDate.c_str(), NAPI_AUTO_LENGTH, &creationDate);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "creationDate", creationDate);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    napi_value lastModified;
    status = napi_create_string_utf8(env, metadata.lastModified.c_str(), NAPI_AUTO_LENGTH, &lastModified);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    status = napi_set_named_property(env, result, "lastModified", lastModified);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    return result;
}

napi_value GetMediaTracks(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[MAX_PATH_LENGTH];
    size_t filePathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::vector<media::MediaTrack> tracks = g_mediaProcessor->GetMediaTracks(filePath);

    napi_value result;
    status = napi_create_array(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }

    for (size_t i = 0; i < tracks.size(); ++i) {
        napi_value track;
        status = napi_create_object(env, &track);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create object");
            return nullptr;
        }

        napi_value type;
        status = napi_create_string_utf8(env,
            (tracks[i].type == media::MediaTrack::Type::VIDEO ? "video" :
             tracks[i].type == media::MediaTrack::Type::AUDIO ? "audio" : "subtitle"),
            NAPI_AUTO_LENGTH, &type);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
            return nullptr;
        }
        status = napi_set_named_property(env, track, "type", type);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
            return nullptr;
        }

        napi_value codec;
        status = napi_create_string_utf8(env, tracks[i].codec.c_str(), NAPI_AUTO_LENGTH, &codec);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
            return nullptr;
        }
        status = napi_set_named_property(env, track, "codec", codec);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
            return nullptr;
        }

        napi_value bitrate;
        status = napi_create_int32(env, tracks[i].bitrate, &bitrate);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
            return nullptr;
        }
        status = napi_set_named_property(env, track, "bitrate", bitrate);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
            return nullptr;
        }

        if (tracks[i].type == media::MediaTrack::Type::AUDIO) {
            napi_value channels;
            status = napi_create_int32(env, tracks[i].channels, &channels);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
                return nullptr;
            }
            status = napi_set_named_property(env, track, "channels", channels);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
                return nullptr;
            }

            napi_value sampleRate;
            status = napi_create_int32(env, tracks[i].sampleRate, &sampleRate);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
                return nullptr;
            }
            status = napi_set_named_property(env, track, "sampleRate", sampleRate);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
                return nullptr;
            }
        } else if (tracks[i].type == media::MediaTrack::Type::VIDEO) {
            napi_value width;
            status = napi_create_int32(env, tracks[i].width, &width);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
                return nullptr;
            }
            status = napi_set_named_property(env, track, "width", width);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
                return nullptr;
            }

            napi_value height;
            status = napi_create_int32(env, tracks[i].height, &height);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
                return nullptr;
            }
            status = napi_set_named_property(env, track, "height", height);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
                return nullptr;
            }

            napi_value frameRate;
            status = napi_create_int32(env, tracks[i].frameRate, &frameRate);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create int32");
                return nullptr;
            }
            status = napi_set_named_property(env, track, "frameRate", frameRate);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
                return nullptr;
            }
        }

        status = napi_set_element(env, result, i, track);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set element");
            return nullptr;
        }
    }

    return result;
}

napi_value TranscodeMedia(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char inputPath[MAX_PATH_LENGTH];
    char outputPath[MAX_PATH_LENGTH];
    size_t inputPathLength = 0;
    size_t outputPathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], inputPath, sizeof(inputPath), &inputPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }
    status = napi_get_value_string_utf8(env, args[1], outputPath, sizeof(outputPath), &outputPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    media::EncoderSettings settings;

    bool success = g_mediaProcessor->TranscodeMedia(inputPath, outputPath, settings);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value ExtractAudio(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 2) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[MAX_PATH_LENGTH];
    char audioPath[MAX_PATH_LENGTH];
    size_t videoPathLength = 0;
    size_t audioPathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }
    status = napi_get_value_string_utf8(env, args[1], audioPath, sizeof(audioPath), &audioPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    bool success = g_mediaProcessor->ExtractAudio(videoPath, audioPath);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value MergeAudioVideo(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[MAX_PATH_LENGTH];
    char audioPath[MAX_PATH_LENGTH];
    char outputPath[MAX_PATH_LENGTH];
    size_t videoPathLength = 0;
    size_t audioPathLength = 0;
    size_t outputPathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }
    status = napi_get_value_string_utf8(env, args[1], audioPath, sizeof(audioPath), &audioPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }
    status = napi_get_value_string_utf8(env, args[2], outputPath, sizeof(outputPath), &outputPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    bool success = g_mediaProcessor->MergeAudioVideo(videoPath, audioPath, outputPath);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value GenerateThumbnail(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 3) {
        napi_throw_error(env, "BAD_ARGS", "Missing arguments");
        return nullptr;
    }

    char videoPath[MAX_PATH_LENGTH];
    char thumbnailPath[MAX_PATH_LENGTH];
    size_t videoPathLength = 0;
    size_t thumbnailPathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], videoPath, sizeof(videoPath), &videoPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }
    status = napi_get_value_string_utf8(env, args[1], thumbnailPath, sizeof(thumbnailPath), &thumbnailPathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    int timeMs = 0;
    status = napi_get_value_int32(env, args[2], &timeMs);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get int32 value");
        return nullptr;
    }

    bool success = g_mediaProcessor->GenerateThumbnail(videoPath, thumbnailPath, timeMs);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value GetMediaInfoJSON(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[MAX_PATH_LENGTH];
    size_t filePathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string json = g_mediaProcessor->GetMediaInfoJson(filePath);

    napi_value result;
    status = napi_create_string_utf8(env, json.c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value CreatePlayer(napi_env env, napi_callback_info info) {
    auto player = std::make_shared<media::MediaPlayer>();
    g_players[env] = player;

    napi_value result;
    napi_status status = napi_create_object(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create object");
        return nullptr;
    }
    return result;
}

napi_value PlayerSetSource(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing file path argument");
        return nullptr;
    }

    char filePath[MAX_PATH_LENGTH];
    size_t filePathLength = 0;
    status = napi_get_value_string_utf8(env, args[0], filePath, sizeof(filePath), &filePathLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->SetSource(filePath);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerPlay(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->Play();

    napi_value result;
    napi_status status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerPause(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->Pause();

    napi_value result;
    napi_status status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerStop(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->Stop();

    napi_value result;
    napi_status status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerSeek(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing position argument");
        return nullptr;
    }

    int positionMs = 0;
    status = napi_get_value_int32(env, args[0], &positionMs);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get int32 value");
        return nullptr;
    }

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->Seek(positionMs);

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerSetVolume(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "BAD_ARGS", "Missing volume argument");
        return nullptr;
    }

    double volume = 0.0;
    status = napi_get_value_double(env, args[0], &volume);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
        return nullptr;
    }

    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    bool success = it->second->SetVolume(static_cast<float>(volume));

    napi_value result;
    status = napi_create_bool(env, success, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create bool");
        return nullptr;
    }
    return result;
}

napi_value PlayerGetState(napi_env env, napi_callback_info info) {
    auto it = g_players.find(env);
    if (it == g_players.end()) {
        napi_throw_error(env, "NO_PLAYER", "No player created");
        return nullptr;
    }

    media::PlayerState state = it->second->GetState();
    const char* stateStr = "IDLE";
    switch (state) {
        case media::PlayerState::PREPARING:
            stateStr = "PREPARING";
            break;
        case media::PlayerState::READY:
            stateStr = "READY";
            break;
        case media::PlayerState::PLAYING:
            stateStr = "PLAYING";
            break;
        case media::PlayerState::PAUSED:
            stateStr = "PAUSED";
            break;
        case media::PlayerState::COMPLETED:
            stateStr = "COMPLETED";
            break;
        case media::PlayerState::ERROR:
            stateStr = "ERROR";
            break;
        default:
            break;
    }

    napi_value result;
    napi_status status = napi_create_string_utf8(env, stateStr, NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

} // namespace

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor mediaProcessorProps[] = {
        {"analyzeMedia", nullptr, AnalyzeMedia, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMediaTracks", nullptr, GetMediaTracks, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"transcodeMedia", nullptr, TranscodeMedia, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"extractAudio", nullptr, ExtractAudio, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"mergeAudioVideo", nullptr, MergeAudioVideo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateThumbnail", nullptr, GenerateThumbnail, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMediaInfoJSON", nullptr, GetMediaInfoJSON, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    napi_value playerObj;
    napi_status status = napi_create_object(env, &playerObj);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create object");
        return nullptr;
    }

    napi_property_descriptor playerProps[] = {
        {"create", nullptr, CreatePlayer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setSource", nullptr, PlayerSetSource, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"play", nullptr, PlayerPlay, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pause", nullptr, PlayerPause, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stop", nullptr, PlayerStop, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"seek", nullptr, PlayerSeek, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setVolume", nullptr, PlayerSetVolume, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getState", nullptr, PlayerGetState, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    status = napi_define_properties(env, exports, sizeof(mediaProcessorProps) / sizeof(mediaProcessorProps[0]), mediaProcessorProps);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to define properties");
        return nullptr;
    }
    status = napi_define_properties(env, playerObj, sizeof(playerProps) / sizeof(playerProps[0]), playerProps);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to define properties");
        return nullptr;
    }
    status = napi_set_named_property(env, exports, "player", playerObj);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to set property");
        return nullptr;
    }

    return exports;
}

NAPI_MODULE(native_module_media, Init)
